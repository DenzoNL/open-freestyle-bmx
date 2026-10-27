"""Compare how the original widens 8- and 16-bit values with how the port does.

A narrow value reaching a call or a function parameter is widened by MOVSX / CWDE (sign) or MOVZX / AND 0xff(ff) /
XOR r,r + MOV AX (zero). A port that casts the other way ((int16_t)group where the original ANDs 0xffff,
zprimLoadGroup) works for every test value below 0x8000 and breaks the real game for ids above.

Checks:
  A. Arguments: for every original call (direct, through a thunk, or through an exe data pointer) in a ported function,
     the widening the original applies to each pushed register argument vs the port's argument expression at the same
     callee and position (a narrow cast, a local / parameter of narrow type, a struct member whose name has one
     narrow type everywhere). Where the original passes the value unwidened, a narrow cast in the port is only
     listed if the callee does not itself narrow that parameter to the same or a smaller width.
  B. Parameters: the widening the original applies to a stack parameter on first use vs the port's declared type
     (signedness and width). A full-width port parameter that the port body does not narrow (cast or mask) is B2.
Only expressions whose type can be told are compared; everything else is skipped. Triage the evidence by hand;
a function checked by hand gets a comment `extend_audit: ok - <why>` above its BMX_ADDR line and is skipped.

Usage: py -I tools/extend_audit.py . [-v]   (needs $BMX_WORK or work/ with bundles and bundles-rw, and the exe)
"""
import re
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
import audit_common as ac  # noqa: E402

repo = Path(sys.argv[1] if len(sys.argv) > 1 else ".").resolve()
VERBOSE = "-v" in sys.argv
ported = ac.load_ported(repo)
bundles = ac.load_bundles(repo)
exe = ac.Exe(ac.exe_path(repo))
resolve = ac.thunk_resolver(bundles, exe)

# ---------- narrow types ----------------------------------------------------------------------------

BASE = {
    "int8_t": "S8", "signed char": "S8", "char": "S8", "uint8_t": "Z8", "unsigned char": "Z8",
    "int16_t": "S16", "short": "S16", "signed short": "S16", "short int": "S16",
    "uint16_t": "Z16", "unsigned short": "Z16", "unsigned short int": "Z16",
}  # fmt: skip
srcs = [p for p in (repo / "src").rglob("*") if p.suffix in (".c", ".h")]
texts = {p: ac.strip_c(p.read_text(errors="replace")) for p in srcs}
kinds = dict(BASE)  # type name -> S8/Z8/S16/Z16
for _ in range(3):  # typedef chains
    for t in texts.values():
        for m in re.finditer(r"\btypedef\s+((?:unsigned |signed )?\w+(?: int)?)\s+(\w+)\s*;", t):
            if m.group(1) in kinds and m.group(2) not in kinds:
                kinds[m.group(2)] = kinds[m.group(1)]
WIDE_TYPES = r"(?:int|unsigned|unsigned int|uint32_t|int32_t|uintptr_t|intptr_t|long|size_t|uint64_t|int64_t)"
NARROW = "|".join(sorted((re.escape(k).replace(r"\ ", r"\s+") for k in kinds), key=len, reverse=True))
NARROW_RE = re.compile(r"\b(" + NARROW + r")\b")
WIDTH = lambda k: int(k[1:])  # noqa: E731


def type_kind(tt):
    """S8/Z8/S16/Z16 for a plain narrow integer type text, "W" for a full-width integer, None otherwise."""
    tt = re.sub(r"\b(const|volatile|register|static)\b", "", tt)
    tt = re.sub(r"\s+", " ", tt).strip()
    if "*" in tt or "[" in tt:
        return None
    if tt in kinds:
        return kinds[tt]
    if re.fullmatch(WIDE_TYPES, tt):
        return "W"
    return None


# struct member name -> set of kinds over all declarations of that name in src (ambiguous names are skipped)
member_kinds = {}
for t in texts.values():
    for m in re.finditer(r"\b(?:struct|union)\s*\w*\s*\{", t):
        end = ac.match_close(t, m.end() - 1, "{", "}")
        depth = 0
        flat = ""
        for ch in t[m.end() : end]:  # drop nested bodies
            if ch == "{":
                depth += 1
            elif ch == "}":
                depth -= 1
            elif depth == 0:
                flat += ch
        for decl in flat.split(";"):
            mm = re.match(r"^(.*?)\b(\w+)\s*(\[[^\]]*\])?\s*(?::\s*\d+)?$", decl.strip(), re.S)
            if mm and mm.group(1).strip():
                member_kinds.setdefault(mm.group(2), set()).add(None if mm.group(3) else type_kind(mm.group(1)))

# struct type name -> {field: kind}; reached through the declared type of the base expression
struct_fields = {}
for t in texts.values():
    for m in re.finditer(r"(typedef\s+)?(?:struct|union)\s*(\w*)\s*\{", t):
        end = ac.match_close(t, m.end() - 1, "{", "}")
        flat, depth = "", 0
        for ch in t[m.end() : end]:
            depth += (ch == "{") - (ch == "}")
            if depth == 0 and ch != "}":
                flat += ch
        fields = {}
        for decl in flat.split(";"):
            mm = re.match(r"^(.*?)(\w+)\s*(\[[^\]]*\])?\s*(?::\s*\d+)?$", decl.strip(), re.S)
            if mm and mm.group(1).strip():
                k = None if "*" in mm.group(1) else type_kind(mm.group(1))
                if mm.group(3):  # array of narrow integers: the field name indexes to that kind
                    fields[mm.group(2) + "[]"] = k
                else:
                    fields[mm.group(2)] = k
        names = [m.group(2)] if m.group(2) else []
        if m.group(1):
            tail = re.match(r"\s*(\w+)\s*;", t[end + 1 :])
            if tail:
                names.append(tail.group(1))
        for n in names:
            struct_fields[n] = fields

# ---------- original side: registers and widening ---------------------------------------------------

FAM = {}
for fam, names in {
    "A": ("EAX", "AX", "AL", "AH"), "C": ("ECX", "CX", "CL", "CH"), "D": ("EDX", "DX", "DL", "DH"),
    "B": ("EBX", "BX", "BL", "BH"), "SI": ("ESI", "SI"), "DI": ("EDI", "DI"), "BP": ("EBP", "BP"),
}.items():  # fmt: skip
    for n in names:
        FAM[n] = fam


def dest_fam(u):
    """Register family written by the first operand of an instruction text, or None."""
    mm = re.match(r"^\w+\s+(\w+)\s*(?:,|$)", u)
    return FAM.get(mm.group(1)) if mm else None


def classify_def(insns, j, fam):
    """Widening left by the instruction at j (which writes register family `fam`): S8/Z8/S16/Z16 or None."""
    u = insns[j][1].upper()
    mm = re.match(r"^(MOVSX|MOVZX)\s+(\w+),\s*(.*)$", u)
    if mm and FAM.get(mm.group(2)) == fam:
        src = mm.group(3)
        if "BYTE" in src or re.fullmatch(r"[ABCD][LH]", src):
            w = "8"
        elif "WORD" in src or re.fullmatch(r"[ABCD]X|SI|DI|BP", src):
            w = "16"
        else:
            return None
        return ("S" if mm.group(1) == "MOVSX" else "Z") + w
    mm = re.match(r"^AND\s+(\w+),\s*(0X[0-9A-F]+)$", u)
    if mm and FAM.get(mm.group(1)) == fam:
        return {0xFFFF: "Z16", 0xFF: "Z8"}.get(int(mm.group(2), 16))
    if u == "CWDE" and fam == "A":
        return "S16"
    mm = re.match(r"^MOV\s+(\w+),", u)
    if mm and FAM.get(mm.group(1)) == fam and not mm.group(1).startswith("E"):
        w = "8" if mm.group(1).endswith(("L", "H")) else "16"
        for k in range(j - 1, max(-1, j - 6), -1):  # a partial write is a zero extension if the register was cleared
            tk = insns[k][1].upper()
            df = dest_fam(tk)
            if df == fam and re.match(r"^(XOR\s+(E\w\w),\s*\2$|MOVZX\s+E\w\w,)", tk):
                return "Z" + w
            if tk.startswith(("CALL", "RET", "J")) or df == fam:
                break
    return None


def trace_push(insns, i, fam):
    """Widening of the register pushed at insn i: S8/Z8/S16/Z16, "W" if loaded as a plain dword, None if unknown."""
    for j in range(i - 1, max(-1, i - 14), -1):
        u = insns[j][1].upper()
        if u.startswith(("CALL", "RET", "JMP")) or re.match(r"^J[A-Z]+\s", u):
            return None
        if u == "CWDE" and fam == "A":
            return "S16"
        if u == "CBW":
            continue
        mm = re.match(r"^(\w+)\s+(\w+)\s*,", u) or re.match(r"^(\w+)\s+(\w+)$", u)
        if not mm or FAM.get(mm.group(2)) != fam:
            continue
        if mm.group(1) not in ("MOV", "MOVSX", "MOVZX", "AND"):
            return None
        k = classify_def(insns, j, fam)
        if k is not None or mm.group(1) != "MOV" or not mm.group(2).startswith("E"):
            return k
        mr = re.match(r"^MOV\s+E\w\w,\s*(E\w\w)$", u)
        if mr and mr.group(1) in FAM:  # register copy: follow the source
            fam = FAM[mr.group(1)]
            continue
        return "W"  # loaded as a full dword (memory, immediate): no widening
    return None


ESP_MEM = re.compile(r"\[ESP(?: \+ (-?0x[0-9a-f]+))?\]", re.I)


def param_widening(addr):
    """{parameter index: S8/Z8/S16/Z16} for the stack parameters that the function narrows on first use.

    Only the straight-line prologue is scanned (up to the first call or jump), so a parameter first used
    later is not seen.
    """
    insns = bundles[addr][1]
    pushed = 0
    out = {}
    for i, (a, t) in enumerate(insns):
        u = t.upper()
        op = u.split()[0]
        mm = ESP_MEM.search(t)
        dst = u.split(",")[0].split()[-1] if "," in u else ""
        if mm and op in ("MOV", "MOVSX", "MOVZX") and dst in FAM:
            e = (int(mm.group(1), 16) if mm.group(1) else 0) - pushed
            idx = (e - 4) // 4
            if e >= 4 and (e - 4) % 4 == 0 and idx not in out:
                fam = FAM[dst]
                w = None
                if op != "MOV" or not dst.startswith("E"):
                    w = classify_def(insns, i, fam)
                else:
                    for k in range(i + 1, min(len(insns), i + 4)):
                        uk = insns[k][1].upper()
                        if re.match(r"^(MOVSX|MOVZX)\s+" + dst + r",\s*(AX|AL|BX|BL|CX|CL|DX|DL)$", uk) or re.match(
                            r"^AND\s+" + dst + r",\s*0X(FFFF|FF)$", uk
                        ):
                            w = classify_def(insns, k, fam)
                            break
                        if re.match(r"^(MOV|LEA|XOR)\s+" + dst, uk):
                            break
                out[idx] = w
        if op == "PUSH":
            pushed += 4
        elif op == "POP":
            pushed -= 4
        elif op == "CALL" or op.startswith("J") or op == "RET":
            break
        elif re.match(r"^(ADD|SUB) ESP,", u):
            n = int(u.split(",")[1], 16)
            pushed += n if u.startswith("SUB") else -n
    return {k: v for k, v in out.items() if v}


_pw = {}


def callee_param_widening(key):
    if key[0] != "fn" or key[1] not in bundles:
        return None
    if key[1] not in _pw:
        _pw[key[1]] = param_widening(key[1])
    return _pw[key[1]]


CALL_D = re.compile(r"^CALL\s+0x([0-9a-f]+)$", re.I)
CALL_I = re.compile(r"^CALL\s+dword ptr \[(0x[0-9a-f]+)\]$", re.I)
orig_sites = {}  # ported addr -> {(kind, id): [(arg index, widening, insn addr, text)]}
for fa in ported:
    if fa not in bundles:
        continue
    insns = bundles[fa][1]
    out = {}
    for i, (a, t) in enumerate(insns):
        md, mi = CALL_D.match(t), CALL_I.match(t)
        if not (md or mi):
            continue
        key = ("fn", resolve(int(md.group(1), 16))) if md else ("var", int(mi.group(1), 16))
        k = 0
        for j in range(i - 1, max(-1, i - 24), -1):
            u = insns[j][1].upper()
            if u.startswith(("CALL", "RET", "JMP")) or re.match(r"^(J[A-Z]+|ADD ESP)\s", u):
                break
            if u.startswith("PUSH"):
                mp = re.match(r"^PUSH\s+(E\w\w)$", u)
                if mp and mp.group(1) in FAM:
                    w = trace_push(insns, j, FAM[mp.group(1)])
                    if w:
                        out.setdefault(key, []).append((k, w, insns[j][0], insns[j][1] + " <- " + w))
                k += 1
            elif u.startswith("SUB ESP,"):
                k += 2  # a double argument
    orig_sites[fa] = out

# ---------- the port's side -------------------------------------------------------------------------

names_to_key = {}
for a, p in ported.items():
    names_to_key[p["name"]] = ("fn", a)
for t in texts.values():
    for m in re.finditer(r"\bORIG_FN(?:_STDCALL)?\(\s*(0x[0-9a-fA-F]+)\s*,\s*[^,]+,\s*(\w+)\s*,", t):
        names_to_key.setdefault(m.group(2), ("fn", int(m.group(1), 16)))
    for m in re.finditer(r"#define\s+(\w+)\s+ORIG_VAR\((0x[0-9a-fA-F]+),", t):
        names_to_key.setdefault(m.group(1), ("var", int(m.group(2), 16)))


# globals (ORIG_VAR macros and plain file-scope variables), and the return types of functions
global_kinds = {}
global_types = {}  # name -> struct / typedef name of a global declared through ORIG_VAR
elem_kinds = {}  # name of a pointer / array of narrow integers -> kind of its elements
ret_kinds = {}
for t in texts.values():
    for m in re.finditer(r"#define\s+(\w+)\s+ORIG_VAR\(\s*0x[0-9a-fA-F]+\s*,\s*([^,]+?)\s*,\s*\w+\s*\)", t):
        k = type_kind(m.group(2))
        if k:
            global_kinds[m.group(1)] = k
        if re.fullmatch(r"\w+", m.group(2)):
            global_types[m.group(1)] = m.group(2)
        mm = re.match(r"^(.*?)\s*\*$", m.group(2))
        if mm and type_kind(mm.group(1)):
            elem_kinds[m.group(1)] = type_kind(mm.group(1))
    for m in re.finditer(r"^(?:static\s+)?((?:const\s+)?[A-Za-z_][\w ]*?)\s+(\*?)(\w+)\s*(\[[^\]]*\])?\s*(?:=[^;]*)?;", t, re.M):
        k = type_kind(m.group(1))
        if k and k != "W":
            if m.group(4) or m.group(2):
                elem_kinds.setdefault(m.group(3), k)
            else:
                global_kinds.setdefault(m.group(3), k)
    for m in re.finditer(r"^(?:static\s+|inline\s+|extern\s+)*((?:const\s+)?[A-Za-z_][\w ]*?)\s+(\w+)\s*\([^;{}]*\)\s*[{;]", t, re.M):
        k = type_kind(m.group(1))
        if k and k != "W":
            ret_kinds.setdefault(m.group(2), k)
    for m in re.finditer(r"\bORIG_FN(?:_STDCALL)?\(\s*0x[0-9a-fA-F]+\s*,\s*([^,]+?)\s*,\s*(\w+)\s*,", t):
        k = type_kind(m.group(1))
        if k and k != "W":
            ret_kinds.setdefault(m.group(2), k)


def base_type(tt):
    tt = re.sub(r"(const|volatile|struct|union|static|register)", "", tt).replace("*", " ").strip()
    return tt if re.fullmatch(r"\w+", tt) else None


def local_kinds(p):
    env = {}
    for prm in ac.split_args(p["params"]):
        mm = re.match(r"^(.*?)(\w+)$", prm.strip())
        if mm:
            if base_type(mm.group(1)):
                env["@" + mm.group(2)] = base_type(mm.group(1))
            if "[" in prm or "*" in mm.group(1):
                k = type_kind(mm.group(1).replace("*", "")) if "[" not in prm else None
                if k and k != "W":
                    env[mm.group(2) + "[]"] = k
            else:
                env[mm.group(2)] = type_kind(mm.group(1))
    for m in re.finditer(
        r"(?:^|[;{}])\s*((?:const\s+|static\s+|volatile\s+)*[A-Za-z_][\w ]*?)\s+([^;{}()=]+?(?:=[^;{}]*)?);", p["body"]
    ):
        tk = type_kind(m.group(1))
        bt = base_type(m.group(1))
        if tk is None and not NARROW_RE.search(m.group(1)) and bt not in struct_fields:
            continue
        for d in ac.split_args(m.group(2)):
            nm = re.match(r"^(\**)\s*(\w+)", d.strip())
            if nm and bt in struct_fields:
                env["@" + nm.group(2)] = bt
                continue
            if nm:
                if nm.group(1) or "[" in d.split("=")[0]:
                    if tk and tk != "W":
                        env[nm.group(2) + "[]"] = tk
                else:
                    env[nm.group(2)] = tk
    return env


CAST = re.compile(r"^\(\s*((?:const\s+)?[A-Za-z_][\w ]*?)\s*\)\s*(.*)$", re.S)
DEREF_CAST = re.compile(r"^\*\s*\(\s*((?:const\s+|volatile\s+)*[A-Za-z_][\w ]*?)\s*\*\s*\)")


def expr_kind(e, env):
    """Widening that C semantics apply to the argument expression: S8/Z8/S16/Z16, or None if unknown / full width."""
    e = e.strip()
    while e.startswith("(") and ac.match_close(e, 0, "(", ")") == len(e) - 1:
        e = e[1:-1].strip()
    mm = CAST.match(e)
    if mm and type_kind(mm.group(1)) is not None:
        tk = type_kind(mm.group(1))
        return tk if tk != "W" else expr_kind(mm.group(2), env)
    mm = re.match(r"^(.+?)\s*&\s*(0x[fF]{2}|0x[fF]{4})$", e)
    if mm:
        return "Z16" if len(mm.group(2)) == 6 else "Z8"
    mm = DEREF_CAST.match(e)
    if mm:
        k = type_kind(mm.group(1))
        return k if k and k != "W" else None
    if re.fullmatch(r"\w+", e):
        k = env.get(e) if e in env else global_kinds.get(e)
        return k if k != "W" else None
    mm = re.fullmatch(r"(\w+)\s*\(.*\)", e, re.S)
    if mm and ac.match_close(e, e.index("("), "(", ")") == len(e) - 1:
        return ret_kinds.get(mm.group(1))
    mm = re.fullmatch(r"(\w+)\s*\[[^\[\]]*\]", e)
    if mm:
        return env.get(mm.group(1) + "[]") or elem_kinds.get(mm.group(1))
    mm = re.fullmatch(r"(\w+)\s*(?:->|\.)\s*(\w+)\s*(\[[^\[\]]*\])?", e)
    if mm:  # base->field with a declared base type
        tname = env.get("@" + mm.group(1)) or global_types.get(mm.group(1))
        fields = struct_fields.get(tname)
        if fields is not None:
            k = fields.get(mm.group(2) + ("[]" if mm.group(3) else ""))
            return k if k != "W" else None
    mm = re.fullmatch(r"[\w\.\->\[\]\s\(\)\*]*?(?:->|\.)\s*(\w+)\s*(\[[^\[\]]*\])?", e)
    if mm and not re.search(r"[+\-/|^%<>]", e.replace("->", "")):
        ks = member_kinds.get(mm.group(1))
        if ks and len(ks) == 1:
            k = next(iter(ks))
            return k if k != "W" else None
    return None


def reviewed(p):
    """A function whose comment says `extend_audit: ok` has been checked by hand."""
    return "extend_audit: ok" in p["pre"] or "extend_audit: ok" in p["body_raw"]


findings, unknown, weak = [], [], []
n_args = n_benign = 0
for fa, p in ported.items():
    if reviewed(p) or fa not in orig_sites:
        continue
    env = local_kinds(p)
    body = p["body"]
    for name, key in names_to_key.items():
        if key not in orig_sites[fa]:
            continue
        for m in re.finditer(r"\b" + re.escape(name) + r"\s*\(", body):
            if m.start() > 0 and body[m.start() - 1] in ".>":
                continue
            po = m.end() - 1
            args = ac.split_args(body[po + 1 : ac.match_close(body, po, "(", ")")])
            line = p["line"] + body.count("\n", 0, m.start())
            for idx, arg in enumerate(args):
                seen = [x for x in orig_sites[fa][key] if x[0] == idx]
                ok = {x[1] for x in seen}
                if not ok:
                    continue
                pk = expr_kind(arg, env)
                n_args += 1
                if pk is None:
                    unknown.append(f"{p['path']}:{line}: {name} arg {idx} = {arg.strip()[:70]}   (original: {sorted(ok)})")
                    continue
                if pk in ok:
                    continue
                if ok == {"W"}:
                    # the original does not widen at the call: harmless if the callee narrows to this width or less
                    cw = (callee_param_widening(key) or {}).get(idx)
                    if cw and WIDTH(cw) <= WIDTH(pk):
                        n_benign += 1
                        continue
                    ev = [f"{x[2]:08x}: {x[3]}" for x in seen[:2]]
                    weak.append((fa, p["name"], f"{p['path']}:{line}", f"{name} arg {idx}: port narrows to {pk} ({arg.strip()[:60]}), the original passes the dword; the callee {'narrows it to ' + cw if cw else 'is not seen narrowing it'}", ev))
                    continue
                ev = [f"{x[2]:08x}: {x[3]}" for x in seen[:3]]
                findings.append((fa, p["name"], f"{p['path']}:{line}", f"{name} arg {idx}: port passes {pk} ({arg.strip()[:60]}); the original widens differently", ev))

# ---------- B: parameters --------------------------------------------------------------------------


def narrows_in_body(body, name, w):
    """True if the port itself narrows parameter `name` the way the original does (cast or mask)."""
    n = re.escape(name)
    mask = {"Z8": r"0x[fF]{2}\b|255\b", "Z16": r"0x[fF]{4}\b|65535\b"}.get(w)
    if mask and re.search(r"\b" + n + r"\s*&=?\s*(" + mask + r")|(" + mask + r")\s*&\s*\(?\s*" + n + r"\b", body):
        return True
    for t, k in kinds.items():
        if k == w and re.search(r"\(\s*" + re.escape(t).replace(r"\ ", r"\s+") + r"\s*\)\s*\(?\s*" + n + r"\b", body):
            return True
    return False


param_findings, widen_findings = [], []
n_params = 0
for fa, p in ported.items():
    if fa not in bundles or reviewed(p):
        continue
    prms = ac.split_args(p["params"])
    for idx, w in param_widening(fa).items():
        if idx >= len(prms):
            continue
        n_params += 1
        pk = type_kind(re.sub(r"\w+$", "", prms[idx].strip()))
        name = re.sub(r"^.*?(\w+)\s*$", r"\1", prms[idx].strip())
        where = f"{p['path']}:{p['line']}"
        if pk and pk != "W" and pk != w:
            param_findings.append((fa, p["name"], where, f"parameter {idx} ({prms[idx].strip()}) is {pk}, the original widens it as {w}", []))
        elif pk == "W" and not narrows_in_body(p["body"], name, w):
            widen_findings.append((fa, p["name"], where, f"parameter {idx} ({prms[idx].strip()}) is full width, the original widens it as {w}", []))

if VERBOSE:
    print(f"== {len(unknown)} narrow original arguments whose port expression could not be typed:")
    for u in unknown:
        print("  " + u)
for title, items in (
    ("A2. port narrows an argument that the original passes as a plain dword (only a problem if the callee uses more bits)", weak if VERBOSE else []),
    ("A. argument widening: the port's cast differs from the original's", findings),
    ("B. parameter widening: declared type differs from the original's", param_findings),
    ("B2. full-width parameter that the original narrows and the port never does", widen_findings),
):
    print(f"== {title}: {len(weak) if title.startswith('A2') else len(items)}" + (" (-v lists them)" if title.startswith("A2") and not VERBOSE else ""))
    for fa, nm, where, msg, ev in sorted(items):
        print(f"  {fa:08x} {nm} ({where})\n      {msg}")
        for e in ev:
            print(f"        original {e}")
print(
    f"\nextend_audit: {len(ported)} ported functions; {n_args} original pushes seen at callees the port names, "
    f"{n_args - len(unknown)} with a typable port expression ({len(unknown)} untypable or full width, -v lists them; "
    f"{n_benign} set aside because the callee narrows anyway); {n_params} narrowed parameters compared.\n"
    f"findings: {len(findings)} in A, {len(param_findings)} in B, {len(widen_findings)} in B2 ({len(weak)} weak ones in A2)"
)
