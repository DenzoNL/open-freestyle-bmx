# Hosted 32-bit Windows target for the native build (OpenFreestyleBMX.exe): llvm-mingw provides the C library
# and Windows headers. Game objects add the freestanding flags themselves (see CMakeLists.txt).
set(CMAKE_SYSTEM_NAME Windows)
set(CMAKE_SYSTEM_PROCESSOR x86)

if(DEFINED ENV{OPENDMFBMX_MINGW_CLANG})
  set(CMAKE_C_COMPILER "$ENV{OPENDMFBMX_MINGW_CLANG}")
else()
  set(CMAKE_C_COMPILER i686-w64-mingw32-clang)
endif()
set(CMAKE_RC_COMPILER_INIT i686-w64-mingw32-windres)

# SSE2 maths on purpose, same as the hook build: the ported code models the x87 explicitly.
set(CMAKE_C_FLAGS_INIT "-msse2 -mfpmath=sse -fno-math-errno -fms-extensions")
