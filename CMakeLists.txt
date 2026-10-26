cmake_minimum_required(VERSION 3.28)
project(opendmfbmx C)

set(CMAKE_C_STANDARD 17)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

option(BMX_HOOK "Build against the original NgBMX.exe (i686 hook/oracle phase)" ON)

option(BMX_NATIVE32 "Build bmxnative.exe: our own 32-bit exe that rebuilds the original address space" OFF)
set(BMX_WORK_DIR "${CMAKE_SOURCE_DIR}/work" CACHE PATH "Directory with labels.raw (names in trap reports)")

set(BMX_WARNINGS -Wall -Wextra -Werror -Wno-unused-parameter -Wno-microsoft-anon-tag)

file(GLOB_RECURSE BMX_GAME_SOURCES CONFIGURE_DEPENDS src/zge/*.c src/bmx/*.c src/rw/*.c)
file(GLOB_RECURSE BMX_COMPAT_SOURCES CONFIGURE_DEPENDS src/compat/*.c)

add_library(bmx_game OBJECT ${BMX_GAME_SOURCES} ${BMX_COMPAT_SOURCES})
target_include_directories(bmx_game PUBLIC src)
target_compile_options(bmx_game PRIVATE ${BMX_WARNINGS})
# Debug aid: record entries/exits of every game function for the hook DLL crash report (src/hook/crash.c).
# Use a separate build dir (it changes code generation; bmxtest has no __cyg_profile_* hooks).
option(BMX_CALLTRACE "Instrument game functions for the hook crash report" OFF)
if(BMX_CALLTRACE)
  target_compile_options(bmx_game PRIVATE -finstrument-functions)
endif()
if(BMX_HOOK OR BMX_NATIVE32)
  target_compile_definitions(bmx_game PUBLIC BMX_HOOK=1)
  find_package(Python3 REQUIRED COMPONENTS Interpreter)
  add_custom_command(OUTPUT ${CMAKE_BINARY_DIR}/hooks_table.c
    COMMAND Python3::Interpreter ${CMAKE_SOURCE_DIR}/tools/gen_hooks.py ${CMAKE_BINARY_DIR}/hooks_table.c
            ${CMAKE_SOURCE_DIR}/src
    DEPENDS ${BMX_GAME_SOURCES} ${CMAKE_SOURCE_DIR}/tools/gen_hooks.py)
endif()

if(BMX_NATIVE32)
  # The hook toolchain is freestanding; here only the game objects are (the loader is hosted C).
  target_compile_options(bmx_game PRIVATE -ffreestanding -fno-builtin -fno-stack-protector
    -Wno-sign-compare) # enum constants are unsigned under the mingw target's ABI, int under msvc's
  enable_language(ASM)
  file(GLOB BMX_NATIVE_SOURCES CONFIGURE_DEPENDS src/native/*.c)
  # range.S must be the first object on the link line: its block has to start the image.
  add_library(bmx_range OBJECT src/native/range.S)
  # The driver would put the C runtime start files (crt2.o, crtbegin.o) before it: pass them ourselves, after.
  foreach(crt crt2.o crtbegin.o)
    execute_process(COMMAND ${CMAKE_C_COMPILER} -print-file-name=${crt} OUTPUT_VARIABLE crtPath
      OUTPUT_STRIP_TRAILING_WHITESPACE)
    list(APPEND BMX_CRT_STARTFILES ${crtPath})
  endforeach()
  add_executable(bmxnative $<TARGET_OBJECTS:bmx_range> ${BMX_CRT_STARTFILES} $<TARGET_OBJECTS:bmx_game>
    ${BMX_NATIVE_SOURCES} ${CMAKE_BINARY_DIR}/hooks_table.c)
  target_link_options(bmxnative PRIVATE -nostartfiles)
  target_include_directories(bmxnative PRIVATE src)
  target_compile_definitions(bmxnative PRIVATE _WIN32_WINNT=0x0A00 BMX_SOURCE_DIR="${CMAKE_SOURCE_DIR}" BMX_WORK_DIR="${BMX_WORK_DIR}")
  target_compile_options(bmxnative PRIVATE ${BMX_WARNINGS})
  # Fixed base, no ASLR. Headers sit at 0x3f0000 and the first section (.orig, src/native/range.c) at
  # 0x400000, so the exe itself owns the range the rebuilt original image needs. Everything else follows
  # above 0x6b3000.
  target_link_options(bmxnative PRIVATE -Wl,--image-base=0x3f0000 -Wl,--section-alignment=0x10000
    -Wl,--disable-dynamicbase -Wl,--disable-high-entropy-va)
  target_link_libraries(bmxnative PRIVATE bcrypt winmm)
endif()

if(BMX_HOOK)

  # Differential-test DLL: ported code + test cases, loaded next to NgBMX.exe in the Unicorn harness.
  file(GLOB_RECURSE BMX_TEST_SOURCES CONFIGURE_DEPENDS tests/cases/*.c)
  add_library(bmxtest SHARED ${BMX_TEST_SOURCES} ${CMAKE_BINARY_DIR}/hooks_table.c $<TARGET_OBJECTS:bmx_game>)
  target_include_directories(bmxtest PRIVATE src tests/cases)
  target_compile_definitions(bmxtest PRIVATE BMX_HOOK=1)
  # Test code is unoptimised on purpose: stub logs written to file-static arrays that nothing reads
  # would otherwise be removed as dead stores, and the harness would compare nothing. The ported code
  # under test (bmx_game objects) keeps the normal optimisation level. For the same reason, those
  # write-only statics are intended; newer clang (21+) flags them as unused-but-set globals.
  target_compile_options(bmxtest PRIVATE ${BMX_WARNINGS} -O0 -Wno-unknown-warning-option
    -Wno-unused-but-set-global)
  target_link_options(bmxtest PRIVATE -Wl,-base:0x10000000 -Wl,-fixed -Wl,-noentry)

  # In-game hook DLL (dev only): dinput.dll proxy that redirects ported functions.
  add_library(bmxhook SHARED src/hook/hook.c src/hook/trace.c src/hook/crash.c ${CMAKE_BINARY_DIR}/hooks_table.c $<TARGET_OBJECTS:bmx_game>)
  target_include_directories(bmxhook PRIVATE src)
  target_compile_definitions(bmxhook PRIVATE BMX_HOOK=1)
  if(BMX_CALLTRACE)
    target_compile_definitions(bmxhook PRIVATE BMX_CALLTRACE=1)
  endif()
  target_compile_options(bmxhook PRIVATE ${BMX_WARNINGS})
  set_target_properties(bmxhook PROPERTIES OUTPUT_NAME dinput)
  target_link_options(bmxhook PRIVATE -Wl,-base:0x10000000 -Wl,-entry:DllMain
    -Wl,-def:${CMAKE_SOURCE_DIR}/src/hook/dinput.def)
  target_link_libraries(bmxhook PRIVATE ${CMAKE_BINARY_DIR}/kernel32.lib)
  add_custom_command(OUTPUT ${CMAKE_BINARY_DIR}/kernel32.lib
    COMMAND llvm-dlltool -m i386 -k -d ${CMAKE_SOURCE_DIR}/src/hook/kernel32.def -l ${CMAKE_BINARY_DIR}/kernel32.lib
    DEPENDS src/hook/kernel32.def)
  add_custom_target(kernel32_implib DEPENDS ${CMAKE_BINARY_DIR}/kernel32.lib)
  add_dependencies(bmxhook kernel32_implib)
endif()
