# Freestanding 32-bit Windows target for the hook phase: our code runs inside (or alongside, in the
# emulator) the original NgBMX.exe and uses its CRT, heap and globals.
set(CMAKE_SYSTEM_NAME Windows)
set(CMAKE_SYSTEM_PROCESSOR x86)

if(DEFINED ENV{OPENDMFBMX_CROSS_CLANG})
  set(CMAKE_C_COMPILER "$ENV{OPENDMFBMX_CROSS_CLANG}")
else()
  set(CMAKE_C_COMPILER clang)
endif()
if(DEFINED ENV{OPENDMFBMX_LLD_LINK})
  set(CMAKE_LINKER "$ENV{OPENDMFBMX_LLD_LINK}")
else()
  set(CMAKE_LINKER lld-link)
endif()
set(CMAKE_C_COMPILER_TARGET i686-pc-windows-msvc)
set(CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY)

# SSE2 maths on purpose: ported code models the original x87 arithmetic explicitly (double = value in
# an x87 register at 53-bit precision, float = value stored to memory), which only holds with strict
# IEEE semantics. LLVM's x87 backend does not round float operations to 24 bits.
set(CMAKE_C_FLAGS_INIT "-ffreestanding -fno-builtin -fno-stack-protector -fno-math-errno -msse2 -mfpmath=sse -fms-extensions")
# No -noentry here: it silently overrides a target's -entry (the hook DLL's DllMain never ran).
# bmxtest sets it itself; the emulator loads that DLL without calling an entry point.
set(CMAKE_SHARED_LINKER_FLAGS_INIT "-fuse-ld=lld -nostdlib -Wl,-machine:x86")

# No Windows SDK or MSVC runtime: drop CMake's default import libraries and runtime selection.
set(CMAKE_C_STANDARD_LIBRARIES "" CACHE STRING "" FORCE)
set(CMAKE_MSVC_RUNTIME_LIBRARY "")
string(APPEND CMAKE_SHARED_LINKER_FLAGS_INIT " -Wl,-nodefaultlib")
