"""Find function pointers in the exe's data whose initial value is a ported function that calls through
the same pointer (directly or in a cycle).

A ported function that reaches the game's API through a data pointer (ORIG_VAR of function-pointer type,
or a `CALL [abs]` in the original) works in the emulator tests when the harness stubs the callee, but
loops forever in the real game when the pointer's initial value is the hooked function itself
(zrwTimeGetTime at 0x489cb0 calls [0x561480], whose initial value is 0x489cb0).

Two views:
  1. every `CALL/JMP dword ptr [abs]` in the original disassembly of any function, resolved through the
     initial value in the exe: edges f -> target. Reports self loops and cycles that contain a ported function.
  2. every ORIG_VAR of function-pointer type whose initial value is a ported function (informational).

Usage: py -I tools/selfref_audit.py .   (needs $BMX_WORK or work/, $BMX_EXE or work/NgBMX.exe)
"""
import re
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
import audit_common as ac  # noqa: E402

repo = Path(sys.argv[1] if len(sys.argv) > 1 else ".").resolve()
ported = ac.load_ported(repo)
bundles = ac.load_bundles(repo)
exe = ac.Exe(ac.exe_path(repo))
resolve = ac.thunk_resolver(bundles, exe)


def initial(slot):
    v = exe.u32(slot)
    return None if v is None else resolve(v)



IND = re.compile(r"^(?:CALL|JMP)\s+dword ptr \[(0x[0-9a-f]+)\]$", re.I)
edges = {}  # func -> {(slot, target, insn addr)}
LOAD = re.compile(r"^MOV\s+(E[A-Z]{2}),dword ptr \[(0x[0-9a-f]+)\]$", re.I)
CALLREG = re.compile(r"^(?:CALL|JMP)\s+(E[A-Z]{2})$", re.I)
for fa, (_, insns) in bundles.items():
    for i, (a, t) in enumerate(insns):
        slot = None
        m = IND.match(t)
        if m:
            slot = int(m.group(1), 16)
        else:
            m = CALLREG.match(t)
            if m:  # MOV reg,[abs] a few instructions earlier, reg untouched since
                for _, prev in reversed(insns[max(0, i - 6) : i]):
                    ml = LOAD.match(prev)
                    if ml and ml.group(1).upper() == m.group(1).upper():
                        slot = int(ml.group(2), 16)
                        break
                    if re.match(r"^\w+\s+" + m.group(1) + r"\b", prev, re.I) and not prev.upper().startswith(("PUSH", "CMP", "TEST")):
                        break
        if slot is None:
            continue
        tgt = initial(slot)
        if tgt in bundles:
            edges.setdefault(fa, set()).add((slot, tgt, a))


def name(a):
    p = ported.get(a)
    return f"{a:08x} {p['name']}" if p else f"{a:08x}" + ("" if a in bundles else " ?")


fnty = set()
srcs = [p for p in (repo / "src").rglob("*") if p.suffix in (".c", ".h")]
for path in srcs:
    for m in re.finditer(r"typedef[^;{]*?\(\s*(?:BMX_\w+\s*)?\*\s*(\w+)\s*\)\s*\(", path.read_text(errors="replace")):
        fnty.add(m.group(1))
vars_ = set()
for path in srcs:
    for m in re.finditer(r"#define\s+(\w+)\s+ORIG_VAR\((0x[0-9a-fA-F]+),\s*(\w+),", path.read_text(errors="replace")):
        if m.group(3) in fnty:
            vars_.add((int(m.group(2), 16), m.group(1)))
# edges from the port: ORIG_VAR function pointers used in a ported body, resolved through the exe's initial value
port_edges = {}
for f, p in ported.items():
    for slot, nm in vars_:
        if re.search(r"\b" + re.escape(nm) + r"\b", p["body"]) and initial(slot) in bundles:
            port_edges.setdefault(f, set()).add((slot, initial(slot), nm))
orig_adj = {f: {t for _, t, _ in e} for f, e in edges.items()}
adj = {f: set(orig_adj.get(f, ())) | {t for _, t, _ in port_edges.get(f, ())} for f in set(edges) | set(port_edges)}
cycles = {}


def dfs(start, cur, path):
    for nxt in adj.get(cur, ()):
        if nxt == start:
            cycles.setdefault(frozenset(path), tuple(path))
        elif nxt not in path and len(path) < 6:
            dfs(start, nxt, path + [nxt])


for f in adj:
    dfs(f, f, [f])

hit = [c for c in cycles.values() if any(x in ported for x in c)]
for c in sorted(hit, key=lambda c: (len(c), c)):
    kind = "direct recursion" if len(c) == 1 else f"cycle of {len(c)}"
    print(f"{kind}: " + " -> ".join(name(x) for x in c) + f" -> {name(c[0])}")
    for f in c:
        p = ported.get(f)
        where = f" ({p['path']}:{p['line']})" if p else ""
        for slot, tgt, a in sorted(edges.get(f, ())):
            if tgt in c:
                print(f"    original {a:08x}: call via [{slot:08x}] (initial value {tgt:08x}) in {name(f)}")
        for slot, tgt, nm in sorted(port_edges.get(f, ())):
            if tgt in c:
                print(f"    port: {nm} [{slot:08x}] (initial value {tgt:08x}) used in {name(f)}{where}")

to_ported = [(s, n, initial(s)) for s, n in sorted(vars_) if initial(s) in ported]
print(f"\n{len(ported)} ported functions; {sum(len(e) for e in edges.values())} indirect calls through exe data "
      f"that resolve to a function")
print(f"{len(vars_)} function-pointer ORIG_VARs, {len(to_ported)} start out pointing at a ported function:")
for slot, nm, v in to_ported:
    print(f"    [{slot:08x}] {nm} -> {name(v)}")
print(f"{len(hit)} recursion/cycle finding(s) involving a ported function")
