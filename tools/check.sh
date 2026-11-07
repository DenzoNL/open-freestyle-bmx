#!/usr/bin/env bash
# Everything CI checks, runnable locally: format, build, clang-tidy, CSV freshness, differential tests.
# Differential tests are skipped when the original NgBMX.exe is not available.
set -euo pipefail
root=$(cd "$(dirname "$0")/.." && pwd)
cd "$root"
mapfile -t sources < <(git ls-files -co --exclude-standard 'src/*.c' 'src/*.h' 'tests/cases/*.c' 'tests/cases/*.h')
echo "== clang-format (${SECONDS} s)"
clang-format --dry-run -Werror "${sources[@]}"
echo "== build (hook preset) (${SECONDS} s)"
cmake --preset hook >/dev/null
cmake --build --preset hook
echo "== clang-tidy (${SECONDS} s)"
# src/native, src/platform, src/driver, src/png and src/data are only built by the native32 preset (hosted mingw, SDL3), not in
# build/hook's compile database; tidy them with -p build/native32 (docs/NATIVE.md).
mapfile -t units < <(git ls-files -co --exclude-standard 'src/*.c' 'tests/cases/*.c' ':!src/native/*' ':!src/platform/*' ':!src/driver/*' ':!src/png/*' ':!src/data/*')
# nixpkgs' clang-tidy wrapper puts the host libc on CPATH, which breaks the freestanding i686-windows
# units on Linux (gnu/stubs-32.h). Use the unwrapped clang-tidy next to the flake's cross clang.
tidy=(clang-tidy)
if [[ -n ${OPENDMFBMX_CROSS_CLANG:-} && -x $(dirname "$OPENDMFBMX_CROSS_CLANG")/clang-tidy ]]; then
    tidy=("$(dirname "$OPENDMFBMX_CROSS_CLANG")/clang-tidy"
        "--extra-arg=-resource-dir=$("$OPENDMFBMX_CROSS_CLANG" -print-resource-dir)")
fi
# One clang-tidy per batch of units, in parallel (serially it took ~2 minutes).
printf '%s\0' "${units[@]}" | xargs -0 -n 16 -P "${BMX_JOBS:-$(nproc)}" "${tidy[@]}" --quiet -p build/hook
echo "== functions.csv (${SECONDS} s)"
"${PYTHON:-python3}" -I tools/progress.py --check >/dev/null
"${PYTHON:-python3}" -I -m py_compile tools/*.py tests/oracle/*.py
echo "== pointer widths (${SECONDS} s)"
# Declarations that pass a pointer where the ported function takes a 32-bit integer (docs/PORTING.md "X2e"), or
# an integer / pointer where it takes a float (X3: 64-bit passes floats in vector registers).
"${PYTHON:-python3}" -I tools/width_audit.py . | tail -n 20
echo "== native imports (${SECONDS} s)"
# The native exe's import table must stay free of the Windows DLLs the platform layer replaced (docs/PLATFORM.md
# 4f): native64 (the release build) and native32 (the test build, docs/RELEASE-PLAN.md D3), whichever exist (CI
# without them is unaffected); needs llvm-objdump (llvm-mingw has it).
# sdl3: window, input, audio, GPU. kernel32: the loader / trap machinery and the thin CRT layer (VirtualProtect,
# AddVectoredExceptionHandler, LoadLibrary for the dev checks, ...). api-ms-win-crt-*: the UCRT's forwarders,
# which the mingw CRT imports.
allowed='^(sdl3\.dll|kernel32\.dll|api-ms-win-crt-[a-z0-9-]+\.dll)$'
checked=0
for build in native64 native32; do
    exe=build/$build/OpenFreestyleBMX.exe
    [[ -e $exe ]] || continue
    mapfile -t imported < <("${OBJDUMP:-llvm-objdump}" -p "$exe" | sed -n 's/^[[:space:]]*DLL Name: //p' | tr -d '')
    if [[ ${#imported[@]} -eq 0 ]]; then
        echo "import check: no DLL names found in $exe (llvm-objdump missing?)" >&2
        exit 1
    fi
    bad=0
    for dll in "${imported[@]}"; do
        if [[ ! ${dll,,} =~ $allowed ]]; then
            echo "import check: $exe imports $dll, which is not on the allowlist (tools/check.sh)" >&2
            bad=1
        fi
    done
    [[ $bad -eq 0 ]] || exit 1
    echo "ok: $build: ${#imported[@]} imported DLLs, all allowed"
    checked=1
done
[[ $checked -eq 1 ]] || echo "skipped: no build/native64 or build/native32"
echo "== native32 data (${SECONDS} s)"
# The original's data in the source (src/data, docs/NATIVE.md): every byte and pointer of the compiled blocks
# against NgBMX.exe's. Needs build/native32's objects and the exe.
if [[ -d build/native32/CMakeFiles/bmx_data.dir && -e ${BMX_EXE:-work/NgBMX.exe} ]]; then
    "${PYTHON:-python3}" -I tools/gen_data.py --check
else
    echo "skipped: build/native32 or NgBMX.exe not available"
fi
echo "== differential tests (${SECONDS} s)"
if [[ -e ${BMX_EXE:-work/NgBMX.exe} ]]; then
    tests/oracle/run.sh ${BMX_JOBS:+-j "$BMX_JOBS"} # BMX_JOBS: fewer workers when the PC is in use
else
    echo "skipped: NgBMX.exe not available"
fi
