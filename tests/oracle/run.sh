#!/usr/bin/env bash
# Run the differential tests. Extra args go to oracle.py (-k NAME, -n ITERS, -v).
set -euo pipefail
root=$(cd "$(dirname "$0")/../.." && pwd)
exe=${BMX_EXE:-$root/work/NgBMX.exe}
dll=${BMX_TEST_DLL:-$root/build/hook/bmxtest.dll}
if [[ ! -e $exe ]]; then
    echo "NgBMX.exe not found (set BMX_EXE or run tools/setup-work.sh)" >&2
    exit 2
fi
exec python3 -I "$root/tests/oracle/oracle.py" --exe "$exe" --dll "$dll" "$@"
