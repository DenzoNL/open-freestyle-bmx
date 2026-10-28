#!/usr/bin/env python3
"""Generate data/functions-rw.csv: the RenderWare functions to port, grouped into src/rw/<group>.c.

Needs work/bundles-rw (or $BMX_WORK/bundles-rw; Ghidra export of 0x4a4a80..0x54b000, see docs/RENDERWARE.md) and work/labels.raw.
Same columns as functions.csv (addr,size,file,confidence,name,status).

Which functions: the static call closure of the RW entry functions, i.e. every ORIG_FN declaration
(and every BMX_ADDR annotation) in src/ with an address >= 0x4a4a80, plus the closure of the INDIRECT
targets: functions reached only through function pointers (engine/plugin tables, default callbacks,
constructors/destructors given to the plugin registry, stream callbacks, free-list slots):
  1. code addresses ported code stores or passes as pointers: RW_CB(type, addr) and BMX_FNPTR(type, addr, fn)
     in src/ (a #define'd name is resolved), and the initial value of every ORIG_VAR(addr, ...) global
     when it is the start of an RW function (function-pointer variables; read from the exe with pefile);
  2. dwords in the exe's .rdata/.data/.data1 that equal the start of an RW function (statically
     initialised tables, e.g. the D3D7 device table at 0x566fe4);
  3. immediates in the RW disassembly: `PUSH 0x4xxxxx`, `MOV [mem],0x4xxxxx`, `MOV reg,0x4xxxxx` that are
     function starts (tables filled at run time).
A target needs a Ghidra function in work/bundles-rw; tools/ghidra/SeedPtrs.java creates them from the same
data dwords and immediates (RW's `lea esi,[esi+0]` padding included), so re-export after changing it.
Callbacks the game passes by address that still have no bundle are listed on stderr.

Driver: the Direct3D 7 device driver is replaced by our SDL3 driver and never ported. Seeds: the function
pointers in DRIVER_TABLES (the RwDevice, the rwD3D3Standards table and the core extension table at 0x69bee0
with the driver's Im3D/render state hooks) and every function whose community
label contains "D3D". Their closure (direct calls, plus the pointers the driver stores itself, e.g. the
camera/raster plugin constructors), minus everything the non-driver roots reach, goes to file
`lib/d3d7driver`, group rw/driver, status `skip`.

Reclassified ranges (RECLASS): render-only code to `lib/d3d7driver`, CRT duplicates to `lib/crt`, and code the game
never reaches (checked by an in-game breakpoint trace) to `lib/unreached`.

Skipped functions (status `skip`, file `lib/crt`, `lib/libpng` or `lib/zlib`) are not ported, because
the replacement uses the platform CRT or a real image decoder:
  - CRT: the statically linked MSVC CRT sits in two blocks, 0x4a4a80..0x4b0000 (malloc/heap, stdio,
    printf/scanf, mem*/str*, _ftol, __aull*, SEH, file handles) and 0x51d000.. (_strupr, _strcmpi,
    setjmp/longjmp, time/locale tail); everything in these blocks is skipped. RW code lies in between.
  - libpng/zlib: the functions reachable from RtPNGImageRead (0x5068a0) and from no other entry,
    located in 0x506000..0x51d000, and not named Rw*/Rp*/Rt*/_rw*. Names `png_*` or addresses below
    0x518db0 (the inflate code starts there; 0x5114b0 too) split libpng from zlib. RtPNGImageRead
    itself, the RW glue around the decoder, stays `orig`.

Groups (docs/RENDERWARE.md): an entry function gets its group from its address (RW links one module
after the other, ranges below; checked against the names and the game files calling it), or from its
name where that says more. A function that is no entry gets the group of the entry reaching it; if
several groups reach it, the lowest-level of two (LEVEL order), and rw/core.c for three or more (error
reporting, plugin registry, free lists, memory). Rows of already ported functions keep their file.
"""
import csv
import json
import os
import re
import sys
from collections import defaultdict
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
WORK = Path(os.environ.get("BMX_WORK", ROOT / "work"))  # work/ is gitignored: a worktree points here
BUNDLES = WORK / "bundles-rw"
LABELS = WORK / "labels.raw"
OUT = ROOT / "data/functions-rw.csv"
RW_START = 0x4A4A80
RW_END = 0x54C159  # end of _rwcseg; bundles are exported up to 0x54b000 only
CRT_BLOCKS = [(0x4A4A80, 0x4B0000), (0x51D000, 0x54B000)]
PNG_RANGE = (0x506000, 0x51D000)
ZLIB_FROM = 0x518DB0
# The D3D driver's statically initialised tables: its RwDevice (fpSystem, render state, Im2D/Im3D) and the
# rwD3D3Standards (id, function) pairs that D3D3System installs in RwEngineInstance->stdFunc.
DRIVER_TABLES = [(0x566FE4, 0x38), (0x56701C, 0x94), (0x69BEE0, 0x10)]
CB_RE = re.compile(r"\b(?:RW_CB|BMX_FNPTR)\(\s*[\w\s*]+,\s*(\w+)")
DEFINE_RE = re.compile(r"^#define\s+(\w+)\s+\(?(0x[0-9a-fA-F]+)\)?\s*$", re.M)
VAR_RE = re.compile(r"ORIG_VAR\(\s*(0x[0-9a-fA-F]+)")
IMM_RE = re.compile(r"^(?:PUSH\s+|MOV\s+(?:dword ptr )?(?:\[[^\]]*\]|[A-Z]{2,3}),)0x([0-9a-fA-F]{6,8})$")
ENTRY_RE = re.compile(r"ORIG_FN\(\s*(0x[0-9a-fA-F]+)|^BMX_ADDR\((0x[0-9a-fA-F]+)\)", re.M)

# Lowest level first: used to pick the owner of an internal reached from two groups.
LEVEL = ["core", "engine", "stream", "math", "plugins", "texture", "light", "camera", "im", "charset", "clump",
         "world", "collision"]

# Start address of each module range in the RW library -> group (entry functions).
RANGES = [
    (0x4B1280, "clump"), (0x4B3190, "plugins"), (0x4B3400, "clump"), (0x4B8000, "world"),
    (0x4C1B00, "collision"), (0x4C2200, "light"), (0x4C3000, "clump"), (0x4CB000, "charset"),
    (0x4CC000, "world"), (0x4CF000, "engine"), (0x4D0500, "math"), (0x4D8000, "engine"),
    (0x4DCB00, "texture"), (0x4DE400, "camera"), (0x4DED80, "engine"), (0x4DEF00, "im"),
    (0x4DF500, "texture"), (0x4E2000, "stream"), (0x4E3800, "texture"), (0x4E8270, "stream"), (0x4E8300, "engine"), (0x500000, "plugins"), (0x5068A0, "texture"),
    (0x506E00, "texture"),
]
# Names win over the range where they are unambiguous.
NAME_RULES = [
    (r"^(_rwPluginRegistry|_rwObjectHasFrame|RwFreeList|_rwerror|RwErrorSet|_rwString)", "core"),
    (r"Register(Plugin|PluginStream)$|PluginAttach$", "plugins"),
    (r"^(RwMatrix|RwFrame|_rwFrame|rwV3d|RwBBox)", "math"),
    (r"^RwCamera", "camera"),
    (r"^(RwEngine|RwOs|RwResources|_rwResources|RwErr|_rwMemory|CorePluginAttach|EngineOpen|_rwGetNum)", "engine"),
    (r"^RtCharset", "charset"),
    (r"^(RwTex|RwImage|RwRaster|RtBMP|RtPNG)", "texture"),
    (r"^(RwStream|_rwStream|RwMem|RwOSGetFileInterface|_rwFS|rwf)", "stream"),
    (r"^RpLight", "light"),
    (r"^RpWorld", "world"),
    (r"^(RpClump|RpAtomic|RpGeometry|RpMaterial)", "clump"),
]


# Reclassification of `orig` functions after the pointer pass (checked by hand on the pseudo-C, 2026-10-28): what only
# puts pixels on screen, or builds the pixel data a driver-chosen raster format needs, goes to the SDL3 driver set
# (`lib/d3d7driver`); what the CRT layer of bmxnative.exe (src/native/crt.c) already provides goes to `lib/crt`.
# (lo, hi, file, why); lo <= addr < hi. Ported rows are never touched.
RECLASS = [
    # Instancing pipelines of world sectors and atomics (resource arena entries, vertex buffers in the D3D vertex
    # format, morph interpolation, lighting and uv effects into them): rpWorldPipelineInit/rpAtomicPipelineInit
    # hang them in the pipeline, the device draws the result.
    (0x4C8BA0, 0x4CB000, "lib/d3d7driver", "world/atomic instancing pipelines"),
    # Instance buffer allocators and the driver's instance hook (0x4eb0d0 calls a function pointer of the driver);
    # their callers are the pipelines above and the D3D mesh code.
    (0x4EACC0, 0x4EB100, "lib/d3d7driver", "instance buffer allocators"),
    # PVS generation: renders the world from sample points into an offscreen camera, reads the pixels back
    # (RwImageSetFromRaster) and ORs the visible sectors into the PVS rows. Only used by zrwWorldSetupPvs (the
    # game normally loads precomputed PVS data) and its result only culls drawing. 0x4ce250 is the sampling
    # traversal, 0x4ccd30..0x4cd510 the sample callback, 0x4cdd14 its renderer; 0x4ce620 (row setup) stays.
    (0x4CCD30, 0x4CE000, "lib/d3d7driver", "PVS generation by offscreen rendering"),
    (0x4CE250, 0x4CE620, "lib/d3d7driver", "PVS generation sampling traversal"),
    # Colour quantiser, palette matching, dithering and image resampling that turn an RwImage into the paletted or
    # resized pixels of a raster. The raster format and size come from the device (0x4e7fc0 asks the device), and
    # the SDL3 driver offers 32-bit rasters of any size, so these never run. 0x4e0160 converts the mip levels of a
    # paletted raster, 0x4f9c90/0x4f9db0 average blocks for the D3D3 raster standards (data at 0x567290).
    (0x4E0160, 0x4E0B10, "lib/d3d7driver", "palettised mip conversion"),
    (0x4EF450, 0x4F2E00, "lib/d3d7driver", "image quantiser, dither, resample for device rasters"),
    (0x4F9C90, 0x4F9E80, "lib/d3d7driver", "D3D3 raster block average"),
    # The CRT layer already implements these string helpers the engine's string table points at.
    (0x4EE1D0, 0x4EE300, "lib/crt", "rwstricmp, rwstrupr, rwstrlwr, rwstrchr, rwstrrchr (crt.c)"),
    (0x4E8290, 0x4E82D0, "lib/crt", "rwfexist (crt.c)"),
    # Tri-strip generator behind _rpMeshGenerate (0x4c4a30, ported; it calls 0x4c6490/0x4c7560 only for geometry
    # flagged rpGEOMETRYTRISTRIP). Never reached in game: an in-game breakpoint trace (2026-10-28, boot, menus, level
    # load and riding) hit _rpMeshGenerate but none of 0x4c60a0..0x4c8870. bmxnative's trap handler names them if a
    # later run does reach them.
    (0x4C60A0, 0x4C8BA0, "lib/unreached", "tri-strip generator (not called by the game, in-game trace)"),
]


# Bodies behind a stub too short for a hook: the ported function sits at the stub's address and contains the body
# (a 5 byte jump at the stub would overwrite the first bytes of the body), so the body gets no hook of its own.
# Rows stay in the CSV as `skip` in their group's file.
FOLDED = {
    0x4C24C4: ("rw/light.c", "body of RpLightDestroy (0x4c24c0, 4 byte stub)"),
    0x4B5AB4: ("rw/clump.c", "body of RpGeometryDestroy (0x4b5ab0, 4 byte stub)"),
    0x4B21E4: ("rw/clump.c", "body of RpClumpDestroy (0x4b21e0, 4 byte stub)"),
    0x4DF764: ("rw/texture.c", "body of RwTextureDestroy (0x4df760, 4 byte stub)"),
}


def reclass(a):
    for lo, hi, f, _ in RECLASS:
        if lo <= a < hi:
            return f
    return None


def in_crt(a):
    return any(lo <= a < hi for lo, hi in CRT_BLOCKS)


def name_group(name):
    for pat, g in NAME_RULES:
        if re.search(pat, name):
            return g
    return None


def entry_group(a, name):
    g = name_group(name)
    if g:
        return g
    g = "core"
    for start, grp in RANGES:
        if a >= start:
            g = grp
    return g


def exe_image():
    """(dword reader, [(start, size)] of the data sections) of the original exe, via pefile."""
    import pefile
    pe = pefile.PE(os.environ.get("BMX_EXE", str(WORK / "NgBMX.exe")))
    base = pe.OPTIONAL_HEADER.ImageBase
    img = pe.get_memory_mapped_image()
    data = [(base + s.VirtualAddress, s.Misc_VirtualSize) for s in pe.sections
            if s.Name.rstrip(b"\0") in (b".rdata", b".data", b".data1", b"_rwdseg")]
    return (lambda a: int.from_bytes(img[a - base:a - base + 4], "little")), data


def indirect_roots(B):
    """Function-pointer candidates (see the docstring): {addr: kind} for every RW-range code address ported
    code or the exe's data/RW code uses as a pointer, whether a bundle exists or not."""
    kind = {}
    srcs = [p.read_text() for p in sorted((ROOT / "src").rglob("*")) if p.suffix in (".c", ".h")]
    defs = {}
    for t in srcs:
        defs.update({m.group(1): int(m.group(2), 16) for m in DEFINE_RE.finditer(t)})
    dword, data = exe_image()

    def add(a, k):
        if a is not None and RW_START <= a < RW_END:
            kind.setdefault(a, k)

    for t in srcs:
        for m in CB_RE.finditer(t):
            v = m.group(1)
            add(int(v, 16) if v.startswith("0x") else defs.get(v), "RW_CB")
        for m in VAR_RE.finditer(t):
            a = int(m.group(1), 16)
            if any(va <= a < va + n for va, n in data):
                add(dword(a), "ORIG_VAR")
    for va, n in data:
        for a in range(va, va + n - 3, 4):
            add(dword(a), "data")
    for b in B.values():
        for a in immediates(b):
            add(a, "imm")
    return kind


def immediates(b):
    """Function starts stored or pushed as immediates in one bundle's disassembly."""
    out = set()
    for line in b["disasm"].splitlines():
        m = IMM_RE.match(line.split(None, 1)[1]) if " " in line else None
        if m:
            out.add(int(m.group(1), 16))
    return out


def main():
    labels = {}
    for line in LABELS.read_text().splitlines():
        addr, _, name = line.partition("\t")
        labels[int(addr, 16)] = name
    B = {int(p.stem, 16): json.loads(p.read_text()) for p in BUNDLES.glob("*.json")}

    def callees(a):
        return [int(c.split()[0], 16) for c in B[a]["callees"] if not c.startswith("EXT:")]

    ported, old, kept = {}, set(), {}
    if OUT.exists():
        with OUT.open() as fh:
            rows_old = list(csv.DictReader(fh))
        ported = {int(r["addr"], 16): r for r in rows_old if r["status"] == "ported"}
        old = {int(r["addr"], 16) for r in rows_old}
        kept = {int(r["addr"], 16): r["file"] for r in rows_old if r["status"] == "skip"}  # skip rows stay skipped
    entries = set()
    for p in sorted((ROOT / "src").rglob("*")):
        if p.suffix in (".c", ".h"):
            for m in ENTRY_RE.finditer(p.read_text()):
                a = int(m.group(1) or m.group(2), 16)
                if a >= RW_START:
                    entries.add(a)
    entries |= set(ported)
    missing = sorted(a for a in entries if a not in B)
    for a in missing:
        print(f"no bundle (not a Ghidra function, called by pointer?): {a:#x} {labels.get(a, '')}", file=sys.stderr)
    entries -= set(missing)

    def closure(roots):
        seen, st = set(), list(roots)
        while st:
            a = st.pop()
            if a in seen or a not in B:
                continue
            seen.add(a)
            st += callees(a)
        return seen

    def closure_ptr(roots, stop=()):
        """Like closure, but also follows the function pointers a function stores or pushes."""
        seen, st = set(), list(roots)
        while st:
            a = st.pop()
            if a in seen or a in stop or a not in B:
                continue
            seen.add(a)
            st += callees(a) + sorted(immediates(B[a]) & B.keys())
        return seen

    # indirect targets; the driver is what only the D3D7 device table (and what it installs) reaches
    cand = indirect_roots(B)
    kinds = {a: k for a, k in cand.items() if a in B}
    inside = lambda a: any(f < a < f + b["size"] for f, b in B.items())  # noqa: E731
    for a in sorted(set(cand) - B.keys()):
        if not inside(a) and a < 0x54B000:
            print(f"no bundle for pointer target {a:#x} ({cand[a]}): create the function in Ghidra", file=sys.stderr)
    dword, _ = exe_image()
    table = {dword(t + i) for t, n in DRIVER_TABLES for i in range(0, n, 4)} & B.keys()
    table |= {a for a in B if re.search("D3D", labels.get(a, ""), re.I)}  # community labels of the driver
    dcl = closure_ptr(table)
    ncl = closure_ptr(entries | {a for a in kinds if a not in dcl})
    driver = dcl - ncl
    direct = set(entries)
    entries |= {a for a in kinds if a not in driver}
    allf = closure_ptr(entries) | driver
    for a in sorted(allf):
        for c in callees(a):
            if c not in B:
                print(f"callee without bundle: {c:#x} (from {a:#x})", file=sys.stderr)

    # libpng/zlib: reachable only through RtPNGImageRead
    png_root = next((a for a in entries if B[a]["name"] == "RtPNGImageRead"), None)
    skip = {}
    for a in allf:
        if a in driver:
            skip[a] = "lib/d3d7driver"
        elif in_crt(a):
            skip[a] = "lib/crt"
    if png_root:
        pngcl = closure_ptr([png_root])  # roots only the decoder reaches (libpng callbacks) belong to it
        others = {a for a, k in kinds.items() if k in ("RW_CB", "ORIG_VAR") or a not in pngcl}
        # the ported glue declares the libpng API it calls (png_*) as ORIG_FN; those are library, not RW
        glue = {a for a in direct if a != png_root and not labels.get(a, B[a]["name"]).startswith("png_")}
        only = pngcl - closure(glue) - closure_ptr(others, stop={png_root}) - {png_root}
        for a in only:
            name = labels.get(a, B[a]["name"])
            if PNG_RANGE[0] <= a < PNG_RANGE[1] and not re.match(r"(Rw|Rp|Rt|_rw)", name) and a not in skip:
                zlib = not name.startswith("png_") and (a >= ZLIB_FROM or a == 0x5114B0)
                skip[a] = "lib/zlib" if zlib else "lib/libpng"
    skip.update((a, f) for a, f in kept.items() if a in allf and a not in driver)
    skip.update((a, reclass(a)) for a in allf if reclass(a) and a not in ported)
    skip.update((a, FOLDED[a][0]) for a in allf if a in FOLDED and a not in ported)
    # group of every entry; internals take the groups of the entries that reach them
    egroup = {a: entry_group(a, labels.get(a, B[a]["name"])) for a in entries if a not in skip}
    reach = defaultdict(set)
    for e, g in egroup.items():
        seen, st = set(), [e]
        while st:
            a = st.pop()
            if a in seen or a in skip or a not in B:
                continue
            seen.add(a)
            reach[a].add(g)
            st += [c for c in callees(a) if c not in entries]
    rows = []
    for a in sorted(allf):
        name = labels.get(a, B[a]["name"])
        if a in skip:
            file, status = skip[a], "skip"
        else:
            status = "orig"
            if a in egroup:
                g = egroup[a]
            elif name_group(name):
                g = name_group(name)
            else:
                s = sorted(reach[a], key=LEVEL.index)
                g = s[0] if len(s) == 1 or (len(s) == 2) else "core"
            file = f"rw/{g}.c"
        conf = "label" if a in labels else "closure"
        if a in ported:
            file, status = ported[a]["file"], "ported"
        rows.append({"addr": f"{a:08x}", "size": B[a]["size"], "file": file, "confidence": conf,
                     "name": name, "status": status})
    with OUT.open("w", newline="") as fh:
        w = csv.DictWriter(fh, fieldnames=["addr", "size", "file", "confidence", "name", "status"],
                           lineterminator="\n")
        w.writeheader()
        w.writerows(rows)
    per = defaultdict(lambda: [0, 0, 0])
    for r in rows:
        per[r["file"]][0] += 1
        per[r["file"]][1] += int(r["size"])
        per[r["file"]][2] += int(r["addr"], 16) not in old
    for f, (n, b, new) in sorted(per.items()):
        print(f"{f:16} {n:4} functions {b:7} bytes, {new:3} new")
    fresh = [r for r in rows if int(r["addr"], 16) not in old]
    drv = sum(r["file"] == "lib/d3d7driver" for r in fresh)
    port = sum(r["status"] == "orig" for r in fresh)
    print(f"new rows {len(fresh)}: driver (skip) {drv}, to port {port} "
          f"({sum(int(r['size']) for r in fresh if r['status'] == 'orig')} bytes), other skip {len(fresh) - drv - port}")
    print(f"{OUT.relative_to(ROOT)}: {len(rows)} functions, {len(entries)} entries")


if __name__ == "__main__":
    main()
