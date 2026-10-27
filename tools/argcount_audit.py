"""Compare the argument counts the port declares with what the original code takes and is given.

Wrong argument counts pass the emulator tests when the test stub for the callee has the same wrong
signature, but corrupt the stack in the real game (zrwCameraEnd: rwSceneFind takes 1 argument and
rwSceneRelease 2, the original reuses a push).

Checks (counts in 4-byte stack slots; double/int64 are 2):
  1. ORIG_FN / ORIG_FN_STDCALL declarations vs the callee's own disassembly: RET n (exact for stdcall),
     and the highest [ESP+n] / [EBP+n] stack read relative to the pushes so far (a lower bound: unread
     arguments are invisible). declared < read, or a RET n / convention mismatch, is a strong finding.
  2. Definitions of ported functions (BMX_ADDR) vs the same estimate, same rules.
  3. Original call sites of every callee with a known declaration (ORIG_FN, ported function):
     the caller's ADD ESP,n after a call (or after several calls with merged cleanup) must equal the
     declared slots of all those calls together; a difference means the declaration, or a push reused
     by two calls, is wrong.
  4. The same for calls through function-pointer ORIG_VARs (typedef parameter lists).
Expect noise (varargs, register arguments, unused arguments, jump tables): triage by hand.

Usage: py -I tools/argcount_audit.py . [-a] [-v]   (-v: also list pushed-but-unread extra arguments; -a: also mismatches in callers that are not ported; needs $BMX_WORK or work/ with bundles and bundles-rw)
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

WIDE = re.compile(r"\b(double|int64_t|uint64_t|long\s+long)\b")


def slots_of(params):
    """Stack slots of a C parameter list, or None for varargs."""
    n = 0
    for p in ac.split_args(params):
        if p == "...":
            return None
        n += 2 if WIDE.search(p) and "*" not in p else 1
    return n


def imm(s):
    s = s.strip()
    return int(s, 16) if s.lower().startswith(("0x", "-0x")) else int(s)


# ---------- declarations ---------------------------------------------------------------------------

decls = []  # dict(addr, name, params, slots, std, path, line)
srcs = [p for p in (repo / "src").rglob("*") if p.suffix in (".c", ".h")]
for path in srcs:
    text = ac.strip_c(path.read_text(errors="replace"))
    for m in re.finditer(r"\bORIG_FN(_STDCALL)?\(\s*0x", text):
        po = text.index("(", m.start())
        pc = ac.match_close(text, po, "(", ")")
        parts = ac.split_args(text[po + 1 : pc])
        if len(parts) < 4:
            continue
        params = ",".join(parts[3:]).strip()
        if params.startswith("(") and params.endswith(")"):
            params = params[1:-1]
        decls.append(
            dict(
                addr=int(parts[0], 16),
                name=parts[2],
                slots=slots_of(params),
                std=bool(m.group(1)),
                path=str(path.relative_to(repo)).replace("\\", "/"),
                line=text.count("\n", 0, m.start()) + 1,
            )
        )

fn_typedefs = {}
for path in srcs:
    text = ac.strip_c(path.read_text(errors="replace"))
    for m in re.finditer(r"typedef[^;{]*?\(\s*(BMX_\w+\s*)?\*\s*(\w+)\s*\)\s*\(", text):
        po = m.end() - 1
        fn_typedefs[m.group(2)] = (slots_of(text[po + 1 : ac.match_close(text, po, "(", ")")]), "STDCALL" in (m.group(1) or ""))
fn_vars = {}  # slot -> [(name, typedef, slots)]
for path in srcs:
    for m in re.finditer(r"#define\s+(\w+)\s+ORIG_VAR\((0x[0-9a-fA-F]+),\s*(\w+),", path.read_text(errors="replace")):
        if m.group(3) in fn_typedefs:
            fn_vars.setdefault(int(m.group(2), 16), []).append((m.group(1), m.group(3), *fn_typedefs[m.group(3)]))

known = {}  # addr -> (name, declared slots, where, stdcall)
for a, p in ported.items():
    known[a] = (p["name"], slots_of(p["params"]), f"{p['path']}:{p['line']}", False)
for d in decls:
    known.setdefault(d["addr"], (d["name"], d["slots"], f"{d['path']}:{d['line']}", d["std"]))

# ---------- original-side facts -------------------------------------------------------------------

RETN = re.compile(r"^RET\s+(0x[0-9a-f]+)$", re.I)
ESP_MEM = re.compile(r"(qword ptr |dword ptr |word ptr |byte ptr )?\[ESP(?: \+ (-?0x[0-9a-f]+))?\]", re.I)
EBP_MEM = re.compile(r"(qword ptr |dword ptr )?\[EBP \+ (0x[0-9a-f]+)\]", re.I)
JCC = re.compile(r"^(J\w+)\s+0x([0-9a-f]+)$", re.I)


def ret_n(addr):
    vals = []
    for _, t in bundles[addr][1]:
        if t.upper() == "RET":
            vals.append(0)
        elif RETN.match(t):
            vals.append(int(RETN.match(t).group(1), 16))
    return max(vals) if vals else None


RET_N = {a: ret_n(a) for a in bundles}


EST_WHY = {}


def estimate(addr):
    """Highest stack slot read from the caller's frame (lower bound of the argument count)."""
    insns = bundles[addr][1]
    pushed = 0
    delta_at = {}
    mx = 0
    written = set()
    frame = len(insns) > 1 and insns[0][1].upper() == "PUSH EBP" and insns[1][1].upper() == "MOV EBP,ESP"
    track = True
    for i, (a, t) in enumerate(insns):
        u = t.upper()
        op = u.split()[0]
        if a in delta_at:
            pushed = delta_at[a]
        if track:
            for m in ESP_MEM.finditer(t):
                e = (int(m.group(2), 16) if m.group(2) else 0) - pushed
                comma = t.find(",")
                if op in ("MOV", "FST", "FSTP", "FIST", "FISTP") and (comma < 0 or m.start() < comma):
                    written.add(e)  # a store into the frame: a local, not an argument read
                    continue
                if e >= 4 and e not in written:
                    w = (e - 4) // 4 + (2 if (m.group(1) or "").lower() == "qword ptr " else 1)
                    if w > mx:
                        mx, EST_WHY[addr] = w, f"{a:08x}: {t}"
        if frame:
            for m in EBP_MEM.finditer(t):
                d = int(m.group(2), 16)
                if d >= 8:
                    w = (d - 8) // 4 + (2 if (m.group(1) or "").lower() == "qword ptr " else 1)
                    if w > mx:
                        mx, EST_WHY[addr] = w, f"{a:08x}: {t}"
        if op == "PUSH":
            pushed += 4
        elif op == "POP":
            pushed -= 4
        elif re.match(r"^(ADD|SUB) ESP,(-?0x[0-9a-f]+|\d+)$", u, re.I):
            n = imm(u.split(",")[1].lower())
            pushed += n if u.startswith("SUB") else -n
        elif op == "CALL":
            m = re.match(r"CALL\s+0x([0-9a-f]+)$", t, re.I)
            tgt = int(m.group(1), 16) if m else None
            if tgt in RET_N and RET_N[tgt]:
                pushed -= RET_N[tgt]
            elif i and re.match(r"^MOV EAX,0x[0-9a-f]{4,}$", insns[i - 1][1], re.I) and imm(insns[i - 1][1].split(",")[1]) >= 0x1000:
                pushed += imm(insns[i - 1][1].split(",")[1])  # __chkstk: ESP -= EAX
        elif u.startswith(("MOV ESP,", "LEA ESP,")) or re.match(r"^(ADD|SUB) ESP,E", u):
            track = False
        elif op == "JMP" and "[" in u and "*" in u:  # jump table: every case starts at this depth
            mt = re.search(r"\*0x[0-9a-f]+\s*\+ (0x[0-9a-f]+)\]", t, re.I)
            if mt:
                end = addr + bundles[addr][0]["size"]
                for k in range(256):
                    v = exe.u32(int(mt.group(1), 16) + 4 * k)
                    if v is None or not addr <= v < end:
                        break
                    delta_at.setdefault(v, pushed)
        m = JCC.match(t)
        if m and int(m.group(2), 16) > a:
            delta_at.setdefault(int(m.group(2), 16), pushed)
    return mx


EST = {a: estimate(a) for a in bundles}

findings = {1: [], 2: [], 3: [], 4: [], 5: []}

# 5. the same address declared with different argument counts in different files
by_addr = {}
for d in decls:
    by_addr.setdefault(d["addr"], []).append((d["slots"], d["name"], f"{d['path']}:{d['line']}", d["std"]))
for a, p in ported.items():
    by_addr.setdefault(a, []).append((slots_of(p["params"]), p["name"], f"{p['path']}:{p['line']}", "STDCALL" in p["ret"]))
for a, lst in by_addr.items():
    if len({x[0] for x in lst if x[0] is not None}) > 1 or len({x[3] for x in lst}) > 1:
        findings[5].append((lst[0][1], lst[0][2], a, ["declared differently"], [f"{x[2]}: {x[1]} {x[0]} slots{' stdcall' if x[3] else ''}" for x in lst]))


def check(kind, nm, where, addr, declared, std):
    if declared is None or addr not in bundles:
        return
    read, rn = EST[addr], RET_N.get(addr) or 0
    msgs = []
    if std and rn and rn // 4 != declared:
        msgs.append(f"STDCALL declared {declared} slots, original RET {rn:#x} = {rn // 4}")
    if not std and rn:
        msgs.append(f"declared cdecl but the original callee pops {rn // 4} slots (RET {rn:#x})")
    if not std and not rn and declared < read < 256:  # beyond 256: the push tracking lost sync
        msgs.append(f"declared {declared} slots, original reads {read} stack slots (deepest read {EST_WHY.get(addr)})")
    if msgs:
        findings[kind].append((nm, where, addr, msgs, []))


for d in decls:
    check(1, d["name"], f"{d['path']}:{d['line']}", d["addr"], d["slots"], d["std"])
for a, p in ported.items():
    check(2, p["name"], f"{p['path']}:{p['line']}", a, slots_of(p["params"]), "STDCALL" in p["ret"])

# ---------- original call sites -------------------------------------------------------------------

CALL_D = re.compile(r"^CALL\s+0x([0-9a-f]+)$", re.I)
CALL_I = re.compile(r"^CALL\s+dword ptr \[(0x[0-9a-f]+)\]$", re.I)
ADD_ESP = re.compile(r"^ADD ESP,(0x[0-9a-f]+|\d+)$", re.I)


def cname(a):
    return known[a][0] if a in known else f"{a:08x}"


def call_info(t):
    """(key, name, declared slots, stdcall) for a CALL instruction, or None."""
    md, mi = CALL_D.match(t), CALL_I.match(t)
    if md:
        a = resolve(int(md.group(1), 16))
        if RET_N.get(a) or (a in known and known[a][3]):
            return ("std", a, cname(a), 0)
        if a in known:
            return ("fn", a, known[a][0], known[a][1])
        return ("fn", a, f"{a:08x}", None)
    if mi:
        s = int(mi.group(1), 16)
        vs = fn_vars.get(s)
        if vs and len({(v[2], v[3]) for v in vs}) == 1:
            return ("std" if vs[0][3] else "var", s, vs[0][0], vs[0][2])
        return ("var", s, f"[{s:08x}]", None)
    return None


n_unported = n_benign = n_reuse = 0
benign = []
groups = {}  # (key, id) -> (name, [evidence])
n_groups = 0
for fa, (_, insns) in bundles.items():
    pending = []
    for i, (a, t) in enumerate(insns):
        if t.upper().startswith(("RET", "JMP")):
            pending = []
        info = call_info(t)
        if info is None:
            if ADD_ESP.match(t):
                pending = []
            elif t.upper().startswith("CALL"):  # register / memory indirect: argument count unknown
                pending.append(("?", 0, "?", None, a))
            continue
        kind, tid, nm, slots = info
        if kind == "std":
            continue
        nxt = insns[i + 1][1] if i + 1 < len(insns) else ""
        mm = ADD_ESP.match(nxt)
        pending.append((kind, tid, nm, slots, a))
        if not mm:
            continue
        actual = imm(mm.group(1)) // 4
        members, pending = pending, []
        if any(m_[3] is None for m_ in members):
            continue
        n_groups += 1
        expected = sum(m_[3] for m_ in members)
        tail = [x[1].upper() for x in insns[i + 2 : i + 8]]
        epilogue = False
        for x in tail:  # ADD ESP,n; POP...; RET frees the locals too
            if x.startswith("POP"):
                continue
            epilogue = x.startswith("RET")
            break
        if expected != actual and not (epilogue and actual > expected):
            if fa not in ported and "-a" not in sys.argv:
                n_unported += 1  # original code calling original code: the declaration is what is off, if anything
                continue
            lo = next(k for k, (xa, _) in enumerate(insns) if xa == members[0][4])
            span = [x[1] for x in insns[lo : i + 1]]
            if any(re.match(r"^(FSTP? (float|qword) ptr \[ESP( \+ 0x[0-9a-f]+)?\]|MOV dword ptr \[ESP( \+ 0x[0-9a-f]+)?\],)", x, re.I) for x in span):
                n_reuse += 1  # an argument slot is overwritten in place (FP argument, reused push): the sum is not exact
                continue
            if actual > expected and all(EST.get(m_[1], 99) <= m_[3] for m_ in members if m_[0] == "fn"):
                n_benign += 1  # more pushed than declared, but no callee reads beyond its declaration
                benign.append(f"{a:08x} in {cname(fa)}: caller cleans {actual} slots, declared " + " + ".join(f"{m_[2]}({m_[3]})" for m_ in members))
                continue
            desc = " + ".join(f"{m_[2]}({m_[3]})" for m_ in members)
            for m_ in members:
                groups.setdefault((m_[0], m_[1]), (m_[2], []))[1].append(
                    f"{a:08x} in {cname(fa)}: caller cleans {actual} slots, declared {desc} = {expected}"
                )

for (kind, tid), (nm, ev) in sorted(groups.items()):
    if kind == "fn":
        where = known[tid][2] if tid in known else ""
        findings[3].append((nm, where, tid, [f"{len(ev)} original call site(s) with a different cleanup"], ev[:4]))
    else:
        findings[4].append((nm, f"[{tid:08x}]", tid, [f"{len(ev)} original call site(s) with a different cleanup"], ev[:4]))

titles = {
    1: "ORIG_FN declaration vs original callee",
    2: "ported definition vs original function",
    3: "original call sites vs declared count (ORIG_FN / ported callees)",
    4: "original indirect call sites vs function-pointer typedef",
    5: "one address declared with different argument counts",
}
for k in (1, 2, 3, 4, 5):
    print(f"== {k}. {titles[k]}: {len(findings[k])}")
    for nm, where, addr, msgs, extra in sorted(findings[k], key=lambda f: f[2]):
        print(f"  {addr:08x} {nm} ({where})")
        for m in msgs:
            print(f"      {m}")
        for e in extra:
            print(f"        {e}")
if "-v" in sys.argv:
    print("== extra arguments pushed that the callee does not read (benign unless read through a register):")
    for b in benign:
        print("  " + b)
print(
    f"\nargcount_audit: {len(decls)} ORIG_FN declarations, {len(ported)} ported functions, {n_groups} call groups "
    f"with a known count; set aside: {n_unported} in unported callers (-a), {n_benign} unread extra arguments "
    f"(-v), {n_reuse} with in-place argument slots.\nfindings: "
    + ", ".join(f"{len(findings[k])} in check {k}" for k in findings)
)
