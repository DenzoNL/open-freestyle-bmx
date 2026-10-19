"""Minimal, deterministic Win32 environment for running NgBMX.exe code in Unicorn.

Only what the MSVC6 CRT startup and the game's file/time code need is implemented. Every other import
traps (the harness reports it), so tests never silently depend on unemulated behaviour.

All mutable state lives in `Win32.state` (a plain dict) so the harness can snapshot and restore it
together with memory. Files are read from a game root on the host (case-insensitive lookup); writes
go to an in-memory overlay and never touch the disk.
"""
import copy
import os
import struct
from pathlib import Path

from unicorn.x86_const import (
    UC_X86_REG_CS,
    UC_X86_REG_DS,
    UC_X86_REG_EAX,
    UC_X86_REG_EIP,
    UC_X86_REG_ES,
    UC_X86_REG_ESP,
    UC_X86_REG_FS,
    UC_X86_REG_GDTR,
    UC_X86_REG_GS,
    UC_X86_REG_SS,
)

GDT_BASE = 0x7FE00000
TEB_BASE = 0x7FE01000
PEB_BASE = 0x7FE02000
STRINGS_BASE = 0x7FE03000
VALLOC_BASE, VALLOC_SIZE = 0x30000000, 0x08000000

INVALID_HANDLE = 0xFFFFFFFF
ERROR_FILE_NOT_FOUND = 2
ERROR_INVALID_HANDLE = 6
FAKE_HEAP = 0x00DE0000
STD_HANDLES = {0xFFFFFFF6: 0x10, 0xFFFFFFF5: 0x14, 0xFFFFFFF4: 0x18}  # stdin, stdout, stderr

# Directory aliases from the shipped install layout to the folders unshield extracts.
ALIASES = {"bin/audio": "Audio", "bin/movie": "Movies"}


class ProcessExit(Exception):
    pass


def _gdt_entry(base, limit, access, flags):
    return (
        (limit & 0xFFFF)
        | ((base & 0xFFFFFF) << 16)
        | (access << 40)
        | (((limit >> 16) & 0xF) << 48)
        | (flags << 52)
        | (((base >> 24) & 0xFF) << 56)
    )


class Win32:
    def __init__(self, machine, game_root=None):
        self.m = machine
        self.uc = machine.uc
        self.game_root = Path(game_root) if game_root else None
        self.log = []
        self.state = {
            "last_error": 0,
            "tick": 1000,  # milliseconds; advances only through Sleep and time queries
            "valloc": VALLOC_BASE,
            "next_handle": 0x100,
            "files": {},  # handle -> {"path": str, "pos": int}
            "overlay": {},  # virtual path -> bytes (written files)
        }
        self._setup_teb()
        self.stubs = self._build_stubs()

    # --- process environment -------------------------------------------------------------------
    def _setup_teb(self):
        uc = self.uc
        uc.mem_map(GDT_BASE, 0x10000)
        teb = bytearray(0x1000)
        struct.pack_into("<I", teb, 0x00, 0xFFFFFFFF)  # SEH chain end
        struct.pack_into("<I", teb, 0x04, self.m.stack_top)
        struct.pack_into("<I", teb, 0x08, self.m.stack_bottom)
        struct.pack_into("<I", teb, 0x18, TEB_BASE)
        struct.pack_into("<I", teb, 0x30, PEB_BASE)
        uc.mem_write(TEB_BASE, bytes(teb))
        # GDT: null, flat code, flat data, FS -> TEB
        entries = [0, _gdt_entry(0, 0xFFFFF, 0x9B, 0xC), _gdt_entry(0, 0xFFFFF, 0x93, 0xC), _gdt_entry(TEB_BASE, 0xFFF, 0x93, 0x4)]
        uc.mem_write(GDT_BASE, b"".join(struct.pack("<Q", e) for e in entries))
        uc.reg_write(UC_X86_REG_GDTR, (0, GDT_BASE, len(entries) * 8 - 1, 0))
        # Once a GDT is loaded every segment must point at a flat 32-bit descriptor.
        uc.reg_write(UC_X86_REG_CS, 1 << 3)
        for reg in (UC_X86_REG_SS, UC_X86_REG_DS, UC_X86_REG_ES, UC_X86_REG_GS):
            uc.reg_write(reg, 2 << 3)
        uc.reg_write(UC_X86_REG_FS, 3 << 3)
        self.cmdline = self._static_str(b"NgBMX.exe\0", 0)
        self.modname = self._static_str(b"C:\\BMX\\NgBMX.exe\0", 0x40)
        self.env = self._static_str(b"\0\0", 0x80)

    def _static_str(self, data, off):
        self.uc.mem_write(STRINGS_BASE + off, data)
        return STRINGS_BASE + off

    def snapshot(self):
        return copy.deepcopy(self.state)

    def restore(self, s):
        self.state = copy.deepcopy(s)

    # --- helpers ---------------------------------------------------------------------------------
    def _u32(self, a):
        return struct.unpack("<I", self.uc.mem_read(a, 4))[0]

    def _w32(self, a, v):
        self.uc.mem_write(a, struct.pack("<I", v & 0xFFFFFFFF))

    def _cstr(self, a):
        return self.m.read_cstr(a)

    def _handle(self):
        h = self.state["next_handle"]
        self.state["next_handle"] += 4
        return h

    def _resolve(self, vpath):
        """Map a game path (relative to the install dir) to a host file, case-insensitively."""
        p = vpath.replace("\\", "/")
        if len(p) > 1 and p[1] == ":":
            p = p[2:]
        p = p.lstrip("./").lstrip("/")
        key = p.lower()
        for src, dst in ALIASES.items():
            if key.startswith(src + "/"):
                p = dst + p[len(src) :]
                break
        if self.game_root is None:
            return key, None
        cur = self.game_root
        for part in [x for x in p.split("/") if x]:
            if not cur.is_dir():
                return key, None
            match = next((c for c in os.listdir(cur) if c.lower() == part.lower()), None)
            if match is None:
                return key, None
            cur = cur / match
        return key, cur if cur.is_file() else None

    def _file_data(self, key, host):
        if key in self.state["overlay"]:
            return self.state["overlay"][key]
        return host.read_bytes() if host else b""

    # --- dispatch --------------------------------------------------------------------------------
    def call(self, name):
        """Run the stub for an import; returns False when it is not emulated."""
        stub = self.stubs.get(name.split("!")[-1])
        if stub is None:
            return False
        argc, fn = stub
        esp = self.uc.reg_read(UC_X86_REG_ESP)
        ret = self._u32(esp)
        args = [self._u32(esp + 4 + 4 * i) for i in range(argc)]
        value = fn(*args)
        self.uc.reg_write(UC_X86_REG_EAX, (value or 0) & 0xFFFFFFFF)
        self.uc.reg_write(UC_X86_REG_ESP, esp + 4 + 4 * argc)  # stdcall
        self.uc.reg_write(UC_X86_REG_EIP, ret)
        return True

    def _build_stubs(self):
        s = self.state

        def ok(*_):
            return 1

        def zero(*_):
            return 0

        def heap_alloc(heap, flags, n):
            p = self.m.alloc(n)
            if flags & 0x8:  # HEAP_ZERO_MEMORY
                self.uc.mem_write(p, b"\0" * n)
            return p

        def heap_realloc(heap, flags, old, n):
            p = self.m.alloc(n)
            if old:
                self.uc.mem_write(p, bytes(self.uc.mem_read(old, min(n, self.m.sizes.get(old, 0)))))
            return p

        def virtual_alloc(addr, size, kind, prot):
            if addr:
                return addr  # commit inside an earlier reservation
            p = s["valloc"]
            s["valloc"] += (size + 0xFFFF) & ~0xFFFF
            if s["valloc"] > VALLOC_BASE + VALLOC_SIZE:
                return 0
            return p

        def get_tick(*_):
            s["tick"] += 1
            return s["tick"]

        def sleep(ms):
            s["tick"] += ms

        def qpc(out):
            s["tick"] += 1
            self.uc.mem_write(out, struct.pack("<Q", s["tick"] * 1000))
            return 1

        def qpf(out):
            self.uc.mem_write(out, struct.pack("<Q", 1_000_000))
            return 1

        def system_time(out):
            # 2000-11-22 12:00:00.000, a Wednesday
            self.uc.mem_write(out, struct.pack("<8H", 2000, 11, 3, 22, 12, 0, 0, 0))

        def get_version(*_):
            return 0x0A280105  # Windows XP (5.1.2600)

        def get_version_ex(info):
            self.uc.mem_write(info + 4, struct.pack("<4I", 5, 1, 2600, 2))
            return 1

        def startup_info(info):
            self.uc.mem_write(info, struct.pack("<I", 68) + b"\0" * 64)

        def std_handle(which):
            return STD_HANDLES.get(which, INVALID_HANDLE)

        def file_type(h):
            return 2 if h in STD_HANDLES.values() else (1 if h in s["files"] else 0)

        def module_file_name(mod, buf, size):
            name = b"C:\\BMX\\NgBMX.exe"
            self.uc.mem_write(buf, name[: size - 1] + b"\0")
            return min(len(name), size - 1)

        def mb2wc(cp, flags, src, srclen, dst, dstlen):
            data = bytes(self.uc.mem_read(src, srclen)) if srclen != 0xFFFFFFFF else self._cstr(src).encode("latin-1") + b"\0"
            if dstlen:
                self.uc.mem_write(dst, data.decode("latin-1").encode("utf-16-le")[: dstlen * 2])
            return len(data)

        def wc2mb(cp, flags, src, srclen, dst, dstlen, default, used):
            n = srclen if srclen != 0xFFFFFFFF else None
            raw = bytearray()
            i = 0
            while n is None or i < n:
                ch = struct.unpack("<H", self.uc.mem_read(src + 2 * i, 2))[0]
                raw.append(ch & 0xFF if ch < 0x100 else ord("?"))
                i += 1
                if n is None and ch == 0:
                    break
            if dstlen:
                self.uc.mem_write(dst, bytes(raw[:dstlen]))
            return len(raw)

        def cpinfo(cp, info):
            self.uc.mem_write(info, struct.pack("<I", 1) + b"?\0" + b"\0" * 12)
            return 1

        def create_file(name, access, share, sec, disposition, flags, template):
            path = self._cstr(name)
            key, host = self._resolve(path)
            creating = disposition in (1, 2, 4)  # CREATE_NEW, CREATE_ALWAYS, OPEN_ALWAYS
            if host is None and key not in s["overlay"] and not creating:
                self.log.append(f"CreateFileA miss: {path}")
                s["last_error"] = ERROR_FILE_NOT_FOUND
                return INVALID_HANDLE
            if disposition in (2, 5) or (creating and host is None and key not in s["overlay"]):
                s["overlay"][key] = b""  # CREATE_ALWAYS / TRUNCATE_EXISTING / new file
            h = self._handle()
            s["files"][h] = {"key": key, "host": str(host) if host else None, "pos": 0}
            return h

        def _f(h):
            f = s["files"].get(h)
            if f is None:
                s["last_error"] = ERROR_INVALID_HANDLE
            return f

        def read_file(h, buf, n, nread, overlapped):
            f = _f(h)
            if f is None:
                return 0
            data = self._file_data(f["key"], Path(f["host"]) if f["host"] else None)
            chunk = data[f["pos"] : f["pos"] + n]
            self.uc.mem_write(buf, chunk)
            f["pos"] += len(chunk)
            if nread:
                self._w32(nread, len(chunk))
            return 1

        def write_file(h, buf, n, nwritten, overlapped):
            if h in STD_HANDLES.values():
                self.log.append(bytes(self.uc.mem_read(buf, n)).decode("latin-1"))
            else:
                f = _f(h)
                if f is None:
                    return 0
                data = bytearray(self._file_data(f["key"], Path(f["host"]) if f["host"] else None))
                data[f["pos"] : f["pos"] + n] = bytes(self.uc.mem_read(buf, n))
                s["overlay"][f["key"]] = bytes(data)
                f["pos"] += n
            if nwritten:
                self._w32(nwritten, n)
            return 1

        def set_file_pointer(h, dist, dist_high, method):
            f = _f(h)
            if f is None:
                return INVALID_HANDLE
            size = len(self._file_data(f["key"], Path(f["host"]) if f["host"] else None))
            d = struct.unpack("<i", struct.pack("<I", dist))[0]
            base = (0, f["pos"], size)[method]
            f["pos"] = max(0, base + d)
            if dist_high:
                self._w32(dist_high, 0)
            return f["pos"]

        def get_file_size(h, high):
            f = _f(h)
            if f is None:
                return INVALID_HANDLE
            if high:
                self._w32(high, 0)
            return len(self._file_data(f["key"], Path(f["host"]) if f["host"] else None))

        def close_handle(h):
            s["files"].pop(h, None)
            return 1

        def set_end_of_file(h):
            f = _f(h)
            if f is None:
                return 0
            data = self._file_data(f["key"], Path(f["host"]) if f["host"] else None)
            s["overlay"][f["key"]] = data[: f["pos"]]
            return 1

        def ctype1(ch):
            c = chr(ch)
            f = 0
            if c.isupper():
                f |= 0x1 | 0x100
            if c.islower():
                f |= 0x2 | 0x100
            if c.isdigit() and ch < 0x80:
                f |= 0x4
            if c in " \t\n\r\v\f":
                f |= 0x8
            if c in " \t":
                f |= 0x40
            if ch < 0x20 or ch == 0x7F:
                f |= 0x20
            if c.isalpha() and not (f & 0x3):
                f |= 0x100
            if 0x20 < ch < 0x7F and not c.isalnum():
                f |= 0x10
            if c in "0123456789abcdefABCDEF":
                f |= 0x80
            return f

        def _chars(src, n, wide):
            if wide:
                if n == 0xFFFFFFFF:
                    out = []
                    while (ch := struct.unpack("<H", self.uc.mem_read(src + 2 * len(out), 2))[0]) != 0:
                        out.append(ch)
                    return out + [0]
                return list(struct.unpack(f"<{n}H", self.uc.mem_read(src, 2 * n)))
            if n == 0xFFFFFFFF:
                return list(self._cstr(src).encode("latin-1")) + [0]
            return list(self.uc.mem_read(src, n))

        def string_type(info, src, n, out, wide):
            chars = _chars(src, n, wide)
            if info != 1:  # only CT_CTYPE1
                return 0
            self.uc.mem_write(out, b"".join(struct.pack("<H", ctype1(c) if c < 0x100 else 0) for c in chars))
            return 1

        def lcmap(locale, flags, src, n, dst, dstlen, wide):
            chars = _chars(src, n, wide)
            if flags & 0x200:
                chars = [ord(chr(c).upper()) if c < 0x100 and len(chr(c).upper()) == 1 and ord(chr(c).upper()) < 0x100 else c for c in chars]
            elif flags & 0x100:
                chars = [ord(chr(c).lower()) if c < 0x100 and ord(chr(c).lower()) < 0x100 else c for c in chars]
            if dstlen:
                if wide:
                    self.uc.mem_write(dst, b"".join(struct.pack("<H", c) for c in chars[:dstlen]))
                else:
                    self.uc.mem_write(dst, bytes(c & 0xFF for c in chars[:dstlen]))
            return len(chars)

        def compare_string(locale, flags, a, na, b, nb, wide):
            x, y = _chars(a, na, wide), _chars(b, nb, wide)
            if x and x[-1] == 0 and na == 0xFFFFFFFF:
                x = x[:-1]
            if y and y[-1] == 0 and nb == 0xFFFFFFFF:
                y = y[:-1]
            if flags & 1:  # NORM_IGNORECASE
                x = [ord(chr(c).lower()) for c in x]
                y = [ord(chr(c).lower()) for c in y]
            return 1 if x < y else (3 if x > y else 2)

        def last_error(*_):
            return s["last_error"]

        def reg_missing(*_):
            return ERROR_FILE_NOT_FOUND

        def debug_string(p):
            self.log.append("ODS: " + self._cstr(p))

        def exit_process(code, *_):
            raise ProcessExit(f"process exit {code}")

        return {
            # CRT startup and heap
            "GetVersion": (0, get_version),
            "GetVersionExA": (1, get_version_ex),
            "HeapCreate": (3, lambda *_: FAKE_HEAP),
            "HeapDestroy": (1, ok),
            "HeapAlloc": (3, heap_alloc),
            "HeapReAlloc": (4, heap_realloc),
            "HeapFree": (3, ok),
            "VirtualAlloc": (4, virtual_alloc),
            "VirtualFree": (3, ok),
            "GetCommandLineA": (0, lambda: self.cmdline),
            "GetEnvironmentStrings": (0, lambda: self.env),
            "GetEnvironmentStringsW": (0, zero),
            "FreeEnvironmentStringsA": (1, ok),
            "FreeEnvironmentStringsW": (1, ok),
            "GetEnvironmentVariableA": (3, zero),
            "SetEnvironmentVariableA": (2, ok),
            "GetStartupInfoA": (1, startup_info),
            "GetStdHandle": (1, std_handle),
            "SetStdHandle": (2, ok),
            "GetFileType": (1, file_type),
            "SetHandleCount": (1, lambda n: n),
            "GetModuleHandleA": (1, lambda name: 0x400000 if name == 0 else 0),
            "GetModuleFileNameA": (3, module_file_name),
            "GetACP": (0, lambda: 1252),
            "GetOEMCP": (0, lambda: 437),
            "GetCPInfo": (2, cpinfo),
            "MultiByteToWideChar": (6, mb2wc),
            "WideCharToMultiByte": (8, wc2mb),
            "GetStringTypeW": (4, lambda info, src, n, out: string_type(info, src, n, out, True)),
            "GetStringTypeA": (5, lambda loc, info, src, n, out: string_type(info, src, n, out, False)),
            "LCMapStringW": (6, lambda *a: lcmap(*a, True)),
            "LCMapStringA": (6, lambda *a: lcmap(*a, False)),
            "CompareStringW": (6, lambda *a: compare_string(*a, True)),
            "CompareStringA": (6, lambda *a: compare_string(*a, False)),
            "GetProcAddress": (2, zero),
            "LoadLibraryA": (1, zero),
            "FreeLibrary": (1, ok),
            "InitializeCriticalSection": (1, zero),
            "DeleteCriticalSection": (1, zero),
            "EnterCriticalSection": (1, zero),
            "LeaveCriticalSection": (1, zero),
            "GetLastError": (0, last_error),
            "IsBadWritePtr": (2, zero),
            "OutputDebugStringA": (1, debug_string),
            "SetConsoleCtrlHandler": (2, ok),
            "ExitProcess": (1, exit_process),
            "TerminateProcess": (2, exit_process),
            "GetCurrentProcess": (0, lambda: 0xFFFFFFFF),
            # time (deterministic)
            "GetTickCount": (0, get_tick),
            "timeGetTime": (0, get_tick),
            "Sleep": (1, sleep),
            "QueryPerformanceCounter": (1, qpc),
            "QueryPerformanceFrequency": (1, qpf),
            "GetLocalTime": (1, system_time),
            "GetSystemTime": (1, system_time),
            "timeBeginPeriod": (1, zero),
            "timeEndPeriod": (1, zero),
            # files
            "CreateFileA": (7, create_file),
            "ReadFile": (5, read_file),
            "WriteFile": (5, write_file),
            "SetFilePointer": (4, set_file_pointer),
            "GetFileSize": (2, get_file_size),
            "CloseHandle": (1, close_handle),
            "FlushFileBuffers": (1, ok),
            "SetEndOfFile": (1, set_end_of_file),
            # registry: behave as if nothing is configured
            "RegOpenKeyExA": (5, reg_missing),
            "RegCreateKeyExA": (9, reg_missing),
            "RegQueryValueExA": (6, reg_missing),
            "RegCloseKey": (1, zero),
        }
