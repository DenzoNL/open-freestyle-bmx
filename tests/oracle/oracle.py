#!/usr/bin/env python3
"""Differential test harness: original NgBMX.exe code vs our port, side by side in Unicorn.

Both images are mapped into one emulated 32-bit address space:
  - NgBMX.exe at its preferred base (0x400000), imports trapped to Python stubs
  - bmxtest.dll (ported code + tests/cases) at 0x10000000, linked -fixed

bmxtest.dll exports `dt_cases`, a null-terminated table of DtCase (see tests/cases/dt.h). For every case
and iteration the harness: restores a baseline snapshot, runs the case's setup() to build arguments,
snapshots again, runs the ORIGINAL function, captures registers + all writable memory, rewinds, runs OUR
function, captures again, and compares. Heap calls are intercepted at the exe's CRT entry points and
served by a deterministic bump allocator, so allocation addresses match between the two runs.
"""
import argparse
import hashlib
import math
import struct
import sys
import time
import zlib
from dataclasses import dataclass, field

import os
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))  # -I drops the script dir

import pefile
from unicorn import UC_ARCH_X86, UC_HOOK_CODE, UC_MODE_32, UC_PROT_ALL, UC_PROT_EXEC, UC_PROT_READ, Uc, UcError
from unicorn.x86_const import (
    UC_X86_REG_EAX,
    UC_X86_REG_EDX,
    UC_X86_REG_EIP,
    UC_X86_REG_ESP,
    UC_X86_REG_FP0,
    UC_X86_REG_FPCW,
    UC_X86_REG_FPSW,
    UC_X86_REG_FPTAG,
)

STACK_BASE, STACK_SIZE = 0x00100000, 0x00100000
HEAP_BASE, HEAP_SIZE = 0x20000000, 0x04000000
TRAP_BASE, TRAP_SIZE = 0x7FF00000, 0x00010000
RET_MAGIC = TRAP_BASE + TRAP_SIZE - 0x10
HYPER_FILL = TRAP_BASE + TRAP_SIZE - 0x20  # dt_fill(); keep in sync with tests/cases/dt.c

CRT_ENTRY, WINMAIN = 0x004A51F9, 0x0048A460

# MSVC6 CRT heap entry points inside NgBMX.exe
CRT_MALLOC, CRT_FREE, CRT_CALLOC, CRT_REALLOC = 0x004A50D2, 0x004A4D7F, 0x004AD2E4, 0x0051D67E

# Exe CRT routines run natively by the harness (see Machine.accelerate); identical semantics.
EXE_ACCEL = {0x004ACCC0: "memset"}

# zdebugFail: the engine's fatal-assert handler. Reaching it ends the run as an "abort" event that is
# compared between both sides (message, file, line) instead of popping a message box.
ABORT_FUNCS = (0x0047ED10,)

# DtCase flags (keep in sync with tests/cases/dt.h)
RET_MASK = 0x0F
RET_VOID, RET_INT, RET_FLOAT, RET_DOUBLE, RET_INT64, RET_INT16, RET_INT8 = 0, 1, 2, 3, 4, 5, 6
F_FLOAT_MEM = 0x10  # compare differing aligned dwords in memory as floats (ULP tolerance)
F_NEEDS_ASSETS = 0x40  # case reads original game files


@dataclass
class Case:
    name: str
    orig: int
    impl: int
    argc: int
    flags: int
    setup: int
    iters: int
    ulps: int


@dataclass
class Capture:
    eax: int = 0
    edx: int = 0
    st0: float = 0.0
    abort: tuple = None
    files: tuple = None
    mem: dict = field(default_factory=dict)


def f80_to_float(mant, exp):
    sign = -1.0 if exp & 0x8000 else 1.0
    e = exp & 0x7FFF
    if e == 0 and mant == 0:
        return 0.0 * sign
    if e == 0x7FFF:
        return sign * (math.inf if (mant << 1) & ((1 << 64) - 1) == 0 else math.nan)
    return sign * math.ldexp(mant, e - 16383 - 63)


def f32(bits):
    return struct.unpack("<f", struct.pack("<I", bits))[0]


def ulp_dist32(a, b):
    def key(x):
        return x if x < 0x80000000 else 0x80000000 - (x & 0x7FFFFFFF)

    return abs(key(a) - key(b))


class Machine:
    stack_bottom, stack_top = STACK_BASE, STACK_BASE + STACK_SIZE

    def __init__(self, exe_path, dll_path, game_root=None):
        self.uc = Uc(UC_ARCH_X86, UC_MODE_32)
        self.writable = []  # (base, size)
        self.trap_names = {}
        self.unexpected_import = None
        self.heap_ptr = HEAP_BASE
        self.exe = self._load(exe_path, trap_imports=True)
        self.dll = self._load(dll_path, trap_imports=False)
        self.uc.mem_map(STACK_BASE, STACK_SIZE, UC_PROT_ALL)
        self.uc.mem_map(HEAP_BASE, HEAP_SIZE, UC_PROT_ALL)
        self.uc.mem_map(TRAP_BASE, TRAP_SIZE, UC_PROT_ALL)
        self.uc.mem_write(TRAP_BASE, b"\xc3" * TRAP_SIZE)
        self.writable.append((HEAP_BASE, None))  # sized dynamically by heap_ptr
        self.uc.hook_add(UC_HOOK_CODE, self._on_trap, begin=TRAP_BASE, end=TRAP_BASE + TRAP_SIZE - 1)
        for at, size in self._protect_later:
            self.uc.mem_protect(at, size, UC_PROT_READ | UC_PROT_EXEC)
        for addr in (CRT_MALLOC, CRT_FREE, CRT_CALLOC, CRT_REALLOC):
            self.uc.hook_add(UC_HOOK_CODE, self._on_crt, begin=addr, end=addr)
        for addr in ABORT_FUNCS:
            self.uc.hook_add(UC_HOOK_CODE, self._on_abort, begin=addr, end=addr)
        self.sizes = {}
        self.abort = None
        from win32 import Win32

        self.win32 = Win32(self, game_root)
        self.accel_targets = dict(EXE_ACCEL)
        self.accel_hooks = []
        import random

        self.fill_rng = random.Random(0)

    def _load(self, path, trap_imports):
        pe = pefile.PE(path)
        base = pe.OPTIONAL_HEADER.ImageBase
        size = (pe.OPTIONAL_HEADER.SizeOfImage + 0xFFF) & ~0xFFF
        self.uc.mem_map(base, size, UC_PROT_ALL)
        self.uc.mem_write(base, pe.header)
        protect = []
        for s in pe.sections:
            data = s.get_data()
            self.uc.mem_write(base + s.VirtualAddress, data)
            if not s.Characteristics & 0x80000000:  # not IMAGE_SCN_MEM_WRITE: code, .rdata, headers
                vsize = (max(s.Misc_VirtualSize, s.SizeOfRawData) + 0xFFF) & ~0xFFF
                protect.append((base + s.VirtualAddress, vsize))
            if s.Characteristics & 0x80000000:  # IMAGE_SCN_MEM_WRITE
                vsize = max(s.Misc_VirtualSize, s.SizeOfRawData)
                self.writable.append((base + s.VirtualAddress, (vsize + 0xFFF) & ~0xFFF))
        # Guest writes to code or read-only data fault (a test that pokes a constant would otherwise
        # corrupt every later case, since only writable sections are snapshotted). The harness itself
        # still patches code (dt_stub) through host-side writes. Import tables are patched below first.
        self._protect_later = getattr(self, "_protect_later", []) + protect
        imports = getattr(pe, "DIRECTORY_ENTRY_IMPORT", [])
        if imports and not trap_imports:
            names = [e.dll.decode() for e in imports]
            raise SystemExit(f"{path}: test DLL must not import anything, found {names}")
        slot = TRAP_BASE + len(self.trap_names) * 8
        for e in imports:
            for imp in e.imports:
                name = f"{e.dll.decode()}!{(imp.name or b'ord').decode()}"
                self.trap_names[slot] = name
                self.uc.mem_write(imp.address, struct.pack("<I", slot))
                slot += 8
        pe.close()
        return pe

    # --- heap -------------------------------------------------------------------------------
    def alloc(self, n):
        p = self.heap_ptr
        self.heap_ptr += (max(n, 1) + 15) & ~15
        if self.heap_ptr > HEAP_BASE + HEAP_SIZE:
            raise RuntimeError("emulated heap exhausted")
        self.sizes[p] = n
        return p

    def _arg(self, i):
        esp = self.uc.reg_read(UC_X86_REG_ESP)
        return struct.unpack("<I", self.uc.mem_read(esp + 4 + 4 * i, 4))[0]

    def _ret(self, value):
        esp = self.uc.reg_read(UC_X86_REG_ESP)
        ret = struct.unpack("<I", self.uc.mem_read(esp, 4))[0]
        self.uc.reg_write(UC_X86_REG_EAX, value & 0xFFFFFFFF)
        self.uc.reg_write(UC_X86_REG_ESP, esp + 4)  # cdecl: caller pops args
        self.uc.reg_write(UC_X86_REG_EIP, ret)

    def _on_crt(self, uc, addr, size, _):
        if addr == CRT_MALLOC:
            self._ret(self.alloc(self._arg(0)))
        elif addr == CRT_CALLOC:
            n = self._arg(0) * self._arg(1)
            p = self.alloc(n)
            uc.mem_write(p, b"\0" * n)
            self._ret(p)
        elif addr == CRT_FREE:
            self._ret(0)
        elif addr == CRT_REALLOC:
            old, n = self._arg(0), self._arg(1)
            p = self.alloc(n)
            if old:
                uc.mem_write(p, bytes(uc.mem_read(old, min(n, self.sizes.get(old, 0)))))
            self._ret(p)

    def accelerate(self, exclude=()):
        """Run memset/memcpy/memmove natively, except at the addresses under test."""
        for h in self.accel_hooks:
            self.uc.hook_del(h)
        self.accel_hooks = [
            self.uc.hook_add(UC_HOOK_CODE, self._on_accel, begin=a, end=a)
            for a in self.accel_targets
            if a not in exclude
        ]

    def _on_accel(self, uc, addr, size, _):
        name = self.accel_targets[addr]
        dst, b, n = self._arg(0), self._arg(1), self._arg(2)
        if name == "memset":
            uc.mem_write(dst, bytes([b & 0xFF]) * n)
        elif n:
            uc.mem_write(dst, bytes(uc.mem_read(b, n)))  # memcpy and memmove: read fully, then write
        self._ret(dst)

    def _on_abort(self, uc, addr, size, _):
        why, file, line = self._arg(0), self._arg(1), self._arg(2)
        self.abort = (self.read_cstr(why) if why else None, self.read_cstr(file) if file else None, line)
        uc.emu_stop()

    def _on_trap(self, uc, addr, size, _):
        if addr == RET_MAGIC:
            uc.emu_stop()
            return
        if addr == HYPER_FILL:
            p, n = self._arg(0), self._arg(1)
            uc.mem_write(p, self.fill_rng.randbytes(n))
            self._ret(0)
            return
        name = self.trap_names.get(addr, hex(addr))
        from win32 import ProcessExit

        try:
            if self.win32.call(name):
                return
        except ProcessExit as e:
            self.abort = ("exit", str(e), 0)
            uc.emu_stop()
            return
        self.unexpected_import = name
        uc.emu_stop()

    def crt_startup(self):
        """Run the exe's real MSVC6 startup (heap, stdio, argv, environment, initialisers) and stop
        on entry to WinMain, leaving a properly initialised process."""

        def on_winmain(uc, addr, size, _):
            uc.emu_stop()

        h = self.uc.hook_add(UC_HOOK_CODE, on_winmain, begin=WINMAIN, end=WINMAIN)
        sp = STACK_BASE + STACK_SIZE - 0x1000 - 4
        self.uc.mem_write(sp, struct.pack("<I", RET_MAGIC))
        self.uc.reg_write(UC_X86_REG_ESP, sp)
        self.uc.reg_write(UC_X86_REG_FPCW, 0x027F)
        try:
            self.uc.emu_start(CRT_ENTRY, RET_MAGIC, count=50_000_000)
        finally:
            self.uc.hook_del(h)
        if self.unexpected_import:
            raise RuntimeError(f"CRT startup called unemulated import {self.unexpected_import}")
        if self.uc.reg_read(UC_X86_REG_EIP) != WINMAIN:
            raise RuntimeError(f"CRT startup did not reach WinMain (eip={self.uc.reg_read(UC_X86_REG_EIP):#x})")

    # --- execution --------------------------------------------------------------------------
    def snapshot(self):
        regions = [(b, s if s is not None else self.heap_ptr - HEAP_BASE) for b, s in self.writable]
        return {
            "mem": {b: bytes(self.uc.mem_read(b, s)) for b, s in regions if s},
            "heap_ptr": self.heap_ptr,
            "sizes": dict(self.sizes),
            "win32": self.win32.snapshot(),
        }

    def restore(self, snap):
        for b, data in snap["mem"].items():
            self.uc.mem_write(b, data)
        # anything allocated after the snapshot is garbage now; zero it for determinism
        if self.heap_ptr > snap["heap_ptr"]:
            self.uc.mem_write(snap["heap_ptr"], b"\0" * (self.heap_ptr - snap["heap_ptr"]))
        self.heap_ptr = snap["heap_ptr"]
        self.sizes = dict(snap["sizes"])
        self.win32.restore(snap["win32"])

    def call(self, fn, args, budget=50_000_000):
        sp = STACK_BASE + STACK_SIZE - 0x1000
        frame = struct.pack("<I", RET_MAGIC) + b"".join(struct.pack("<I", a & 0xFFFFFFFF) for a in args)
        sp -= len(frame)
        self.uc.mem_write(sp, frame)
        self.uc.reg_write(UC_X86_REG_ESP, sp)
        self.uc.reg_write(UC_X86_REG_EAX, 0)
        self.uc.reg_write(UC_X86_REG_EDX, 0)
        self.uc.reg_write(UC_X86_REG_FPCW, 0x027F)  # Win32 default: 53-bit precision, all masked
        self.uc.reg_write(UC_X86_REG_FPSW, 0)
        self.uc.reg_write(UC_X86_REG_FPTAG, 0xFFFF)
        self.unexpected_import = None
        self.abort = None
        try:
            self.uc.emu_start(fn, RET_MAGIC, count=budget)
        except UcError as e:
            raise RuntimeError(f"emulation fault in {fn:#x}: {e}") from None
        if self.unexpected_import:
            raise RuntimeError(f"{fn:#x} called unemulated import {self.unexpected_import}")

    def st0(self):
        top = (self.uc.reg_read(UC_X86_REG_FPSW) >> 11) & 7
        mant, exp = self.uc.reg_read(UC_X86_REG_FP0 + top)
        return f80_to_float(mant, exp)

    def capture(self, flags):
        c = Capture(self.uc.reg_read(UC_X86_REG_EAX), self.uc.reg_read(UC_X86_REG_EDX))
        c.abort = self.abort
        c.files = (self.win32.state["overlay"], {h: f["pos"] for h, f in self.win32.state["files"].items()})
        if c.abort is None and flags & RET_MASK in (RET_FLOAT, RET_DOUBLE):
            c.st0 = self.st0()
        c.mem = self.snapshot()["mem"]
        return c

    def apply_stubs(self):
        """Patch `jmp stub` over every entry registered with dt_stub(); returns the bytes to restore.

        (Persistent per-address code hooks were tried instead and made runs about twice as slow.)
        """
        table, count_addr = self.stub_table
        count = self.read_u32(count_addr)
        if count > 128:
            raise RuntimeError(f"dt_stub: {count} stubs registered, at most 128 supported")
        saved = []
        for i in range(count):
            orig, ours, stub = struct.unpack("<3I", self.uc.mem_read(table + 12 * i, 12))
            ours = ours or getattr(self, "ports", {}).get(orig, 0)
            for at in (orig, ours):
                if at:
                    saved.append((at, bytes(self.uc.mem_read(at, 5))))
                    self.uc.mem_write(at, b"\xe9" + struct.pack("<i", stub - (at + 5)))
                    self.uc.ctl_remove_cache(at, at + 5)  # code translated by an earlier case is stale
        return saved

    def remove_stubs(self, saved):
        for at, data in reversed(saved):
            self.uc.mem_write(at, data)
            self.uc.ctl_remove_cache(at, at + len(data))

    def read_u32(self, addr):
        return struct.unpack("<I", self.uc.mem_read(addr, 4))[0]

    def read_cstr(self, addr):
        out = bytearray()
        while (b := self.uc.mem_read(addr + len(out), 1)[0]) != 0:
            out.append(b)
        return out.decode(errors="replace")


# DtCase layout (tests/cases/dt.h): name, orig, impl, argc, flags, setup, iters, ulps  (8 x u32)
# DtCtx layout: seed, args[8]
CASE_FMT = "<8I"


def load_cases(m, dll_path):
    pe = pefile.PE(dll_path)
    exports = {e.name.decode(): pe.OPTIONAL_HEADER.ImageBase + e.address for e in pe.DIRECTORY_ENTRY_EXPORT.symbols}
    pe.close()
    begin = m.read_u32(exports["dt_cases_begin"])
    end = m.read_u32(exports["dt_cases_end"])
    cases = []
    # cases are gathered from several objects; the linker may pad between them with zeros
    for at in range(begin, end, 32):
        raw = struct.unpack(CASE_FMT, m.uc.mem_read(at, 32))
        if raw[0]:
            cases.append(Case(m.read_cstr(raw[0]), *raw[1:]))
    init = []
    at = exports["dt_init"]
    while fn := m.read_u32(at):
        init.append(fn)
        at += 4
    m.stub_table = (exports["dt_stubs"], exports["dt_stub_count"])
    m.ports = {}
    if "dt_ports" in exports:
        table, count = m.read_u32(exports["dt_ports"]), m.read_u32(m.read_u32(exports["dt_port_count"]))
        for i in range(count):
            orig, ours = struct.unpack("<2I", m.uc.mem_read(table + 8 * i, 8))
            m.ports[orig] = ours
    if "dt_accel" in exports:
        names = ["memset", "memcpy", "memmove"]
        at = exports["dt_accel"]
        for name in names:
            fn = m.read_u32(at)
            if not fn:
                break
            m.accel_targets[fn] = name
            at += 4
    return cases, exports["dt_ctx"], init


def diff(case, a, b):
    problems = []
    if a.abort != b.abort:
        problems.append(f"assert orig={a.abort} ours={b.abort}")
    if a.files != b.files:
        problems.append(f"file state differs: orig={a.files!r:.200} ours={b.files!r:.200}")
    rk = case.flags & RET_MASK if a.abort is None else RET_VOID
    if rk == RET_INT16 and (a.eax & 0xFFFF) != (b.eax & 0xFFFF):
        problems.append(f"ax orig={a.eax & 0xFFFF:#x} ours={b.eax & 0xFFFF:#x}")
    if rk == RET_INT8 and (a.eax & 0xFF) != (b.eax & 0xFF):
        problems.append(f"al orig={a.eax & 0xFF:#x} ours={b.eax & 0xFF:#x}")
    if rk == RET_INT and a.eax != b.eax:
        problems.append(f"eax orig={a.eax:#x} ours={b.eax:#x}")
    if rk == RET_INT64 and (a.eax, a.edx) != (b.eax, b.edx):
        problems.append(f"edx:eax orig={a.edx:#x}:{a.eax:#x} ours={b.edx:#x}:{b.eax:#x}")
    if rk in (RET_FLOAT, RET_DOUBLE):
        x, y = a.st0, b.st0
        same = (math.isnan(x) and math.isnan(y)) or x == y
        if not same:
            fmt = "<f" if rk == RET_FLOAT else "<d"
            ix = struct.unpack("<I" if rk == RET_FLOAT else "<Q", struct.pack(fmt, x))[0]
            iy = struct.unpack("<I" if rk == RET_FLOAT else "<Q", struct.pack(fmt, y))[0]
            if rk != RET_FLOAT or ulp_dist32(ix, iy) > case.ulps:
                problems.append(f"st0 orig={x!r} ours={y!r}")
    for base in a.mem:
        ma, mb = a.mem[base], b.mem.get(base, b"")
        if ma == mb:
            continue
        n = min(len(ma), len(mb))
        if len(ma) != len(mb):
            problems.append(f"region {base:#x}: size orig={len(ma)} ours={len(mb)} (allocation mismatch)")
        bad = []
        off = 0
        while off < n:
            if ma[off] == mb[off]:
                off += 1
                continue
            w = off & ~3
            if case.flags & F_FLOAT_MEM:
                va, vb = struct.unpack_from("<I", ma, w)[0], struct.unpack_from("<I", mb, w)[0]
                if ulp_dist32(va, vb) <= case.ulps:
                    off = w + 4
                    continue
            bad.append(w)
            off = w + 4
        for w in bad[:8]:
            va, vb = struct.unpack_from("<I", ma, w)[0], struct.unpack_from("<I", mb, w)[0]
            problems.append(f"mem {base + w:#010x}: orig={va:#010x} ({f32(va):g}) ours={vb:#010x} ({f32(vb):g})")
        if len(bad) > 8:
            problems.append(f"... {len(bad) - 8} more differing dwords in region {base:#x}")
    return problems


def default_game_root(exe):
    """Install root: the exe's directory, or the unshield extraction root for .../Program_Files."""
    d = Path(exe).resolve().parent
    return d.parent if d.name.lower() == "program_files" else d


def run(exe, dll, only=None, iters_override=None, verbose=False, game_root=None, timing=False, shard=None):
    digest = hashlib.sha256(open(exe, "rb").read()).hexdigest()
    m = Machine(exe, dll, game_root or os.environ.get("BMX_GAME_DIR") or default_game_root(exe))
    cases, ctx, init = load_cases(m, dll)
    m.accelerate()
    m.crt_startup()
    for fn in init:
        m.call(fn, [])
    baseline = m.snapshot()
    have_assets = m.win32.game_root is not None and (m.win32.game_root / "Assets").is_dir()
    failed = skipped = 0
    durations = []
    selected = [c for c in cases if not only or any(o in c.name for o in only)]
    if shard is not None:
        index, count = shard
        selected = selected[index::count]
    for case in selected:
        if case.flags & F_NEEDS_ASSETS and not have_assets:
            skipped += 1
            if verbose:
                print(f"skip {case.name} (no game assets)")
            continue
        iters = iters_override or case.iters or 1
        errors = []
        if case.orig in m.accel_targets or case.impl in m.accel_targets:
            m.accelerate(exclude=(case.orig, case.impl))
        elif len(m.accel_hooks) != len(m.accel_targets):
            m.accelerate()
        started = time.perf_counter()
        for it in range(iters):
            m.restore(baseline)
            m.uc.mem_write(ctx, struct.pack("<I", it))
            m.fill_rng.seed(it * 2654435761 + zlib.crc32(case.name.encode()))
            try:
                if case.setup:
                    m.call(case.setup, [ctx])
                    if m.abort is not None:
                        raise RuntimeError(f"setup() hit an assert / exit: {m.abort} (fixture bug)")
                args = [m.read_u32(ctx + 4 + 4 * i) for i in range(case.argc)]
                saved = m.apply_stubs()
                try:
                    pre = m.snapshot()
                    m.call(case.orig, args)
                    a = m.capture(case.flags)
                    m.restore(pre)
                    m.call(case.impl, args)
                    b = m.capture(case.flags)
                finally:
                    m.remove_stubs(saved)
            except RuntimeError as e:
                errors.append(f"iter {it}: {e}")
                break
            p = diff(case, a, b)
            if p:
                errors.append(f"iter {it} args={[hex(x) for x in args]}: " + "; ".join(p))
                if len(errors) >= 3:
                    break
        durations.append((time.perf_counter() - started, case.name, iters))
        status = "FAIL" if errors else "ok"
        if errors:
            failed += 1
        if errors or verbose:
            print(f"{status:4} {case.name} ({iters} iters)")
        for e in errors:
            print(f"     {e}")
    total = len(selected)
    if shard is not None:
        for secs, name, iters in durations:
            print(f"SHARD-TIME {secs} {iters} {name}")
        print(f"SHARD-RESULT {failed} {skipped} {total} {digest}")
        return failed == 0
    if timing:
        for secs, name, iters in sorted(durations, reverse=True)[:25]:
            print(f"{secs:8.2f}s  {iters:5} iters  {1000 * secs / iters:7.2f} ms/iter  {name}")
    total -= skipped
    extra = f", {skipped} skipped without game assets" if skipped else ""
    print(f"{total - failed}/{total} cases passed{extra} (exe sha256 {digest[:12]})")
    return failed == 0


def main():
    ap = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    ap.add_argument("--exe", required=True, help="path to the original NgBMX.exe")
    ap.add_argument("--dll", required=True, help="path to build/hook/bmxtest.dll")
    ap.add_argument("-k", action="append", help="only run cases whose name contains this (repeatable)")
    ap.add_argument("-n", type=int, help="override iteration count")
    ap.add_argument("-v", action="store_true", help="list passing cases too")
    ap.add_argument("--timing", action="store_true", help="print the slowest cases")
    ap.add_argument("-j", type=int, default=0, help="worker processes (default: performance cores; 1 = serial)")
    ap.add_argument("--shard", help=argparse.SUPPRESS)
    a = ap.parse_args()
    if a.shard:
        i, n = (int(x) for x in a.shard.split("/"))
        sys.exit(0 if run(a.exe, a.dll, a.k, a.n, a.v, shard=(i, n)) else 1)
    jobs = a.j or default_jobs()
    if jobs <= 1:
        sys.exit(0 if run(a.exe, a.dll, a.k, a.n, a.v, timing=a.timing) else 1)
    sys.exit(0 if run_parallel(a, jobs) else 1)


def default_jobs():
    try:
        import subprocess

        return int(subprocess.run(["sysctl", "-n", "hw.perflevel0.physicalcpu"], capture_output=True).stdout)
    except (OSError, ValueError):
        return os.cpu_count() or 1


def run_parallel(a, jobs):
    """Run the cases in `jobs` worker processes (each its own emulator) and merge their reports."""
    import subprocess

    base = [sys.executable, "-I", __file__, "--exe", a.exe, "--dll", a.dll]
    for k in a.k or []:
        base += ["-k", k]
    if a.n:
        base += ["-n", str(a.n)]
    if a.v:
        base.append("-v")
    procs = [subprocess.Popen(base + ["--shard", f"{i}/{jobs}"], stdout=subprocess.PIPE, text=True) for i in range(jobs)]
    failed = skipped = total = 0
    digest = ""
    durations = []
    ok = True
    for p in procs:
        out, _ = p.communicate()
        for line in out.splitlines():
            if line.startswith("SHARD-RESULT "):
                f, s_, t, digest = line.split()[1:]
                failed += int(f)
                skipped += int(s_)
                total += int(t)
            elif line.startswith("SHARD-TIME "):
                secs, iters, name = line.split(maxsplit=3)[1:]
                durations.append((float(secs), name, int(iters)))
            else:
                print(line)
        ok = ok and p.returncode == 0
    if a.timing:
        for secs, name, iters in sorted(durations, reverse=True)[:25]:
            print(f"{secs:8.2f}s  {iters:5} iters  {1000 * secs / iters:7.2f} ms/iter  {name}")
    total -= skipped
    extra = f", {skipped} skipped without game assets" if skipped else ""
    print(f"{total - failed}/{total} cases passed{extra} (exe sha256 {digest[:12]}, {jobs} workers)")
    return ok and failed == 0


if __name__ == "__main__":
    main()
