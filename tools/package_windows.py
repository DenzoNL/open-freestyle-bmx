"""Packs the Windows release zip (docs/RELEASE-PLAN.md, release steps).

    py -I tools/package_windows.py [--build build/native64] [--sdl-license <SDL's LICENSE.txt>] [--out dist]

Writes <out>/OpenFreestyleBMX-<version>-windows-x64.zip with OpenFreestyleBMX.exe (debug symbols stripped into a
copy; the build keeps its own), SDL3.dll, README.md, LICENSE and THIRD-PARTY-NOTICES.txt (SDL3's zlib licence
and dr_flac's), and prints its SHA-256. The version comes from android/app/build.gradle's versionName, so the
two downloads always carry the same number. Never adds game files: only the names above go into the zip.
"""
import argparse
import hashlib
import os
import re
import shutil
import subprocess
import sys
import tempfile
import zipfile

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))


def version():
    gradle = open(os.path.join(ROOT, "android", "app", "build.gradle"), encoding="utf-8").read()
    m = re.search(r"versionName\s*=\s*'([^']+)'", gradle)
    if not m:
        sys.exit("no versionName in android/app/build.gradle")
    return m.group(1)


def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("--build", default=os.path.join(ROOT, "build", "native64"))
    ap.add_argument("--sdl-license", help="SDL's LICENSE.txt (default: next to the SDL3 the build used)")
    ap.add_argument("--strip", default="llvm-strip", help="llvm-strip (llvm-mingw has it)")
    ap.add_argument("--out", default=os.path.join(ROOT, "dist"))
    args = ap.parse_args()

    exe = os.path.join(args.build, "OpenFreestyleBMX.exe")
    dll = os.path.join(args.build, "SDL3.dll")
    for path in (exe, dll):
        if not os.path.exists(path):
            sys.exit(f"missing {path}: build the native64 preset first")
    sdl_license = args.sdl_license
    if sdl_license is None:
        cache = open(os.path.join(args.build, "CMakeCache.txt"), encoding="utf-8", errors="replace").read()
        m = re.search(r"^BMX_SDL3_DIR:PATH=(.*)$", cache, re.M)
        if m:
            sdl_license = os.path.join(os.path.dirname(m.group(1).strip()), "LICENSE.txt")
    if sdl_license is None or not os.path.exists(sdl_license):
        sys.exit("SDL's LICENSE.txt not found: pass --sdl-license")
    license_file = os.path.join(ROOT, "LICENSE")
    if not os.path.exists(license_file):
        sys.exit("no LICENSE in the repository yet (the user picks one: docs/RELEASE-PLAN.md)")

    notices = (
        "Open Freestyle BMX includes the following third-party software.\n\n"
        "== SDL 3 (SDL3.dll), https://libsdl.org ==\n\n"
        + open(sdl_license, encoding="utf-8", errors="replace").read().strip()
        + "\n\n== dr_flac, https://github.com/mackron/dr_libs ==\n\n"
        + open(os.path.join(ROOT, "third_party", "dr_flac", "LICENSE"), encoding="utf-8", errors="replace").read().strip()
        + "\n"
    )
    os.makedirs(args.out, exist_ok=True)
    name = f"OpenFreestyleBMX-{version()}-windows-x64.zip"
    target = os.path.join(args.out, name)
    with tempfile.TemporaryDirectory() as tmp:
        stripped = os.path.join(tmp, "OpenFreestyleBMX.exe")
        shutil.copyfile(exe, stripped)
        subprocess.run([args.strip, "--strip-debug", stripped], check=True)
        with zipfile.ZipFile(target, "w", zipfile.ZIP_DEFLATED, compresslevel=9) as z:
            z.write(stripped, "OpenFreestyleBMX.exe")
            z.write(dll, "SDL3.dll")
            z.write(os.path.join(ROOT, "README.md"), "README.md")
            z.write(license_file, "LICENSE.txt")
            z.writestr("THIRD-PARTY-NOTICES.txt", notices)
    digest = hashlib.sha256(open(target, "rb").read()).hexdigest()
    print(f"{target}\nsha256 {digest}")


if __name__ == "__main__":
    main()
