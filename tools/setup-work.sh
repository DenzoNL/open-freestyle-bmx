#!/usr/bin/env bash
# Point the repo at an original install and (re)generate work/: exe link + Ghidra bundles.
# Usage: tools/setup-work.sh /path/to/install/NgBMX.exe [labels.tsv]
set -euo pipefail
root=$(cd "$(dirname "$0")/.." && pwd)
exe=$(cd "$(dirname "$1")" && pwd)/$(basename "$1")
want=$(cat "$root/tools/known_exe.txt")
have=$(shasum -a 256 "$exe" 2>/dev/null | cut -d' ' -f1 || sha256sum "$exe" | cut -d' ' -f1)
if [[ $have != "$want" ]]; then
    echo "unexpected NgBMX.exe (sha256 $have, want $want)" >&2
    exit 1
fi
mkdir -p "$root/work"
ln -sf "$exe" "$root/work/NgBMX.exe"
if command -v ghidra-analyzeHeadless >/dev/null || command -v analyzeHeadless >/dev/null; then
    "$root/tools/ghidra/export.sh" "$exe" "${2:-}"
    python3 -I "$root/tools/modmap.py"
else
    echo "Ghidra not available: skipped bundle export (only needed for porting, not for tests)"
fi
