"""Find ported functions declared void whose original callers read EAX after the call.

Ghidra shows some functions as void that leave a value in EAX (typically init functions ending in
MOV EAX,1) which callers test. A void port leaves garbage there; the differential test of a void
function doesn't compare EAX, so only this check (or the real game) catches it.

Usage: python3 -I tools/void_eax_audit.py .   (needs work/bundles and work/bundles-rw; $BMX_WORK overrides work/)
"""
import json
import os
import re
import sys
from pathlib import Path

repo = Path(sys.argv[1])
ANNOT = re.compile(r"^BMX_ADDR\((0x[0-9a-fA-F]+)\)\s*\n(?:static\s+)?(?:__attribute__\(\(.*?\)\)\s*)*((?:static\s+)?[\w\s\*]+?)\b(\w+)\s*\(", re.M)
INSN = re.compile(r"^([0-9a-f]{8})\s+(.*)$")
EAX_READ = re.compile(r"\b(EAX|AX|AL|AH)\b")
EAX_WRITE_DEST = re.compile(r"^(MOV|MOVZX|MOVSX|LEA|POP|XOR|SUB|SETNZ|SETZ|SET\w+|CDQ)\s+(EAX|AX|AL)\b")

voids = {}
for path in sorted((repo / "src").rglob("*.c")):
    for m in ANNOT.finditer(path.read_text(errors="replace")):
        ret = m.group(2).replace("static", "").strip()
        if ret == "void":
            voids[int(m.group(1), 16)] = (m.group(3), str(path.relative_to(repo)))


def disasm(bundle):
    out = [INSN.match(x.strip()) for x in bundle.get("disasm", "").splitlines()]
    return [(int(m.group(1), 16), m.group(2).strip()) for m in out if m]


hits = {}
work = Path(os.environ.get("BMX_WORK", repo / "work"))
for f in sorted([*(work / "bundles").glob("*.json"), *(work / "bundles-rw").glob("*.json")]):
    insns = disasm(json.loads(f.read_text()))
    for i, (addr, text) in enumerate(insns):
        m = re.match(r"CALL\s+0x([0-9a-f]+)$", text, re.I)
        if not m or int(m.group(1), 16) not in voids:
            continue
        target = int(m.group(1), 16)
        for _, nxt in insns[i + 1 : i + 6]:
            op = nxt.split()[0].upper() if nxt.split() else ""
            if op.startswith("J") or op in ("RET", "CALL"):
                break
            if EAX_WRITE_DEST.match(nxt.upper()) and not re.match(r"^(XOR|SUB)\s+(EAX|AX|AL),\s*(?!\2)", nxt.upper()):
                break
            if re.match(r"^(FNSTSW\s+AX|OR\s+EAX,0xffffffff)$", nxt, re.I):  # writes, not reads
                break
            if EAX_READ.search(nxt.upper()):
                hits.setdefault(target, []).append(f"{addr:08x}: {nxt}")
                break

print(f"{len(voids)} ported void functions, {len(hits)} with callers that read EAX after the call")
for target, uses in sorted(hits.items()):
    name, path = voids[target]
    print(f"\n{target:08x} {name} ({path}) — {len(uses)} use(s)")
    for u in uses[:4]:
        print("    " + u)
