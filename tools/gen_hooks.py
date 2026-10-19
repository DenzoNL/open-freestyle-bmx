#!/usr/bin/env python3
"""Generate the hook table from `BMX_ADDR(0x...)` annotations preceding function definitions.

Usage: tools/gen_hooks.py OUT.c SRC_DIR
"""
import re
import sys
from pathlib import Path

ANNOT = re.compile(r"^BMX_ADDR\((0x[0-9a-fA-F]+)\)\s*\n(?:static\s+)?[\w\s\*]+?\b(\w+)\s*\(", re.M)


def scan(src_dir):
    seen = {}
    for path in sorted(Path(src_dir).rglob("*.c")):
        for m in ANNOT.finditer(path.read_text()):
            addr, name = int(m.group(1), 16), m.group(2)
            if addr in seen:
                sys.exit(f"{path}: {addr:#x} already ported as {seen[addr]}")
            seen[addr] = name
    return seen


def main():
    out, src = sys.argv[1], sys.argv[2]
    hooks = scan(src)
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
