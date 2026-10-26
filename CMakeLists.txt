cmake_minimum_required(VERSION 3.28)
project(opendmfbmx C)

set(CMAKE_C_STANDARD 17)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

option(BMX_HOOK "Build against the original NgBMX.exe (i686 hook/oracle phase)" ON)

set(BMX_WARNINGS -Wall -Wextra -Werror -Wno-unused-parameter -Wno-microsoft-anon-tag)

file(GLOB_RECURSE BMX_GAME_SOURCES CONFIGURE_DEPENDS src/zge/*.c src/bmx/*.c)
file(GLOB_RECURSE BMX_COMPAT_SOURCES CONFIGURE_DEPENDS src/compat/*.c)

add_library(bmx_game OBJECT ${BMX_GAME_SOURCES} ${BMX_COMPAT_SOURCES})
target_include_directories(bmx_game PUBLIC src)
target_compile_options(bmx_game PRIVATE ${BMX_WARNINGS})
if(BMX_HOOK)
  target_compile_definitions(bmx_game PUBLIC BMX_HOOK=1)
endif()

if(BMX_HOOK)
  find_package(Python3 REQUIRED COMPONENTS Interpreter)
  add_custom_command(OUTPUT ${CMAKE_BINARY_DIR}/hooks_table.c
    COMMAND Python3::Interpreter ${CMAKE_SOURCE_DIR}/tools/gen_hooks.py ${CMAKE_BINARY_DIR}/hooks_table.c
            ${CMAKE_SOURCE_DIR}/src
    DEPENDS ${BMX_GAME_SOURCES} ${CMAKE_SOURCE_DIR}/tools/gen_hooks.py)

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
  target_link_options(bmxtest PRIVATE -Wl,-base:0x10000000 -Wl,-fixed)

  # In-game hook DLL (dev only): dinput.dll proxy that redirects ported functions.
  add_library(bmxhook SHARED src/hook/hook.c src/hook/trace.c ${CMAKE_BINARY_DIR}/hooks_table.c $<TARGET_OBJECTS:bmx_game>)
  target_include_directories(bmxhook PRIVATE src)
  target_compile_definitions(bmxhook PRIVATE BMX_HOOK=1)
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
