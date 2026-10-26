#!/usr/bin/env python3
"""Generate data/functions-rw.csv: the RenderWare functions to port, grouped into src/rw/<group>.c.

Needs work/bundles-rw (or $BMX_WORK/bundles-rw; Ghidra export of 0x4a4a80..0x54b000, see docs/RENDERWARE.md) and work/labels.raw.
Same columns as functions.csv (addr,size,file,confidence,name,status).

Which functions: the static call closure of the RW entry functions, i.e. every ORIG_FN declaration
(and every BMX_ADDR annotation) in src/ with an address >= 0x4a4a80. Calls through function-pointer
tables (the D3D driver, plugin callbacks) are invisible to it; callbacks the game passes by address
(rwBmpImageWrite 0x4e7eb0, rwWorldPvsDefaultCallback 0x4cd510) are not Ghidra functions and have no
bundle, so they are listed on stderr instead of in the CSV.

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
CRT_BLOCKS = [(0x4A4A80, 0x4B0000), (0x51D000, 0x54B000)]
PNG_RANGE = (0x506000, 0x51D000)
ZLIB_FROM = 0x518DB0
ENTRY_RE = re.compile(r"ORIG_FN\(\s*(0x[0-9a-fA-F]+)|^BMX_ADDR\((0x[0-9a-fA-F]+)\)", re.M)

# Lowest level first: used to pick the owner of an internal reached from two groups.
LEVEL = ["engine", "stream", "math", "plugins", "texture", "light", "camera", "im", "charset", "clump",
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


def main():
    labels = {}
    for line in LABELS.read_text().splitlines():
        addr, _, name = line.partition("\t")
        labels[int(addr, 16)] = name
    B = {int(p.stem, 16): json.loads(p.read_text()) for p in BUNDLES.glob("*.json")}

    def callees(a):
        return [int(c.split()[0], 16) for c in B[a]["callees"] if not c.startswith("EXT:")]

    ported = {}
    if OUT.exists():
        with OUT.open() as fh:
            ported = {int(r["addr"], 16): r for r in csv.DictReader(fh) if r["status"] == "ported"}
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

    allf = closure(entries)
    for a in sorted(allf):
        for c in callees(a):
            if c not in B:
                print(f"callee without bundle: {c:#x} (from {a:#x})", file=sys.stderr)

    # libpng/zlib: reachable only through RtPNGImageRead
    png_root = next((a for a in entries if B[a]["name"] == "RtPNGImageRead"), None)
    skip = {}
    for a in allf:
        if in_crt(a):
            skip[a] = "lib/crt"
    if png_root:
        only = closure([png_root]) - closure(entries - {png_root}) - {png_root}
        for a in only:
            name = labels.get(a, B[a]["name"])
            if PNG_RANGE[0] <= a < PNG_RANGE[1] and not re.match(r"(Rw|Rp|Rt|_rw)", name) and a not in skip:
                zlib = not name.startswith("png_") and (a >= ZLIB_FROM or a == 0x5114B0)
                skip[a] = "lib/zlib" if zlib else "lib/libpng"
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
    per = defaultdict(lambda: [0, 0])
    for r in rows:
        per[r["file"]][0] += 1
        per[r["file"]][1] += int(r["size"])
    for f, (n, b) in sorted(per.items()):
        print(f"{f:16} {n:4} functions {b:7} bytes")
    print(f"{OUT.relative_to(ROOT)}: {len(rows)} functions, {len(entries)} entries")


if __name__ == "__main__":
    main()
