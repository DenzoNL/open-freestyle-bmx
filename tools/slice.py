#!/usr/bin/env python3
"""Plan porting slices and build self-contained work packets for them.

  tools/slice.py plan [--target 60]       print slices: groups of whole files, bottom-up by call graph
  tools/slice.py packet FILE... [-o OUT]  write a Markdown work packet for those source files

A packet holds everything a porter needs and nothing else: the CSV rows, each function's pseudo-C,
disassembly where the function touches the x87 FPU (pseudo-C is unreliable there), strings,
callers/callees with their port status, and ORIG_FN stubs for unported callees outside the slice.
"""
import argparse
import csv
import json
import re
from collections import defaultdict
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
FPU = re.compile(r"^\S+\s+F[A-Z]+", re.M)


def load():
    with (ROOT / "data/functions.csv").open() as fh:
        rows = {int(r["addr"], 16): r for r in csv.DictReader(fh)}
    bundles = {}
    for a in rows:
        p = ROOT / f"work/bundles/{a:08x}.json"
        if p.exists():
            bundles[a] = json.loads(p.read_text())
    return rows, bundles


def callees(b):
    for c in b["callees"]:
        if not c.startswith("EXT:"):
            yield int(c.split()[0], 16)


def plan(target):
    rows, bundles = load()
    files = defaultdict(list)
    for a, r in rows.items():
        if r["status"] == "orig":
            files[r["file"]].append(a)
    fof = {a: r["file"] for a, r in rows.items()}
    deps = {f: {fof[c] for a in fs if a in bundles for c in callees(bundles[a]) if c in fof} - {f} for f, fs in files.items()}
    order, done = [], set()
    while len(done) < len(files):
        # pick files whose deps are done; break cycles by fewest unmet deps
        ready = sorted((len(deps[f] - done), -len(files[f]), f) for f in files if f not in done)
        f = ready[0][2]
        order.append(f)
        done.add(f)
    slices, cur, n = [], [], 0
    for f in order:
        if cur and n + len(files[f]) > target * 1.5:
            slices.append((cur, n))
            cur, n = [], 0
        cur.append(f)
        n += len(files[f])
        if n >= target:
            slices.append((cur, n))
            cur, n = [], 0
    if cur:
        slices.append((cur, n))
    for i, (fs, n) in enumerate(slices, 1):
        b = sum(int(rows[a]["size"]) for f in fs for a in files[f])
        print(f"slice {i:3}: {n:4} funcs {b:7} bytes  {' '.join(fs)}")


def packet(sel, out, addrs=None, exclude=()):
    rows, bundles = load()
    if addrs:
        mine = sorted(a for a in addrs if a in rows)
    else:
        mine = sorted(a for a, r in rows.items() if r["file"] in sel and a not in exclude)
    mine_set = set(mine)
    lines = [f"# Work packet: {', '.join(sel) if sel else 'selected functions'}", ""]
    lines += [f"{len(mine)} functions. Port all of them; follow CONTRIBUTING.md exactly.", ""]
    lines += ["| addr | size | file | confidence | ghidra name | status |", "|---|---|---|---|---|---|"]
    for a in mine:
        r = rows[a]
        lines.append(f"| {r['addr']} | {r['size']} | {r['file']} | {r['confidence']} | {r['name']} | {r['status']} |")
    ext = {}
    for a in mine:
        for c in callees(bundles[a]):
            if c not in mine_set and c in rows and rows[c]["status"] == "orig":
                ext[c] = bundles.get(c, {}).get("signature", "?")
    if ext:
        lines += ["", "## Unported callees outside this slice", "",
                  "Declare these with ORIG_FN in the owning header if not already declared (fix the types):", "", "```c"]
        for c, sig in sorted(ext.items()):
            lines.append(f"// {c:#010x} [{rows[c]['file']}] {sig}")
        lines.append("```")
    for a in mine:
        b = bundles[a]
        fpu = bool(FPU.search(b["disasm"]))
        lines += ["", f"## {a:#010x} {b['name']} ({b['size']} bytes{', x87' if fpu else ''})", ""]
        callers = ", ".join(c + (" (in slice)" if int(c, 16) in mine_set else "") for c in b["callers"][:12])
        lines.append(f"- callers: {callers or 'none (called through pointers?)'}")
        ce = []
        for c in b["callees"]:
            if c.startswith("EXT:"):
                ce.append(c)
            else:
                t = int(c.split()[0], 16)
                st = "slice" if t in mine_set else rows.get(t, {}).get("status", "lib")
                ce.append(f"{c} [{st}]")
        lines.append(f"- callees: {', '.join(ce) or 'none'}")
        if b["strings"]:
            lines.append(f"- strings: {json.dumps(b['strings'])}")
        if b["globals"]:
            lines.append(f"- data refs: {', '.join(b['globals'][:20])}")
        lines += ["", "```c", b["pseudo_c"].strip(), "```"]
        if fpu:
            lines += ["", "Disassembly (authoritative for FP):", "", "```", b["disasm"].rstrip(), "```"]
    text = "\n".join(lines) + "\n"
    out.parent.mkdir(parents=True, exist_ok=True)
    out.write_text(text)
    print(f"{out}: {len(mine)} functions, {len(text) // 4} tokens (approx)")


def main():
    ap = argparse.ArgumentParser()
    sub = ap.add_subparsers(dest="cmd", required=True)
    p = sub.add_parser("plan")
    p.add_argument("--target", type=int, default=60)
    k = sub.add_parser("packet")
    k.add_argument("files", nargs="*")
    k.add_argument("-o", type=Path)
    k.add_argument("--addrs", help="comma-separated function addresses instead of whole files")
    k.add_argument("--exclude", default="", help="comma-separated addresses to leave out of whole files")
    a = ap.parse_args()
    if a.cmd == "plan":
        plan(a.target)
    else:
        addrs = [int(x, 16) for x in a.addrs.split(",")] if a.addrs else None
        exclude = {int(x, 16) for x in a.exclude.split(",") if x}
        name = "+".join(Path(f).stem for f in a.files) or "custom"
        packet(a.files, a.o or ROOT / f"work/slices/{name}.md", addrs, exclude)


if __name__ == "__main__":
    main()
