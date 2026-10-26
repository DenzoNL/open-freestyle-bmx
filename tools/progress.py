#!/usr/bin/env python3
"""Update the function CSVs from BMX_ADDR annotations and print per-file progress.

Usage: tools/progress.py [--check]   (--check: fail if a CSV is out of date, for CI)

Two lists: data/functions.csv (the game) and data/functions-rw.csv (the RenderWare library, from
tools/rwmap.py; files rw/<group>.c). A BMX_ADDR belongs to the list that has its address. Progress is
reported separately; in the RW list `skip` rows (CRT, libpng, zlib) are not counted as to-do.
"""
import csv
import re
import sys
from collections import defaultdict
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
LISTS = [("game", ROOT / "data/functions.csv"), ("RenderWare", ROOT / "data/functions-rw.csv")]
ANNOT = re.compile(r"^BMX_ADDR\((0x[0-9a-fA-F]+)\)", re.M)


def update(rows, ported):
    """Set status/file of rows from the annotations; returns True if anything changed."""
    changed = False
    for r in rows:
        a = int(r["addr"], 16)
        status = "ported" if a in ported else ("skip" if r["status"] == "skip" else "orig")
        if status != r["status"] or (a in ported and r["file"] != ported[a].removesuffix(".c") + ".c"):
            changed = True
            r["status"] = status
            if a in ported:
                r["file"] = ported[a]
    return changed


def report(rows):
    per = defaultdict(lambda: [0, 0, 0, 0])
    for r in rows:
        if r["status"] == "skip":
            continue
        k = per[r["file"]]
        k[0] += 1
        k[1] += int(r["size"])
        if r["status"] == "ported":
            k[2] += 1
            k[3] += int(r["size"])
    tn = sum(v[0] for v in per.values())
    tb = sum(v[1] for v in per.values())
    pn = sum(v[2] for v in per.values())
    pb = sum(v[3] for v in per.values())
    for f, (n, b, p, pbyt) in sorted(per.items()):
        if p:
            print(f"{f:42} {p:4}/{n:<4} {100 * pbyt / b:5.1f}% bytes")
    print(f"{'TOTAL':42} {pn:4}/{tn:<4} {100 * pb / max(tb, 1):5.1f}% bytes ({pb}/{tb})")


def main():
    ported = {}
    for p in sorted((ROOT / "src").rglob("*.c")):
        for m in ANNOT.finditer(p.read_text()):
            ported[int(m.group(1), 16)] = p.relative_to(ROOT / "src").as_posix()
    lists = []
    for name, path in LISTS:
        if path.exists():
            with path.open() as fh:
                lists.append((name, path, list(csv.DictReader(fh))))
    known = {int(r["addr"], 16) for _, _, rows in lists for r in rows}
    unknown = sorted(set(ported) - known)
    if unknown:
        sys.exit("BMX_ADDR not in data/functions*.csv: " + ", ".join(f"{a:#x}" for a in unknown))
    stale = []
    for name, path, rows in lists:
        mine = {a: f for a, f in ported.items() if a in {int(r["addr"], 16) for r in rows}}
        if update(rows, mine):
            stale.append(path.name)
            if "--check" not in sys.argv:
                with path.open("w", newline="") as fh:
                    w = csv.DictWriter(fh, fieldnames=list(rows[0]), lineterminator="\n")
                    w.writeheader()
                    w.writerows(rows)
    if stale and "--check" in sys.argv:
        sys.exit(f"{', '.join(stale)} stale: run tools/progress.py")
    for name, _, rows in lists:
        if name != "game":
            print(f"\n== {name} ({sum(r['status'] == 'skip' for r in rows)} library functions skipped)")
        report(rows)


if __name__ == "__main__":
    main()
