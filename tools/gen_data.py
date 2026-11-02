#!/usr/bin/env python3
"""The original's data as C source (docs/PORTING.md X1b / X2a, docs/NATIVE.md "The original's data in the source").

Usage:
  tools/gen_data.py [--build build/native32] [--exe work/NgBMX.exe]   generate src/data from the exe (once)
  tools/gen_data.py --check [--build build/native32]                   compare the built src/data with the exe

The native exe needs NgBMX.exe's initial data: the globals' values, the tables, the strings, the pointers between
them and into the code. This tool turned it into C once; the result is committed (src/data) and maintained like the
code, and the build does not need the exe. Re-running it overwrites src/data (needs a configured build/native32).

How (X2a):
- Every original global becomes its own definition (a "unit"): typed as the source declares it (ORIG_VAR's type; the
  layout from the compiler: each declaring file is compiled with a probe that declares a pointer to the type, and
  the debug metadata of the LLVM IR gives every field's offset and type). Declarations inside another one (its
  fields) are aliases of it. A type the data files cannot see (local to a .c file) gets a same-layout copy.
- Globals the code uses as one piece stay together as a group (a struct whose members keep their original offsets,
  asserted): declarations that span several others without containing them (views), the ranges in RANGES (copied
  or cleared at once), a global and the bytes after it that nothing refers to (a table declared shorter than it
  is reads on into them). EXTENTS bounds views whose declared type is bigger than what the code uses. A group
  starts as if at the 16-byte boundary below it (a `lead_` member), so its members keep their alignment.
- Bytes no declaration names but the original's code or a pointer refers to are units too: strings (s<Text>),
  pointers (p_<addr>), plausible floats, other words or bytes (d_<addr>). Fewer than 16 zero bytes nothing refers
  to are padding and dropped.
- Initialisers come from the exe's bytes: integers, floats with the shortest digits that give the same bits,
  strings as literals, pointers (from the exe's relocations) as &global.field expressions or, into the code, as the
  name of the function serving that address (tools/gen_origmap.py's map; incremental-link thunks followed;
  addresses nothing serves get a stub that reports them).
- Output: src/data/<module>.c (the definitions, by the module that declares them), globals.h (their types and
  declarations), addrs.h (ORIG_VAR's address -> definition + offset, included by src/compat/orig.h), units.c (every
  definition's original range: bmxDataFind and --check).
- The result is compiled with -fsyntax-only, and the compiler's complaints are fed back: casts where a pointer's type
  differs, declarations for functions no header declares, a declaration that does not sit at its offset becomes an
  alias. clang-format formats the files.
- --check compares the compiled objects with the exe (each definition in its own section, -fdata-sections): every
  byte, and every pointer's target (gate: tools/check.sh).
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
    def __init__(self, path, addr, type_text, name, line, spelled=None):
        self.path, self.addr, self.type_text, self.name, self.line = path, addr, type_text, name, line
        self.spelled = spelled or f"{addr:#010x}"
        self.type = None  # the probed type (Type)

    def __repr__(self):
        return f"{self.path}:{self.line} {self.addr:#010x} {self.type_text} {self.name}"


def scan_sites():
    """Every ORIG_VAR(addr, type, name) with a literal address in the native build's sources."""
    sites, odd = [], []
    for path in sorted(list(SRC.rglob("*.c")) + list(SRC.rglob("*.h"))):
        rel = path.relative_to(SRC).as_posix()
        if rel.startswith(("hook/", "data/")) or rel == "compat/orig.h":
            continue
        text = path.read_text()
        for m in re.finditer(r"\bORIG_VAR\(", text):
            args = macro_args(text, m.end() - 1)
            line = text.count("\n", 0, m.start()) + 1
            if args is not None and args[0] == "addr" and rel == "rw/d3dpipe.c":
                continue
            if args is None or len(args) != 3 or not re.fullmatch(r"0x[0-9a-fA-F]+", args[0]):
                odd.append((rel, line, args))
                continue
            sites.append(Site(rel, int(args[0], 16), " ".join(args[1].split()), args[2], line, args[0]))
        # d3dpipe.c's K(addr) wraps ORIG_VAR: its uses name the addresses.
        if rel == "rw/d3dpipe.c":
            for m in re.finditer(r"\bK\((0x[0-9a-fA-F]+)\)", text):
                line = text.count("\n", 0, m.start()) + 1
                sites.append(Site(rel, int(m.group(1), 16), "const float", f"rwD3DK{m.group(1)}", line,
                                  m.group(1)))
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
    return keep + ["-DBMX_DATA_PROBE", "-O0", "-gdwarf-4", "-w", "-S", "-emit-llvm", "-o", str(out_ll), str(probe_c)]


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
        self.tag = "Bmx" + name[3:] if name else None  # bmxData -> struct BmxData
        self.rng, self.label, self.group, self.module = None, None, False, None
        # A group starts as if at the 16-byte boundary below its first address (`lead` bytes of nothing first), so
        # its members keep their original alignment.
        self.base, self.lead = lo, 0
        self.members = []

    def __contains__(self, a):
        return self.lo <= a < self.hi


# Thunk table of the incremental linker (`jmp rel32`; a pointer to one stands for its target) and the dinput.lib
# object tables the linker put into .text (data that the input formats in .rdata point at).
THUNKS = (0x00401005, 0x00402617)
INPUT_TABLES = (0x004AFC60, 0x004B0F20)


# The extent the code really uses where a declaration's type is bigger (X2a): such a declaration is a view.
EXTENTS = {
    ("gComdataRecords", 0x00551FD8): 0x1680,  # 0x1200 records declared, SAVE.DAT's kSaveBytesRecords used
    ("gPhysRiders", 0x00691F40): 2 * 0xF70,  # 4 riders declared, 2 used (one per player; gPlayerRecs: 2 * 0xf70)
    ("gActionMapTable", 0x00551FB8): 12,  # 256 bytes declared, unused; the table is gGameLevelInfoIndex's 12
}

# Ranges the code treats as one piece (one memcpy or memset over several globals): their globals become one
# group, in their original order and at their original offsets.
RANGES = [
    (0x00551FD8, 0x1680, "bmxSaveRecords", "SAVE.DAT's records: memcard.c copies them as one range"),
    (0x006952A0, 0x98, "bmxSaveProfiles", "SAVE.DAT's profiles: memcard.c copies them as one range"),
    (0x006955E0, 0xC3C, "bmxSaveOptions", "SAVE.DAT's options and riders: memcard.c copies them as one range"),
    (0x00697780, 0x34, "bmxWinMainGlobals", "WinMain's globals: main.c's initGlobals clears them in one memset"),
    (0x006741E0, 0xAC, "bmxZrwWorldState", "the world's state: zrwworld.c clears it in one memset from gZrwWorldReady"),
]

MODULE_TEXT = {
    "action": "the game (src/bmx/com/action)", "menu": "the front end (src/bmx/com/menu)",
    "bmxzge": "the game's engine layer (src/bmx/com/zge)", "pc": "the PC layer (src/bmx/pc)",
    "zge": "the engine (src/zge)", "zglrw": "the engine's RenderWare layer (src/zge/com/zglrw)",
    "rw": "RenderWare (src/rw)", "native": "the native exe's own code (src/native, src/platform, src/driver)",
    "misc": "no module (data before the first global)", "imports": "the import slots",
    "inputtables": "dinput.lib's object tables",
}


def module_of(path):
    """The data file (src/data/<module>.c) of a global declared in `path`."""
    parts = path.split("/")
    if parts[0] == "bmx":
        return {"action": "action", "menu": "menu", "zge": "bmxzge"}.get(parts[2], "pc") if parts[1] == "com" \
            else "pc"
    if parts[0] == "zge":
        return "zglrw" if "zglrw" in parts else "zge"
    if parts[0] == "rw":
        return "rw"
    return "native"


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
        if e.kind == "base" and e.enc == "char" and not relocs and \
                all(b == 0 or 0x20 <= b < 0x7F or b in (9, 10, 13) for b in raw):
            # Strings one after the other (the ones after the first named by nothing): one literal.
            last = max(i for i, b in enumerate(raw) if b)
            return ("s", c_string(raw[:last + 1]).replace("\\x00", "\\0"))
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


def raw_spellings():
    """{address as written: address} of BMX_ORIG(0x...) in the code."""
    out = {}
    for path in list(SRC.rglob("*.c")) + list(SRC.rglob("*.h")):
        if path.relative_to(SRC).as_posix().startswith(("hook/", "compat/orig.h", "data/")):
            continue
        for m in re.finditer(r"\bBMX_ORIG\(\s*(0x[0-9a-fA-F]+)\s*\)", path.read_text()):
            out[m.group(1)] = int(m.group(1), 16)
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
        if (s.addr - block.base) % align_of(s.type):
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
            if s.addr + s.type.size > block.hi and (s.name, s.addr) not in EXTENTS:
                sys.exit(f"{s}: runs past the end of {block.name or hex(block.lo)} ({block.hi:#x})")
        views = [s for s in bsites if s.addr + s.type.size > block.hi]
        bsites = [s for s in bsites if s not in views]
        if block.kind == "imports":
            # .idata from its start, so the slots keep their offsets in the page; only the slots are used.
            members = []
            for a in range(block.lo, block.hi, 4):
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
                m.type = self.guess_floats(m, block)
        block.members = sorted(members + fill, key=lambda m: m.addr)
        for s in bsites + views:
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
                if (x - block.base) % 4:
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
                if (data_from - block.base) % 4 == 0 and n % 4 == 0:
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

    def guess_floats(self, m, block):
        """A type for bytes nothing names: floats or doubles when every value reads as a plausible one (constants
        and tuning tables), a scalar for a single word, else the words or bytes as they are."""
        import struct

        raw = self.exe.read(m.addr, m.size)

        def plausible(text, digits):
            mant = re.sub(r"e.*$", "", text.rstrip("f")).replace("-", "").replace(".", "").lstrip("0")
            return len(mant) <= digits

        if m.size % 8 == 0 and (m.addr - block.base) % 8 == 0 and m.size >= 8:
            vals = [raw[i:i + 8] for i in range(0, m.size, 8)]
            if all(not any(v) or 0x3E0 <= (int.from_bytes(v, "little") >> 52) & 0x7FF <= 0x420 and
                   plausible(float_literal(v, 8), 10) for v in vals) and any(any(v) for v in vals):
                elem = Type("base", 8, "double", enc="float")
                return elem if len(vals) == 1 else Type("array", m.size, elem=elem, count=len(vals))
        if m.size % 4 == 0 and (m.addr - block.base) % 4 == 0:
            vals = [raw[i:i + 4] for i in range(0, m.size, 4)]
            if all(not any(v) or 0x60 <= (int.from_bytes(v, "little") >> 23) & 0xFF <= 0xA0 and
                   plausible(float_literal(v, 4), 7) for v in vals) and any(any(v) for v in vals):
                elem = Type("base", 4, "float", enc="float")
                return elem if len(vals) == 1 else Type("array", m.size, elem=elem, count=len(vals))
            if len(vals) == 1:
                return Type("base", 4, "uint32_t", enc="uint")
        return m.type

    # -- units: one definition each ---------------------------------------------------------------------------------

    def plan_units(self, block):
        """Cuts a section block into units, each one definition: a global (with the declarations inside it as its
        fields), a group of globals the code treats as one piece (a view over several of them, a range copied or
        cleared at once: a struct whose members keep their original offsets), or bytes something points at.
        Bytes nothing refers to stay with the unit before them (an array declared shorter than it is reads on into
        them), except padding: fewer than 16 zero bytes."""
        if block.kind == "imports" or block.name == "bmxInputTables":
            block.group = True
            block.label = block.what
            return [block]
        lo_b, hi_b = block.lo, block.hi
        exe = self.exe
        iv = [(s.addr, s.addr + EXTENTS.get((s.name, s.addr), s.type.size), None) for s in self.sites
              if s.addr in block]
        iv += [(lo, lo + n, (lo, lo + n, name, why)) for lo, n, name, why in RANGES if lo in block]
        iv.sort(key=lambda x: (x[0], -x[1]))
        comps = []
        for lo, hi, rng in iv:
            if comps and lo < comps[-1][1]:
                comps[-1][1] = max(comps[-1][1], hi)
                comps[-1][2] = comps[-1][2] or rng
            else:
                comps.append([lo, hi, rng])
        # A declaration of an array of one element (or none) is the start of a table whose length the code does not
        # say (gPhysMotions[1] is read as a table that runs on through gActionMotions): it stays with the global
        # after it.
        for s in self.sites:
            t = s.type.strip()
            if s.addr in block and t.kind == "array" and t.count <= 1:
                i = next(k for k, c in enumerate(comps) if c[0] <= s.addr < c[1])
                if i + 1 < len(comps):
                    self.log.append(f"{s}: an open-ended table, kept with what follows it at {comps[i + 1][0]:#x}")
                    comps[i][1] = comps[i + 1][1]
                    comps[i][2] = comps[i][2] or comps[i + 1][2]
                    del comps[i + 1]
        for c in comps:
            if c[1] > hi_b:
                sys.exit(f"{c[0]:#x}..{c[1]:#x} runs past the end of {block.name}")
        comp_at = {c[0]: c for c in comps}
        inside = self.bisect

        def in_comp(a):
            i = inside.bisect_right([c[0] for c in comps], a) - 1
            return i >= 0 and comps[i][0] < a < comps[i][1]

        refs = {a for a in self.targets | self.raw if lo_b <= a < hi_b and not in_comp(a)}
        starts = sorted(set(comp_at) | refs)

        def blank(lo, hi):
            return (block.kind == "zero" or not any(exe.read(lo, hi - lo))) and not self.relocs_in(lo - 3, hi)

        spans = []
        if starts and lo_b < starts[0] and not blank(lo_b, starts[0]):
            spans.append((lo_b, starts[0], None))  # unreferenced data at the section's start
        for i, s in enumerate(starts):
            nxt = starts[i + 1] if i + 1 < len(starts) else hi_b
            c = comp_at.get(s)
            end = c[1] if c else nxt
            if end < nxt and not (blank(end, nxt) and nxt - end < 16):
                end = nxt
            spans.append((s, end, c[2] if c else None))
        units = []
        for lo, hi, rng in spans:
            u = Block(None, lo, hi, block.kind, None, block.what, const=block.const)
            u.rng = rng[2:] if rng and rng[0] == lo and rng[1] <= hi else None
            if rng and not u.rng:
                self.log.append(f"{rng[2]} ({rng[3]}) is inside a larger group at {lo:#x}..{hi:#x}")
            u.base = lo & ~0xF
            units.append(u)
        return units

    def finish_unit(self, u, taken):
        """After plan_block: a unit of one member is that global; else a group struct. Names it (unique among all
        units: they are the program's symbols)."""
        # Bytes no declaration names and without pointers: one string (with the strings after it that nothing
        # names) or one array, not a struct of pieces.
        if (u.kind == "init" and len(u.members) > 1 and not any(m.kind == "site" or m.aliases for m in u.members)
                and not self.relocs_in(u.lo - 3, u.hi)):
            first = u.members[0]
            raw = self.exe.read(u.lo, u.hi - u.lo)
            if first.kind == "str":
                m = Member(u.lo, u.hi - u.lo, first.name, array_of("char", 1, u.hi - u.lo), "str")
            else:
                m = Member(u.lo, u.hi - u.lo, first.name, array_of("uchar", 1, u.hi - u.lo), "data")
                if not any(raw):
                    m.kind = "zero"
                elif len(raw) % 4 == 0:
                    m.type = self.guess_floats(Member(u.lo, len(raw), "", array_of("uint", 4, len(raw) // 4),
                                                      "data"), u)
                    if m.type.kind == "array" and m.type.elem.size == 1 or m.type.kind == "base" and m.type.size == 1:
                        m.type = array_of("uint", 4, len(raw) // 4)
                    if m.type.kind == "array" and m.type.strip().elem.enc == "uint" and len(raw) == 4:
                        m.type = Type("base", 4, "uint32_t", enc="uint")
            u.members = [m]
        # Neighbouring pointers nothing points into become one array (tables of functions).
        merged = []
        for m in u.members:
            prev = merged[-1] if merged else None
            if (m.kind == "ptr" and m.type.kind == "pointer" and prev is not None and prev.kind in ("ptr", "ptrs")
                    and prev.type.strip().kind in ("pointer", "array") and prev.end == m.addr
                    and getattr(prev.type.strip(), "elem", prev.type).strip().kind == "pointer"
                    and m.addr not in self.targets and m.addr not in self.raw and not m.aliases):
                n = prev.size // 4 + 1
                prev.type = Type("array", 4 * n, elem=Type("pointer", 4, target=None), count=n)
                prev.size, prev.kind = 4 * n, "ptrs"
                continue
            merged.append(m)
        u.members = merged
        u.group = not (len(u.members) == 1 and u.members[0].addr == u.lo and u.members[0].size == u.hi - u.lo)
        u.lead = u.lo - u.base if u.group else 0
        if u.rng:
            name, u.label = u.rng
        elif u.group:
            first = next((m for m in u.members if m.kind == "site"), u.members[0])
            name = first.name + "Group"
            u.label = "globals the code uses as one piece (a view over them, or a table declared shorter)"
        else:
            name = u.members[0].name
            u.label = None
        if name in taken:
            name = f"{name}_{u.lo:08x}"
        if name in self.renames:
            name = self.renames[name]
        taken.add(name)
        u.name = name
        if not u.group:
            u.members[0].name = name

    def plan_all(self, blocks):
        units, taken = [], set()
        module = "misc"
        for b in blocks:
            for u in self.plan_units(b):
                if u is b:
                    self.plan_block(b)
                    u.module = "imports" if b.kind == "imports" else "inputtables"
                    taken.add(b.name)
                    units.append(u)
                    continue
                self.plan_block(u)
                self.finish_unit(u, taken)
                ss = sorted((s for m in u.members for s in ([m.site] if m.site else []) + m.aliases),
                            key=lambda s: s.addr)
                if ss:
                    module = module_of(ss[0].path)
                u.module = module
                units.append(u)
        units.sort(key=lambda u: u.lo)
        self.blocks = units
        self.unit_los = [u.lo for u in units]

    def ref(self, u, m):
        """The C expression of member m of unit u."""
        return f"{u.name}.{m.name}" if u.group else u.name

    def member_at(self, block, a):
        ms = block.members
        i = self.bisect.bisect_right([m.addr for m in ms], a) - 1
        return ms[i] if i >= 0 and ms[i].addr <= a < ms[i].end else None

    def locate(self, a):
        """(unit, member, offset) of a data address; one past a member's end counts as that member's."""
        i = self.bisect.bisect_right(self.unit_los, a) - 1
        for u in self.blocks[max(i - 1, 0):i + 1][::-1]:
            if u.lo <= a <= u.hi:
                m = self.member_at(u, a) if a < u.hi else None
                if m is None and a > u.lo:
                    m = self.member_at(u, a - 1)
                if m is not None:
                    return u, m, a - m.addr
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
        u, m, off = self.locate(v)
        if u is None:
            if self.dropped(v):
                # Into padding that was dropped (a pointer one past an array's end lands there).
                u, m, off = self.locate(v - 1)
                off += 1
            else:
                raise SystemExit(f"pointer to {v:#x}: not in any unit")
        base = self.ref(u, m)
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

    def dropped(self, v):
        return self.locate(v - 1)[0] is not None

    # -- output ---------------------------------------------------------------------------------------------------

    def declare(self, m, unit, name=None):
        """The declaration of member m (its name, or `name`), without the semicolon."""
        name = name or m.name
        if m.type.kind == "struct" and m.type.name == "BmxUnalignedPtr":
            return f"BmxUnalignedPtr {name}"
        if m.kind != "site":
            return spell(m.type, name)
        s = m.site
        if id(s) not in self.visible:
            if not unit.group:
                return f"BmxType_{name} {name}"
            return spell(s.type, name)
        text = s.type_text
        if s.type.kind == "const" and text.startswith("const "):
            text = text[6:]
        if re.fullmatch(r"[\w\s]+\**", text):
            return f"{text} {name}"
        return f"__typeof__({text}) {name}"

    def mirrored(self, u):
        m = u.members[0]
        return not u.group and m.kind == "site" and id(m.site) not in self.visible

    def build_inits(self):
        em = Emitter(self)
        bad = []
        for u in self.blocks:
            if u.kind != "init":
                continue
            self.current_file = u.module + ".c"
            for m in u.members:
                try:
                    m.init = em.value(m.type, m.addr, (u.lo, m.name))
                except Unrepresentable as e:
                    if m.kind != "site":
                        raise
                    bad.append((m.site, str(e)))
        return bad

    def comment(self, u, m):
        aliases = sorted({s.name for s in m.aliases if s.name != m.name})
        return f"{m.addr:#010x}" + (", also " + ", ".join(aliases) if aliases else "")

    def write(self):
        files = {"globals.h": (self.globals_text(), {})}
        for module in sorted({u.module for u in self.blocks}):
            files[module + ".c"] = self.module_text(module)
        files["addrs.h"] = (self.addrs_text(), {})
        files["units.c"] = (self.units_text(), {})
        return files

    def globals_text(self):
        """src/data/globals.h: the types and declarations of every definition in src/data."""
        L = ["/*", " * The original's globals as C definitions (docs/PORTING.md X2a): their declarations, the types of the",
             " * groups (globals the code uses as one piece: a struct whose members keep their original offsets, asserted",
             " * below) and of the globals whose own type is local to a .c file (a same-layout copy). Included by the",
             " * data files only (src/data/data.h); the game's code reaches the globals through ORIG_VAR (data/addrs.h).",
             " *",
             " * Generated once by tools/gen_data.py from NgBMX.exe; from here on it is source like the code.",
             " */",
             "#ifndef BMX_DATA_GLOBALS_H", "#define BMX_DATA_GLOBALS_H", ""]
        names = sorted(({u.name for u in self.blocks} | {m.name for u in self.blocks for m in u.members})
                       & self.site_names)
        L.append("/* The headers' ORIG_VAR macros of these names: here the names are the definitions. */")
        L += [f"#undef {n}" for n in names]
        for u in self.blocks:
            qual = "const " if u.const else ""
            if not u.group:
                m = u.members[0]
                if self.mirrored(u):
                    L.append("")
                    L.append(f"/* The type is local to {m.site.path}. */")
                    L.append(f"typedef {spell(m.site.type, 'BmxType_' + u.name)};")
                continue
            L += ["", f"/* {u.label[0].upper() + u.label[1:]}, {u.lo:#010x}..{u.hi:#010x}. */",
                  f"struct BmxGroup_{u.name} {{"]
            if u.lead:
                L.append(f"    uint8_t lead_[{u.lead}]; /* nothing: the members' offsets keep their original alignment */")
            for m in u.members:
                decl = self.declare(m, u) + ";"
                note = f" /* {self.comment(u, m)}" + (f"; the type is local to {m.site.path}" if m.kind == "site" and
                                                      id(m.site) not in self.visible else "") + " */"
                L.append("    " + decl + note)
            L.append("};")
            L.append(f"#define AT(member, addr) _Static_assert(offsetof(struct BmxGroup_{u.name}, member) == (addr) - "
                     f"{u.base:#010x}, #member)")
            L += [f"AT({m.name}, {m.addr:#010x});" for m in u.members]
            L.append("#undef AT")
            last = u.members[-1].name
            L.append(f"_Static_assert(offsetof(struct BmxGroup_{u.name}, {last}) + "
                     f"sizeof(((struct BmxGroup_{u.name} *)0)->{last}) == {u.hi - u.base:#x}, \"{u.name}\");")
        L += ["", "/* Every definition, by original address. */"]
        for u in self.blocks:
            qual = "const " if u.const else ""
            if u.group:
                L.append(f"extern {qual}struct BmxGroup_{u.name} {u.name}; /* {u.lo:#010x} */")
            else:
                L.append(f"extern {qual}{self.declare(u.members[0], u, u.name)}; /* {self.comment(u, u.members[0])} */")
        L += ["", "#endif"]
        return L

    def module_text(self, module):
        units = [u for u in self.blocks if u.module == module]
        file = module + ".c"
        L = ["/*", f" * The original's globals of {MODULE_TEXT.get(module, module)}, with their initial values (docs/PORTING.md",
             " * X2a; types and declarations in src/data/globals.h).",
             " *",
             " * Generated once by tools/gen_data.py from NgBMX.exe; from here on it is source like the code.",
             " */",
             '#include "data/data.h"', ""]
        decls = sorted(self.declared.get(file, ()))
        if decls:
            L.append("/* Functions the data points at that no header declares (assembler name = C name). */")
            L += [f'void {n}(void) __asm__("_{n}");' for n in decls]
            L.append("")
        unserved = sorted(self.unserved.get(file, ()))
        if unserved:
            L.append("/* Code nothing serves (unported, unreachable): calling one reports the address and stops. */")
            L += [f"BMX_DATA_UNSERVED({a:08x})" for a in unserved]
            L.append("")
        ptr_lines = {}
        for u in units:
            qual = "const " if u.const else ""
            head = f"{qual}struct BmxGroup_{u.name} {u.name} __attribute__((aligned(16)))" if u.group else \
                f"{qual}{self.declare(u.members[0], u, u.name)}"
            if not u.group:
                L.append(f"/* {self.comment(u, u.members[0])} */")
            inits = [m for m in u.members if m.init is not None] if u.kind == "init" else []
            if not inits:
                L.append(head + ";")
            elif not u.group:
                lines, pl = render(u.members[0].init, self.fixes, indent="")
                lines[0] = f"{head} = " + lines[0]
                lines[-1] += ";"
                for k, key in pl.items():
                    ptr_lines[len(L) + k] = key
                L += lines
            else:
                L.append(f"{head} = {{")
                for m in inits:
                    lines, pl = render(m.init, self.fixes)
                    lines[0] = f"    .{m.name} = " + lines[0]
                    lines[-1] += ","
                    for k, key in pl.items():
                        ptr_lines[len(L) + k] = key
                    L += lines
                L.append("};")
            if u.kind == "imports":
                L += self.import_table(u)
            L.append("")
        return L, ptr_lines

    def addrs_text(self):
        """src/data/addrs.h: each original data address the code names -> the definition at it (ORIG_VAR)."""
        L = ["/*", " * Where each original data address that the code names (ORIG_VAR, BMX_ORIG) is now: the definition at it",
             " * in src/data, plus the offset in it (docs/PORTING.md X2a). The definitions are declared here with an opaque",
             " * type (ORIG_VAR gives the type at the use); src/data/globals.h has their real types.",
             " *",
             " * Generated by tools/gen_data.py.", " */",
             "#ifndef BMX_DATA_ADDRS_H", "#define BMX_DATA_ADDRS_H", ""]
        wanted = self.spellings
        macros, used = [], []
        for sp in sorted(wanted, key=lambda s: (wanted[s], s)):
            a = wanted[sp]
            u, m, off = self.locate(a)
            if u is None:
                sys.exit(f"{sp}: named in the code, in no unit")
            if u not in used:
                used.append(u)
            off = a - u.lo + u.lead
            macros.append(f"#define BMX_DATA_{sp} ((char *)&bmxAt_{u.name}" + (f" + {off:#x})" if off else ")"))
        for u in used:
            L.append(f'extern struct BmxDataOpaque bmxAt_{u.name} __asm__("_{u.name}");')
        L += [""] + macros + ["", "#endif", ""]
        return L

    def units_text(self):
        L = ["/*", " * Every definition of src/data with the original address range it holds, sorted (bmxDataFind,",
             " * src/native/loader.c: original addresses computed at run time, RenderWare's tables; tools/gen_data.py",
             " * --check). Generated by tools/gen_data.py.", " */",
             '#include "data/data.h"', "", "const BmxDataUnit bmxDataUnits[] = {"]
        L += [f"    {{{u.lo:#010x}, {u.hi:#010x}, (const void *)&{u.name}}}," if not u.lead else
              f"    {{{u.lo:#010x}, {u.hi:#010x}, (const char *)&{u.name} + {u.lead}}}," for u in self.blocks]
        L += ["};", f"const uint32_t bmxDataUnitCount = {len(self.blocks)};", ""]
        return L

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
            if not b.lo <= slot < b.hi:
                sys.exit(f"{a:#x} ({what}): slot {slot:#x} is not in the import slots")
            dll, name = self.exe.imports[slot]
            if f"{dll.rsplit('.', 1)[0].upper()}!{name}" != what:
                sys.exit(f"{a:#x}: jumps through {dll}!{name}, IMPORT_THUNKS says {what}")
            m = self.member_at(b, slot)
            L.append(f"BMX_IMPORT_THUNK({a:08x}, {slot - b.lo:#x}); /* {what} */")
            L.append(f"_Static_assert(offsetof(struct BmxGroup_bmxImports, {m.name}) == {slot - b.lo:#x}, "
                     f"\"{m.name}\");")
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
                      ["-DBMX_DATA_PROBE", "-O0", "-gdwarf-4", "-w", "-S", "-emit-llvm", "-o", str(ll), str(probe_c)])
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


def data_h(headers):
    L = ["/*", " * Included by the data files (src/data/<module>.c): the game's headers, for the globals' types, the",
         " * declarations of the definitions and the helpers of the generated code (docs/PORTING.md X2a). Generated once",
         " * by tools/gen_data.py.", " */",
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
          "", '#include "data/globals.h"', "", "#endif", ""]
    return "\n".join(L)


def generate(build, exe_path):
    out = SRC / "data"
    out.mkdir(exist_ok=True)
    work = build / "gendata"
    sites, odd = scan_sites()
    if odd:
        sys.exit("ORIG_VAR with a computed address (write the base address, index the result): " +
                 ", ".join(f"{p}:{l}" for p, l, _ in odd))
    probe(sites, build, work)
    exe = Exe(exe_path)
    data_sec = next(s for s in exe.sections if s[0] == ".data")
    blocks = make_blocks(exe, data_sec[2])
    blocks = [b for b in blocks if b.name != "bmxBss"]
    for b in blocks:
        if b.kind == "imports":
            b.lo = b.base = b.iat  # the slots only
    (out / "data.h").write_text(data_h(data_headers()))
    (out / "globals.h").write_text("/* (being generated) */\n")
    visible = check_visible(sites, build, work)
    for s in sites:
        if not any(s.addr in b for b in blocks):
            sys.exit(f"{s}: not in any data block")
    spellings = {s.spelled: s.addr for s in sites}
    spellings.update(raw_spellings())
    for sp, a in spellings.items():
        if not any(a in b for b in blocks):
            sys.exit(f"{sp}: not in any data block")
    gen = Gen(exe, sites, visible)
    gen.log = []
    gen.site_names = {s.name for s in sites}
    gen.spellings = spellings
    gen.renames = {}

    def plan():
        while True:
            gen.plan_all(blocks)
            bad = gen.build_inits()
            if not bad:
                return
            for s, why in bad:
                gen.log.append(f"{s}: not a member, {why}")
                gen.demoted.add((s.addr, s.name))

    plan()
    for rounds in range(10):
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
                # A name a header already uses for something else (a function, a type): the definition gets the
                # original address as a suffix.
                m = re.search(r"(?:redefinition of|conflicting types for|redefinition of '\w+' as different kind of "
                              r"symbol) '?(\w+)'?", msg)
                if m and name == "globals.h" and any(u.name == m.group(1) for u in gen.blocks):
                    u = next(u for u in gen.blocks if u.name == m.group(1))
                    gen.renames[u.name] = f"{u.name}_{u.lo:08x}"
                    replan = True
                    continue
                m = re.search(r"static assertion failed due to requirement .*: (\w+)$|static assertion failed: (\w+)$", msg)
                if m:
                    mname = m.group(1) or m.group(2)
                    hit = [mm for b in gen.blocks for mm in b.members if mm.name == mname and mm.site is not None]
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
            plan()
            continue
        if not progress:
            break
    else:
        sys.exit("the data files still do not compile")
    for stale in out.iterdir():
        if stale.name not in files and stale.name != "data.h":
            stale.unlink()
    # The project's formatting (the gate checks it); clang-format 21 like tools/check.sh.
    r = run_quiet([os.environ.get("CLANG_FORMAT", "clang-format"), "-i"] +
                  [str(out / n) for n in sorted(files) + ["data.h"]])
    if r.returncode != 0:
        sys.exit(f"clang-format: {r.stderr[-2000:]}")
    stats(gen, sites, visible)
    log = work / "gen_data.log"
    log.write_text("\n".join(gen.log) + "\n")
    print(f"log: {log}")


def stats(gen, sites, visible):
    units = gen.blocks
    groups = [u for u in units if u.group]
    kinds = {}
    for u in units:
        if not u.group:
            k = u.members[0].kind
            kinds[k] = kinds.get(k, 0) + 1
    gen.log.append("")
    gen.log.append(f"{len(groups)} groups (globals the code uses as one piece):")
    for u in groups:
        names = [m.name for m in u.members if m.kind == "site"]
        gen.log.append(f"  {u.name} {u.lo:#010x}..{u.hi:#010x} ({u.hi - u.lo} bytes, {len(u.members)} members: "
                       f"{' '.join(names[:10])}{' ...' if len(names) > 10 else ''})")
    tails = [(u, u.members[-1]) for u in units if u.group and u.members[0].kind == "site" and len(u.members) == 2
             and u.members[1].kind in ("zero", "data")]
    gen.log.append(f"{len(tails)} of them a global and the unreferenced bytes after it")
    print(f"{len(units)} definitions: {len(units) - len(groups)} single ({kinds}), {len(groups)} groups")
    print(f"{len(sites)} sites, {len(visible)} with a type the data files see; {len(gen.demoted)} demoted")
    print("modules:", ", ".join(f"{m} {sum(1 for u in units if u.module == m)}"
                                for m in sorted({u.module for u in units})))


# ---- the check: the compiled data against the exe --------------------------------------------------------------

def check(exe, build):
    """Every definition's bytes equal the exe's at its original address, and every pointer in it points where the
    exe's did: data pointers to the same original address, code pointers at the function serving the exe's
    target."""
    from gen_origmap import build_map

    owners = {a: n for a, (n, _) in build_map(SRC).items()}
    table = (SRC / "data" / "units.c").read_text()
    units, leads = {}, {}
    for m in re.finditer(r"\{(0x[0-9a-f]+), (0x[0-9a-f]+), \(const (?:void|char) \*\)&(\w+)(?: \+ (?:BMX_DATA_OFF\((\d+), \d+\)|(\d+)))?\}",
                         table):
        units[m.group(3)] = (int(m.group(1), 16), int(m.group(2), 16))
        leads[m.group(3)] = int(m.group(4) or m.group(5) or 0)  # the 32-bit lead (tools/data_off64.py)
    if not units:
        return ["src/data/units.c: no units"], {}
    objdir = build / "CMakeFiles" / "bmx_data.dir" / "src" / "data"
    errors, counts = [], {"definitions": 0, "bytes": 0, "data pointers": 0, "code pointers": 0}

    def expect_code(v):
        if v in owners:
            return owners[v]
        if THUNKS[0] <= v < THUNKS[1] and exe.read(v, 1)[0] == 0xE9:
            dest = (v + 5 + int.from_bytes(exe.read(v + 1, 4), "little", signed=True)) & 0xFFFFFFFF
            return owners.get(dest, f"unserved_{dest:08x}")
        return f"unserved_{v:08x}"

    found = set()
    for obj in sorted(objdir.glob("*.c.obj")):
        secs, syms = read_coff(obj)
        for name, value, secno, cls in syms.values():
            uname = name[1:] if name.startswith("_") else name
            if uname not in units or secno <= 0 or cls != 2:
                continue
            lo, hi = units[uname]
            if uname == "bmxImports":
                found.add(uname)
                continue
            sname, raw, rels = secs[secno - 1]
            if sname.startswith(".bss") or not raw:
                ours = bytes(hi - lo)
            else:
                ours = raw[value + leads[uname]:value + leads[uname] + hi - lo]
            if len(ours) != hi - lo:
                errors.append(f"{uname}: {len(ours)} bytes in {obj.name}, the original has {hi - lo}")
                continue
            found.add(uname)
            counts["definitions"] += 1
            theirs = exe.read(lo, hi - lo)
            relmap = {}
            start = value + leads[uname]
            for va, si, typ in rels:
                if start <= va < start + hi - lo:
                    if typ != 6:  # IMAGE_REL_I386_DIR32
                        errors.append(f"{uname}: relocation type {typ} at {va - start:#x}")
                    relmap[va - start] = syms[si]
            for off in range(hi - lo):
                a = lo + off
                if any(a - k in exe.relocs for k in range(1, 4)):
                    continue
                if a in exe.relocs:
                    if off not in relmap:
                        errors.append(f"{a:#x} ({uname}): the exe has a pointer here, ours has none")
                        continue
                    target = exe.relocs[a]
                    tname, tvalue, tsec, tcls = relmap[off]
                    addend = int.from_bytes(ours[off:off + 4], "little")
                    if tname.startswith(".") and tcls == 3:  # a section symbol: the static function in it
                        tname = next((s[0] for s in syms.values() if s[2] == tsec and not s[0].startswith(".")
                                      and s[1] == addend), tname)
                        addend = 0
                    got = tname[1:] if tname.startswith("_") else tname
                    if exe.is_code(target) and not INPUT_TABLES[0] <= target < INPUT_TABLES[1]:
                        want = expect_code(target)
                        if got != want or addend != 0:
                            errors.append(f"{a:#x} ({uname}): code pointer to {got}+{addend}, the exe's is "
                                          f"{target:#x} ({want})")
                        counts["code pointers"] += 1
                    else:
                        if got not in units or units[got][0] + addend - leads[got] != target:
                            errors.append(f"{a:#x} ({uname}): data pointer to {got}+{addend:#x}, the exe's is "
                                          f"{target:#x}")
                        counts["data pointers"] += 1
                    continue
                if off in relmap:
                    errors.append(f"{a:#x} ({uname}): ours has a pointer here, the exe has none")
                    continue
                if ours[off] != theirs[off]:
                    errors.append(f"{a:#x} ({uname}): byte {ours[off]:#04x}, the exe's is {theirs[off]:#04x}")
                counts["bytes"] += 1
    for uname in sorted(set(units) - found):
        errors.append(f"{uname}: not found in the objects of {objdir}")
    return errors, counts


def main():
    import argparse

    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("--build", default=str(ROOT / "build" / "native32"), help="native32 build directory")
    ap.add_argument("--exe", default=os.environ.get("BMX_EXE", str(ROOT / "work" / "NgBMX.exe")))
    ap.add_argument("--check", action="store_true", help="compare the built data objects with the exe")
    args = ap.parse_args()
    build = Path(args.build)
    if args.check:
        errors, counts = check(Exe(args.exe), build)
        for e in errors[:50]:
            print(e)
        print(f"{len(errors)} differences; checked {counts}")
        sys.exit(1 if errors or not counts.get("bytes") else 0)
    generate(build, args.exe)


if __name__ == "__main__":
    main()
