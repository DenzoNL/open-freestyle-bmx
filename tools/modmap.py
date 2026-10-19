#!/usr/bin/env python3
"""Assign every exported function to an original source file.

Anchors come from assert/source-path strings (MSVC6 __FILE__) and community labels of the form
"file.c:Name". MSVC links each object file contiguously, so unanchored runs between two anchors of
the same file inherit it; runs between different files are split at the cut that maximises call-graph
affinity. Writes data/functions.csv (committed: addresses, sizes and names only, no original code).
"""
import csv
import json
import re
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
PATH_RE = re.compile(r"C:\\(zaxis\\Bmx\\src|NgEngine\\Zge)\\(\S+?\.c)$", re.I)


def norm(m):
    root, rel = m
    return ("zge/" if root.lower().startswith("ngengine") else "bmx/") + rel.replace("\\", "/").lower()


def main():
    bundles = {}
    import os

    for f in sorted(Path(os.environ.get("BMX_BUNDLES", ROOT / "work/bundles")).glob("*.json")):
        b = json.loads(f.read_text())
        bundles[int(b["addr"], 16)] = b
    addrs = sorted(bundles)
    anchor = {}
    for a in addrs:
        b = bundles[a]
        files = {norm(m) for s in b["strings"] for m in PATH_RE.findall(s)}
        if len(files) == 1:
            anchor[a] = files.pop()
    edges = {a: set() for a in addrs}
    for a in addrs:
        for c in bundles[a]["callees"]:
            if not c.startswith("EXT:"):
                t = int(c.split()[0], 16)
                if t in edges:
                    edges[a].add(t)
                    edges[t].add(a)
    assigned, conf = {}, {}
    anchored = [a for a in addrs if a in anchor]
    for a in anchored:
        assigned[a], conf[a] = anchor[a], "assert"
    # leading run before the first anchor
    idx = {a: i for i, a in enumerate(addrs)}
    for prev, nxt in zip([None] + anchored, anchored + [None]):
        lo = idx[prev] + 1 if prev else 0
        hi = idx[nxt] if nxt else len(addrs)
        run = addrs[lo:hi]
        if not run:
            continue
        fp = anchor.get(prev)
        fn = anchor.get(nxt)
        if fp == fn or fn is None or fp is None:
            f = fp or fn
            for a in run:
                assigned[a], conf[a] = f, "contiguous" if fp == fn else "edge"
            continue
        nodes_p = {x for x in addrs if assigned.get(x) == fp}
        nodes_n = {x for x in addrs if anchor.get(x) == fn}
        best, best_score = 0, -1
        for cut in range(len(run) + 1):
            score = sum(len(edges[x] & (nodes_p | set(run[:cut]))) for x in run[:cut]) + sum(
                len(edges[x] & (nodes_n | set(run[cut:]))) for x in run[cut:]
            )
            if score > best_score:
                best, best_score = cut, score
        for a in run[:best]:
            assigned[a], conf[a] = fp, "affinity"
        for a in run[best:]:
            assigned[a], conf[a] = fn, "affinity"
    out = ROOT / "data/functions.csv"
    with out.open("w", newline="") as fh:
        w = csv.writer(fh)
        w.writerow(["addr", "size", "file", "confidence", "name", "status"])
        for a in addrs:
            b = bundles[a]
            w.writerow([f"{a:08x}", b["size"], assigned[a], conf[a], b["name"], "orig"])
    per = {}
    for a in addrs:
        per.setdefault(assigned[a], [0, 0])
        per[assigned[a]][0] += 1
        per[assigned[a]][1] += bundles[a]["size"]
    for f, (n, s) in per.items():
        print(f"{n:5} {s:7} {f}", file=sys.stderr)


if __name__ == "__main__":
    main()
