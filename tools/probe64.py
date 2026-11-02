#!/usr/bin/env python3
"""64-bit inventory (docs/PORTING.md X2): compiles every unit of the native32 build for x86_64 with -fsyntax-only
and lists the layout asserts that fail, per file, and how many other errors there are. Run from the repository
root after configuring native32. --list prints every failing assert (path:line) and every other error.

    py -I tools/probe64.py [--list] [x86_64 clang]
"""
import collections
import concurrent.futures
import json
import os
import re
import subprocess
import sys

ROOT = os.getcwd()
ARGS = [a for a in sys.argv[1:] if a != '--list']
LIST = '--list' in sys.argv[1:]
CC64 = ARGS[0] if ARGS else 'x86_64-w64-mingw32-clang'
E = json.load(open(os.path.join(ROOT, 'build', 'native32', 'compile_commands.json')))


def run(e):
    a = e['command'].replace('SDL3-3.4.18/i686-w64-mingw32', 'SDL3-3.4.18/x86_64-w64-mingw32').split()
    out = []
    skip = False
    for x in a[1:]:
        if skip:
            skip = False
            continue
        if x == '-o':
            skip = True
            continue
        if x in ('-c', '-Werror', '-msse2', '-mfpmath=sse', '-MD') or x.startswith('-MF') or x.startswith(
                '-DBMX_SOURCE_DIR') or x.startswith('-DBMX_WORK_DIR'):
            continue
        out.append(x)
    return subprocess.run([CC64, '-fsyntax-only', '-ferror-limit=0'] + out, cwd=e['directory'], capture_output=True,
                          text=True).stderr


u = set()
other = set()


def rel(path):
    return os.path.relpath(path, ROOT).replace(chr(92), '/')


with concurrent.futures.ThreadPoolExecutor(int(os.environ.get("BMX_JOBS", "16"))) as ex:
    for err in ex.map(run, E):
        for line in err.splitlines():
            m = re.match(r'^(.*?):(\d+):\d+: error: (.*)', line)
            if not m:
                continue
            if m.group(3).startswith('static assertion failed'):
                u.add((rel(m.group(1)), int(m.group(2))))
            else:
                other.add((rel(m.group(1)), int(m.group(2)), m.group(3)))
c = collections.Counter(f for f, _ in u)
outside = sum(v for f, v in c.items() if not f.startswith('src/data/'))
print(len(u), 'distinct failing layout asserts in', len(c), 'files;', outside, 'outside src/data;', len(other),
      'other errors')
for f, v in c.most_common(30):
    print(f'{v:5} {f}')
if LIST:
    for f, n in sorted(u):
        print(f'{f}:{n}')
    for f, n, msg in sorted(other):
        print(f'{f}:{n}: {msg}')
