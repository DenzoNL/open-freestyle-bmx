#!/usr/bin/env python3
"""Generate the map from original code addresses to the functions that serve them in the native exe
(docs/PORTING.md X1a, docs/NATIVE.md "The original's data in the source").

Usage: tools/gen_origmap.py OUT_DIR SRC_DIR

The native exe (BMX_IMAGE) runs none of the original's code and maps nothing at its addresses, so every original
code address still named in the source or stored in the original's data needs a function of ours. They come from
every table that used to put a jump at an original address:
- the ported functions (`BMX_ADDR` annotations, the hook table of tools/gen_hooks.py);
- the C runtime layer (src/native/crt.c, its `E(addr, fn, name)` table);
- the PNG reader (src/png/png.c, bmxPngJumps) and the SDL3 driver (src/driver/device.c, bmxDriverJumps);
- the special cases below: one RenderWare stub that falls into a ported body, unported functions that only
  trap, and the exe's own `jmp [import slot]` thunks that ported code calls.

Outputs:
- origmap.h: `#define BMX_ORIGFN_0x004a50d2 "_crtMalloc"`, the assembler name of the function serving each
  address; `ORIG_FN` in BMX_IMAGE mode (src/compat/orig.h) declares its function with that name.
- origmap.c: the same map as a sorted table (bmxOrigMap: bmxOrigPtr finds RenderWare's callbacks in it, the
  trap handler names addresses with it) and the trap stubs.

tools/gen_data.py uses the same map for the code pointers in the original's data (src/data).

Every `ORIG_FN*` address in the source must be in the map (the build fails otherwise, here with a list).
"""
import re
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
from gen_hooks import scan  # noqa: E402

# RpWorldDestroy (0x4ba9c0) is a four byte stub (`mov eax, [esp+4]`) that falls into its body at 0x4ba9c4, which
# is ported as rpWorldDestroyBody (regparm(1), rw/world.c). bmxRpWorldDestroy is the stub as a cdecl function.
SPECIAL = {0x004BA9C0: "bmxRpWorldDestroy"}

# Not ported because nothing reaches them (data/functions-rw.csv `skip`), but a pointer or an ORIG_FN declaration
# still names them: they get a stub that reports the address and stops, as the int3 traps did.
TRAPS = {
    0x0047ED10: "EpicFail (zdebugFail, the fatal assertion handler; not ported)",
    0x004A63C5: "CRT float exception setup (pcprofile.c crtFpInit)",
    0x004A6415: "CRT float exception probe (pcprofile.c crtFpInit)",
    0x004A643E: "CRT float helper (pcprofile.c crtFpInit)",
    0x004A6498: "CRT float helper (pcprofile.c crtFpInit)",
    0x004A64E6: "CRT float helper (pcprofile.c crtFpInit)",
    0x004A64FE: "CRT float helper (pcprofile.c crtFpInit)",
    0x004A6803: "CRT _cfltcvt (pcprofile.c crtFpInit)",
    0x004C6490: "_rpMeshGenerateTriStrip (tri-strip generator, lib/unreached)",
    0x004C7560: "_rpMeshGenerateTriStrip variant (tri-strip generator, lib/unreached)",
    0x004CD510: "rwWorldPvsDefaultCallback (PVS creation progress, D3D7 driver)",
    0x004D6590: "rwD3DSetFogDensity (D3D7 driver)",
    0x004E0160: "rwRasterMipmapPaletted (D3D7 driver)",
    0x004EF4E0: "rwQuantHistogram (palette quantiser, D3D7 driver)",
    0x004F0DA0: "rwQuantBuildPalette (palette quantiser, D3D7 driver)",
    0x004F19E0: "rwQuantMapPixels (palette quantiser, D3D7 driver)",
    0x004F1F90: "rwQuantClose (palette quantiser, D3D7 driver)",
    0x004F2070: "rwQuantOpen (palette quantiser, D3D7 driver)",
    0x004F25D0: "rwQuantResample (palette quantiser, D3D7 driver)",
}

# The exe's `jmp [import slot]` thunks that ported code calls by address (ORIG_FN_STDCALL). Each is served by a stub
# in src/data/imports.c (BMX_IMPORT_THUNK, made by tools/gen_data.py, which checked the slot against the exe) that
# jumps through the same import slot, so the call reaches the same stand-in or Windows function as the original's.
IMPORT_THUNKS = {
    0x004AF9D0: "AVIFIL32!AVIFileInit",
    0x004AF9D6: "AVIFIL32!AVIFileExit",
    0x004AF9DC: "AVIFIL32!AVIStreamReadFormat",
    0x004AF9E2: "AVIFIL32!AVIStreamInfoA",
    0x004AF9E8: "AVIFIL32!AVIStreamRead",
    0x004AF9EE: "AVIFIL32!AVIStreamStart",
    0x004AF9F4: "AVIFIL32!AVIStreamLength",
    0x004AF9FA: "AVIFIL32!AVIStreamTimeToSample",
    0x004AFA00: "MSVFW32!DrawDibOpen",
    0x004AFA06: "AVIFIL32!AVIStreamGetFrameOpen",
    0x004AFA0C: "AVIFIL32!AVIStreamSampleToTime",
    0x004AFA12: "AVIFIL32!AVIFileRelease",
    0x004AFA18: "AVIFIL32!AVIFileGetStream",
    0x004AFA24: "AVIFIL32!AVIFileOpenA",
    0x004AFA2A: "AVIFIL32!AVIStreamRelease",
    0x004AFA30: "AVIFIL32!AVISaveOptionsFree",
    0x004AFA36: "MSVFW32!DrawDibClose",
    0x004AFA3C: "AVIFIL32!AVIStreamGetFrameClose",
    0x004AFA42: "MSVFW32!DrawDibDraw",
    0x004AFA48: "AVIFIL32!AVIStreamGetFrame",
    0x004AFC58: "DINPUT!DirectInputCreateEx",
}

ORIG_FN_SITE = re.compile(r"\bORIG_FN(?:_STDCALL|_REGPARM)?\(\s*(?:\d+\s*,\s*)?(0x[0-9a-fA-F]+)\s*,\s*[^,]+,\s*(\w+)")
CANONICAL = re.compile(r"0x[0-9a-f]{8}$")
# BMX_FNPTR(type, addr, fn) and the files' own wrappers of it.
FNPTR_SITE = re.compile(r"\b(BMX_FNPTR|HANDLER|CLIP_FN|D3D_FN)\(")
THUNKS = (0x00401005, 0x00402617)


def jump_table(path, array):
    """{orig: function} of a `const BmxHook <array>[] = {{0x..., (void *)fn}, ...};` table."""
    text = Path(path).read_text()
    m = re.search(re.escape(array) + r"\[\]\s*=\s*\{(.*?)\n\};", text, re.S)
    if not m:
        sys.exit(f"{path}: {array} not found")
    return {int(a, 16): fn for a, fn in re.findall(r"\{\s*(0x[0-9a-fA-F]+),\s*\(void \*\)(\w+)\s*\}", m.group(1))}


def crt_table(path):
    return {int(a, 16): fn for a, fn in re.findall(r"^\s*E\((0x[0-9a-fA-F]+),\s*(\w+),", Path(path).read_text(), re.M)}


def build_map(src):
    """{orig: (assembler name, origin)}; later tables override earlier ones, as the old loader's jumps did (crt only
    where no port was, then the driver and the PNG reader over the ports)."""
    owners = {}
    for addr, fn in crt_table(src / "native" / "crt.c").items():
        owners[addr] = (fn, "crt")
    for addr, fn in scan(src).items():
        owners[addr] = (fn, "port")
    for addr, fn in jump_table(src / "driver" / "device.c", "bmxDriverJumps").items():
        owners[addr] = (fn, "driver")
    for addr, fn in jump_table(src / "png" / "png.c", "bmxPngJumps").items():
        owners[addr] = (fn, "png")
    for addr, fn in SPECIAL.items():
        owners[addr] = (fn, "special")
    for addr in TRAPS:
        if addr in owners:
            sys.exit(f"{addr:#x} is listed as a trap but served by {owners[addr][0]}: remove it from TRAPS")
        owners[addr] = (f"bmxOrigTrap_{addr:08x}", "trap")
    for addr in IMPORT_THUNKS:
        if addr in owners:
            sys.exit(f"{addr:#x} is listed as an import thunk but served by {owners[addr][0]}")
        owners[addr] = (f"bmxImportThunk_{addr:08x}", "import")
    return owners


def macro_args(text, start):
    """The top-level arguments of the macro call whose '(' is at text[start], or None."""
    args, depth, i, begin = [], 0, start, start + 1
    while i < len(text):
        c = text[i]
        if c == "(":
            depth += 1
        elif c == ")":
            depth -= 1
            if depth == 0:
                args.append(text[begin:i].strip())
                return args
        elif c == "," and depth == 1:
            args.append(text[begin:i].strip())
            begin = i + 1
        i += 1
    return None


def check_sites(src, owners):
    """Every ORIG_FN address must be served; every BMX_FNPTR (and its wrappers) must name the function that serves
    its address, which natively is the pointer itself. Incremental-link thunk addresses (0x401005..0x402617) stand
    for the function they jump to, which only the exe knows; those BMX_FNPTR names are not checked here."""
    errors = []
    for path in sorted(list(src.rglob("*.c")) + list(src.rglob("*.h"))):
        if path.parent.name == "hook" or path.name == "orig.h":
            continue
        text = path.read_text()
        rel = path.relative_to(src.parent).as_posix()
        for m in ORIG_FN_SITE.finditer(text):
            spelled, name = m.group(1), m.group(2)
            where = f"{rel}: ORIG_FN {spelled} ({name})"
            if not CANONICAL.match(spelled):
                errors.append(f"{where}: write the address as 0x%08x (lower case), the generated names use that")
            elif int(spelled, 16) not in owners:
                errors.append(f"{where}: no function serves this address (not ported, not in a jump table)")
        for m in FNPTR_SITE.finditer(text):
            args = macro_args(text, m.end() - 1)
            if args is None or text[m.start():].startswith("BMX_FNPTR(type") or "#define" in text[
                    text.rfind("\n", 0, m.start()):m.start()]:
                continue
            addrs = [a for a in args if re.fullmatch(r"0x[0-9a-fA-F]+", a)]
            fn = re.sub(r"^(\([^()]*(\([^()]*\))*[^()]*\)\s*)+", "", args[-1]).strip("() ")
            if len(addrs) != 1 or not re.fullmatch(r"\w+", fn):
                continue
            addr = int(addrs[0], 16)
            where = f"{rel}: {m.group(1)} {addrs[0]} ({fn})"
            if addr in owners:
                if owners[addr][0] != fn:
                    errors.append(f"{where}: the function serving this address is {owners[addr][0]}")
            elif not THUNKS[0] <= addr < THUNKS[1]:
                errors.append(f"{where}: no function serves this address")
    if errors:
        sys.exit("\n".join(errors))


def write_if_changed(path, text):
    if not path.exists() or path.read_text() != text:
        path.write_text(text)


def main():
    out, src = Path(sys.argv[1]), Path(sys.argv[2])
    owners = build_map(src)
    check_sites(src, owners)
    addrs = sorted(owners)

    head = ["/* Generated by tools/gen_origmap.py: the function serving each original code address. */",
            "#ifndef BMX_ORIGMAP_H", "#define BMX_ORIGMAP_H", ""]
    head += [f'#define BMX_ORIGFN_0x{a:08x} "_{owners[a][0]}"' for a in addrs]
    head += ["", "#endif", ""]
    write_if_changed(out / "origmap.h", "\n".join(head))

    c = ["/* Generated by tools/gen_origmap.py (docs/NATIVE.md \"The original's data in the source\"). */",
         '#include "native/native.h"', '#include "origmap.h"', ""]
    # Declared under their assembler names: the C prototypes differ and some are static elsewhere by name only.
    for i, a in enumerate(addrs):
        c.append(f'void bmxOrigImpl{i}(void) __asm__(BMX_ORIGFN_0x{a:08x});')
    c += [""]
    for a, what in sorted(TRAPS.items()):
        c.append(f"/* {what} */")
        c.append(f"void bmxOrigTrap_{a:08x}(void)")
        c.append(f"{{")
        c.append(f"    bmxOrigTrap(0x{a:08x}, (uint32_t)(uintptr_t)__builtin_return_address(0));")
        c.append(f"}}")
    c += ["", "const BmxOrigEntry bmxOrigMap[] = {"]
    kinds = {"crt": "kBmxOrigCrt", "port": "kBmxOrigPort", "driver": "kBmxOrigDriver", "png": "kBmxOrigPng",
             "special": "kBmxOrigSpecial", "trap": "kBmxOrigTrapStub", "import": "kBmxOrigImport"}
    c += [f"    {{0x{a:08x}, (const void *)bmxOrigImpl{i}, {kinds[owners[a][1]]}}}," for i, a in enumerate(addrs)]
    c += ["};", f"const uint32_t bmxOrigMapCount = {len(addrs)};", ""]
    write_if_changed(out / "origmap.c", "\n".join(c))


if __name__ == "__main__":
    main()
