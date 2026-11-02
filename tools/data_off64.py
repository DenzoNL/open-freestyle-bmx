#!/usr/bin/env python3
"""The 64-bit offsets of src/data (docs/PORTING.md X2c).

src/data keeps the original's globals that the code uses as one piece together in groups (structs whose members sit
at the original offsets where pointers are 32 bits wide). src/data/addrs.h reaches a global inside a group, or a
field inside a global, as the definition plus an offset; src/data/units.c points every unit at its first byte (past
a group's `lead_` bytes); src/data/globals.h asserts every group member's offset and every group's extent. Where
pointers are 64 bits wide the compiler lays the groups out differently (pointers grow, `lead_` bytes no longer line
up the alignment), so each such offset is written BMX_DATA_OFF(o32, o64) (compat/layout.h) and asserted on both
widths. This tool computes the o64 values: it compiles a probe that includes src/data's headers for i686 and
x86_64 (the native32 build's flags, layout asserts switched off), reads the offsets from the LLVM IR and rewrites
globals.h, addrs.h and units.c. Offsets equal on both widths stay plain numbers. Re-run it after changing a type in
a group (the asserts name the members whose offsets moved); it is idempotent.

    py -I tools/data_off64.py [x86_64 clang]       (repository root, build/native32 configured; then clang-format)

An offset into a member (a field of it) is only converted when the member is the same size on both widths; other
ones stop the tool (the code should name the field instead of a byte offset).
"""
import json
import os
import re
import subprocess
import sys
import tempfile

ROOT = os.getcwd()
CC64 = sys.argv[1] if len(sys.argv) > 1 else 'x86_64-w64-mingw32-clang'
GLOBALS = os.path.join(ROOT, 'src', 'data', 'globals.h')
ADDRS = os.path.join(ROOT, 'src', 'data', 'addrs.h')
UNITS = os.path.join(ROOT, 'src', 'data', 'units.c')

W = r'[\s\\]*'  # whitespace, possibly across a macro's continued lines
HEX = r'0x[0-9a-f]+'
# A number, or BMX_DATA_OFF(o32, o64): group 1 is o32.
OFF = rf'(?:BMX_DATA_OFF\({W}({HEX}|\d+),{W}(?:{HEX}|\d+){W}\)|({HEX}|\d+))'
AT_DEFINE = re.compile(rf'#define AT\(member, addr(?:, off64)?\){W}_Static_assert\({W}offsetof\(struct BmxGroup_(\w+), '
                       rf'member\){W}=={W}(?:BMX_DATA_OFF\()?{W}\(addr\){W}-{W}({HEX})(?:,{W}off64\))?,{W}#member\)')
AT_USE = re.compile(rf'^AT\((\w+), ({HEX})(?:, {HEX})?\);$', re.M)
SIZE_ASSERT = re.compile(rf'_Static_assert\({W}offsetof\(struct BmxGroup_(\w+), (\w+)\){W}\+{W}sizeof\(\(\(struct '
                         rf'BmxGroup_\1 \*\)0\)->\2\){W}=={W}{OFF},{W}"\1"\);')
ADDR = re.compile(rf'#define BMX_DATA_(\w+){W}\(\(char \*\)&bmxAt_(\w+) \+{W}{OFF}\)')
UNIT = re.compile(rf'\{{({HEX}), ({HEX}), \(const char \*\)&(\w+) \+ {OFF}\}}')
EXTERN = re.compile(r'^extern (?:const )?(?:struct BmxGroup_(\w+) (\w+)|.*?\b(\w+)(?:\[[^]]*\])*);', re.M)


def num(m, first):
    """The o32 of an OFF match whose groups start at `first`."""
    return int(m.group(first) or m.group(first + 1), 0)


def compile_probe(exprs, cc, target64):
    """Each expression's value, compiled like src/data/misc.c (for x86_64 when target64)."""
    cmds = json.load(open(os.path.join(ROOT, 'build', 'native32', 'compile_commands.json')))
    entry = next(e for e in cmds if e['file'].replace(chr(92), '/').endswith('src/data/misc.c'))
    with tempfile.TemporaryDirectory() as tmp:
        probe = os.path.join(tmp, 'probe.c')
        out = os.path.join(tmp, 'probe.ll')
        with open(probe, 'w') as f:
            f.write('#pragma clang diagnostic ignored "-Wkeyword-macro"\n#define _Static_assert(c, m)\n'
                    '#include "data/data.h"\nconst unsigned long long bmxProbe[] = {\n')
            f.write(''.join(f'    (unsigned long long)({e}),\n' for e in exprs))
            f.write('};\n')
        args = entry['command'].split()
        if target64:
            args = [a.replace('SDL3-3.4.18/i686-w64-mingw32', 'SDL3-3.4.18/x86_64-w64-mingw32') for a in args]
        keep, skip = [], False
        for a in args[1:]:
            if skip:
                skip = False
                continue
            if a == '-o':
                skip = True
                continue
            if a in ('-c', '-Werror', '-msse2', '-mfpmath=sse', '-MD') or a.startswith('-MF') or a.endswith('misc.c'):
                continue
            keep.append(a)
        compiler = cc if target64 else args[0]
        r = subprocess.run([compiler] + keep + ['-w', '-S', '-emit-llvm', '-o', out, probe], cwd=entry['directory'],
                           capture_output=True, text=True)
        if r.returncode != 0:
            sys.exit(r.stderr[-4000:])
        ll = open(out).read()
    m = re.search(r'@bmxProbe = .*?constant \[\d+ x i64\] \[(.*?)\]', ll)
    values = [int(v) for v in re.findall(r'i64 (-?\d+)', m.group(1))]
    if len(values) != len(exprs):
        sys.exit('probe: wrong count')
    return values


def offset(o32, o64, hexa=True):
    f = (lambda v: f'{v:#x}') if hexa else str
    return f(o32) if o32 == o64 else f'BMX_DATA_OFF({f(o32)}, {f(o64)})'


def main():
    g = open(GLOBALS).read()
    a = open(ADDRS).read()
    units = open(UNITS).read()

    # Group members (AT) and extents, with their 32-bit offsets from the source.
    groups = {}
    for m in AT_DEFINE.finditer(g):
        name, base = m.group(1), int(m.group(2), 16)
        end = g.index('#undef AT', m.end())
        members = [(mm.group(1), int(mm.group(2), 16) - base) for mm in AT_USE.finditer(g, m.end(), end)]
        groups[name] = {'base': base, 'members': members, 'span': (m.start(), end)}
    sizes = {m.group(1): (m.group(2), num(m, 3)) for m in SIZE_ASSERT.finditer(g)}
    if len(groups) != g.count('\nstruct BmxGroup_') or len(sizes) != len(groups):
        sys.exit(f'{len(groups)} AT blocks and {len(sizes)} size asserts for {g.count(chr(10) + "struct BmxGroup_")} '
                 f'groups')

    # Definitions: name -> its group, or None for a plain definition.
    defs = {}
    for m in EXTERN.finditer(g):
        if m.group(1):
            defs[m.group(2)] = m.group(1)
        elif m.group(3):
            defs[m.group(3)] = None

    exprs, keys = [], []

    def ask(key, expr):
        keys.append(key)
        exprs.append(expr)

    for name, gr in groups.items():
        for member, _ in gr['members']:
            ask(('at', name, member), f'offsetof(struct BmxGroup_{name}, {member})')
            ask(('msize', name, member), f'sizeof(((struct BmxGroup_{name} *)0)->{member})')
    for name, (last, _) in sizes.items():
        ask(('size', name), f'offsetof(struct BmxGroup_{name}, {last}) + sizeof(((struct BmxGroup_{name} *)0)->{last})')
    for u in sorted({m.group(2) for m in ADDR.finditer(a) if defs.get(m.group(2), 0) is None}):
        ask(('usize', u), f'sizeof({u})')

    v32 = dict(zip(keys, compile_probe(exprs, None, False)))
    v64 = dict(zip(keys, compile_probe(exprs, CC64, True)))

    # The 32-bit values must be the source's.
    for name, gr in groups.items():
        for member, off in gr['members']:
            if v32[('at', name, member)] != off:
                sys.exit(f'{name}.{member}: i686 offset {v32[("at", name, member)]:#x}, source says {off:#x}')
        if v32[('size', name)] != sizes[name][1]:
            sys.exit(f'{name}: i686 extent {v32[("size", name)]:#x}, source says {sizes[name][1]:#x}')

    # addrs.h: a global inside a group, a field inside a definition.
    bad = []
    count = {'addrs': 0, 'units': 0, 'groups': 0}

    def addr_sub(m):
        spelling, unit, off = m.group(1), m.group(2), num(m, 3)
        group = defs.get(unit)
        o64 = off
        if group is None:
            if v32[('usize', unit)] != v64[('usize', unit)]:
                bad.append(f'{spelling}: offset {off:#x} into {unit}, whose size differs')
        else:
            inside = [(mem, o) for mem, o in groups[group]['members'] if o <= off < o + v32[('msize', group, mem)]]
            if not inside:
                bad.append(f'{spelling}: offset {off:#x} into {unit} lands on no member')
            else:
                mem, o = inside[-1]
                if off != o and v32[('msize', group, mem)] != v64[('msize', group, mem)]:
                    bad.append(f'{spelling}: offset {off - o:#x} into {group}.{mem}, whose size differs')
                o64 = v64[('at', group, mem)] + off - o
        count['addrs'] += o64 != off
        return f'#define BMX_DATA_{spelling} ((char *)&bmxAt_{unit} + {offset(off, o64)})'

    a = ADDR.sub(addr_sub, a)
    if bad:
        sys.exit('\n'.join(bad))
    if '#include "compat/layout.h"' not in a:
        a = a.replace('#define BMX_DATA_ADDRS_H\n', '#define BMX_DATA_ADDRS_H\n\n#include "compat/layout.h"\n', 1)

    # units.c: a group's unit starts at its first member.
    def unit_sub(m):
        lo, hi, group, lead = m.group(1), m.group(2), m.group(3), num(m, 4)
        first = groups[group]['members'][0][0]
        if v32[('at', group, first)] != lead:
            sys.exit(f'units.c: {group} + {lead}, its first member is at {v32[("at", group, first)]}')
        o64 = v64[('at', group, first)]
        count['units'] += o64 != lead
        return f'{{{lo}, {hi}, (const char *)&{group} + {offset(lead, o64, hexa=False)}}}'

    units = UNIT.sub(unit_sub, units)

    # globals.h: per group, the AT macro takes the 64-bit offset where any member moves.
    out, pos = [], 0
    for name, gr in sorted(groups.items(), key=lambda kv: kv[1]['span'][0]):
        lo, hi = gr['span']
        moved = any(v64[('at', name, mem)] != off for mem, off in gr['members'])
        count['groups'] += moved or v64[('size', name)] != sizes[name][1]
        base = f'{gr["base"]:#010x}'
        if moved:
            head = (f'#define AT(member, addr, off64) _Static_assert(offsetof(struct BmxGroup_{name}, member) == '
                    f'BMX_DATA_OFF((addr) - {base}, off64), #member)')
            uses = [f'AT({mem}, {gr["base"] + off:#010x}, {v64[("at", name, mem)]:#x});' for mem, off in gr['members']]
        else:
            head = (f'#define AT(member, addr) _Static_assert(offsetof(struct BmxGroup_{name}, member) == (addr) - '
                    f'{base}, #member)')
            uses = [f'AT({mem}, {gr["base"] + off:#010x});' for mem, off in gr['members']]
        out += [g[pos:lo], head + '\n' + '\n'.join(uses) + '\n']
        pos = hi
    out.append(g[pos:])
    g = ''.join(out)

    def size_sub(m):
        name, last = m.group(1), m.group(2)
        value = offset(sizes[name][1], v64[('size', name)])
        return (f'_Static_assert(offsetof(struct BmxGroup_{name}, {last}) + sizeof(((struct BmxGroup_{name} *)0)'
                f'->{last}) == {value}, "{name}");')

    g = SIZE_ASSERT.sub(size_sub, g)

    open(GLOBALS, 'w', newline='\n').write(g)
    open(ADDRS, 'w', newline='\n').write(a)
    open(UNITS, 'w', newline='\n').write(units)
    print(f'{count["groups"]} groups, {count["addrs"]} addrs.h offsets and {count["units"]} units.c entries take a '
          f'64-bit value')


main()
