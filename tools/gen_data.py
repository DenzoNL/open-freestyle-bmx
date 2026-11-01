#!/usr/bin/env python3
"""The original's data as C source (docs/PORTING.md X1b, docs/NATIVE.md "The original's data in the source").

Usage:
  tools/gen_data.py [--build build/native32] [--exe work/NgBMX.exe]   generate src/data from the exe (once)
  tools/gen_data.py --check [--build build/native32]                   compare the built src/data with the exe

The native exe needs NgBMX.exe's initial data: the globals' values, the tables, the strings, the pointers between
them and into the code. This tool turned it into C once; the result is committed (src/data) and maintained like the
code, and the build does not need the exe. Re-running it overwrites src/data.

How:
- Every original data section becomes one C object (a "block": .rdata, .data's initialised part and its zero tail,
  the import slots, .data1, _rwdseg, and the dinput.lib tables the linker put into .text) whose struct keeps the
  original's exact layout. The code relies on globals being next to each other (SAVE.DAT copies one range of
  them, byte-array views span many globals, tables are declared smaller than they are), so the blocks are not cut
  into separate symbols; ORIG_VAR(addr) is the address's place in its block (src/data/blocks.h).
- The members are the ORIG_VAR globals of the source, typed as the source declares them (the type's layout from the
  compiler: each declaring file is compiled with a probe that declares a pointer to the type; the debug metadata of
  the LLVM IR gives every field's offset and type). Of overlapping declarations the outermost one that contains the
  others as its fields wins; one that spans several others without containing them (a view) stays an alias. A
  declaration the data files cannot see (a type local to a .c file) gets a same-layout mirror of standard types.
- The bytes between the globals become members too, split where the original's code or a pointer refers to an
  address: pointers (p_<addr>), strings (s<Text>), plausible floats, other words or bytes (d_<addr>), zeros
  (pad_<addr>).
- Initialisers come from the exe's bytes: integers, floats with the shortest digits that give the same bits,
  strings as literals, pointers (from the exe's relocations) as &block.member.field expressions or, into the code,
  as the name of the function serving that address (tools/gen_origmap.py's map; incremental-link thunks followed;
  addresses nothing serves get a stub that reports them).
- The result is compiled with -fsyntax-only, and the compiler's complaints are fed back: casts where a pointer's type
  differs, declarations for functions no header declares, a declaration that does not sit at its offset becomes an
  alias. clang-format formats the files.
- --check compares the compiled objects with the exe: every byte, and every pointer's target (gate: tools/check.sh).
"""
import concurrent.futures
import json
import os
import re
import subprocess
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
from gen_origmap import macro_args  # noqa: E402

ROOT = Path(__file__).resolve().parent.parent
SRC = ROOT / "src"


# ---- ORIG_VAR sites ------------------------------------------------------------------------------------------

class Site:
    def __init__(self, path, addr, type_text, name, line):
        self.path, self.addr, self.type_text, self.name, self.line = path, addr, type_text, name, line
        self.type = None  # the probed type (Type)

    def __repr__(self):
        return f"{self.path}:{self.line} {self.addr:#010x} {self.type_text} {self.name}"


def scan_sites():
    """Every ORIG_VAR(addr, type, name) with a literal address in the native build's sources."""
    sites, odd = [], []
    for path in sorted(list(SRC.rglob("*.c")) + list(SRC.rglob("*.h"))):
        rel = path.relative_to(SRC).as_posix()
        if rel.startswith("hook/") or rel == "compat/orig.h":
            continue
        text = path.read_text()
        for m in re.finditer(r"\bORIG_VAR\(", text):
            args = macro_args(text, m.end() - 1)
            line = text.count("\n", 0, m.start()) + 1
            if args is None or len(args) != 3 or not re.fullmatch(r"0x[0-9a-fA-F]+", args[0]):
                odd.append((rel, line, args))
                continue
            sites.append(Site(rel, int(args[0], 16), " ".join(args[1].split()), args[2], line))
        # The two macros that wrap ORIG_VAR with a computed address: their uses name the addresses.
        if rel == "rw/d3dpipe.c":
            for m in re.finditer(r"\bK\((0x[0-9a-fA-F]+)\)", text):
                line = text.count("\n", 0, m.start()) + 1
                sites.append(Site(rel, int(m.group(1), 16), "const float", f"rwD3DK{m.group(1)}", line))
        if rel == "rw/clump.c":
            for m in re.finditer(r"\brwMatrixNormalErrorLimit\((\d)\)", text):
                line = text.count("\n", 0, m.start()) + 1
                n = int(m.group(1))
                sites.append(Site(rel, 0x0069B8B4 + 4 * n, "float", f"rwMatrixNormalLimit{n}", line))
    return sites, odd


# ---- types, from the compiler's debug metadata ---------------------------------------------------------------

class Type:
    """kind: base (enc 'int' / 'uint' / 'float' / 'bool' / 'char'), pointer (target: Type or None, func: bool),
    struct / union (members: [(name or None, offset in bytes, Type, bit offset, bit size)]), array (elem, count),
    enum, typedef (name, target)."""

    def __init__(self, kind, size, name=None, **kw):
        self.kind, self.size, self.name = kind, size, name
        self.__dict__.update(kw)

    def strip(self):
        """Without typedefs and qualifiers."""
        t = self
        while t is not None and t.kind in ("typedef", "const", "volatile"):
            t = t.target
        return t

    def __repr__(self):
        return f"<{self.kind} {self.name} {self.size}>"


DI_LINE = re.compile(r"^!(\d+) = (?:distinct )?!(\w+)\((.*)\)\s*$")
DI_LIST = re.compile(r"^!(\d+) = (?:distinct )?!\{(.*)\}\s*$")


def di_fields(body):
    out = {}
    for m in re.finditer(r'(\w+): ("(?:[^"\\]|\\.)*"|![0-9]+|null|[^,]+)', body):
        out[m.group(1)] = m.group(2).strip()
    return out


def parse_types(ll_text):
    """{probe variable name: Type} from an LLVM IR file's debug metadata."""
    nodes, lists = {}, {}
    for line in ll_text.splitlines():
        m = DI_LINE.match(line)
        if m:
            nodes[m.group(1)] = (m.group(2), di_fields(m.group(3)))
            continue
        m = DI_LIST.match(line)
        if m:
            lists[m.group(1)] = [x.strip().lstrip("!") for x in m.group(2).split(",") if x.strip()]
    cache = {}

    def ref(v):
        return None if v in (None, "null") else v.lstrip("!")

    def name_of(f):
        return f["name"].strip('"') if "name" in f else None

    def build(nid):
        if nid is None:
            return None
        if nid in cache:
            return cache[nid]
        kind, f = nodes[nid]
        size = int(f.get("size", "0")) // 8
        if kind == "DIBasicType":
            enc = f.get("encoding", "")
            e = {"DW_ATE_float": "float", "DW_ATE_signed": "int", "DW_ATE_unsigned": "uint",
                 "DW_ATE_signed_char": "char", "DW_ATE_unsigned_char": "uchar", "DW_ATE_boolean": "bool"}[enc]
            t = Type("base", size, name_of(f), enc=e)
            cache[nid] = t
            return t
        if kind == "DISubroutineType":
            t = Type("function", 0)
            cache[nid] = t
            return t
        if kind == "DIDerivedType":
            tag = f["tag"]
            if tag == "DW_TAG_pointer_type":
                t = Type("pointer", 4)
                cache[nid] = t
                t.target = build(ref(f.get("baseType")))
                return t
            if tag in ("DW_TAG_typedef", "DW_TAG_const_type", "DW_TAG_volatile_type", "DW_TAG_restrict_type"):
                k = {"DW_TAG_typedef": "typedef", "DW_TAG_const_type": "const", "DW_TAG_volatile_type": "volatile",
                     "DW_TAG_restrict_type": "const"}[tag]
                t = Type(k, 0, name_of(f))
                cache[nid] = t
                t.target = build(ref(f.get("baseType")))
                t.size = t.target.size if t.target else 0
                return t
            raise ValueError(f"derived {tag}")
        if kind == "DICompositeType":
            tag = f["tag"]
            if tag == "DW_TAG_array_type":
                t = Type("array", size)
                cache[nid] = t
                t.elem = build(ref(f.get("baseType")))
                dims = []
                for sub in lists[ref(f["elements"])]:
                    sk, sf = nodes[sub]
                    dims.append(int(sf.get("count", "-1")))
                # Multi-dimensional: nest from the innermost.
                inner = t.elem
                for d in reversed(dims[1:]):
                    inner = Type("array", inner.size * d, elem=inner, count=d)
                t.elem, t.count = inner, dims[0]
                if t.count < 0:
                    t.count = 0
                return t
            if tag in ("DW_TAG_structure_type", "DW_TAG_union_type"):
                t = Type("struct" if tag == "DW_TAG_structure_type" else "union", size, name_of(f), members=[])
                cache[nid] = t
                if "DIFlagFwdDecl" in f.get("flags", ""):
                    t.incomplete = True
                    return t
                for mid in lists.get(ref(f.get("elements")), []):
                    mk, mf = nodes[mid]
                    if mf.get("tag") != "DW_TAG_member":
                        continue
                    mt = build(ref(mf.get("baseType")))
                    bits = int(mf.get("offset", "0"))
                    if "DIFlagBitField" in mf.get("flags", ""):
                        storage = int(mf.get("extraData", "i64 0").split()[-1])
                        t.members.append((name_of(mf), storage // 8, mt, bits - storage, int(mf["size"])))
                    else:
                        t.members.append((name_of(mf), bits // 8, mt, None, None))
                return t
            if tag == "DW_TAG_enumeration_type":
                t = Type("enum", size, name_of(f))
                cache[nid] = t
                t.target = build(ref(f.get("baseType")))
                return t
            raise ValueError(f"composite {tag}")
        raise ValueError(f"node {kind}")

    out = {}
    for nid, (kind, f) in nodes.items():
        if kind == "DIGlobalVariable" and (name_of(f) or "").startswith("bmxprobe_"):
            ptr = build(ref(f["type"]))
            out[name_of(f)] = ptr.target
    return out


def compile_commands(build):
    cc = json.loads((build / "compile_commands.json").read_text())
    out = {}
    for e in cc:
        out[Path(e["file"]).resolve()] = e["command"]
    return out


def probe_command(cmd, probe_c, out_ll):
    """The TU's command line with the probe as input: no optimisation, DWARF types, IR output."""
    args = cmd.split(" ")
    keep, skip = [], False
    for a in args:
        if skip:
            skip = False
            continue
        if a in ("-o", "-c"):
            skip = True
            continue
        if a in ("-Werror", "-g") or a.startswith("-O"):
            continue
        keep.append(a)
    return keep + ["-O0", "-gdwarf-4", "-w", "-S", "-emit-llvm", "-o", str(out_ll), str(probe_c)]


def probe(sites, build, work):
    """Gives every site its Type: one probe TU per source file, which includes that file and declares a pointer to
    each site's type after it (header sites are probed in a .c file that includes the header)."""
    cmds = compile_commands(build)
    units = {}
    # Header -> the compiled .c files that include it, directly or through other headers.
    direct = {}
    for path in list(SRC.rglob("*.c")) + list(SRC.rglob("*.h")):
        rel = path.relative_to(SRC).as_posix()
        for inc in re.findall(r'^#include "([^"]+)"', path.read_text(), re.M):
            for base in (path.parent, SRC):
                if (base / inc).exists():
                    hdr = (base / inc).resolve().relative_to(SRC.resolve()).as_posix()
                    direct.setdefault(rel, set()).add(hdr)
                    break
    includers = {}
    for path in cmds:
        try:
            rel = path.relative_to(SRC.resolve()).as_posix()
        except ValueError:
            continue
        seen, todo, depth = set(), [(rel, 0)], {}
        while todo:
            f, d = todo.pop()
            for h in direct.get(f, ()):
                if h not in seen:
                    seen.add(h)
                    depth[h] = d
                    todo.append((h, d + 1))
        for h in seen:
            includers.setdefault(h, []).append((depth[h], rel))
    for i, s in enumerate(sites):
        if s.path.endswith(".c"):
            tu = s.path
        else:
            cands = includers.get(s.path, [])
            stem = Path(s.path).stem
            cands = sorted(cands, key=lambda c: (Path(c[1]).stem != stem, c[0], c[1]))
            if not cands:
                sys.exit(f"{s}: no translation unit includes {s.path}")
            tu = cands[0][1]
        if (SRC / tu).resolve() not in cmds:
            sys.exit(f"{s}: {tu} is not compiled in {build}")
        units.setdefault(tu, []).append((i, s))
    work.mkdir(parents=True, exist_ok=True)
    jobs = []
    for n, (tu, members) in enumerate(sorted(units.items())):
        probe_c = work / f"probe{n:03d}.c"
        lines = [f'#include "{(SRC / tu).as_posix()}"']
        for i, s in members:
            lines.append(f"__typeof__({s.type_text}) *bmxprobe_{i};")
        text = "\n".join(lines) + "\n"
        if not probe_c.exists() or probe_c.read_text() != text:
            probe_c.write_text(text)
        out_ll = work / f"probe{n:03d}.ll"
        jobs.append((probe_command(cmds[(SRC / tu).resolve()], probe_c, out_ll), probe_c, out_ll))

    def run(job):
        cmd, probe_c, out_ll = job
        flags = getattr(subprocess, "BELOW_NORMAL_PRIORITY_CLASS", 0)
        r = subprocess.run(cmd, capture_output=True, text=True, creationflags=flags)
        return None if r.returncode == 0 else f"{probe_c}: {r.stderr[-2000:]}"

    with concurrent.futures.ThreadPoolExecutor(max_workers=int(os.environ.get("BMX_JOBS", "3"))) as ex:
        errors = [e for e in ex.map(run, jobs) if e]
    if errors:
        sys.exit("\n".join(errors))
    for cmd, probe_c, out_ll in jobs:
        types = parse_types(out_ll.read_text())
        for name, t in types.items():
            sites[int(name.split("_")[1])].type = t
    missing = [s for s in sites if s.type is None]
    if missing:
        sys.exit("no type for: " + ", ".join(map(str, missing[:10])))


# ---- the original exe --------------------------------------------------------------------------------------------

class Exe:
    """The original's sections (virtual sizes, zero tails included) and its relocations: every absolute address the
    linker wrote into the image, by location."""

    def __init__(self, path):
        import pefile

        pe = pefile.PE(str(path), fast_load=True)
        base = pe.OPTIONAL_HEADER.ImageBase
        self.pe = pe
        self.sections = []  # (name, lo, hi, is code, bytes)
        for s in pe.sections:
            lo = base + s.VirtualAddress
            raw = s.get_data()[: s.Misc_VirtualSize]
            raw = raw + bytes(s.Misc_VirtualSize - len(raw))
            self.sections.append((s.Name.rstrip(b"\0").decode(), lo, lo + s.Misc_VirtualSize,
                                  bool(s.Characteristics & 0x20), raw))
        self.relocs = {}
        pe.parse_data_directories(directories=[pefile.DIRECTORY_ENTRY["IMAGE_DIRECTORY_ENTRY_BASERELOC"],
                                               pefile.DIRECTORY_ENTRY["IMAGE_DIRECTORY_ENTRY_IMPORT"]])
        for block in pe.DIRECTORY_ENTRY_BASERELOC:
            for e in block.entries:
                if e.type == 3:
                    self.relocs[base + e.rva] = None
        for at in self.relocs:
            self.relocs[at] = self.u32(at)
        # Import slots: address -> (dll, name).
        self.imports = {}
        for d in pe.DIRECTORY_ENTRY_IMPORT:
            for imp in d.imports:
                self.imports[imp.address] = (d.dll.decode(), imp.name.decode() if imp.name else f"#{imp.ordinal}")

    def section(self, a):
        for sec in self.sections:
            if sec[1] <= a < sec[2]:
                return sec
        return None

    def read(self, a, n):
        sec = self.section(a)
        if sec is None or a + n > sec[2]:
            raise ValueError(f"{a:#x}+{n} is outside the exe's sections")
        return sec[4][a - sec[1]:a - sec[1] + n]

    def u32(self, a):
        return int.from_bytes(self.read(a, 4), "little")

    def is_code(self, a):
        sec = self.section(a)
        return sec is not None and sec[3]


# ---- blocks: the original's data ranges, each one C object with the original's exact layout ----------------------

class Block:
    def __init__(self, name, lo, hi, kind, file, what, const=False):
        self.name, self.lo, self.hi, self.kind, self.file, self.what, self.const = name, lo, hi, kind, file, what, const
        self.tag = "Bmx" + name[3:]  # bmxData -> struct BmxData
        self.members = []

    def __contains__(self, a):
        return self.lo <= a < self.hi


# Thunk table of the incremental linker (`jmp rel32`; a pointer to one stands for its target) and the dinput.lib
# object tables the linker put into .text (data that the input formats in .rdata point at).
THUNKS = (0x00401005, 0x00402617)
INPUT_TABLES = (0x004AFC60, 0x004B0F20)


def make_blocks(exe, split):
    """The blocks; each starts on a page like its section did (pointers into them keep the original's low 12 bits, as
    they did in X1a's image), except the input tables, which only the real DirectInput reads."""
    data_end = next(s for s in exe.sections if s[0] == ".data")[2]
    iat = exe.pe.OPTIONAL_HEADER.DATA_DIRECTORY[12]
    iat_lo = 0x400000 + iat.VirtualAddress
    blocks = [
        Block("bmxRdata", 0x0054D000, 0x00550BD8, "init", "rdata.c", "the original's .rdata", const=True),
        Block("bmxData", 0x00551000, split, "init", "data.c", "the original's .data, initialised part"),
        Block("bmxBss", split, data_end, "zero", "bss.c", "the original's .data, zero-initialised tail"),
        Block("bmxImports", 0x00699000, iat_lo + iat.Size, "imports", "imports.c",
              "the original's .idata up to the end of its import slots"),
        Block("bmxData1", 0x0069B000, 0x0069F086, "init", "data1.c", "the original's .data1"),
        Block("bmxRwBss", 0x006A0000, 0x006A0109, "zero", "rwbss.c", "the original's _rwdseg (RenderWare data)"),
        Block("bmxInputTables", INPUT_TABLES[0], INPUT_TABLES[1], "init", "inputtables.c",
              "dinput.lib's object tables, which the linker put into the original's .text", const=True),
    ]
    for b in blocks:
        b.iat = iat_lo
        b.align = 16 if b.name == "bmxInputTables" else 4096
    return blocks


# ---- type helpers -------------------------------------------------------------------------------------------------

def align_of(t):
    t = t.strip()
    if t.kind == "base":
        return min(t.size, 8) or 1
    if t.kind in ("pointer",):
        return 4
    if t.kind == "enum":
        return t.size or 4
    if t.kind == "array":
        return align_of(t.elem)
    if t.kind in ("struct", "union"):
        return max([align_of(m[2]) for m in t.members] + [1])
    return 1


def same_type(a, b):
    if a is None or b is None:
        return False
    a, b = a.strip(), b.strip()
    if a.kind != b.kind or a.size != b.size:
        return False
    if a.kind in ("struct", "union", "enum"):
        return a.name == b.name and (a.name is not None or a is b)
    if a.kind == "base":
        return a.enc == b.enc
    if a.kind == "array":
        return a.count == b.count and same_type(a.elem, b.elem)
    return a.kind == "pointer"


def subobject(t, off, size):
    """Is there an object of `size` bytes starting at `off` inside t (t itself, a member, an element)?"""
    t = t.strip()
    if off == 0 and t.size == size:
        return True
    if t.kind == "array" and t.elem.size:
        i, r = divmod(off, t.elem.size)
        return i < t.count and subobject(t.elem, r, size)
    if t.kind in ("struct", "union"):
        return any(bits is None and mo <= off < mo + mt.size and subobject(mt, off - mo, size)
                   for _, mo, mt, bits, _ in t.members)
    return False


def leaf_at(t, off):
    """(leaf type, leaf start) of the scalar at `off` in t; for unions the pointer member when there is one."""
    t = t.strip()
    if t.kind == "array" and t.elem.size:
        i, r = divmod(off, t.elem.size)
        if i >= t.count:
            return None, None
        lt, ls = leaf_at(t.elem, r)
        return lt, None if ls is None else ls + i * t.elem.size
    if t.kind in ("struct", "union"):
        found = (None, None)
        for _, mo, mt, bits, _ in t.members:
            if bits is None and mo <= off < mo + mt.size:
                lt, ls = leaf_at(mt, off - mo)
                if ls is not None:
                    if lt.strip().kind == "pointer":
                        return lt, ls + mo
                    found = found if found[0] is not None else (lt, ls + mo)
        return found
    return t, 0


def ptr_capable(t, off):
    """Can the 4 bytes at `off` in t hold a relocated pointer (a pointer, or a 32-bit integer)?"""
    lt, ls = leaf_at(t, off)
    if lt is None or ls != off:
        return False
    lt = lt.strip()
    return lt.kind == "pointer" or lt.kind in ("base", "enum") and lt.size == 4 and getattr(lt, "enc", "") != "float"


STD_TYPEDEFS = {"int8_t", "uint8_t", "int16_t", "uint16_t", "int32_t", "uint32_t", "int64_t", "uint64_t", "size_t",
                "uintptr_t", "intptr_t"}


def spell(t, decl):
    """A C declaration of `decl` with a type of the same layout as t, using only standard types (a mirror of a type
    the data files cannot see)."""
    if t.kind == "typedef":
        if t.name in STD_TYPEDEFS:
            return f"{t.name} {decl}"
        return spell(t.target, decl)
    if t.kind in ("const", "volatile"):
        return spell(t.target, decl)
    if t.kind == "base":
        base = {("float", 4): "float", ("float", 8): "double", ("char", 1): "char", ("uchar", 1): "unsigned char",
                ("bool", 1): "_Bool"}.get((t.enc, t.size))
        if base is None:
            base = ("u" if t.enc == "uint" else "") + f"int{t.size * 8}_t"
        return f"{base} {decl}"
    if t.kind == "enum":
        return f"int{t.size * 8}_t {decl}"
    if t.kind == "pointer":
        return f"void *{decl}"
    if t.kind == "array":
        return spell(t.elem, f"{decl}[{t.count}]")
    if t.kind in ("struct", "union"):
        fields = []
        for n, mo, mt, bits, bsize in t.members:
            if bits is not None:
                fields.append(f"{spell(mt, '').strip()} {n or ''} : {bsize};")
            else:
                fields.append(spell(mt, n or "").strip() + ";")
        return f"{t.kind} {{ {' '.join(fields)} }} {decl}"
    raise ValueError(f"cannot spell {t}")


def array_of(enc, size, count):
    return Type("array", size * count, elem=Type("base", size, enc=enc), count=count)


# ---- initialisers -------------------------------------------------------------------------------------------------

class Unrepresentable(Exception):
    pass


class Ptr:
    """A pointer in the data, written as an expression; `key` identifies it for the compiler feedback."""

    def __init__(self, key, expr, int_leaf):
        self.key, self.expr, self.int_leaf = key, expr, int_leaf


def float_literal(raw, size):
    import struct

    fmt = "<f" if size == 4 else "<d"
    v = struct.unpack(fmt, raw)[0]
    suffix = "f" if size == 4 else ""
    if v != v:
        bits = int.from_bytes(raw, "little")
        return f"__builtin_nan{suffix}(\"{bits & ((1 << (23 if size == 4 else 52)) - 1):#x}\")"
    if v in (float("inf"), float("-inf")):
        return ("-" if v < 0 else "") + f"__builtin_inf{suffix}()"
    from decimal import Decimal

    text = repr(v)
    for p in range(1, 18):
        text = f"{v:.{p}g}"
        if struct.pack(fmt, float(text)) == raw:
            break
    # The shortest digits that give the same bits; written without an exponent where that stays short.
    if "e" in text and 1e-5 <= abs(v) < 1e16:
        text = format(Decimal(text), "f")
    if "e" not in text and "." not in text:
        text += ".0"
    return text + suffix


def c_string(raw):
    out = []
    for b in raw:
        c = chr(b)
        if c in "\\\"":
            out.append("\\" + c)
        elif 0x20 <= b < 0x7F:
            # A hex escape would swallow the hex digits after it: end the literal there.
            if out and out[-1].startswith("\\x") and c in "0123456789abcdefABCDEF":
                out.append('" "')
            out.append(c)
        elif c == "\n":
            out.append("\\n")
        elif c == "\t":
            out.append("\\t")
        elif c == "\r":
            out.append("\\r")
        else:
            out.append(f"\\x{b:02x}")
    return '"' + "".join(out) + '"'


def is_text(raw):
    """Bytes that read as one NUL-terminated string (only zeros after the NUL)."""
    if b"\0" not in raw:
        return False
    n = raw.index(b"\0")
    if n == 0 or any(raw[n:]):
        return False
    text = raw[:n]
    printable = sum(1 for b in text if 0x20 <= b < 0x7F or b in (9, 10, 13))
    return printable == len(text) and n >= 3


class Emitter:
    """Builds initialiser trees: ('s', text) | ('p', Ptr) | ('l', [(designator or None, node)])."""

    def __init__(self, gen):
        self.gen = gen

    def value(self, t, a, key):
        """Initialiser of a t at original address a; None when it is all zero."""
        exe, gen = self.gen.exe, self.gen
        s = t.strip()
        if s.kind in ("base", "enum", "pointer"):
            if a in exe.relocs:
                if s.size != 4 or getattr(s, "enc", "") == "float":
                    raise Unrepresentable(f"pointer at {a:#x} in a {s}")
                expr = gen.pointer_expr(exe.relocs[a], s.target if s.kind == "pointer" else None)
                return ("p", Ptr(key, expr, s.kind != "pointer"))
            for r in range(a - 3, a + s.size):
                if r in exe.relocs and r != a:
                    raise Unrepresentable(f"pointer at {r:#x} across a scalar at {a:#x}")
            raw = exe.read(a, s.size)
            if not any(raw):
                return None
            v = int.from_bytes(raw, "little")
            if s.kind == "pointer":
                return ("s", f"(void *){v:#x}")
            enc = getattr(s, "enc", "int")
            if enc == "float":
                return ("s", float_literal(raw, s.size))
            if enc == "bool":
                return ("s", str(v))
            signed = enc in ("int", "char") or s.kind == "enum"
            if signed and v >= 1 << (s.size * 8 - 1):
                v -= 1 << (s.size * 8)
            if v < 0:
                return ("s", str(v))
            if v >= 0x10000:
                return ("s", f"{v:#x}")
            return ("s", str(v))
        if s.kind == "array":
            return self.array(s, a, key)
        if s.kind == "struct":
            return self.struct(s, a, key)
        if s.kind == "union":
            return self.union(s, a, key)
        raise Unrepresentable(f"{s} at {a:#x}")

    def array(self, s, a, key):
        e, es = s.elem.strip(), s.elem.size
        if s.count == 0 or es == 0:
            return None
        raw = self.gen.exe.read(a, s.size)
        relocs = self.gen.relocs_in(a - 3, a + s.size)
        if not any(raw) and not relocs:
            return None
        if e.kind == "base" and es == 1 and e.enc in ("char", "uchar") and is_text(raw) and not relocs:
            return ("s", c_string(raw[:raw.index(b"\0")]))
        if e.kind == "base" and es == 1 and not relocs:
            # Bytes: one list, positional up to the last non-zero one.
            last = max(i for i, b in enumerate(raw) if b)
            return ("l", [(None, ("s", str(b if e.enc in ("uchar", "uint") or b < 0x80 else b - 256)))
                          for b in raw[:last + 1]])
        items = []
        for i in range(s.count):
            if not any(raw[i * es:(i + 1) * es]) and not self.gen.relocs_in(a + i * es - 3, a + (i + 1) * es):
                continue
            v = self.value(s.elem, a + i * es, key + (i,))
            if v is not None:
                items.append((i, v))
        if not items:
            return None
        last = items[-1][0]
        if len(items) * 2 < last + 1 and last + 1 > 8:
            return ("l", [(f"[{i}]", v) for i, v in items])
        full = dict(items)
        zero = ("l", []) if e.kind in ("struct", "union", "array") else ("s", "0")
        return ("l", [(None, full.get(i, zero)) for i in range(last + 1)])

    def check_padding(self, s, a, covered):
        raw = self.gen.exe.read(a, s.size)
        for i, b in enumerate(raw):
            if b and not covered[i]:
                raise Unrepresentable(f"non-zero padding at {a + i:#x} in {s}")

    def struct(self, s, a, key):
        items, covered = [], bytearray(s.size)
        for n, mo, mt, bits, bsize in s.members:
            ms = mt.strip()
            if bits is not None:
                unit = int.from_bytes(self.gen.exe.read(a + mo, ms.size), "little")
                v = (unit >> bits) & ((1 << bsize) - 1)
                for k in range(bits // 8, (bits + bsize + 7) // 8):
                    covered[mo + k] = 1
                if v:
                    if getattr(ms, "enc", "uint") == "int" and v >= 1 << (bsize - 1):
                        v -= 1 << bsize
                    items.append((f".{n}", ("s", str(v))))
                continue
            for k in range(mt.size):
                covered[mo + k] = 1
            v = self.value(mt, a + mo, key + (n or mo,))
            if v is None:
                continue
            if n is None:  # anonymous member: its designators reach through it
                if v[0] != "l":
                    raise Unrepresentable(f"anonymous scalar member in {s}")
                items.extend(v[1])
            else:
                items.append((f".{n}", v))
        self.check_padding(s, a, covered)
        return ("l", items) if items else None

    def union(self, s, a, key):
        raw = self.gen.exe.read(a, s.size)
        relocs = self.gen.relocs_in(a, a + s.size)
        if not any(raw) and not relocs:
            return None
        for n, mo, mt, bits, bsize in s.members:
            if bits is not None or mt.size != s.size:
                continue
            if any(not ptr_capable(mt, r - a) for r in relocs):
                continue
            try:
                v = self.value(mt, a, key + (n,))
            except Unrepresentable:
                continue
            if n is None:
                return ("l", v[1]) if v and v[0] == "l" else None
            return ("l", [(f".{n}", v)])
        raise Unrepresentable(f"no member of {s} at {a:#x} represents its bytes")


def has_ptr(node):
    if node is None:
        return False
    if node[0] == "p":
        return True
    if node[0] == "l":
        return any(has_ptr(v) for _, v in node[1])
    return False


def render(node, fixes, indent="    "):
    """The initialiser as lines, indented as clang-format would (it leaves very large initialisers as they are); a
    pointer gets a line of its own (the compiler's feedback reports by line). Returns (lines, {line index: pointer
    key})."""
    markers = []

    def text(node, ind):
        kind = node[0]
        if kind == "s":
            return node[1]
        if kind == "p":
            p = node[1]
            expr = p.expr
            fix = fixes.get(p.key)
            if p.int_leaf:
                expr = f"(uintptr_t){expr}" if re.fullmatch(r"[\w.\[\]&]+", expr) else f"(uintptr_t)({expr})"
            elif fix:
                expr = f"({fix}){expr}" if re.fullmatch(r"[\w.\[\]&]+", expr) else f"({fix})({expr})"
            markers.append(p.key)
            return f"\x01{len(markers) - 1}\x01" + expr
        items = node[1]
        inner = ind + "    "
        parts = [((d + " = ") if d else "") + text(v, inner) for d, v in items]
        nested = sum(1 for _, v in items if v[0] == "l" and v[1]) > 1
        if has_ptr(node) or nested:
            return "{\n" + "".join(f"{inner}{part},\n" for part in parts) + ind + "}"
        one = "{" + ", ".join(parts) + "}"
        if len(ind) + len(one) <= 100 or any("\n" in part for part in parts):
            return one
        # Long lists of scalars: packed into lines of up to 110 columns.
        lines, cur = [], inner
        for part in parts:
            if len(cur) + len(part) + 2 > 110 and cur.strip():
                lines.append(cur.rstrip())
                cur = inner
            cur += part + ", "
        lines.append(cur.rstrip().rstrip(","))
        return "{\n" + "\n".join(lines) + "\n" + ind + "}"

    lines, ptr_lines = [], {}
    for line in text(node, indent).split("\n"):
        m = re.search("\x01(\\d+)\x01", line)
        if m:
            ptr_lines[len(lines)] = markers[int(m.group(1))]
            line = re.sub("\x01\\d+\x01", "", line)
        lines.append(line)
    return lines, ptr_lines


# ---- the generator ------------------------------------------------------------------------------------------------

class Member:
    def __init__(self, addr, size, name, type, kind, site=None):
        self.addr, self.size, self.name, self.type, self.kind, self.site = addr, size, name, type, kind, site
        self.init = None  # initialiser tree
        self.aliases = []  # other ORIG_VAR names at or inside this member

    @property
    def end(self):
        return self.addr + self.size


def raw_addresses():
    """Original data addresses written as numbers in the code (BMX_ORIG(0x...)): starts of objects too."""
    out = set()
    for path in list(SRC.rglob("*.c")) + list(SRC.rglob("*.h")):
        if path.relative_to(SRC).as_posix().startswith(("hook/", "compat/orig.h", "data/")):
            continue
        for m in re.finditer(r"\bBMX_ORIG(?:_PTR)?\(\s*(0x[0-9a-fA-F]+)", path.read_text()):
            out.add(int(m.group(1), 16))
    return out


def string_name(raw, taken):
    """A readable member name from a string's text: sNgBmxExe, sVailTrails; None when the text has no word."""
    words = re.findall(r"[A-Za-z0-9]+", raw.decode("latin-1"))
    if not words:
        return None
    name = "s" + "".join(w[:1].upper() + w[1:] for w in words)
    name = name[:40]
    return None if name in taken else name


class Gen:
    def __init__(self, exe, sites, visible):
        import bisect

        self.bisect = bisect
        self.exe, self.sites, self.visible = exe, sites, visible
        self.reloc_locs = sorted(exe.relocs)
        self.targets = set(exe.relocs.values())
        self.raw = raw_addresses()
        from gen_origmap import build_map, IMPORT_THUNKS

        self.owners = {a: n for a, (n, kind) in build_map(SRC).items()}
        self.import_thunks = IMPORT_THUNKS
        self.demoted = set()  # (addr, name) of sites that cannot be members (their bytes become fillers)
        self.fixes = {}  # pointer key -> cast text
        self.declared = {}  # file -> {function names to declare}
        self.unserved = {}  # file -> {code addresses nothing serves}

    def relocs_in(self, lo, hi):
        i = self.bisect.bisect_left(self.reloc_locs, lo)
        j = self.bisect.bisect_left(self.reloc_locs, hi)
        return self.reloc_locs[i:j]

    # -- members --------------------------------------------------------------------------------------------------

    def score(self, s):
        t = s.type.strip()
        sc = 100 if id(s) in self.visible else 0
        sc += {"struct": 30, "union": 25, "pointer": 10}.get(t.kind, 0)
        if t.kind == "array":
            sc += 20 if t.elem.size > 1 else 0
        if t.kind == "base":
            sc += 10 if t.enc == "float" else 5
        sc += 2 * sum(1 for r in self.relocs_in(s.addr, s.addr + t.size) if ptr_capable(s.type, r - s.addr))
        sc += 1 if s.path.endswith(".h") else 0
        return sc

    def select(self, bsites):
        """The sites that become members: no site that another one overlaps without being a part of it (a view,
        such as a byte array over several globals), the outermost of nested ones, the best typed of equal ones."""
        cands = sorted((s for s in bsites if (s.addr, s.name) not in self.demoted), key=lambda s: s.type.size)
        accepted = []
        for s in cands:
            lo, hi = s.addr, s.addr + s.type.size
            view = False
            for o in accepted:
                olo, ohi = o.addr, o.addr + o.type.size
                if olo < hi and lo < ohi and (olo, ohi) != (lo, hi):
                    if not (lo <= olo and ohi <= hi and subobject(s.type, olo - lo, ohi - olo)):
                        view = True
                        break
            if not view:
                accepted.append(s)
        groups = {}
        for s in accepted:
            groups.setdefault((s.addr, s.type.size), []).append(s)
        best = sorted((max(g, key=lambda s: (self.score(s), s.name)) for g in groups.values()),
                      key=lambda s: (s.addr, -s.type.size))
        chosen, end = [], 0
        for s in best:
            if s.addr >= end:
                chosen.append(s)
                end = s.addr + s.type.size
        return chosen

    def site_ok(self, s, block):
        """Can the site be a member: aligned at its offset, every pointer in its bytes on a pointer-sized
        scalar."""
        if (s.addr - block.lo) % align_of(s.type):
            return f"misaligned for its type (alignment {align_of(s.type)})"
        if block.kind == "init":
            for r in self.relocs_in(s.addr, s.addr + s.type.size):
                if not ptr_capable(s.type, r - s.addr):
                    return f"pointer at {r:#x} not on a pointer or 32-bit field"
            for r in self.relocs_in(s.addr - 3, s.addr):
                if r + 4 > s.addr:
                    return f"pointer at {r:#x} runs into it"
            r = self.relocs_in(s.addr + s.type.size - 3, s.addr + s.type.size)
            if r:
                return f"pointer at {r[0]:#x} runs out of it"
        return None

    def plan_block(self, block):
        bsites = [s for s in self.sites if s.addr in block]
        for s in bsites:
            if s.addr + s.type.size > block.hi:
                sys.exit(f"{s}: runs past the end of {block.name} ({block.hi:#x})")
        if block.kind == "imports":
            # .idata from its start, so the slots keep their offsets in the page; only the slots are used.
            members = [Member(block.lo, block.iat - block.lo, f"pad_{block.lo:08x}", array_of("uchar", 1,
                                                                                         block.iat - block.lo), "zero")]
            for a in range(block.iat, block.hi, 4):
                dll, name = self.exe.imports.get(a, (None, None))
                mname = f"{dll.rsplit('.', 1)[0].upper()}_{name}" if dll else f"end_{a:08x}"
                members.append(Member(a, 4, mname, Type("pointer", 4, target=None), "slot"))
            block.members = members
            return
        while True:
            chosen = self.select(bsites)
            bad = [(s, why) for s in chosen for why in [self.site_ok(s, block)] if why]
            if not bad:
                break
            for s, why in bad:
                self.log.append(f"{s}: not a member, {why}")
                self.demoted.add((s.addr, s.name))
        members = []
        taken = set()
        for s in chosen:
            name = s.name if s.name not in taken else f"{s.name}_{s.addr:08x}"
            taken.add(name)
            members.append(Member(s.addr, s.type.size, name, s.type, "site", site=s))
        # The gaps: pointers, strings, other bytes; split where the code or a pointer refers to an address.
        fill, pos = [], block.lo
        for m in members + [None]:
            nxt = m.addr if m else block.hi
            if pos < nxt:
                fill.extend(self.fillers(block, pos, nxt, taken))
            pos = m.end if m else pos
        # A filler where a site starts that is not a member (a view over several members, or a declaration that
        # cannot hold the bytes) takes that site's name: the name the code uses for what starts there.
        by_addr = {}
        for s in sorted(bsites, key=lambda s: s.name):
            by_addr.setdefault(s.addr, []).append(s)
        for m in fill:
            for s in by_addr.get(m.addr, []):
                if s.name not in taken and m.kind != "zero" or s.name not in taken and m.size >= s.type.size:
                    taken.discard(m.name)
                    m.name = s.name
                    taken.add(s.name)
                    break
        for m in fill:
            if m.kind == "data":
                m.type = self.guess_floats(m)
        block.members = sorted(members + fill, key=lambda m: m.addr)
        for s in bsites:
            m = self.member_at(block, s.addr)
            if m.site is not s:
                m.aliases.append(s)

    def fillers(self, block, lo, hi, taken):
        exe = self.exe
        cuts = {lo, hi}
        for a in self.targets | self.raw:
            if lo < a < hi:
                cuts.add(a)
        ptrs = set(self.relocs_in(lo, hi)) if block.kind == "init" else set()
        for r in ptrs:
            cuts.update((r, r + 4))
        cuts = sorted(c for c in cuts if lo <= c <= hi)
        out = []
        for x, y in zip(cuts, cuts[1:]):
            referenced = x in self.targets or x in self.raw
            if x in ptrs:
                ptr = Type("pointer", 4, target=None)
                if (x - block.lo) % 4:
                    # A pointer off its natural alignment (in a packed table): a packed one-member struct.
                    ptr = Type("struct", 4, "BmxUnalignedPtr", members=[("p", 0, ptr, None, None)])
                out.append(Member(x, 4, f"p_{x:08x}", ptr, "ptr"))
                continue
            if block.kind == "zero" or not any(exe.read(x, y - x)):
                out.append(Member(x, y - x, f"{'d' if referenced else 'pad'}_{x:08x}", array_of("uchar", 1, y - x),
                                  "zero"))
                continue
            out.extend(self.split_text(block, x, y, referenced, taken))
        return out

    def split_text(self, block, lo, hi, referenced, taken):
        """Strings (NUL-terminated text, padded with zeros to the next 4 bytes) and the bytes between them."""
        exe, out, pos, data_from = self.exe, [], lo, lo
        raw = exe.read(lo, hi - lo)

        def flush(upto):
            if data_from < upto:
                n = upto - data_from
                if data_from % 4 == 0 and n % 4 == 0:
                    t = array_of("uint", 4, n // 4)
                else:
                    t = array_of("uchar", 1, n)
                out.append(Member(data_from, n, f"d_{data_from:08x}", t, "data"))

        while pos < hi:
            i = pos - lo
            nul = raw.find(b"\0", i)
            text = raw[i:nul] if nul >= 0 else b""
            ok = nul >= 0 and text and all(0x20 <= b < 0x7F or b in (9, 10, 13) for b in text) and \
                (len(text) >= 3 or pos == lo and referenced) and (pos % 4 == 0 or pos == lo)
            if not ok:
                pos += 1
                continue
            end = nul + 1
            while end < len(raw) and raw[end] == 0 and (lo + end) % 4:
                end += 1
            flush(pos)
            name = string_name(text, taken) or f"s_{pos:08x}"
            if name in taken:
                name = f"{name}_{pos:08x}"
            taken.add(name)
            out.append(Member(pos, end - i, name, array_of("char", 1, end - i), "str"))
            pos = lo + end
            data_from = pos
        flush(hi)
        # Bytes between strings that are only zeros: padding.
        for m in out:
            if m.kind == "data" and not any(exe.read(m.addr, m.size)):
                m.kind, m.name, m.type = "zero", f"pad_{m.addr:08x}", array_of("uchar", 1, m.size)
        return out

    def guess_floats(self, m):
        """A type for bytes nothing names: floats or doubles when every value reads as a plausible one (constants
        and tuning tables), a scalar for a single word, else the words or bytes as they are."""
        import struct

        raw = self.exe.read(m.addr, m.size)

        def plausible(text, digits):
            mant = re.sub(r"e.*$", "", text.rstrip("f")).replace("-", "").replace(".", "").lstrip("0")
            return len(mant) <= digits

        if m.size % 8 == 0 and m.addr % 8 == 0 and m.size >= 8:
            vals = [raw[i:i + 8] for i in range(0, m.size, 8)]
            if all(not any(v) or 0x3E0 <= (int.from_bytes(v, "little") >> 52) & 0x7FF <= 0x420 and
                   plausible(float_literal(v, 8), 10) for v in vals) and any(any(v) for v in vals):
                elem = Type("base", 8, "double", enc="float")
                return elem if len(vals) == 1 else Type("array", m.size, elem=elem, count=len(vals))
        if m.size % 4 == 0 and m.addr % 4 == 0:
            vals = [raw[i:i + 4] for i in range(0, m.size, 4)]
            if all(not any(v) or 0x60 <= (int.from_bytes(v, "little") >> 23) & 0xFF <= 0xA0 and
                   plausible(float_literal(v, 4), 7) for v in vals) and any(any(v) for v in vals):
                elem = Type("base", 4, "float", enc="float")
                return elem if len(vals) == 1 else Type("array", m.size, elem=elem, count=len(vals))
            if len(vals) == 1:
                return Type("base", 4, "uint32_t", enc="uint")
        return m.type

    def member_at(self, block, a):
        ms = block.members
        i = self.bisect.bisect_right([m.addr for m in ms], a) - 1
        return ms[i] if i >= 0 and ms[i].addr <= a < ms[i].end else None

    def locate(self, a):
        """(block, member, offset) of a data address; one past a member's end counts as that member's."""
        for b in self.blocks:
            if b.lo <= a <= b.hi:
                m = self.member_at(b, a) if a < b.hi else None
                if m is None and a > b.lo:
                    m = self.member_at(b, a - 1)
                if m is not None:
                    return b, m, a - m.addr
        return None, None, None

    # -- pointers -------------------------------------------------------------------------------------------------

    def code_name(self, v, file):
        if v in self.owners:
            return self.owners[v]
        if THUNKS[0] <= v < THUNKS[1] and self.exe.read(v, 1)[0] == 0xE9:
            dest = (v + 5 + int.from_bytes(self.exe.read(v + 1, 4), "little", signed=True)) & 0xFFFFFFFF
            if dest in self.owners:
                return self.owners[dest]
            v = dest
        self.unserved.setdefault(file, set()).add(v)
        return f"unserved_{v:08x}"

    def pointer_expr(self, v, want):
        if self.exe.is_code(v) and not INPUT_TABLES[0] <= v < INPUT_TABLES[1]:
            return self.code_name(v, self.current_file)
        b, m, off = self.locate(v)
        if b is None:
            raise SystemExit(f"pointer to {v:#x}: not in any data block")
        base = f"{b.name}.{m.name}"
        want = want.strip() if want is not None else None
        if want is not None and want.kind == "function":
            want = None
        if off == m.size:
            return f"(void *)((char *)&{base} + {off})" if m.size else f"&{base}"
        path = find_path(m.type, off, want)
        if path is not None:
            expr = f"&{base}{path}"
            if expr.endswith("[0]"):
                expr = expr[1:-3]
            return expr
        return f"(void *)((char *)&{base} + {off})"

    # -- output ---------------------------------------------------------------------------------------------------

    def declare(self, m, block):
        if m.type.kind == "struct" and m.type.name == "BmxUnalignedPtr":
            return f"BmxUnalignedPtr {m.name};"
        if m.kind != "site":
            return spell(m.type, m.name) + ";"
        s = m.site
        if id(s) not in self.visible:
            return spell(s.type, m.name) + ";  /* the type is local to " + s.path + " */"
        text = s.type_text
        if s.type.kind == "const" and not block.const and text.startswith("const "):
            text = text[6:]
        if re.fullmatch(r"[\w\s]+\**", text):
            return f"{text} {m.name};"
        return f"__typeof__({text}) {m.name};"

    def build_inits(self):
        em = Emitter(self)
        bad = []
        for b in self.blocks:
            if b.kind != "init":
                continue
            self.current_file = b.file
            for m in b.members:
                try:
                    m.init = em.value(m.type, m.addr, (b.name, m.name))
                except Unrepresentable as e:
                    if m.kind != "site":
                        raise
                    bad.append((m.site, str(e)))
        return bad

    def write(self):
        files = {"layout.h": (self.layout_text(), {})}
        for b in self.blocks:
            files[b.file] = self.block_text(b)
        return files

    def layout_text(self):
        """src/data/layout.h: every block's members (names, types, original addresses) and their offsets."""
        L = ["/*", " * The layout of the original's data blocks (docs/PORTING.md X1b): one struct per block, its members the",
             " * globals at their original offsets (asserted below), the bytes nothing names as d_/p_/pad_<address>",
             " * and strings as s<Text>. The values are in the block's file (src/data/<block>.c).",
             " *",
             " * Generated once by tools/gen_data.py from NgBMX.exe; from here on it is source like the code.",
             " */",
             "#ifndef BMX_DATA_LAYOUT_H", "#define BMX_DATA_LAYOUT_H", ""]
        names = sorted({m.name for b in self.blocks for m in b.members} & self.site_names)
        L.append("/* The headers' ORIG_VAR macros of these names: here they are the members. */")
        L += [f"#undef {n}" for n in names]
        for b in self.blocks:
            L += ["", f"/* {b.what[0].upper() + b.what[1:]}, {b.lo:#010x}..{b.hi:#010x}. */", f"struct {b.tag} {{"]
            for m in b.members:
                decl = self.declare(m, b)
                aliases = sorted({s.name for s in m.aliases if s.name != m.name})
                comment = f"{m.addr:#010x}" + (", also " + ", ".join(aliases) if aliases else "")
                if "/*" in decl:
                    decl = decl.replace("/* ", f"/* {comment}; ")
                else:
                    decl += f" /* {comment} */"
                L.append("    " + decl)
            L += ["};", ""]
            # The members' sizes add up to the block's: padding the compiler inserted anywhere would move the end.
            # The globals' own offsets are asserted too, as the record of where each one is.
            sited = [m for m in b.members if m.kind == "site"]
            if sited:
                L.append(f"#define AT(member, addr) _Static_assert(offsetof(struct {b.tag}, member) == (addr) - "
                         f"{b.lo:#010x}, #member)")
                L += [f"AT({m.name}, {m.addr:#010x});" for m in sited]
                L.append("#undef AT")
            last = b.members[-1].name
            L.append(f"_Static_assert(offsetof(struct {b.tag}, {last}) + sizeof(((struct {b.tag} *)0)->{last}) == "
                     f"{b.hi - b.lo:#x}, \"{b.name}\");")
        L += ["", "#endif"]
        return L

    def block_text(self, b):
        L = ["/*", f" * {b.what[0].upper() + b.what[1:]}, {b.lo:#010x}..{b.hi:#010x}, as one object with the original's",
             " * layout (src/data/layout.h, docs/PORTING.md X1b): code relies on globals being next to each other (the",
             " * save data copied as one range, tables used past their declared ends), so the members keep their",
             " * original offsets; ORIG_VAR(addr) reaches into it (src/compat/orig.h).",
             " *",
             " * Generated once by tools/gen_data.py from NgBMX.exe; from here on it is source like the code.",
             " */",
             '#include "data/data.h"', ""]
        decls = sorted(self.declared.get(b.file, ()))
        if decls:
            L.append("/* Functions the data points at that no header declares (assembler name = C name). */")
            L += [f'void {n}(void) __asm__("_{n}");' for n in decls]
            L.append("")
        unserved = sorted(self.unserved.get(b.file, ()))
        if unserved:
            L.append("/* Code nothing serves (unported, unreachable): calling one reports the address and stops. */")
            L += [f"BMX_DATA_UNSERVED({a:08x})" for a in unserved]
            L.append("")
        ptr_lines = {}
        qual = "const " if b.const else ""
        if b.kind != "init":
            L.append(f"{qual}struct {b.tag} {b.name} __attribute__((aligned({b.align})));")
        else:
            L.append(f"{qual}struct {b.tag} {b.name} __attribute__((aligned({b.align}))) = {{")
            for m in b.members:
                if m.init is None:
                    continue
                lines, pl = render(m.init, self.fixes)
                lines[0] = f"    .{m.name} = " + lines[0]
                lines[-1] += ","
                for k, key in pl.items():
                    ptr_lines[len(L) + k] = key
                L += lines
            L.append("};")
        if b.kind == "imports":
            L += self.import_table(b)
        return L, ptr_lines

    def import_table(self, b):
        L = ["", "/* What the loader fills each slot with (src/native/loader.c). */",
             "const BmxImportSlot bmxImportSlots[] = {"]
        n = 0
        for m in b.members:
            if m.addr in self.exe.imports:
                dll, name = self.exe.imports[m.addr]
                L.append(f'    {{"{dll}", "{name}", &bmxImports.{m.name}}},')
                n += 1
        L += ["};", f"const uint32_t bmxImportSlotCount = {n};", ""]
        # The original's own `jmp [slot]` thunks that ported code calls (tools/gen_origmap.py IMPORT_THUNKS).
        L.append("/* The original's `jmp [import slot]` thunks that ported code calls by address (ORIG_FN_STDCALL). */")
        for a, what in sorted(self.import_thunks.items()):
            code = self.exe.read(a, 6)
            if code[:2] != b"\xff\x25":
                sys.exit(f"{a:#x} ({what}) is not a jmp [slot] in the exe")
            slot = int.from_bytes(code[2:], "little")
            if slot not in b:
                sys.exit(f"{a:#x} ({what}): slot {slot:#x} is not in the import slots")
            dll, name = self.exe.imports[slot]
            if f"{dll.rsplit('.', 1)[0].upper()}!{name}" != what:
                sys.exit(f"{a:#x}: jumps through {dll}!{name}, IMPORT_THUNKS says {what}")
            m = self.member_at(b, slot)
            L.append(f"BMX_IMPORT_THUNK({a:08x}, {slot - b.lo:#x}); /* {what} */")
            L.append(f"_Static_assert(offsetof(struct {b.tag}, {m.name}) == {slot - b.lo:#x}, \"{m.name}\");")
        return L


def find_path(t, off, want):
    """The designator path (".a.b[3]") to a subobject at `off` in t of the type `want`; with want None (void *) the
    outermost subobject at off."""
    if off == 0 and (want is None or same_type(t, want)):
        return ""
    s = t.strip()
    if s.kind == "array" and s.elem.size:
        i, r = divmod(off, s.elem.size)
        if i < s.count:
            p = find_path(s.elem, r, want)
            if p is not None:
                return f"[{i}]{p}"
    if s.kind == "struct":
        for n, mo, mt, bits, _ in s.members:
            if bits is None and n is not None and mo <= off < mo + mt.size:
                p = find_path(mt, off - mo, want)
                if p is not None:
                    return f".{n}{p}"
    return None


# ---- what the data files can see -------------------------------------------------------------------------------

# Headers the data files include for the globals' types: every game header, except these.
HEADER_DIRS = ("compat", "rw", "zge", "bmx")
HEADER_SKIP = {
    "bmx/com/action/physics_roll.h",  # its PhysDebugVertex and ngifDrawLine3D clash with physics.h's
}


def data_headers():
    return sorted(p.relative_to(SRC).as_posix() for d in HEADER_DIRS for p in (SRC / d).rglob("*.h")
                  if p.relative_to(SRC).as_posix() not in HEADER_SKIP)


def deep_same(a, b, seen=None):
    seen = seen if seen is not None else set()
    a, b = a.strip(), b.strip()
    if (id(a), id(b)) in seen:
        return True
    seen.add((id(a), id(b)))
    if a.kind != b.kind or a.size != b.size or a.name != b.name and a.kind in ("struct", "union", "enum"):
        return False
    if a.kind == "array":
        return a.count == b.count and deep_same(a.elem, b.elem, seen)
    if a.kind in ("struct", "union"):
        if len(a.members) != len(b.members):
            return False
        return all(ma[0] == mb[0] and ma[1] == mb[1] and ma[3] == mb[3] and ma[4] == mb[4] and
                   deep_same(ma[2], mb[2], seen) for ma, mb in zip(a.members, b.members))
    if a.kind == "base":
        return a.enc == b.enc
    return True


def data_command(build):
    """The compiler command line of a game object (the data files are compiled like them), as a list without
    input and output."""
    cmds = compile_commands(build)
    cmd = next(c for p, c in cmds.items() if "bmx_game.dir" in c and p.suffix == ".c")
    args, skip, out = cmd.split(" "), False, []
    for a in args:
        if skip:
            skip = False
            continue
        if a in ("-o", "-c"):
            skip = True
            continue
        out.append(a)
    return out


def run_quiet(cmd):
    flags = getattr(subprocess, "BELOW_NORMAL_PRIORITY_CLASS", 0)
    return subprocess.run(cmd, capture_output=True, text=True, creationflags=flags)


def check_visible(sites, build, work):
    """The sites whose type the data files see under the same name and with the same layout."""
    base = data_command(build)
    lines = ['#include "data/data.h"']
    first = len(lines) + 1
    live = list(range(len(sites)))
    while True:
        text = "\n".join(lines + [f"__typeof__({sites[i].type_text}) *bmxvis_{i};" for i in live]) + "\n"
        probe_c = work / "visible.c"
        probe_c.write_text(text)
        ll = work / "visible.ll"
        r = run_quiet(base[:1] + [a for a in base[1:] if a not in ("-Werror", "-g") and not a.startswith("-O")] +
                      ["-O0", "-gdwarf-4", "-w", "-S", "-emit-llvm", "-o", str(ll), str(probe_c)])
        bad = {int(m.group(1)) - first for m in re.finditer(r"visible\.c:(\d+):\d+: error", r.stderr)}
        if r.returncode == 0:
            break
        if not bad:
            sys.exit(r.stderr[-3000:])
        live = [i for k, i in enumerate(live) if k not in bad]
    types = parse_types(ll.read_text().replace("bmxvis_", "bmxprobe_"))
    out = set()
    for name, t in types.items():
        s = sites[int(name.split("_")[1])]
        if deep_same(t, s.type):
            out.add(id(s))
    return out


# ---- compiler feedback ---------------------------------------------------------------------------------------------

INCOMPATIBLE = re.compile(r"(?:incompatible (?:function )?pointer types|discards qualifiers).*?initializing '([^']+)'"
                          r"|initializing '([^']+)'.*?discards qualifiers")


def compile_check(files, build, out):
    """-fsyntax-only on every data file; returns {file: [(line, message)]} of the errors."""
    base = data_command(build)

    def one(name):
        r = run_quiet(base + ["-fsyntax-only", "-ferror-limit=0", str(out / name)])
        errs = []
        for m in re.finditer(r"^(.*?):(\d+):\d+: (error|warning): (.*)$", r.stderr, re.M):
            errs.append((Path(m.group(1)).name, int(m.group(2)), m.group(4)))
        if r.returncode != 0 and not errs:
            errs.append((name, 0, r.stderr[-2000:]))
        return name, errs

    with concurrent.futures.ThreadPoolExecutor(max_workers=int(os.environ.get("BMX_JOBS", "3"))) as ex:
        return dict(ex.map(one, files))


# ---- the check: the compiled data against the exe --------------------------------------------------------------

def read_coff(path):
    """Sections (name, bytes, relocations [(offset, symbol index)]) and symbols [(name, value, section, class)] of a
    COFF object."""
    import struct

    d = Path(path).read_bytes()
    nsec, symptr, nsym = struct.unpack_from("<HxxxxII", d, 2)
    strtab = symptr + nsym * 18
    secs = []
    for i in range(nsec):
        name, vsize, vaddr, rawsize, rawptr, relptr, _, nrel = struct.unpack_from("<8sIIIIIIH", d, 20 + i * 40)
        name = name.rstrip(b"\0").decode()
        if name.startswith("/"):
            off = int(name[1:])
            name = d[strtab + off:d.index(b"\0", strtab + off)].decode()
        raw = d[rawptr:rawptr + rawsize] if rawptr else bytes(rawsize)
        rels = []
        for k in range(nrel):
            va, sym, typ = struct.unpack_from("<IIH", d, relptr + k * 10)
            rels.append((va, sym, typ))
        secs.append((name, raw, rels))
    syms, i = {}, 0
    while i < nsym:
        name, value, sec, typ, cls, aux = struct.unpack_from("<8sIhHBB", d, symptr + i * 18)
        if name[:4] == b"\0\0\0\0":
            off = int.from_bytes(name[4:], "little")
            name = d[strtab + off:d.index(b"\0", strtab + off)].decode()
        else:
            name = name.rstrip(b"\0").decode()
        syms[i] = (name, value, sec, cls)
        i += 1 + aux
    return secs, syms


def check(exe, build, gen_blocks):
    """Every byte of every block equals the exe's, and every pointer points where the exe's did: data pointers to
    the same original address, code pointers at the function serving the exe's target."""
    from gen_origmap import build_map

    owners = {a: n for a, (n, _) in build_map(SRC).items()}
    objdir = build / "CMakeFiles" / "bmx_data.dir" / "src" / "data"
    blocks = {b.name: b for b in gen_blocks}
    errors, counts = [], {"bytes": 0, "data pointers": 0, "code pointers": 0}

    def expect_code(v):
        if v in owners:
            return owners[v]
        if THUNKS[0] <= v < THUNKS[1] and exe.read(v, 1)[0] == 0xE9:
            dest = (v + 5 + int.from_bytes(exe.read(v + 1, 4), "little", signed=True)) & 0xFFFFFFFF
            return owners.get(dest, f"unserved_{dest:08x}")
        return f"unserved_{v:08x}"

    for b in gen_blocks:
        if b.kind != "init":
            continue
        secs, syms = read_coff(objdir / (b.file + ".obj"))
        sym = next((s for s in syms.values() if s[0] == "_" + b.name), None)
        if sym is None:
            errors.append(f"{b.file}: no symbol {b.name}")
            continue
        name, value, secno, _ = sym
        sname, raw, rels = secs[secno - 1]
        ours = raw[value:value + b.hi - b.lo]
        theirs = exe.read(b.lo, b.hi - b.lo)
        relmap = {}
        for va, si, typ in rels:
            if value <= va < value + b.hi - b.lo:
                if typ != 6:  # IMAGE_REL_I386_DIR32
                    errors.append(f"{b.file}: relocation type {typ} at {va - value:#x}")
                relmap[va - value] = syms[si]
        for off in range(0, b.hi - b.lo):
            a = b.lo + off
            if any(a - k in exe.relocs for k in range(1, 4)):
                continue
            if a in exe.relocs:
                if off not in relmap:
                    errors.append(f"{a:#x}: the exe has a pointer here, {b.file} has none")
                    continue
                target = exe.relocs[a]
                sname2, svalue, ssec, scls = relmap[off]
                addend = int.from_bytes(ours[off:off + 4], "little")
                if exe.is_code(target) and not INPUT_TABLES[0] <= target < INPUT_TABLES[1]:
                    want = expect_code(target)
                    got = sname2[1:] if sname2.startswith("_") else sname2
                    if scls == 3 and ssec > 0 and svalue == 0 and sname2.startswith("."):  # section symbol
                        got = next((s[0][1:] if s[0].startswith("_") else s[0]
                                    for s in syms.values() if s[2] == ssec and s[1] == addend and s[3] in (2, 3)
                                    and not s[0].startswith(".")), sname2)
                        addend = 0
                    if got != want or addend != 0:
                        errors.append(f"{a:#x}: code pointer to {got}+{addend}, the exe's is {target:#x} ({want})")
                    counts["code pointers"] += 1
                else:
                    if sname2.startswith(".") and scls == 3:  # section symbol of this object
                        cand = [s for s in syms.values() if s[2] == ssec and s[0].startswith("_bmx")
                                and s[1] <= addend]
                        sym2 = max(cand, key=lambda s: s[1])
                        sname2, addend = sym2[0], addend - sym2[1]
                    tb = blocks.get(sname2[1:])
                    got = tb.lo + addend if tb else None
                    if got != target:
                        errors.append(f"{a:#x}: data pointer to {sname2}+{addend:#x}, the exe's is {target:#x}")
                    counts["data pointers"] += 1
                continue
            if off in relmap:
                errors.append(f"{a:#x}: {b.file} has a pointer here, the exe has none")
                continue
            if ours[off] != theirs[off]:
                errors.append(f"{a:#x}: byte {ours[off]:#04x}, the exe's is {theirs[off]:#04x}")
            counts["bytes"] += 1
    return errors, counts


# ---- main -------------------------------------------------------------------------------------------------------

def blocks_header(blocks):
    """src/data/blocks.h: the blocks' ranges and the address -> place macro of src/compat/orig.h."""
    L = ["/*", " * The original's data blocks (src/data/<block>.c) and where an original data address is in them: each block",
         " * keeps the original's layout, so the address's offset in its block is its offset in the object.",
         " * Generated by tools/gen_data.py.", " */",
         "#ifndef BMX_DATA_BLOCKS_H", "#define BMX_DATA_BLOCKS_H", "", "#include <stdint.h>", ""]
    for b in blocks:
        L.append(f"extern {'const ' if b.const else ''}struct {b.tag} {b.name}; /* {b.what} */")
    L += ["", "/* Never reached for a data address the code names (tools/gen_data.py checks them all). */",
          "char *bmxDataBadAddress(uint32_t addr);", "",
          "/* The block holding original data address addr, as a char pointer. */",
          "#define BMX_DATA_AT(addr) \\"]
    for i, b in enumerate(blocks):
        lead = "    (" if i == 0 else "     : "
        L.append(f"{lead}(uint32_t)(addr) - {b.lo:#010x}u < {b.hi - b.lo:#x}u ? (char *)&{b.name} + ((uint32_t)(addr) - "
                 f"{b.lo:#010x}u) \\")
    L += ["     : bmxDataBadAddress((uint32_t)(addr)))", "", "#endif", ""]
    return "\n".join(L)


def data_h(headers):
    L = ["/*", " * Included by the data files (src/data/<block>.c): the game's headers, for the globals' types, the blocks'",
         " * layout and the helpers of the generated definitions (docs/PORTING.md X1b). Generated once by",
         " * tools/gen_data.py.", " */",
         "#ifndef BMX_DATA_DATA_H", "#define BMX_DATA_DATA_H", "", "#include <stddef.h>", "#include <stdint.h>", "",
         "/* zrwmodel.h and zrwzgl.h name 0x697720 / 0x697724 the other way round (gZrwScale, gZrwInvScale), and",
         " * gZrwFlags with different types: each module's own pair is right for its code, both are seen here. */",
         "#pragma clang diagnostic push", '#pragma clang diagnostic ignored "-Wmacro-redefined"']
    L += [f'#include "{h}"' for h in headers]
    L += ["#pragma clang diagnostic pop", '#include "native/native.h"', "",
          "/* An unported, unreached original function that a pointer in the data names: reports and stops. */",
          "#define BMX_DATA_UNSERVED(addr) \\",
          "    static void unserved_##addr(void) \\",
          "    { \\",
          "        bmxOrigTrap(0x##addr##u, (uint32_t)(uintptr_t)__builtin_return_address(0)); \\",
          "    }", "",
          "/* A pointer the original keeps off its natural alignment (in a packed table). */",
          "typedef struct __attribute__((packed)) BmxUnalignedPtr {", "    void *p;", "} BmxUnalignedPtr;", "",
          "/* An original `jmp [import slot]` thunk that ported code calls by address: the same jump, through our slot. */",
          "#define BMX_IMPORT_THUNK(addr, offset) \\",
          '    __asm__(".globl _bmxImportThunk_" #addr "\\n_bmxImportThunk_" #addr ":\\n\\tjmp *_bmxImports+" #offset "\\n")',
          "", '#include "data/blocks.h"', '#include "data/layout.h"', "", "#endif", ""]
    return "\n".join(L)


def generate(build, exe_path):
    out = SRC / "data"
    out.mkdir(exist_ok=True)
    work = build / "gendata"
    sites, odd = scan_sites()
    probe(sites, build, work)
    exe = Exe(exe_path)
    data_sec = next(s for s in exe.sections if s[0] == ".data")
    raw = data_sec[4]
    last = max(i for i, b in enumerate(raw) if b) + data_sec[1]
    last = max([last] + [r + 4 for r in exe.relocs if data_sec[1] <= r < data_sec[2]])
    split = (last + 1 + 0xFFF) & ~0xFFF  # on a page (make_blocks)
    while any(s.addr < split < s.addr + s.type.size for s in sites):
        split += 0x1000
    blocks = make_blocks(exe, split)
    (out / "blocks.h").write_text(blocks_header(blocks))
    (out / "data.h").write_text(data_h(data_headers()))
    (out / "layout.h").write_text("/* (being generated) */\n")
    visible = check_visible(sites, build, work)
    for s in sites:
        if not any(s.addr in b for b in blocks):
            sys.exit(f"{s}: not in any data block")
    for a in raw_addresses():
        if not any(a in b for b in blocks):
            sys.exit(f"BMX_ORIG({a:#010x}): not in any data block")
    gen = Gen(exe, sites, visible)
    gen.log = []
    gen.site_names = {s.name for s in sites}
    while True:
        for b in blocks:
            gen.plan_block(b)
        gen.blocks = blocks
        bad = gen.build_inits()
        if not bad:
            break
        for s, why in bad:
            gen.log.append(f"{s}: not a member, {why}")
            gen.demoted.add((s.addr, s.name))
    for rounds in range(8):
        files = gen.write()
        for name, (lines, _) in files.items():
            (out / name).write_text("\n".join(lines) + "\n")
        errs = compile_check([f for f in files if f.endswith(".c")], build, out)
        progress = False
        unknown = []
        replan = False
        seen = set()
        for cname, el in errs.items():
            for name, line, msg in el:
                if (name, line, msg) in seen:
                    continue
                seen.add((name, line, msg))
                lines, ptr_lines = files.get(name, ([], {}))
                m = re.search(r"use of undeclared identifier '(\w+)'", msg)
                if m:
                    gen.declared.setdefault(name, set()).add(m.group(1))
                    progress = True
                    continue
                m = INCOMPATIBLE.search(msg)
                if m and line - 1 in ptr_lines:
                    # The type as the compiler spells it, when that is a type name; else through void *.
                    fix = m.group(1) or m.group(2)
                    if not re.fullmatch(r"(const )?[A-Za-z_]\w*( \*+)?", fix):
                        fix = "void *"
                    gen.fixes[ptr_lines[line - 1]] = fix
                    progress = True
                    continue
                if ("expected ')'" in msg or "incompatible calling conventions" in msg) and line - 1 in ptr_lines:
                    gen.fixes[ptr_lines[line - 1]] = "void *"
                    progress = True
                    continue
                m = re.search(r"static assertion failed due to requirement .*: (\w+)$|static assertion failed: (\w+)$", msg)
                if m:
                    mname = m.group(1) or m.group(2)
                    hit = [mm for b in blocks for mm in b.members if mm.name == mname and mm.site is not None]
                    for mm in hit:
                        gen.demoted.add((mm.site.addr, mm.site.name))
                        gen.log.append(f"{mm.site}: not a member, its offset differs (alignment)")
                        replan = True
                    if hit:
                        continue
                unknown.append(f"{name}:{line}: {msg}")
        if unknown:
            sys.exit("\n".join(unknown[:40]))
        if replan:
            while True:
                for b in blocks:
                    gen.plan_block(b)
                bad = gen.build_inits()
                if not bad:
                    break
                for s, why in bad:
                    gen.demoted.add((s.addr, s.name))
            continue
        if not progress:
            break
    else:
        sys.exit("the data files still do not compile")
    # The project's formatting (the gate checks it); clang-format 21 like tools/check.sh.
    r = run_quiet([os.environ.get("CLANG_FORMAT", "clang-format"), "-i"] +
                  [str(out / n) for n in sorted(files) + ["blocks.h", "data.h"]])
    if r.returncode != 0:
        sys.exit(f"clang-format: {r.stderr[-2000:]}")
    stats(blocks, gen, sites, visible)
    log = work / "gen_data.log"
    log.write_text("\n".join(gen.log) + "\n")
    print(f"log: {log}")


def stats(blocks, gen, sites, visible):
    views = []
    for b in blocks:
        for m in b.members:
            for s in m.aliases:
                if s.addr + s.type.size > m.end:
                    views.append(s)
    gen.log.append("")
    gen.log.append(f"{len(views)} views: sites that span more than one member (code relying on adjacency)")
    for s in sorted(views, key=lambda s: s.addr):
        gen.log.append(f"  {s} ({s.type.size} bytes)")
    for b in blocks:
        kinds = {}
        for m in b.members:
            kinds[m.kind] = kinds.get(m.kind, 0) + 1
        print(f"{b.name:15} {b.lo:#010x}..{b.hi:#010x} {len(b.members):5} members {kinds}")
    print(f"{len(sites)} sites, {len(visible)} with a type the data files see; {len(gen.demoted)} demoted")


def main():
    import argparse

    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("--build", default=str(ROOT / "build" / "native32"), help="native32 build directory")
    ap.add_argument("--exe", default=os.environ.get("BMX_EXE", str(ROOT / "work" / "NgBMX.exe")))
    ap.add_argument("--check", action="store_true", help="compare the built data objects with the exe")
    args = ap.parse_args()
    build = Path(args.build)
    if args.check:
        exe = Exe(args.exe)
        # The blocks as src/data/blocks.h has them (BMX_DATA_AT's ranges).
        text = (SRC / "data" / "blocks.h").read_text()
        protos = {b.name: b for b in make_blocks(exe, 0)}
        blocks = []
        for m in re.finditer(r"\(uint32_t\)\(addr\) - (0x[0-9a-f]+)u < (0x[0-9a-f]+)u\s+\? \(char \*\)&(\w+)", text):
            b = protos[m.group(3)]
            b.lo, b.hi = int(m.group(1), 16), int(m.group(1), 16) + int(m.group(2), 16)
            blocks.append(b)
        if len(blocks) != len(protos):
            sys.exit(f"src/data/blocks.h: found {len(blocks)} of the {len(protos)} blocks")
        errors, counts = check(exe, build, blocks)
        for e in errors[:50]:
            print(e)
        print(f"{len(errors)} differences; checked {counts}")
        sys.exit(1 if errors or not counts["bytes"] else 0)
    generate(build, args.exe)


if __name__ == "__main__":
    main()
