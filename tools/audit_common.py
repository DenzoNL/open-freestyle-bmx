"""Shared helpers for the static audits (argcount_audit, selfref_audit, extend_audit).

Not a command. Loads ported functions (BMX_ADDR) with their bodies, the original per-function
disassembly bundles, and initial values of the exe's data. Import with
    sys.path.insert(0, str(Path(__file__).resolve().parent)); import audit_common
because `python -I` drops the script directory from sys.path.
"""
import json
import os
import re
from pathlib import Path

INSN = re.compile(r"^([0-9a-f]{8})\s+(.*)$")
ANNOT = re.compile(r"^BMX_ADDR\((0x[0-9a-fA-F]+)\)\s*\n(?:static\s+)?(?:__attribute__\(\(.*?\)\)\s*)*((?:static\s+)?[\w\s\*]+?)\b(\w+)\s*\(", re.M)


def strip_c(text):
    """Blank out comments and string/char literals, keeping length and newlines."""
    out = []
    i, n = 0, len(text)
    while i < n:
        c = text[i]
        if text.startswith("/*", i):
            j = text.find("*/", i + 2)
            j = n if j < 0 else j + 2
            out.append(re.sub(r"[^\n]", " ", text[i:j]))
            i = j
        elif text.startswith("//", i):
            j = text.find("\n", i)
            j = n if j < 0 else j
            out.append(" " * (j - i))
            i = j
        elif c in "\"'":
            j = i + 1
            while j < n and text[j] != c:
                j += 2 if text[j] == "\\" else 1
            j = min(j + 1, n)
            out.append(c + " " * max(j - i - 2, 0) + (c if j - i >= 2 else ""))
            i = j
        else:
            out.append(c)
            i += 1
    return "".join(out)


def match_close(text, i, open_c, close_c):
    depth = 0
    for j in range(i, len(text)):
        if text[j] == open_c:
            depth += 1
        elif text[j] == close_c:
            depth -= 1
            if depth == 0:
                return j
    return -1


def load_ported(repo):
    """addr -> dict(name, path, ret, params, body, line) for every BMX_ADDR function with a body."""
    ported = {}
    for path in sorted((repo / "src").rglob("*.c")):
        raw = path.read_text(errors="replace")
        text = strip_c(raw)
        for m in ANNOT.finditer(text):
            po = m.end() - 1
            pc = match_close(text, po, "(", ")")
            if pc < 0:
                continue
            k = pc + 1
            while k < len(text) and text[k].isspace():
                k += 1
            if k >= len(text) or text[k] != "{":
                continue
            bc = match_close(text, k, "{", "}")
            ported[int(m.group(1), 16)] = dict(
                name=m.group(3),
                path=str(path.relative_to(repo)).replace("\\", "/"),
                ret=m.group(2).replace("static", "").strip(),
                params=text[po + 1 : pc],
                body=text[k : bc + 1],
                body_raw=raw[k : bc + 1],
                line=text.count("\n", 0, m.start()) + 2,
                pre=raw[max(0, m.start() - 400) : m.start()],  # comments just above the BMX_ADDR line
            )
    return ported


def split_args(s):
    """Split a parameter or argument list at top-level commas."""
    s = s.strip()
    if s in ("", "void"):
        return []
    out, depth, cur = [], 0, ""
    for ch in s:
        if ch in "([{":
            depth += 1
        elif ch in ")]}":
            depth -= 1
        if ch == "," and depth == 0:
            out.append(cur.strip())
            cur = ""
        else:
            cur += ch
    out.append(cur.strip())
    return out


def work_dir(repo):
    return Path(os.environ.get("BMX_WORK", repo / "work"))


def parse_disasm(bundle):
    res = []
    for x in bundle.get("disasm", "").splitlines():
        m = INSN.match(x.strip())
        if m:
            res.append((int(m.group(1), 16), m.group(2).strip()))
    return res


def load_bundles(repo):
    """addr -> (bundle, [(addr, 'MNEMONIC operands')])"""
    work = work_dir(repo)
    out = {}
    for d in ("bundles", "bundles-rw"):
        for f in sorted((work / d).glob("*.json")):
            b = json.loads(f.read_text())
            out[int(b["addr"], 16)] = (b, parse_disasm(b))
    return out


class Exe:
    def __init__(self, path):
        import pefile

        self.pe = pefile.PE(str(path), fast_load=True)
        self.base = self.pe.OPTIONAL_HEADER.ImageBase

    def u8(self, va):
        v = self.u32(va)
        return None if v is None else v & 0xFF

    def s32(self, va):
        v = self.u32(va)
        return None if v is None else v - (1 << 32) if v >= 1 << 31 else v

    def u32(self, va):
        rva = va - self.base
        for s in self.pe.sections:
            if s.VirtualAddress <= rva < s.VirtualAddress + max(s.Misc_VirtualSize, s.SizeOfRawData):
                off = rva - s.VirtualAddress
                if off + 4 > s.SizeOfRawData:
                    return 0
                return int.from_bytes(s.get_data(s.VirtualAddress + off, 4), "little")
        return None


def exe_path(repo):
    return Path(os.environ.get("BMX_EXE", work_dir(repo) / "NgBMX.exe"))


def thunk_resolver(bundles, exe=None):
    """Function resolving `JMP addr` thunks (the 0x4010xx-0x4025xx table of the incremental link) to their target."""
    jump = {}
    for a, (_, insns) in bundles.items():
        if len(insns) == 1:
            m = re.match(r"^JMP\s+0x([0-9a-f]+)$", insns[0][1], re.I)
            if m:
                jump[a] = int(m.group(1), 16)

    def resolve(a):
        for _ in range(4):
            if a in jump:
                a = jump[a]
            elif exe is not None and a not in bundles and exe.u8(a) == 0xE9:  # JMP rel32 thunk that was not bundled
                a = (a + 5 + exe.s32(a + 1)) & 0xFFFFFFFF
            else:
                break
        return a

    return resolve
