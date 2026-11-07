#!/usr/bin/env bash
# Builds the Android APK (docs/PORTING.md "X6 step 1 (Android arm64)"), from the repo root in `nix develop .#android`:
#
#   android/build.sh [debug|release]      (default debug)
#
# 1. SDL3 for arm64-v8a (tools/android_sdl3.sh) unless build/android-sdl3/prefix has it;
# 2. libmain.so (the `android` preset);
# 3. both into android/app/libs/arm64-v8a, then Gradle (nixpkgs' gradle; the Android Gradle plugin comes from
#    Google's Maven repository on the first run).
# Output: android/app/build/outputs/apk/<type>/app-<type>.apk
set -euo pipefail
root=$(cd "$(dirname "$0")/.." && pwd)
type=${1:-debug}
: "${ANDROID_NDK_ROOT:?run in nix develop .#android (or set ANDROID_NDK_ROOT, ANDROID_HOME, SDL3_SRC)}"
: "${SDL3_SRC:?SDL3 source (the version the native libraries were built from)}"
# Git Bash on Windows (no nix; ANDROID_HOME, ANDROID_NDK_ROOT, SDL3_SRC, JAVA_HOME and gradle on PATH set by hand):
# CMake and Gradle are Windows programs and need C:/... paths, not MSYS /c/... ones.
if command -v cygpath >/dev/null 2>&1; then
  for var in ANDROID_NDK_ROOT ANDROID_HOME SDL3_SRC; do
    if [ -n "${!var:-}" ]; then
      export "$var=$(cygpath -m "${!var}")"
    fi
  done
fi

cd "$root"
if [ ! -f build/android-sdl3/prefix/lib/libSDL3.so ]; then
  tools/android_sdl3.sh "$SDL3_SRC"
fi
cmake --preset android
cmake --build --preset android --target bmxnative

libs=android/app/libs/arm64-v8a
mkdir -p "$libs"
cp build/android/libmain.so build/android-sdl3/prefix/lib/libSDL3.so "$libs/"

cd android
gradle --no-daemon "assemble${type^}" -PsdlJava="$SDL3_SRC/android-project/app/src/main/java"
ls -l "app/build/outputs/apk/$type/"
