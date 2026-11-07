#!/usr/bin/env bash
# Builds SDL3 for Android arm64-v8a from source as a shared library (docs/PORTING.md "X6 step 1 (Android arm64)")
# and installs it into a prefix the `android` preset finds (SDL3_DIR = <prefix>/lib/cmake/SDL3).
#
#   tools/android_sdl3.sh [<SDL3 source> [<prefix>]]
#
# Defaults: the source is $SDL3_SRC (`nix develop .#android` sets it to nixpkgs' sdl3.src, the same SDL3 version
# as the desktop builds), the prefix build/android-sdl3/prefix. Needs ANDROID_NDK_ROOT, cmake and ninja. The SDL
# Java sources of the same version (android-project/app/src/main/java/org/libsdl/app) go into the APK:
# android/build.sh copies them from the same source. SDL3 has one C++ file (hidapi), hence the static C++ runtime.
set -euo pipefail
root=$(cd "$(dirname "$0")/.." && pwd)
src=${1:-${SDL3_SRC:?"SDL3 source: pass it or set SDL3_SRC (nix develop .#android)"}}
prefix=${2:-$root/build/android-sdl3/prefix}
build=$root/build/android-sdl3/build
# Git Bash on Windows: CMake is a Windows program and needs C:/... paths, not MSYS /c/... ones.
if command -v cygpath >/dev/null 2>&1; then
  src=$(cygpath -m "$src")
  prefix=$(cygpath -m "$prefix")
  if [ -n "${ANDROID_NDK_ROOT:-}" ]; then
    export ANDROID_NDK_ROOT="$(cygpath -m "$ANDROID_NDK_ROOT")"
  fi
fi

cmake -S "$src" -B "$build" -G Ninja \
  -DCMAKE_TOOLCHAIN_FILE="$root/cmake/aarch64-android.cmake" \
  -DCMAKE_BUILD_TYPE=RelWithDebInfo \
  -DCMAKE_INSTALL_PREFIX="$prefix" \
  -DANDROID_STL=c++_static \
  -DSDL_SHARED=ON -DSDL_STATIC=OFF -DSDL_TEST_LIBRARY=OFF -DSDL_TESTS=OFF -DSDL_EXAMPLES=OFF
cmake --build "$build"
cmake --install "$build"
