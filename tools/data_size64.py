#!/usr/bin/env python3
"""Globals too small for the code's type on 64-bit (docs/PORTING.md X3).

The code reads an original global with ORIG_VAR(addr, type, name); src/data defines it, often as a member of a group
(src/data/globals.h). Where the code's type holds pointers it is larger with 64-bit pointers, and if src/data gives
the address a smaller member (a byte and padding, say, or a 32-bit word) the code reads and writes past it into the
next global. The 32-bit build cannot see this: there both sizes agree. This tool compiles a probe for x86_64 (and
i686) with sizeof of every site's type and of the src/data definition at its address, and lists each site whose type
is larger than the definition on 64-bit while it is not larger on 32-bit.

Sites at an offset inside a definition (a field) are not checked; neither are sites whose type the probe cannot
name (a type local to its .c file): their count is printed.

    py -I tools/data_size64.py [--list] [x86_64 clang]      (repository root, build/native32 configured)
"""
import os
import re
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import data_off64  # noqa: E402
import gen_data  # noqa: E402

ARGS = [a for a in sys.argv[1:] if a != '--list']
CC64 = ARGS[0] if ARGS else 'x86_64-w64-mingw32-clang'


def probe(exprs, target64):
    """compile_probe, dropping the expressions that do not compile; returns {index: value}."""
    ok = list(range(len(exprs)))
    while True:
        try:
            values = data_off64.compile_probe([exprs[i] for i in ok], CC64 if target64 else None, target64, True)
            return dict(zip(ok, values))
        except data_off64.ProbeError as e:
            bad = {int(n) - 5 for n in re.findall(r'probe\.c:(\d+):\d+: error', str(e))}
            print(f'probe: {len(ok)} expressions, {len(bad)} do not compile', file=sys.stderr)
            bad &= set(range(len(ok)))
            if not bad:
                sys.exit(str(e)[-3000:])
            ok = [i for k, i in enumerate(ok) if k not in bad]


def main():
    g = open(data_off64.GLOBALS).read()
    a = open(data_off64.ADDRS).read()
    groups = {}
    for m in data_off64.AT_DEFINE.finditer(g):
        name, base = m.group(1), int(m.group(2), 16)
        end = g.index('#undef AT', m.end())
        groups[name] = [(mm.group(1), int(mm.group(2), 16) - base) for mm in data_off64.AT_USE.finditer(g, m.end(),
                                                                                                         end)]
    defs = {}
    for m in data_off64.EXTERN.finditer(g):
        defs[m.group(2) or m.group(3)] = m.group(1)
    # address -> (definition, 32-bit offset)
    where = {}
    for m in data_off64.ADDR.finditer(a):
        where[int(m.group(1), 16) if m.group(1).startswith('0x') else None] = (m.group(2), data_off64.num(m, 3))
    plain = re.compile(r'#define BMX_DATA_(0x[0-9a-f]+) \(\(char \*\)&bmxAt_(\w+)\)')
    for m in plain.finditer(a):
        where[int(m.group(1), 16)] = (m.group(2), 0)

    sites, _ = gen_data.scan_sites()
    exprs, info, unchecked = [], [], 0
    wanted = set()
    for s in sites:
        d = where.get(s.addr)
        if d is None:
            unchecked += 1
            continue
        name, off = d
        group = defs.get(name, 0)
        if group:
            member = next((mm for mm, o in groups[group] if o == off), None)
            if member is None:
                continue  # inside a member: a field
            wanted.add(group)
            info.append((s, group, member, len(exprs)))
            exprs += [f'sizeof({s.type_text})']
        else:
            if off != 0:
                continue
            info.append((s, None, name, len(exprs)))
            exprs += [f'sizeof({s.type_text})', f'sizeof({name})']
    # Every member's offset and each group's size, for the groups the sites reach.
    at = {}
    for group in sorted(wanted):
        for member, _ in groups[group]:
            at[(group, member)] = len(exprs)
            exprs.append(f'offsetof(struct BmxGroup_{group}, {member})')
        at[(group, None)] = len(exprs)
        exprs.append(f'sizeof(struct BmxGroup_{group})')
    v32, v64 = probe(exprs, False), probe(exprs, True)
    # Types the src/data probe cannot name (local to their .c file): each such file's own compile names them.
    by_file = {}
    for s, _, _, i in info:
        if (i not in v32 or i not in v64) and s.path.endswith('.c'):
            by_file.setdefault(s.path, []).append((s, i))
    for path, items in sorted(by_file.items()):
        types = [f'sizeof({s.type_text})' for s, _ in items]
        for target64, v in ((False, v32), (True, v64)):
            try:
                values = data_off64.compile_probe(types, CC64 if target64 else None, target64, True, 'src/' + path)
            except data_off64.ProbeError as e:
                print(f'probe of src/{path}: {str(e)[:300]}', file=sys.stderr)
                continue
            for (_, i), value in zip(items, values):
                v[i] = value
    bad = checked = 0
    unnamed = set()
    for s, group, member, i in info:
        if i not in v32 or i not in v64:
            unchecked += 1
            unnamed.add(s.type_text)
            continue
        checked += 1
        if group is None:
            if v64[i] > v64[i + 1] and v32[i] <= v32[i + 1]:
                bad += 1
                print(f'src/{s.path}:{s.line}: {s.name} ({s.type_text}, {v64[i]} bytes) at {s.addr:#x}: {member} '
                      f'has {v64[i + 1]} on 64-bit')
            continue
        # The site covers [o32, o32 + size32) on 32-bit; on 64-bit it must end before the member that follows.
        o32 = v32[at[(group, member)]]
        end32 = o32 + v32[i]
        nxt = next((mm for mm, _ in groups[group] if v32[at[(group, mm)]] >= end32), None)
        limit32 = v32[at[(group, nxt)]]
        if end32 > limit32 if nxt is not None else end32 > v32[at[(group, None)]]:
            continue  # longer than its group already on 32-bit (a table declared shorter): not this check's
        avail64 = v64[at[(group, nxt)]] - v64[at[(group, member)]]
        if v64[i] > avail64:
            bad += 1
            print(f'src/{s.path}:{s.line}: {s.name} ({s.type_text}, {v64[i]} bytes) at {s.addr:#x}: '
                  f'{group}.{member} has {avail64} up to {nxt or "the end"} on 64-bit')
    if '--list' in sys.argv:
        print('types the probe cannot name:', ', '.join(sorted(unnamed)))
    print(f'{bad} sites larger than their place in src/data on 64-bit; {checked} checked, {unchecked} unchecked')
    return 1 if bad else 0

if __name__ == '__main__':
    sys.exit(main())
