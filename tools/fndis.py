"""Per-function disassembly of hook-build objects, for checking that a cleanup did not change the code.

Writes one file per function (section addresses stripped, so moving a function or growing its neighbours
does not show up) into OUTDIR/fn. Run it before and after a cleanup and diff the two directories:

    python3 -I tools/fndis.py work/fn-before src/zge/com/zglrw/zrwmodel.c src/zge/zobj.c
    ... edit, rebuild ...
    python3 -I tools/fndis.py work/fn-after src/zge/com/zglrw/zrwmodel.c src/zge/zobj.c
    diff -r work/fn-before/fn work/fn-after/fn

Every difference must be the same computation (other addressing of the same field, constant stores merged,
relocations of renamed symbols). Needs llvm-objdump on PATH (or $OBJDUMP) and a built build/hook.
"""
import os
import re
import shutil
import subprocess
import sys

out, sources = sys.argv[1], sys.argv[2:]
objdump = os.environ.get("OBJDUMP") or shutil.which("llvm-objdump") or "llvm-objdump"
text = ""
for src in sources:
    obj = os.path.join("build/hook/CMakeFiles/bmx_game.dir", src + ".obj")
    if not os.path.exists(obj):
        sys.exit(f"fndis: {obj} not found (build the hook preset first)")
    text += subprocess.run([objdump, "-d", "-r", "--no-show-raw-insn", "--no-leading-addr", obj],
                           capture_output=True, text=True, check=True).stdout

functions = {}
current = None
for line in text.splitlines():
    # Section symbols look like ".weak._name.default.<anything>"; keep the function name only.
    line = re.sub(r"\.weak\.(_\w+)\.default\.[^>+]*", r"\1", line)
    label = re.match(r"^<(.+)>:$", line.strip())
    if label:
        current = label.group(1)
        functions[current] = []
    elif current and line.strip() and not line.startswith(("Disassembly of", ".")) and "file format" not in line:
        functions[current].append(re.sub(r"\b0x[0-9a-f]+ <", "<", line.rstrip()))

os.makedirs(os.path.join(out, "fn"), exist_ok=True)
for name, lines in functions.items():
    with open(os.path.join(out, "fn", re.sub(r"[^\w.]", "_", name) + ".s"), "w") as fh:
        fh.write("\n".join(lines) + "\n")
print(f"fndis: {len(functions)} functions -> {out}/fn")
