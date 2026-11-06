# Android arm64-v8a build of bmxnative as libmain.so (docs/PORTING.md "X6 step 1 (Android arm64)"): the NDK's own
# toolchain file with the ABI and API level fixed. The NDK comes from ANDROID_NDK_ROOT (`nix develop .#android`
# sets it; NDK r29 from nixpkgs' androidenv), else ANDROID_NDK_HOME / ANDROID_NDK.
#
# arm64 has no x87: compat/x87.h takes the portable x87soft path (X5 part 2); CMakeLists.txt passes
# -ffp-contract=off off Windows (no fused multiply-adds). API 26 (Android 8): SDL3 needs 21, the SDL GPU's Vulkan
# backend 24+; the target (an AYN Thor) runs Android 13.
set(ANDROID_ABI arm64-v8a CACHE STRING "")
set(ANDROID_PLATFORM android-26 CACHE STRING "")
set(ANDROID_STL none CACHE STRING "") # C only

foreach(var ANDROID_NDK_ROOT ANDROID_NDK_HOME ANDROID_NDK)
  if(NOT BMX_ANDROID_NDK AND DEFINED ENV{${var}})
    set(BMX_ANDROID_NDK "$ENV{${var}}")
  endif()
endforeach()
if(NOT BMX_ANDROID_NDK)
  message(FATAL_ERROR "Android NDK not found: set ANDROID_NDK_ROOT (nix develop .#android does)")
endif()
include("${BMX_ANDROID_NDK}/build/cmake/android.toolchain.cmake")
