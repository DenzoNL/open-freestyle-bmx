#!/usr/bin/env python3
"""Update data/functions.csv status from BMX_ADDR annotations and print per-file progress.

Usage: tools/progress.py [--check]   (--check: fail if the CSV is out of date, for CI)
"""
import csv
import re
import sys
from collections import defaultdict
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
CSV = ROOT / "data/functions.csv"
ANNOT = re.compile(r"^BMX_ADDR\((0x[0-9a-fA-F]+)\)", re.M)


def main():
    ported = {}
    for p in sorted((ROOT / "src").rglob("*.c")):
        for m in ANNOT.finditer(p.read_text()):
            ported[int(m.group(1), 16)] = p.relative_to(ROOT / "src").as_posix()
    with CSV.open() as fh:
        rows = list(csv.DictReader(fh))
    known = {int(r["addr"], 16) for r in rows}
    unknown = sorted(set(ported) - known)
    if unknown:
        sys.exit("BMX_ADDR not in data/functions.csv: " + ", ".join(f"{a:#x}" for a in unknown))
    changed = False
    for r in rows:
        a = int(r["addr"], 16)
        status = "ported" if a in ported else "orig"
        if status != r["status"] or (a in ported and r["file"] != ported[a].removesuffix(".c") + ".c"):
            changed = True
            r["status"] = status
            if a in ported:
                r["file"] = ported[a]
    if "--check" in sys.argv:
        if changed:
            sys.exit("data/functions.csv is stale: run tools/progress.py")
    elif changed:
        with CSV.open("w", newline="") as fh:
            w = csv.DictWriter(fh, fieldnames=list(rows[0]))
            w.writeheader()
            w.writerows(rows)
    per = defaultdict(lambda: [0, 0, 0, 0])
    for r in rows:
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
    print(f"{'TOTAL':42} {pn:4}/{tn:<4} {100 * pb / tb:5.1f}% bytes ({pb}/{tb})")


if __name__ == "__main__":
    main()
