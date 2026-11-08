cmake_minimum_required(VERSION 3.28)
project(openfreestylebmx C)

set(CMAKE_C_STANDARD 17)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

option(BMX_HOOK "Build against the original NgBMX.exe (i686 hook/oracle phase)" ON)

option(BMX_NATIVE32 "Build OpenFreestyleBMX.exe (target bmxnative), our own exe (32- or 64-bit, by the toolchain: presets native32 / native64)" OFF)
set(BMX_WORK_DIR "${CMAKE_SOURCE_DIR}/work" CACHE PATH "Directory with labels.raw (names in trap reports)")

set(BMX_WARNINGS -Wall -Wextra -Werror -Wno-unused-parameter -Wno-microsoft-anon-tag)

# macOS / Linux (docs/PORTING.md "X5 part 1", "X4 (Linux x64)"): the host compiler with the Windows toolchains'
# language flags (cmake/*.cmake). arm64 compilers fuse a * b + c into one rounding unless told not to
# (docs/PORTING.md X4); x86 has no FMA without -mfma, so the Windows builds need no flag, and on Linux x86_64 it
# only guards against an -march in CFLAGS that brings FMA in.
if(NOT WIN32)
  add_compile_options(-fms-extensions -fno-math-errno -ffp-contract=off)
endif()
# Android: bmxnative is a shared library (libmain.so); its symbols stay hidden (see the bmxnative target below).
if(ANDROID)
  set(CMAKE_C_VISIBILITY_PRESET hidden)
endif()

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
# The portable x87 maths (compat/x87soft.c) in place of the x87 instructions on x86 too: the end-to-end check
# of X5 part 2 (docs/PORTING.md), a replay's BMX_SIM_HASH against the default build's.
option(BMX_X87_SOFT "Use the portable x87 emulation on x86 as well" OFF)
if(BMX_X87_SOFT)
  target_compile_definitions(bmx_game PUBLIC BMX_X87_SOFT=1)
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
  # -Wno-sign-compare: enum constants are unsigned under the mingw target's ABI, int under msvc's.
  # -Wno-cast-function-type-mismatch: natively (BMX_IMAGE) an ORIG_FN name is a function, not a pointer constant,
  # and the callback casts the original's code made between function types become casts of functions.
  target_compile_options(bmx_game PRIVATE -ffreestanding -fno-builtin -fno-stack-protector
    -Wno-sign-compare -Wno-cast-function-type-mismatch)
  enable_language(ASM)
  file(GLOB BMX_NATIVE_SOURCES CONFIGURE_DEPENDS src/native/*.c src/platform/*.c)
  # Native files that read the game's globals (ORIG_VAR) are compiled like the game objects: BMX_HOOK for the
  # target conventions, plus the target-wide BMX_IMAGE (the relocated data copy).
  # The smooth resampling mixer reads the game's mixer state (like src/driver).
  set_source_files_properties(src/platform/smoothmix.c PROPERTIES COMPILE_DEFINITIONS BMX_HOOK=1)
  # The music override (docs/MUSIC.md "Override"): musicgame.c reads the game's music stream like smoothmix.c;
  # the edit list is compiled in from data/music-edits.json; dr_flac (third_party) is built without warnings.
  set_source_files_properties(src/platform/musicgame.c PROPERTIES COMPILE_DEFINITIONS BMX_HOOK=1)
  # BMX_SIM_HASH (docs/WIDESCREEN.md W10.5) reads the rider and camera state too.
  set_source_files_properties(src/native/simhash.c PROPERTIES COMPILE_DEFINITIONS BMX_HOOK=1)
  # BMX_HF_GUARD (docs/HIGHFPS.md HF0) hashes the game's render and simulation state the same way.
  set_source_files_properties(src/native/hfguard.c PROPERTIES COMPILE_DEFINITIONS BMX_HOOK=1)
  # The interpolated draws (docs/HIGHFPS.md HF1-HF3) capture and rebuild the game's poses the same way.
  set_source_files_properties(src/native/hfinterp.c PROPERTIES COMPILE_DEFINITIONS BMX_HOOK=1)
  # [video] full_draw_distance (drawdist.c) sets the game's far clip quality.
  set_source_files_properties(src/native/drawdist.c PROPERTIES COMPILE_DEFINITIONS BMX_HOOK=1)
  set_source_files_properties(third_party/dr_flac/dr_flac.c PROPERTIES COMPILE_OPTIONS -w)
  add_custom_command(OUTPUT ${CMAKE_BINARY_DIR}/music_edits.c
    COMMAND Python3::Interpreter ${CMAKE_SOURCE_DIR}/tools/gen_music_edits.py
            ${CMAKE_SOURCE_DIR}/data/music-edits.json ${CMAKE_BINARY_DIR}/music_edits.c
    DEPENDS ${CMAKE_SOURCE_DIR}/data/music-edits.json ${CMAKE_SOURCE_DIR}/tools/gen_music_edits.py)
  set(BMX_MUSIC_SOURCES src/platform/music.c third_party/dr_flac/dr_flac.c ${CMAKE_BINARY_DIR}/music_edits.c)
  # BMX_IMAGE (src/compat/orig.h, docs/NATIVE.md "Relocated data image"): the original's data is a relocated copy,
  # its code is gone. tools/gen_origmap.py maps every original code address to the function serving it, from the
  # ports, the C runtime layer, the driver and the PNG reader (origmap.h for ORIG_FN, origmap.c for the loader).
  file(GLOB_RECURSE BMX_ORIGMAP_INPUTS CONFIGURE_DEPENDS src/*.c src/*.h)
  add_custom_command(OUTPUT ${CMAKE_BINARY_DIR}/gen/origmap.h ${CMAKE_BINARY_DIR}/gen/origmap.c
    COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_BINARY_DIR}/gen
    COMMAND Python3::Interpreter ${CMAKE_SOURCE_DIR}/tools/gen_origmap.py ${CMAKE_BINARY_DIR}/gen ${CMAKE_SOURCE_DIR}/src
    DEPENDS ${BMX_ORIGMAP_INPUTS} ${CMAKE_SOURCE_DIR}/tools/gen_origmap.py ${CMAKE_SOURCE_DIR}/tools/gen_hooks.py)
  add_custom_target(bmx_origmap DEPENDS ${CMAKE_BINARY_DIR}/gen/origmap.h ${CMAKE_BINARY_DIR}/gen/origmap.c)
  target_compile_definitions(bmx_game PUBLIC BMX_IMAGE=1)
  target_include_directories(bmx_game PUBLIC ${CMAKE_BINARY_DIR}/gen)
  add_dependencies(bmx_game bmx_origmap)
  # The original's data as C (src/data, docs/NATIVE.md "The original's data in the source"; made once by
  # tools/gen_data.py): one object per original data section, compiled like the game objects (its types).
  file(GLOB BMX_DATA_SOURCES CONFIGURE_DEPENDS src/data/*.c)
  add_library(bmx_data OBJECT ${BMX_DATA_SOURCES})
  target_link_libraries(bmx_data PRIVATE bmx_game)
  # One section per definition: tools/gen_data.py --check reads each definition's bytes from its section.
  target_compile_options(bmx_data PRIVATE ${BMX_WARNINGS} -ffreestanding -fno-builtin -fno-stack-protector
    -Wno-sign-compare -Wno-cast-function-type-mismatch -fdata-sections -ffunction-sections)
  add_dependencies(bmx_data bmx_origmap)
  # bmxplatformtest keeps the fixed-address mode for winchan.c (the real mixer against the smooth one): range.S
  # must be the first object on its link line, its block has to start the image.
  if(WIN32) # (not re-indented: the Windows-only part of the native build, up to the matching else)
  add_library(bmx_range OBJECT src/native/range.S)
  # The driver would put the C runtime start files (crt2.o, crtbegin.o) before it: pass them ourselves, after.
  foreach(crt crt2.o crtbegin.o)
    execute_process(COMMAND ${CMAKE_C_COMPILER} -print-file-name=${crt} OUTPUT_VARIABLE crtPath
      OUTPUT_STRIP_TRAILING_WHITESPACE)
    list(APPEND BMX_CRT_STARTFILES ${crtPath})
  endforeach()
  # SDL3 for mingw (the SDL3-<version>/<triple> folder of the SDL release's mingw devel package).
  if(CMAKE_SIZEOF_VOID_P EQUAL 8)
    set(BMX_MINGW_TRIPLE x86_64-w64-mingw32)
  else()
    set(BMX_MINGW_TRIPLE i686-w64-mingw32)
  endif()
  file(GLOB BMX_SDL3_GUESS "$ENV{HOME}/tools/sdl3/SDL3-*/${BMX_MINGW_TRIPLE}"
    "$ENV{USERPROFILE}/tools/sdl3/SDL3-*/${BMX_MINGW_TRIPLE}")
  list(SORT BMX_SDL3_GUESS)
  list(POP_BACK BMX_SDL3_GUESS BMX_SDL3_GUESS_LAST)
  set(BMX_SDL3_DIR "${BMX_SDL3_GUESS_LAST}" CACHE PATH "SDL3 ${BMX_MINGW_TRIPLE} prefix (bin/SDL3.dll, lib/cmake/SDL3)")
  find_package(SDL3 REQUIRED CONFIG PATHS "${BMX_SDL3_DIR}" NO_DEFAULT_PATH)
  else()
    # SDL3 from the system or nix (`nix develop` sets SDL3_ROOT), or -DSDL3_DIR=<prefix>/lib/cmake/SDL3.
    find_package(SDL3 REQUIRED CONFIG)
  endif()
  # The SDL3 render driver (src/driver): native exe only, the hook DLL keeps running the original D3D7 driver.
  # Compiled like the game objects (BMX_HOOK, BMX_IMAGE: RW globals in the relocated data copy) but hosted.
  file(GLOB BMX_DRIVER_SOURCES CONFIGURE_DEPENDS src/driver/*.c)
  add_library(bmx_driver OBJECT ${BMX_DRIVER_SOURCES})
  target_include_directories(bmx_driver PRIVATE src)
  target_compile_definitions(bmx_driver PRIVATE BMX_HOOK=1 BMX_IMAGE=1)
  target_include_directories(bmx_driver PRIVATE ${CMAKE_BINARY_DIR}/gen)
  add_dependencies(bmx_driver bmx_origmap)
  target_compile_options(bmx_driver PRIVATE ${BMX_WARNINGS} -Wno-sign-compare)
  target_link_libraries(bmx_driver PRIVATE SDL3::SDL3)
  # Our PNG reader in place of libpng 1.0.5 (src/png): native exe only, the hook DLL keeps the original.
  # Freestanding like the game objects (all memory and input through libpng's callbacks).
  file(GLOB BMX_PNG_SOURCES CONFIGURE_DEPENDS src/png/*.c)
  add_library(bmx_png OBJECT ${BMX_PNG_SOURCES})
  target_include_directories(bmx_png PRIVATE src)
  target_compile_definitions(bmx_png PRIVATE BMX_HOOK=1 BMX_IMAGE=1)
  target_include_directories(bmx_png PRIVATE ${CMAKE_BINARY_DIR}/gen)
  add_dependencies(bmx_png bmx_origmap)
  target_compile_options(bmx_png PRIVATE ${BMX_WARNINGS} -ffreestanding -fno-builtin -fno-stack-protector)
  # The MSVC compiler helpers the original's code called (i386 only; nothing calls them on x86_64).
  if(CMAKE_SIZEOF_VOID_P EQUAL 4)
    set(BMX_CRT_ASM src/native/crtasm.S)
  endif()
  set(BMX_NATIVE_ALL $<TARGET_OBJECTS:bmx_game> $<TARGET_OBJECTS:bmx_data> $<TARGET_OBJECTS:bmx_driver>
    $<TARGET_OBJECTS:bmx_png>
    ${BMX_NATIVE_SOURCES} ${BMX_CRT_ASM} ${CMAKE_BINARY_DIR}/gen/origmap.c
    third_party/dr_flac/dr_flac.c ${CMAKE_BINARY_DIR}/music_edits.c)
  if(ANDROID)
    # Android (docs/PORTING.md "X6 step 1 (Android arm64)"): libmain.so, whose SDL_main SDL's Java activity calls.
    add_library(bmxnative SHARED ${BMX_NATIVE_ALL})
  else()
    add_executable(bmxnative ${BMX_NATIVE_ALL})
    # The players' name for it (docs/RELEASE-PLAN.md D5, src/compat/product.h): OpenFreestyleBMX.exe.
    set_target_properties(bmxnative PROPERTIES OUTPUT_NAME OpenFreestyleBMX)
  endif()
  target_include_directories(bmxnative PRIVATE src ${CMAKE_BINARY_DIR}/gen)
  target_include_directories(bmxnative SYSTEM PRIVATE third_party/dr_flac)
  target_compile_definitions(bmxnative PRIVATE _WIN32_WINNT=0x0A00 BMX_IMAGE=1 BMX_SOURCE_DIR="${CMAKE_SOURCE_DIR}" BMX_WORK_DIR="${BMX_WORK_DIR}")
  target_compile_options(bmxnative PRIVATE ${BMX_WARNINGS})
  if(NOT WIN32)
    # macOS / Linux: no fixed base (the 28-bit packet links are offsets from an anchor on 64-bit, X2d); SDL3 found
    # through the rpath. The bmxnative_app target packages OpenFreestyleBMX.app (docs/PORTING.md "X5 part 1").
    if(NOT ANDROID)
      set_target_properties(bmxnative PROPERTIES BUILD_RPATH "$<TARGET_FILE_DIR:SDL3::SDL3-shared>")
    endif()
    target_link_libraries(bmxnative PRIVATE SDL3::SDL3 m)
    if(ANDROID)
      # libmain.so next to libSDL3.so in the APK (android/). Every object hidden but SDL_main (SDL_main.h exports
      # it), so the library's own symbols bind locally: the arm64 import thunks (data/data.h) reach bmxImports with
      # adrp, which a preemptible symbol would not allow. logcat through liblog (src/native/android.c).
      set_target_properties(bmxnative PROPERTIES OUTPUT_NAME main)
      target_link_libraries(bmxnative PRIVATE log)
      target_link_options(bmxnative PRIVATE -Wl,--no-undefined -Wl,--fatal-warnings)
    elseif(APPLE)
      # src/data keeps the original's layout, which has pointers at odd addresses (BmxUnalignedPtr: a CRT switch
      # table the port never reaches). Chained fixups need 8-byte aligned pointers; the classic rebase info does not.
      target_link_options(bmxnative PRIVATE -Wl,-no_fixup_chains)
      # OpenFreestyleBMX.app (not part of `all`): cmake --build --preset macos --target bmxnative_app
      add_custom_target(bmxnative_app
        COMMAND ${CMAKE_SOURCE_DIR}/tools/macos_bundle.sh $<TARGET_FILE:bmxnative>
                $<TARGET_FILE:SDL3::SDL3-shared> ${CMAKE_BINARY_DIR}/OpenFreestyleBMX.app
        DEPENDS bmxnative VERBATIM)
    else()
      # Linux (docs/PORTING.md "X4 (Linux x64)"). The tarball (not part of `all`):
      # cmake --build --preset linux --target bmxnative_tarball -> build/linux/OpenFreestyleBMX-linux-x86_64.tar.gz
      add_custom_target(bmxnative_tarball
        COMMAND ${CMAKE_SOURCE_DIR}/tools/linux_package.sh $<TARGET_FILE:bmxnative>
                $<TARGET_FILE:SDL3::SDL3-shared> ${CMAKE_BINARY_DIR}/OpenFreestyleBMX-linux-x86_64
        DEPENDS bmxnative VERBATIM)
    endif()
    add_executable(bmxwidetest tests/platform/test_wide.c src/driver/widemath.c)
    target_include_directories(bmxwidetest PRIVATE src)
    target_compile_options(bmxwidetest PRIVATE ${BMX_WARNINGS})
    add_executable(bmxhftest tests/platform/test_hf.c src/native/hfmath.c)
    target_include_directories(bmxhftest PRIVATE src)
    target_compile_options(bmxhftest PRIVATE ${BMX_WARNINGS})
    # libm is its own library on Linux (part of libSystem on macOS).
    target_link_libraries(bmxwidetest PRIVATE m)
    target_link_libraries(bmxhftest PRIVATE m)
    if(CMAKE_SYSTEM_PROCESSOR MATCHES "^(x86_64|AMD64|amd64)$" AND NOT APPLE)
      # Linux x86_64 has the real x87: the portable x87 against it (docs/PORTING.md "X5 part 2", "X4").
      find_package(Threads REQUIRED)
      add_executable(bmxx87test tests/platform/test_x87.c src/compat/x87soft.c)
      target_include_directories(bmxx87test PRIVATE src)
      target_compile_options(bmxx87test PRIVATE ${BMX_WARNINGS})
      target_link_libraries(bmxx87test PRIVATE Threads::Threads)
    endif()
  else()
  # Fixed base, no ASLR (runs stay alike), away from the original's 0x400000..0x6b3000: nothing of ours may sit at an
  # original address, where a leftover raw address would silently reach it. Below 0x10000000: the original's data
  # is part of this image since X1b (src/data), and the PSX-style ordering tables (ngifpsx.c, zoverii.c, the
  # front end) keep pointers to it in 28 bits, the top 4 being the packet's length.
  target_link_options(bmxnative PRIVATE -Wl,--image-base=0x01000000 -Wl,--disable-dynamicbase
    -Wl,--disable-high-entropy-va)
  target_link_libraries(bmxnative PRIVATE SDL3::SDL3)
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
  # The high frame rate maths (docs/HIGHFPS.md H9.5): limiter, draw scheduler, lerp / nlerp, skip rules.
  add_executable(bmxhftest tests/platform/test_hf.c src/native/hfmath.c)
  target_include_directories(bmxhftest PRIVATE src)
  target_compile_options(bmxhftest PRIVATE ${BMX_WARNINGS})
  # The portable x87 against the real instructions (docs/PORTING.md "X5 part 2 (portable x87)").
  find_package(Threads REQUIRED)
  add_executable(bmxx87test tests/platform/test_x87.c src/compat/x87soft.c)
  target_include_directories(bmxx87test PRIVATE src)
  target_compile_options(bmxx87test PRIVATE ${BMX_WARNINGS})
  target_link_libraries(bmxx87test PRIVATE Threads::Threads)
  # SDL3.dll next to the exe (copy it into the game folder with OpenFreestyleBMX.exe).
  add_custom_command(TARGET bmxnative POST_BUILD COMMAND ${CMAKE_COMMAND} -E copy_if_different
    $<TARGET_FILE:SDL3::SDL3> $<TARGET_FILE_DIR:bmxnative>)
  endif()
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
