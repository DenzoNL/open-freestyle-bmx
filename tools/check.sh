#!/usr/bin/env bash
# Everything CI checks, runnable locally: format, build, clang-tidy, CSV freshness, differential tests.
# Differential tests are skipped when the original NgBMX.exe is not available.
set -euo pipefail
root=$(cd "$(dirname "$0")/.." && pwd)
cd "$root"
mapfile -t sources < <(git ls-files 'src/*.c' 'src/*.h' 'tests/cases/*.c' 'tests/cases/*.h')
echo "== clang-format"
clang-format --dry-run -Werror "${sources[@]}"
echo "== build (hook preset)"
cmake --preset hook >/dev/null
cmake --build --preset hook
echo "== clang-tidy"
mapfile -t units < <(git ls-files 'src/*.c' 'tests/cases/*.c')
clang-tidy --quiet -p build/hook "${units[@]}"
echo "== functions.csv"
python3 -I tools/progress.py --check >/dev/null
python3 -I -m py_compile tools/*.py tests/oracle/*.py
echo "== differential tests"
if [[ -e ${BMX_EXE:-work/NgBMX.exe} ]]; then
    tests/oracle/run.sh
else
    echo "skipped: NgBMX.exe not available"
fi
