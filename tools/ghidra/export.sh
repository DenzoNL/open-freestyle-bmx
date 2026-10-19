#!/usr/bin/env bash
# Rebuild the Ghidra project from the user's NgBMX.exe and export per-function bundles to work/.
# Usage: tools/ghidra/export.sh /path/to/NgBMX.exe [labels.txt]
set -euo pipefail
exe=$1
labels=${2:-}
root=$(cd "$(dirname "$0")/../.." && pwd)
work=$root/work
mkdir -p "$work/ghidra"
headless=$(command -v ghidra-analyzeHeadless || command -v analyzeHeadless)
post=(-postScript SeedILT.java -postScript SeedPtrs.java -postScript SeedPtrs.java)
if [[ -n $labels ]]; then post+=(-postScript ApplyLabels.java "$labels"); fi
post+=(-postScript ExportBundles.java "$work/bundles" 401000 4a4a80)
"$headless" "$work/ghidra" ngbmx -import "$exe" -overwrite -scriptPath "$root/tools/ghidra" "${post[@]}" \
    >"$work/ghidra/export.log" 2>&1
grep -E '> (Seed|Apply|Export)' "$work/ghidra/export.log"
