# Native Linux x86_64 build of bmxnative (docs/PORTING.md "X4 (Linux x64)"): the host's clang and glibc, SDL3
# through its CMake config (`nix develop` sets SDL3_ROOT; elsewhere the distribution's SDL3 package, or
# -DSDL3_DIR=<prefix>/lib/cmake/SDL3). Not a cross toolchain: it only picks the compiler and the flags.
#
# clang only: the sources use clang's diagnostic pragmas and warning names under -Werror (GCC would warn about
# them). OPENDMFBMX_LINUX_CLANG names another clang (a version suffix, a full path).
#
# Same floating-point flags as cmake/x86_64-mingw.cmake: SSE2 for float / double (the x86_64 baseline, spelled
# out), the x87 inline assembly of compat/x87.h for the original's x87 sequences, no FMA contraction (none without
# -mfma anyway; CMakeLists.txt passes -ffp-contract=off off Windows). The x87 control word Linux starts with
# (0x037f) is set to Windows' 0x027f at startup (compat/fpu.h bmxFpuWindowsDefaults).
if(DEFINED ENV{OPENDMFBMX_LINUX_CLANG})
  set(CMAKE_C_COMPILER "$ENV{OPENDMFBMX_LINUX_CLANG}")
else()
  set(CMAKE_C_COMPILER clang)
endif()

set(CMAKE_C_FLAGS_INIT "-msse2 -mfpmath=sse")
