#!/usr/bin/env bash
# Everything CI checks, runnable locally: format, build, clang-tidy, CSV freshness, differential tests.
# Differential tests are skipped when the original NgBMX.exe is not available.
set -euo pipefail
root=$(cd "$(dirname "$0")/.." && pwd)
cd "$root"
mapfile -t sources < <(git ls-files -co --exclude-standard 'src/*.c' 'src/*.h' 'tests/cases/*.c' 'tests/cases/*.h')
echo "== clang-format"
clang-format --dry-run -Werror "${sources[@]}"
echo "== build (hook preset)"
cmake --preset hook >/dev/null
cmake --build --preset hook
echo "== clang-tidy"
# src/native, src/driver and src/png are only built by the native32 preset (hosted mingw, SDL3), not in build/hook's
# compile database; tidy them with -p build/native32 (docs/NATIVE.md).
mapfile -t units < <(git ls-files -co --exclude-standard 'src/*.c' 'tests/cases/*.c' ':!src/native/*' ':!src/driver/*' ':!src/png/*')
# nixpkgs' clang-tidy wrapper puts the host libc on CPATH, which breaks the freestanding i686-windows
# units on Linux (gnu/stubs-32.h). Use the unwrapped clang-tidy next to the flake's cross clang.
tidy=(clang-tidy)
if [[ -n ${OPENDMFBMX_CROSS_CLANG:-} && -x $(dirname "$OPENDMFBMX_CROSS_CLANG")/clang-tidy ]]; then
    tidy=("$(dirname "$OPENDMFBMX_CROSS_CLANG")/clang-tidy"
        "--extra-arg=-resource-dir=$("$OPENDMFBMX_CROSS_CLANG" -print-resource-dir)")
fi
"${tidy[@]}" --quiet -p build/hook "${units[@]}"
echo "== functions.csv"
"${PYTHON:-python3}" -I tools/progress.py --check >/dev/null
"${PYTHON:-python3}" -I -m py_compile tools/*.py tests/oracle/*.py
echo "== differential tests"
if [[ -e ${BMX_EXE:-work/NgBMX.exe} ]]; then
    tests/oracle/run.sh ${BMX_JOBS:+-j "$BMX_JOBS"} # BMX_JOBS: fewer workers when the PC is in use
else
    echo "skipped: NgBMX.exe not available"
fi
