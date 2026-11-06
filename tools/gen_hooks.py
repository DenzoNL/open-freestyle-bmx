#!/usr/bin/env python3
"""Generate the hook table from `BMX_ADDR(0x...)` annotations preceding function definitions.

Usage: tools/gen_hooks.py OUT.c SRC_DIR
"""
import re
import sys
from pathlib import Path

# Optional __attribute__((...)) or BMX_REGPARM(n) before the declaration (naked, regparm): those functions must be hooked too.
ANNOT = re.compile(r"^BMX_ADDR\((0x[0-9a-fA-F]+)\)\s*\n(?:static\s+)?(?:(?:__attribute__\(\(.*?\)\)|BMX_REGPARM\(\w\))\s*)*[\w\s\*]+?\b(\w+)\s*\(", re.M)


def scan(src_dir):
    seen = {}
    for path in sorted(Path(src_dir).rglob("*.c")):
        text = path.read_text()
        matched = 0
        for m in ANNOT.finditer(text):
            matched += 1
            addr, name = int(m.group(1), 16), m.group(2)
            if addr in seen:
                sys.exit(f"{path}: {addr:#x} already ported as {seen[addr]}")
            seen[addr] = name
        # An annotation the pattern can't pair with a definition (e.g. a comment line in between) would silently
        # leave that function unhooked: the hooked game runs the original instead, the native exe traps. Of
        # stacked annotations (guilabel.c) only the last one, next to the definition, is hooked.
        total = len(re.findall(r"^BMX_ADDR\(.*\n(?!BMX_ADDR\()", text, re.M))
        if total != matched:
            sys.exit(f"{path}: {total - matched} BMX_ADDR annotation(s) not directly followed by a function definition")
    return seen


def main():
    out, src = sys.argv[1], sys.argv[2]
    hooks = scan(src)
    # A hook writes a 5 byte jmp: a hooked function closer than that to the next one would have the tail of its jmp
    # overwritten by the next hook (the 4 byte stream stubs at 0x4e3334/0x4e3354 were such a case).
    addrs = sorted(hooks)
    for a, b in zip(addrs, addrs[1:]):
        if b - a < 5:
            sys.exit(f"{hooks[a]} at {a:#x} is {b - a} bytes before {hooks[b]} at {b:#x}: its 5 byte jmp would overlap")
    # Nor may the jmp cover the start of a function that still runs as original code (a body behind a 4 byte stub
    # that original callers call directly).
    import csv

    for table in ("functions.csv", "functions-rw.csv"):
        path = Path(src).parent / "data" / table
        if path.exists():
            for row in csv.DictReader(path.open()):
                b = int(row["addr"], 16)
                if row["status"] == "orig" and any(b - d in hooks for d in range(1, 5)):
                    sys.exit(f"{table}: {b:#x} (orig) starts inside the 5 byte jmp of a hook just before it")
    lines = ['#include "hook/hook.h"', ""]
    lines += [f"void {name}(void);" for name in sorted(set(hooks.values()))]
    lines += ["", "const BmxHook bmxHooks[] = {"]
    lines += [f"    {{{addr:#010x}, (void *){name}}}," for addr, name in sorted(hooks.items())]
    lines += ["    {0, 0},", "};", f"const uint32_t bmxHookCount = {len(hooks)};", ""]
    text = "\n".join(lines)
    if not Path(out).exists() or Path(out).read_text() != text:
        Path(out).write_text(text)


if __name__ == "__main__":
    main()
