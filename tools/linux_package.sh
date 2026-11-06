#!/usr/bin/env bash
# Packs the Linux build into a folder and a tarball (docs/PORTING.md "X4 (Linux x64)"): bmxnative, SDL3's shared
# library in lib/ (under its soname), and the executable's rpath set to $ORIGIN/lib, so the folder does not depend
# on where the build found SDL3. The game files are not part of it: bmxnative runs from the game folder.
# Run by the `bmxnative_tarball` target: cmake --build --preset linux --target bmxnative_tarball
#
#   tools/linux_package.sh <bmxnative> <libSDL3.so> <output folder>    (writes <folder> and <folder>.tar.gz)
#
# Needs patchelf. A build in `nix develop` links against the nix store's glibc and loader: its package runs on
# machines with nix (NixOS); for other distributions or the Steam Deck build outside nix (docs/PORTING.md X4).
set -euo pipefail
exe=$1
sdl=$2
out=$3
soname=$(patchelf --print-soname "$sdl")

rm -rf "$out" "$out.tar.gz"
mkdir -p "$out/lib"
cp "$exe" "$out/bmxnative"
cp -L "$sdl" "$out/lib/$soname"
chmod u+w "$out/bmxnative" "$out/lib/$soname"
# shellcheck disable=SC2016 # $ORIGIN is for the dynamic loader, not the shell
patchelf --set-rpath '$ORIGIN/lib' "$out/bmxnative"
tar -C "$(dirname "$out")" -czf "$out.tar.gz" "$(basename "$out")"
echo "wrote $out and $out.tar.gz"
