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
  file(GLOB BMX_NATIVE_SOURCES CONFIGURE_DEPENDS src/native/*.c src/platform/*.c)
  # The smooth resampling mixer reads the game's mixer state at its original addresses (like src/driver).
  set_source_files_properties(src/platform/smoothmix.c PROPERTIES COMPILE_DEFINITIONS BMX_HOOK=1)
  # The music override (docs/MUSIC.md "Override"): musicgame.c reads the game's music stream like smoothmix.c;
  # the edit list is compiled in from data/music-edits.json; dr_flac (third_party) is built without warnings.
  set_source_files_properties(src/platform/musicgame.c PROPERTIES COMPILE_DEFINITIONS BMX_HOOK=1)
  # BMX_SIM_HASH (docs/WIDESCREEN.md W10.5) reads the rider and camera state at their original addresses too.
  set_source_files_properties(src/native/simhash.c PROPERTIES COMPILE_DEFINITIONS BMX_HOOK=1)
  set_source_files_properties(third_party/dr_flac/dr_flac.c PROPERTIES COMPILE_OPTIONS -w)
  add_custom_command(OUTPUT ${CMAKE_BINARY_DIR}/music_edits.c
    COMMAND Python3::Interpreter ${CMAKE_SOURCE_DIR}/tools/gen_music_edits.py
            ${CMAKE_SOURCE_DIR}/data/music-edits.json ${CMAKE_BINARY_DIR}/music_edits.c
    DEPENDS ${CMAKE_SOURCE_DIR}/data/music-edits.json ${CMAKE_SOURCE_DIR}/tools/gen_music_edits.py)
  set(BMX_MUSIC_SOURCES src/platform/music.c third_party/dr_flac/dr_flac.c ${CMAKE_BINARY_DIR}/music_edits.c)
  # range.S must be the first object on the link line: its block has to start the image.
  add_library(bmx_range OBJECT src/native/range.S)
  # The driver would put the C runtime start files (crt2.o, crtbegin.o) before it: pass them ourselves, after.
  foreach(crt crt2.o crtbegin.o)
    execute_process(COMMAND ${CMAKE_C_COMPILER} -print-file-name=${crt} OUTPUT_VARIABLE crtPath
      OUTPUT_STRIP_TRAILING_WHITESPACE)
    list(APPEND BMX_CRT_STARTFILES ${crtPath})
  endforeach()
  # SDL3 for i686 mingw (the SDL3-<version>/i686-w64-mingw32 folder of the SDL release's mingw devel package).
  file(GLOB BMX_SDL3_GUESS "$ENV{HOME}/tools/sdl3/SDL3-*/i686-w64-mingw32" "$ENV{USERPROFILE}/tools/sdl3/SDL3-*/i686-w64-mingw32")
  list(SORT BMX_SDL3_GUESS)
  list(POP_BACK BMX_SDL3_GUESS BMX_SDL3_GUESS_LAST)
  set(BMX_SDL3_DIR "${BMX_SDL3_GUESS_LAST}" CACHE PATH "SDL3 i686-w64-mingw32 prefix (bin/SDL3.dll, lib/cmake/SDL3)")
  find_package(SDL3 REQUIRED CONFIG PATHS "${BMX_SDL3_DIR}" NO_DEFAULT_PATH)
  # The SDL3 render driver (src/driver): native exe only, the hook DLL keeps running the original D3D7 driver.
  # Compiled like the game objects (BMX_HOOK: RW globals at their original addresses) but hosted.
  file(GLOB BMX_DRIVER_SOURCES CONFIGURE_DEPENDS src/driver/*.c)
  add_library(bmx_driver OBJECT ${BMX_DRIVER_SOURCES})
  target_include_directories(bmx_driver PRIVATE src)
  target_compile_definitions(bmx_driver PRIVATE BMX_HOOK=1)
  target_compile_options(bmx_driver PRIVATE ${BMX_WARNINGS} -Wno-sign-compare)
  target_link_libraries(bmx_driver PRIVATE SDL3::SDL3)
  # Our PNG reader in place of libpng 1.0.5 (src/png): native exe only, the hook DLL keeps the original.
  # Freestanding like the game objects (all memory and input through libpng's callbacks).
  file(GLOB BMX_PNG_SOURCES CONFIGURE_DEPENDS src/png/*.c)
  add_library(bmx_png OBJECT ${BMX_PNG_SOURCES})
  target_include_directories(bmx_png PRIVATE src)
  target_compile_definitions(bmx_png PRIVATE BMX_HOOK=1)
  target_compile_options(bmx_png PRIVATE ${BMX_WARNINGS} -ffreestanding -fno-builtin -fno-stack-protector)
  add_executable(bmxnative $<TARGET_OBJECTS:bmx_range> ${BMX_CRT_STARTFILES} $<TARGET_OBJECTS:bmx_game>
    $<TARGET_OBJECTS:bmx_driver> $<TARGET_OBJECTS:bmx_png> ${BMX_NATIVE_SOURCES} src/native/crtasm.S ${CMAKE_BINARY_DIR}/hooks_table.c
    third_party/dr_flac/dr_flac.c ${CMAKE_BINARY_DIR}/music_edits.c)
  target_link_options(bmxnative PRIVATE -nostartfiles)
  target_include_directories(bmxnative PRIVATE src)
  target_include_directories(bmxnative SYSTEM PRIVATE third_party/dr_flac)
  target_compile_definitions(bmxnative PRIVATE _WIN32_WINNT=0x0A00 BMX_SOURCE_DIR="${CMAKE_SOURCE_DIR}" BMX_WORK_DIR="${BMX_WORK_DIR}")
  target_compile_options(bmxnative PRIVATE ${BMX_WARNINGS})
  # Fixed base, no ASLR. Headers sit at 0x3f0000 and the first section (.orig, src/native/range.c) at
  # 0x400000, so the exe itself owns the range the rebuilt original image needs. Everything else follows
  # above 0x6b3000.
  target_link_options(bmxnative PRIVATE -Wl,--image-base=0x3f0000 -Wl,--section-alignment=0x10000
    -Wl,--disable-dynamicbase -Wl,--disable-high-entropy-va)
  target_link_libraries(bmxnative PRIVATE bcrypt SDL3::SDL3)
  # Checks of the platform stand-ins against Windows (tests/platform): hosted, no game image.
  add_executable(bmxplatformtest $<TARGET_OBJECTS:bmx_range> ${BMX_CRT_STARTFILES} tests/platform/test_platform.c src/platform/kernel.c src/platform/winmm.c
    src/platform/fileio.c src/platform/paths.c src/platform/input.c src/platform/inputcheck.c
    src/platform/config.c src/platform/dsound.c src/platform/soundcheck.c src/platform/smoothmix.c src/platform/waveout.c src/platform/window.c
    src/bmx/pc/zge/sound/winchan.c tests/platform/test_music.c ${BMX_MUSIC_SOURCES})
  target_include_directories(bmxplatformtest SYSTEM PRIVATE third_party/dr_flac)
  # The music override offline: renders the game's tracks from the user's FLACs and compares them with the
  # game's own (docs/MUSIC.md "Override"; needs the game folder and the FLACs).
  add_executable(bmxmusictool tests/platform/musictool.c src/platform/paths.c ${BMX_MUSIC_SOURCES})
  target_include_directories(bmxmusictool PRIVATE src)
  target_include_directories(bmxmusictool SYSTEM PRIVATE third_party/dr_flac)
  target_compile_options(bmxmusictool PRIVATE ${BMX_WARNINGS})
  target_link_libraries(bmxmusictool PRIVATE SDL3::SDL3)
  # The real winchanMix (compared with the smooth mixer) keeps its state at the original addresses: the test
  # owns that range like bmxnative does (range.S first, then the crt start files); the test unprotects it.
  set_source_files_properties(src/bmx/pc/zge/sound/winchan.c PROPERTIES COMPILE_DEFINITIONS BMX_HOOK=1)
  target_link_options(bmxplatformtest PRIVATE -nostartfiles -Wl,--image-base=0x3f0000
    -Wl,--section-alignment=0x10000 -Wl,--disable-dynamicbase -Wl,--disable-high-entropy-va)
  target_include_directories(bmxplatformtest PRIVATE src)
  target_compile_options(bmxplatformtest PRIVATE ${BMX_WARNINGS})
  target_link_libraries(bmxplatformtest PRIVATE SDL3::SDL3 dinput dxguid ole32) # c_dfDIKeyboard / c_dfDIJoystick
  # Our AVI reader / Cinepak decoder against avifil32 / iccvid on the game's movies (needs the files).
  add_executable(bmxmovietest tests/platform/test_movie.c src/platform/avi.c src/platform/cinepak.c
    src/platform/paths.c)
  target_include_directories(bmxmovietest PRIVATE src)
  target_compile_options(bmxmovietest PRIVATE ${BMX_WARNINGS})
  target_link_libraries(bmxmovietest PRIVATE SDL3::SDL3 vfw32 msvfw32 gdi32)
  # The widescreen maths (docs/WIDESCREEN.md W10.3): view window, 2D classifier and transforms, present rect.
  add_executable(bmxwidetest tests/platform/test_wide.c src/driver/widemath.c)
  target_include_directories(bmxwidetest PRIVATE src)
  target_compile_options(bmxwidetest PRIVATE ${BMX_WARNINGS})
  # SDL3.dll next to the exe (copy it into the game folder with bmxnative.exe).
  add_custom_command(TARGET bmxnative POST_BUILD COMMAND ${CMAKE_COMMAND} -E copy_if_different
    $<TARGET_FILE:SDL3::SDL3> $<TARGET_FILE_DIR:bmxnative>)
endif()

if(BMX_HOOK)

  # Differential-test DLL: ported code + test cases, loaded next to NgBMX.exe in the Unicorn harness.
  file(GLOB_RECURSE BMX_TEST_SOURCES CONFIGURE_DEPENDS tests/cases/*.c)
  # src/hook/d3dcom.c: the D3D7 driver cut forwarded to the original device (the native exe has src/driver).
  add_library(bmxtest SHARED ${BMX_TEST_SOURCES} src/hook/d3dcom.c ${CMAKE_BINARY_DIR}/hooks_table.c
    $<TARGET_OBJECTS:bmx_game>)
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
  add_library(bmxhook SHARED src/hook/hook.c src/hook/trace.c src/hook/crash.c src/hook/d3dcom.c
    ${CMAKE_BINARY_DIR}/hooks_table.c $<TARGET_OBJECTS:bmx_game>)
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
