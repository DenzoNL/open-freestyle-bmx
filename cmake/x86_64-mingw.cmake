# Hosted 64-bit Windows target for the native build (docs/PORTING.md X3): llvm-mingw's x86_64 C library and
# Windows headers. Same flags as cmake/i686-mingw.cmake (SSE2 is the x86_64 baseline; the x87 inline assembly
# the ported maths uses exists on x86_64 too).
set(CMAKE_SYSTEM_NAME Windows)
set(CMAKE_SYSTEM_PROCESSOR AMD64)

if(DEFINED ENV{OPENDMFBMX_MINGW64_CLANG})
  set(CMAKE_C_COMPILER "$ENV{OPENDMFBMX_MINGW64_CLANG}")
else()
  set(CMAKE_C_COMPILER x86_64-w64-mingw32-clang)
endif()
set(CMAKE_RC_COMPILER_INIT x86_64-w64-mingw32-windres)

set(CMAKE_C_FLAGS_INIT "-msse2 -mfpmath=sse -fno-math-errno -fms-extensions")
