"""Symbolic x87 tracer: straight-line FPU code -> SSA-style C (double temps, float stores).

usage: x87sym.py START END [names.json] [--calls ADDR=ret,...] [--stack e1,e2 (top first)]
Memory operands are printed via a name map {"ebx": "cam", ...} and field map {"cam+0x128": "cam->x"}.
"""
import json
import os
import re
import sys

import pefile
from capstone import CS_ARCH_X86, CS_MODE_32, Cs

pe = pefile.PE(os.environ.get("BMX_EXE", "work/NgBMX.exe"), fast_load=True)
img = pe.get_memory_mapped_image()
base = pe.OPTIONAL_HEADER.ImageBase

args = sys.argv[1:]
start, end = int(args[0], 16), int(args[1], 16)
names = {}
calls = {}
stack = []
i = 2
while i < len(args):
    if args[i] == "--calls":
        for c in args[i + 1].split(","):
            a, r = c.split("=")
            calls[int(a, 16)] = r
        i += 2
    elif args[i] == "--stack":
        stack = args[i + 1].split(",")
        i += 2
    else:
        names = json.load(open(args[i]))
        i += 1
regmap = names.get("regs", {})
fields = names.get("fields", {})

md = Cs(CS_ARCH_X86, CS_MODE_32)
md.detail = False
tmp = [0]
stored = {}  # memory text -> C name of the value now there (float var)


def memname(op):
    m = re.search(r"(dword|qword|word|tbyte) ptr \[(.*)\]", op)
    size, inner = m.group(1), m.group(2).replace(" ", "")
    mm = re.fullmatch(r"([a-z]+)([+-]0x[0-9a-f]+|[+-]\d+)?", inner)
    if mm:
        reg = mm.group(1)
        off = int(mm.group(2), 0) if mm.group(2) else 0
        key = f"{regmap.get(reg, reg)}+{off:#x}"
        txt = fields.get(key, f"M({regmap.get(reg, reg)},{off:#x})")
    elif re.fullmatch(r"0x[0-9a-f]+", inner):
        txt = fields.get(inner, f"G({inner})")
    else:
        txt = f"M[{inner}]"
    return size, txt, inner


def new(expr):
    tmp[0] += 1
    n = f"t{tmp[0]}"
    print(f"    const double {n} = {expr};")
    return n


def load(op):
    size, txt, inner = memname(op)
    if size == "dword":
        return stored.get(txt, f"(double){txt}") if txt not in stored else stored[txt]
    if size == "qword":
        return stored.get(txt, txt)
    return f"/*{size}*/{txt}"


def st_index(s):
    m = re.fullmatch(r"st\((\d)\)", s.strip())
    return int(m.group(1))


def arith(kind, a, b):
    # kind: add mul sub subr div divr ; a = ST0-ish dest, b = operand ; returns expr text of dest op
    if kind == "add":
        return f"{a} + {b}"
    if kind == "mul":
        return f"{a} * {b}"
    if kind == "sub":
        return f"{a} - {b}"
    if kind == "subr":
        return f"{b} - {a}"
    if kind == "div":
        return f"{a} / {b}"
    if kind == "divr":
        return f"{b} / {a}"


data = img[start - base:end - base]
for ins in md.disasm(data, start):
    mn, op = ins.mnemonic, ins.op_str
    b = bytes(ins.bytes)
    if mn == "call":
        target = int(op, 16) if op.startswith("0x") else None
        print(f"    // {ins.address:08x} call {op}  stack={stack}")
        if target in calls and calls[target] == "ret":
            stack.insert(0, f"ret_{ins.address:x}")
        continue
    if not mn.startswith("f"):
        if mn.startswith("j") or mn in ("ret",):
            print(f"    // {ins.address:08x} {mn} {op}  stack={stack}")
        if "test ah" in f"{mn} {op}" or mn == "sahf":
            print(f"    // {ins.address:08x} {mn} {op}")
        continue
    ops = [o.strip() for o in op.split(",")] if op else []
    base_mn = mn.rstrip("p") if mn not in ("fcomp", "fucomp", "fcompp", "fucompp") else mn
    pop = mn.endswith("p") and mn not in ("fcomp", "fucomp") or mn in ("fcompp", "fucompp")
    if mn in ("fld",):
        if "ptr" in op:
            stack.insert(0, load(op))
        else:
            stack.insert(0, stack[st_index(op)])
    elif mn == "fld1":
        stack.insert(0, "1.0")
    elif mn == "fldz":
        stack.insert(0, "0.0")
    elif mn == "fild":
        size, txt, _ = memname(op)
        stack.insert(0, f"(double)/*int {size}*/{txt}")
    elif mn in ("fst", "fstp"):
        v = stack[0]
        if "ptr" in op:
            size, txt, _ = memname(op)
            if size == "dword":
                print(f"    {txt} = (float)({v});   // {ins.address:08x}")
                stored[txt] = f"(double){txt}"
            else:
                print(f"    {txt} = {v};   // {ins.address:08x} (double)")
                stored[txt] = txt
        else:
            stack[st_index(op)] = v
        if mn == "fstp":
            stack.pop(0)
    elif mn in ("fistp", "fist"):
        size, txt, _ = memname(op)
        print(f"    {txt} = (int)rint({stack[0]});   // {ins.address:08x} {mn}")
        if mn == "fistp":
            stack.pop(0)
    elif mn == "fxch":
        k = st_index(op) if op else 1
        stack[0], stack[k] = stack[k], stack[0]
    elif mn == "fchs":
        stack[0] = new(f"-{stack[0]}")
    elif mn == "fabs":
        stack[0] = new(f"fabs({stack[0]})")
    elif mn == "fsqrt":
        stack[0] = new(f"sqrt({stack[0]})")
    elif mn in ("fsin", "fcos"):
        stack[0] = new(f"{mn}({stack[0]})")
    elif base_mn in ("fadd", "fmul", "fsub", "fsubr", "fdiv", "fdivr"):
        kind = base_mn[1:]
        if len(b) == 2 and b[0] in (0xDC, 0xDE) and b[1] >= 0xC0:
            # destination ST(i), operand ST0 (Intel semantics)
            r = b[1] & 7
            opc = (b[1] >> 3) & 7
            if opc == 0:
                e = f"{stack[r]} + {stack[0]}"
            elif opc == 1:
                e = f"{stack[r]} * {stack[0]}"
            elif opc == 4:
                e = f"{stack[0]} - {stack[r]}"
            elif opc == 5:
                e = f"{stack[r]} - {stack[0]}"
            elif opc == 6:
                e = f"{stack[0]} / {stack[r]}"
            else:
                e = f"{stack[r]} / {stack[0]}"
            stack[r] = new(e)
            if b[0] == 0xDE:
                stack.pop(0)
        elif len(b) == 2 and b[0] == 0xD8 and b[1] >= 0xC0:
            r = b[1] & 7
            stack[0] = new(arith(kind, stack[0], stack[r]))
        else:
            stack[0] = new(arith(kind, stack[0], load(op)))
    elif mn in ("fcom", "fcomp", "fcompp", "fucom", "fucomp", "fucompp"):
        other = load(op) if "ptr" in op else (stack[st_index(ops[0])] if ops else stack[1])
        print(f"    // {ins.address:08x} {mn}: compare {stack[0]} with {other}")
        n = 2 if mn in ("fcompp", "fucompp") else (1 if mn.endswith("p") else 0)
        for _ in range(n):
            stack.pop(0)
    elif mn == "fnstsw":
        print(f"    // {ins.address:08x} fnstsw")
    elif mn in ("fstsw", "fwait", "fnclex", "fldcw", "fnstcw"):
        print(f"    // {ins.address:08x} {mn} {op}")
    else:
        print(f"    // UNHANDLED {ins.address:08x} {mn} {op}")
print(f"    // end stack={stack}")
