"""Find declarations that call a ported function with other parameter widths than its definition.

ORIG_FN declarations bind to the serving function by assembler name, and BMX_FNPTR casts a function to the
type a table or caller wants, so the compiler never compares those parameter lists with the definition. On
32-bit builds a pointer and a 32-bit integer are interchangeable in such a call; on 64-bit builds the
integer side cuts the pointer to its low half (docs/PORTING.md "X2e"). This lists every parameter (and
return value) that is a pointer on one side and a 32-bit integer on the other.

Classes: ptr (pointers, arrays, function pointers, pointer typedefs), word (pointer-sized integers:
intptr_t, uintptr_t, size_t, ptrdiff_t), int (other integers, enums), fp, other (structs by value, void).
ptr/word against int is reported; everything else is the argcount audit's business.

Usage: py -I tools/width_audit.py [repo] [-v]   (exit status 1 when anything is found; -v lists unresolved)
"""
import re
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
import audit_common as ac  # noqa: E402

args = [a for a in sys.argv[1:] if not a.startswith("-")]
verbose = "-v" in sys.argv
repo = Path(args[0] if args else ".").resolve()

INT_WORDS = {"int", "unsigned", "signed", "short", "long", "char", "bool", "_Bool"}
INT_NAMES = re.compile(r"^(u?int(8|16|32)_t|DWORD|BOOL|UINT|WORD|BYTE|LONG|ZAngle|RwBool|RwInt32|RwUInt32|"
                       r"RwInt16|RwUInt16|RwUInt8|RwInt8|HRESULT|MMRESULT)$")
WORD_NAMES = {"intptr_t", "uintptr_t", "size_t", "ptrdiff_t", "ssize_t", "WPARAM", "LPARAM", "LRESULT",
              "UINT_PTR", "LONG_PTR", "DWORD_PTR", "ULONG_PTR"}
FP_NAMES = {"float", "double", "RwReal"}


ARRAYS = set()
FN_SIGS = {}  # function pointer typedef -> (return type, parameter list)


def scan_typedefs():
    """name -> 'ptr' / 'int' / 'word' / 'fp' / 'other' for the project's typedefs (resolved transitively)."""
    raw = {}
    enums = set()
    for path in list((repo / "src").rglob("*.h")) + list((repo / "src").rglob("*.c")):
        text = ac.strip_c(path.read_text(errors="replace"))
        for m in re.finditer(r"\btypedef\b([^;{]*(?:\{[^{}]*(?:\{[^{}]*\}[^{}]*)*\})?[^;]*);", text):
            body = " ".join(m.group(1).split())
            fn = re.search(r"\(\s*(?:\w+\s+)*\*\s*(?:const\s+)?(\w+)\s*\)\s*\(", body)
            if fn:
                raw[fn.group(1)] = "ptr"
                close = ac.match_close(body, fn.end() - 1, "(", ")")
                if close > 0:
                    FN_SIGS[fn.group(1)] = (body[: fn.start()], body[fn.end() : close])
                continue
            if re.match(r"^\s*enum\b", body):
                name = re.search(r"(\w+)\s*$", body)
                if name:
                    enums.add(name.group(1))
                continue
            arr = re.search(r"(\w+)\s*\[[^\]]*\]\s*$", body)
            if arr:  # array typedefs decay to pointers as parameters
                raw[arr.group(1)] = "ptr"
                ARRAYS.add(arr.group(1))
                continue
            name = re.search(r"(\w+)\s*$", body)
            if not name:
                continue
            target = body[: name.start()].strip()
            if "*" in target:
                raw[name.group(1)] = "ptr"
            elif re.match(r"^(struct|union)\b", target):
                raw[name.group(1)] = "other"
            else:
                raw[name.group(1)] = target
    for e in enums:
        raw[e] = "int"
    out = {}

    def resolve(name, depth=0):
        if name in out:
            return out[name]
        v = raw.get(name)
        if v in ("ptr", "int", "word", "fp", "other"):
            out[name] = v
        elif v is None or depth > 20:
            return None
        else:
            out[name] = classify(v, depth + 1) or "other"
        return out[name]

    def classify(t, depth=0):
        return classify_type(t, lambda n: resolve(n, depth))

    for n in raw:
        resolve(n)
    return out


def classify_type(t, lookup):
    t = re.sub(r"\b(const|volatile|restrict|register|struct|union|enum|BMX_CDECL|BMX_STDCALL|__cdecl|__stdcall)\b", " ", t)
    t = " ".join(t.split())
    if "*" in t or "[" in t or "(" in t:
        return "ptr"
    words = t.split()
    if not words:
        return None
    # drop a trailing parameter name: the type is what remains when that leaves a type
    cands = [words] if len(words) == 1 else [words[:-1], words]
    for ws in cands:
        if all(w in INT_WORDS for w in ws):
            return "int"
        if len(ws) == 1:
            w = ws[0]
            if w in WORD_NAMES:
                return "word"
            if w in FP_NAMES:
                return "fp"
            if INT_NAMES.match(w):
                return "int"
            if w == "void":
                return "other"
            c = lookup(w)
            if c:
                return c
    return None


TYPEDEFS = scan_typedefs()


def cls(t):
    return classify_type(t, TYPEDEFS.get)


def params_of(s):
    s = " ".join(s.split())
    if s in ("", "void"):
        return []
    return [p for p in ac.split_args(s)]


def clash(a, b):
    return {a, b} in ({"ptr", "int"}, {"word", "int"})


ported = ac.load_ported(repo)
by_name = {}
for addr, f in ported.items():
    by_name.setdefault(f["name"], f)

findings = []
unresolved = []


def compare(where, decl_ret, decl_params, f):
    dps, fps = params_of(decl_params), params_of(f["params"])
    if "..." in decl_params or "..." in f["params"]:
        return
    if decl_ret is not None:
        a, b = cls(decl_ret), cls(f["ret"])
        if a and b and clash(a, b):
            findings.append(f"{where}: returns {decl_ret.strip()} ({a}), {f['name']} returns {f['ret']} ({b})")
    for i, (dp, fp) in enumerate(zip(dps, fps)):
        a, b = cls(dp), cls(fp)
        if a is None or b is None:
            if verbose:
                unresolved.append(f"{where}: argument {i + 1}: {dp!r} vs {fp!r}")
            continue
        if clash(a, b):
            findings.append(f"{where}: argument {i + 1} is {dp.strip()} ({a}), {f['name']} takes {fp.strip()} ({b})"
                            f"  [{f['path']}:{f['line']}]")


VAR = re.compile(r"\bORIG_VAR\s*\(\s*(0x[0-9a-fA-F]+)\s*,\s*([^,()]+?)\s*,\s*(\w+)\s*\)")
var_types = {}  # addr -> [(type, where)]
ORIG = re.compile(r"\bORIG_FN(?:_STDCALL)?\s*\(\s*(0x[0-9a-fA-F]+)\s*,")
ORIG_RP = re.compile(r"\bORIG_FN_REGPARM\s*\(\s*\d+\s*,\s*(0x[0-9a-fA-F]+)\s*,")
FNPTR = re.compile(r"\bBMX_FNPTR\s*\(")
for path in sorted((repo / "src").rglob("*")):
    if path.suffix not in (".c", ".h") or "compat/orig.h" in path.as_posix():
        continue
    text = ac.strip_c(path.read_text(errors="replace"))
    rel = path.relative_to(repo).as_posix()
    for rx in (ORIG, ORIG_RP):
        for m in rx.finditer(text):
            close = ac.match_close(text, m.start() + text[m.start():].index("("), "(", ")")
            parts = ac.split_args(text[text.index("(", m.start()) + 1 : close])
            if len(parts) < 4:
                continue
            ret, name, params = parts[-3], parts[-2].strip(), parts[-1].strip()
            addr = int(m.group(1), 16)
            line = text.count("\n", 0, m.start()) + 1
            f = ported.get(addr)
            if f is None:
                if verbose:
                    unresolved.append(f"{rel}:{line}: ORIG_FN {name} at {addr:#x} is not a ported function")
                continue
            if params.startswith("(") and params.endswith(")"):
                compare(f"{rel}:{line}: ORIG_FN {name}", ret, params[1:-1], f)
    for m in VAR.finditer(text):
        typ = " ".join(m.group(2).split())
        if "define ORIG_VAR" in text[max(0, m.start() - 20) : m.start()]:
            continue
        line = text.count("\n", 0, m.start()) + 1
        var_types.setdefault(int(m.group(1), 16), []).append((typ, f"{rel}:{line} {m.group(3)}"))
    for m in FNPTR.finditer(text):
        open_p = m.end() - 1
        close = ac.match_close(text, open_p, "(", ")")
        parts = ac.split_args(text[open_p + 1 : close])
        if len(parts) != 3:
            continue
        typ, fn = " ".join(parts[0].split()), parts[2].strip()
        f = by_name.get(fn)
        line = text.count("\n", 0, m.start()) + 1
        if f is None or not re.match(r"^\w+$", fn):
            continue
        fm = re.match(r"^(.*?)\(\s*(?:BMX_CDECL|BMX_STDCALL|__cdecl|__stdcall)?\s*\*\s*\)\s*\((.*)\)$", typ)
        if fm is None:
            tm = TYPEDEFS.get(typ)
            if verbose and tm != "ptr" and typ not in ("void *", "const void *"):
                unresolved.append(f"{rel}:{line}: BMX_FNPTR type {typ!r} not a function pointer")
            continue
        compare(f"{rel}:{line}: BMX_FNPTR {fn}", fm.group(1), fm.group(2), f)

for addr, uses in sorted(var_types.items()):
    classes = {}
    for typ, where in uses:
        c = None if typ in ARRAYS else cls(typ)
        if c in ("ptr", "word", "int"):
            classes.setdefault(c, []).append(f"{typ} ({where})")
    if "int" in classes and ("ptr" in classes or "word" in classes):
        findings.append(f"ORIG_VAR {addr:#010x} declared as " + " and as ".join(
            "; ".join(v) for v in classes.values()))
    # function pointers stored at one address: their return values and parameters must agree
    sigs = [(typ, where, FN_SIGS[typ]) for typ, where in uses if typ in FN_SIGS]
    slots = {}
    for typ, where, (ret, params) in sigs:
        if "..." in params:
            continue
        slots.setdefault("return value", []).append((cls(ret), typ, where))
        for i, p in enumerate(params_of(params)):
            slots.setdefault(f"argument {i + 1}", []).append((cls(p), typ, where))
    for slot, seen in slots.items():
        kinds = {c for c, _, _ in seen}
        if "int" in kinds and ("ptr" in kinds or "word" in kinds):
            findings.append(f"ORIG_VAR {addr:#010x} function pointers disagree on the {slot}: " + "; ".join(
                f"{c} in {t} ({w})" for c, t, w in seen if c in ("int", "ptr", "word")))

for line in findings:
    print(line)
if verbose:
    for line in unresolved:
        print("unresolved:", line)
print(f"{len(findings)} width mismatches")
sys.exit(1 if findings else 0)
