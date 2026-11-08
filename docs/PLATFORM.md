# Platform layer: Windows APIs -> SDL3 (P4, "Windows first")

Status 2026-10-31: **decided; S1-S11 implemented** (sections 4a-4f; S5's check with real pads, S7's
listening test, S9's watch test and the focus test of S10 / S11 are the user's). The user took the recommendation for
every decision P1..P19 (A unless said otherwise). Order: S1-S4 (done, S3 reviewed), input + d-pad (S5, S6: done),
DirectSound (S7: done), movies (S8, S9: done), window and message queue (S10: done; no real DLL is left in the import
table), the native exe's own Win32 (S11: done, 4f; tools/check.sh checks the import table), then S12.
Goal: `bmxnative.exe` (src/native/) as a clean standalone
Windows build whose game and engine code reach the OS only through SDL3 and our own code. The fixed 32-bit memory
layout (image at 0x400000, docs/NATIVE.md) stays; Mac/Linux come after. Vanilla stays vanilla (PLAN.md Decisions):
with every option off, the game sees the same values from the platform as it did from Windows.

Model: docs/SDL3-DRIVER.md (the render side, done to step 3). That cut is at RW's device tables plus five D3D7 call
functions (rw/rwd3d.h); this one is at the **exe's import table**.

## 1. Inventory: what the import table still points at

The original imports 137 functions from 8 DLLs. `fillImports` (src/native/loader.c) fills every slot with the real
function via `LoadLibraryA`/`GetProcAddress`, except two shims: `AVISaveOptionsFree` (loader.c, heap mismatch) and,
in a `BMX_KEYS` run, `DirectInputCreateEx` / `MessageBoxA` (src/native/keys.c). (Inventory as of before S1;
the import table is now src/platform/imports.c, section 4a.) The loader also recreates the exe's
own `jmp [slot]` thunks (e.g. 0x4afc58 DirectInputCreateEx, 0x4af9d0..0x4afa48 AVIFile/DrawDib, 0x47ec3a timeGetTime).

Callers were found by scanning the original's code for `call/jmp/mov [slot]` and mapping each site to
data/functions*.csv (scratch script, not committed), then checking `src/` for the `ORIG_VAR(slot)` / `ORIG_FN(thunk)`
that the ports use. **Ported** = our C calls the slot today (in the hook DLL and the native exe alike). **Dead** =
only original code that is int3 in the native exe calls it (the MSVC CRT, the D3D7 driver, WinMain 0x48a460 whose job
src/native/main.c does with direct Win32 calls), so the slot is filled but never called natively.

### 1a. Reached natively: 73 slots, all from ported code

| Subsystem | DLL!function (slot) | Ported callers (address, src) |
|---|---|---|
| Input | DINPUT!DirectInputCreateEx (0x699588, thunk 0x4afc58) | zpadDirectInputInitialize 0x493160 (zge/pc/pcinput.c); then COM only: IDirectInput7 EnumDevices/CreateDeviceEx, IDirectInputDevice7 SetDataFormat/SetCooperativeLevel/SetProperty/Acquire/Unacquire/Poll/GetDeviceState/GetDeviceInfo/Release (0x493310, 0x493760, 0x4937f0..0x493910) |
| | USER32!GetWindowLongA (0x699794) | 0x493160 (GWL_HINSTANCE, only for DirectInputCreateEx) |
| Audio (DirectSound) | KERNEL32!LoadLibraryA / GetProcAddress / FreeLibrary (0x6995c4 / 0x699660 / 0x6995bc) | winchanInit 0x46d940, winchanProbeDevice 0x46f380, winchanShutdown 0x46df20 (bmx/pc/zge/sound/winchan.c): `LoadLibraryA("DSOUND.DLL")` + `DirectSoundCreate`; then COM only (IDirectSound, IDirectSoundBuffer: the surface is documented in sound/winds.h) |
| | KERNEL32!Initialize/Enter/Leave/DeleteCriticalSection (0x6995d8/cc/d0/d4) | winstrm.c 0x470aa0, 0x470b20, 0x470d10, 0x470fb0, 0x4712f0, 0x471450, 0x471610, 0x4716b0, 0x471dc0 |
| | KERNEL32!CreateFileA / ReadFile / SetFilePointer / CloseHandle (0x6995dc/b8/c8/c0) | winloadLoad 0x46fb20 (winload.c: the .ZSD banks), winstrm.c 0x472030, 0x472200, 0x472250, 0x4723b0 (streamed music/speech) |
| Timer (sound pump) | WINMM!timeGetDevCaps / timeBeginPeriod / timeSetEvent / timeEndPeriod / timeKillEvent (0x699810/14/24/40/34) | bmxSoundOpen 0x44dda0, bmxSoundClose 0x44e090 (bmx/com/zge/bmx/bmxsound.c): a 16 ms periodic callback 0x450020 (zoverii.c "zoverPollTimer") that calls 0x46e010 **winchanPump** (the label "zoverPollInput" in zoverii.h is a misnomer) |
| Time | WINMM!timeGetTime (0x699838, thunk 0x47ec3a) | zrwTimeGetTime 0x489cb0 (also behind the game pointer 0x561480), 0x450020, ztimerUpdate 0x4a4280, ngiffx.c particles, ngapp.c, zrwPlayMovie 0x48bed0; main.c's frame limiter calls the real one directly |
| | KERNEL32!GetTickCount (0x6995fc) | zmoviePlay 0x4a09d0, zmovieUpdate 0x4a0a90 (movie clock) |
| | KERNEL32!QueryPerformanceFrequency / Counter (0x699600/04) | pcProfileInitialize 0x4a45e0, pcProfileElapsed 0x4a46a0 (profiler only) |
| | KERNEL32!Sleep (0x6995f8) | zrwPlayMovie 0x48bed0 |
| Movies (VfW) | AVIFIL32: AVIFileInit/Exit/OpenA/Release/GetStream, AVIStreamInfoA/ReadFormat/Read/Start/Length/TimeToSample/SampleToTime/GetFrameOpen/GetFrame/GetFrameClose/Release/AddRef, AVISaveOptionsFree (18 slots 0x6994d4..0x699518, thunks 0x4af9d0..0x4afa48) | zge/zmovie.c 0x49fb30..0x4a1050 (AVIStreamAddRef is never called) |
| | MSVFW32!DrawDibOpen / DrawDibDraw / DrawDibClose (0x699724/20/28) | zmovieScanStreams 0x4a0400, zmovieDraw 0x4a0db0, zmovieClose 0x4a0780 |
| | USER32!GetDC / ReleaseDC / GetClientRect (0x6997a0/9c/98) | zmovieUpdate 0x4a0a90, zmoviePlay 0x4a09d0 |
| Movie audio | WINMM!waveOutOpen/Prepare/Write/Pause/Restart/Reset/Unprepare/Close, sndPlaySoundA (0x69980c/3c/08/20/1c/28/2c/30/18) | zmovie.c 0x49fbe0, 0x49fcf0, 0x4a0040, 0x4a0270, 0x4a0780, 0x4a0cb0. waveOutOpen uses CALLBACK_WINDOW: MM_WOM_DONE (0x3bd) arrives in zrwPlayMovie's PeekMessage loop, which calls zmovieRefillAudio 0x4a0a70 |
| Window, messages | USER32!PeekMessageA / TranslateMessage / DispatchMessageA (0x6997b4/68/58) | zrwPlayMovie 0x48bed0 (movie loop; main.c's own loop calls the real ones directly) |
| | USER32!DefWindowProcA / AdjustWindowRect / ClipCursor / PostQuitMessage (0x69978c/80/90/74) | zrwMainWndProc 0x48a800 (WM_ACTIVATE -> zrwOnActivate/Deactivate, WM_DESTROY, WM_GETMINMAXINFO locks the size to the mode, WM_KEYDOWN Esc = quit until 0x14 locks keys); PostQuitMessage also zrwEpicFail 0x489cf0 |
| | USER32!ShowCursor (0x6997a8) | zrwSelectDevice 0x489d70, zrwSelectDefaultDevice 0x489ef0 (hide in full screen) |
| | USER32!SetWindowTextA (0x6997c0) | ngifInitialize 0x45b270 (ngapp.c; title "Dave Mirra Freestyle BMX") |
| Device dialog | USER32!DialogBoxParamA / EndDialog / GetDlgItem / SendMessageA / SetFocus (0x699778/5c/60/64/7c) | zrwSelectDevice 0x489d70, zrwDialogProc 0x48a330, zrwDialogInit 0x489f70, zrwDialogSelectDevice 0x48a1a0 (dialog resource 0x68) |
| Message boxes | USER32!MessageBoxA (0x6997bc) | zrwApplicationError 0x489cc0, zrwEpicFail 0x489cf0 (assert box), zrwConfirmQuit 0x48a9b0 |
| Debug | KERNEL32!OutputDebugStringA / DebugBreak (0x6995f0/f4) | zrwDebugOutput 0x489ce0, zrwEpicFail 0x489cf0 |

Indirect Windows dependencies of ported code (no slot of their own): the DirectInput, DirectSound COM objects above;
the HWND in gZrwWindow 0x697780 / gZpadWindow 0x674034, handed to DirectInput, DirectSound, waveOut, GetDC and
SetWindowTextA; DirectDraw through gDirectDraw 0x69730c in zrwConfirmQuit and zmovieUpdate (GDI surface flip), which
the SDL3 driver never sets, so both paths are skipped natively.

### 1b. Dead natively: 64 slots

- **KERNEL32, 49** (MSVC CRT only, replaced by src/native/crt.c): heap (HeapAlloc/Free/ReAlloc/Create/Destroy,
  VirtualAlloc/Free, IsBadWritePtr), startup/exit (GetCommandLineA, GetVersion(Ex)A, GetStartupInfoA,
  GetModuleHandleA, GetModuleFileNameA, ExitProcess, TerminateProcess, GetCurrentProcess, SetHandleCount, GetStdHandle,
  GetFileType, SetStdHandle, environment x5, SetConsoleCtrlHandler, UnhandledExceptionFilter, RtlUnwind,
  RaiseException), low-level I/O (WriteFile, FlushFileBuffers, SetEndOfFile, GetLastError), locale (LCMapStringA/W,
  GetStringTypeA/W, CompareStringA/W, MultiByteToWideChar, WideCharToMultiByte, GetCPInfo, GetACP, GetOEMCP), time
  (GetLocalTime, GetSystemTime, GetTimeZoneInformation: CRT `time`/`ctime` 0x51df10, never reached). The CRT also
  shares ReadFile, CloseHandle, CreateFileA, SetFilePointer, LoadLibraryA, GetProcAddress with the ported code above.
- **USER32, 9** (WinMain 0x48a460 only): RegisterClassA, LoadCursorA, CreateWindowExA, ShowWindow, UpdateWindow,
  SetWindowPos, SetWindowLongA, DestroyWindow; ClientToScreen (D3D7 driver's RasterShowRaster 0x4d4ae0).
- **ADVAPI32, 4**: RegOpenKeyExA/QueryValueExA/CloseKey in 0x48aa10 (the CD check: HKLM
  `Software\Acclaim Entertainment\Dave Mirra Freestyle BMX\1.00.000\` SRCDISK, then "Please Insert Game CD" until
  `<SRCDISK>\bin\movie\acclaim.avi` opens). **Nothing calls 0x48aa10** (no call, jump or pointer in the exe; not in
  functions.csv): the shipped game never checks the CD. RegCreateKeyExA and the rest are the D3D7 driver's own
  settings (0x4d3d80, 0x4db630). **The game uses no registry.**
- **DDRAW, 2**: DirectDrawCreate/EnumerateA, D3D7 driver only.

Settings and progress (options, key map choice 0x6952eb, unlocks) live in `.\save\SAVE.DAT` (memcard.c 0x412130..
0x412340 through zfile -> RW's file system -> the CRT's fopen, i.e. our crt.c). There is no INI file. Every path is
relative to the working directory (the install folder): `bin\audio\*.ZSD`, `bin\movie\*.AVI`, `Scene.Txt`, `assets\`.

### 1c. Windows the native exe itself still uses (our code, not the game's)

loader.c (VirtualProtect, BCryptHash, LoadLibraryA/GetProcAddress for the imports), trap.c (vectored exception
handler), main.c (window class, CreateWindowExA, message loop, WASAPI mute for `BMX_MUTE`), keys.c (dinput.h
types), driver/gpu.c (wraps the game's HWND for SDL), range.S (the .orig section). (Since S10 the window, its
messages and gpu.c's wrapping are SDL's; since S11 main.c, keys.c and gpu.c have no Win32 left, 4f.) The loader/trap/range parts are
the fixed-layout machinery and stay Windows-specific until the 64-bit step; the rest is in scope here.

## 2. The cut: stand-ins at the import slots (P1)

### P1. Where SDL3 attaches

- **A (recommended):** at the import table. The loader fills each reached slot (1a) with our function of the same
  Win32 signature (stdcall), and the COM factories (DirectInputCreateEx, DirectSoundCreate via the
  LoadLibraryA/GetProcAddress shims) hand out stand-in COM objects with the vtable layouts in pcinput.h / winds.h.
  All ported code (pcinput.c, winchan.c, winstrm.c, winload.c, zmovie.c, zrwzgl.c) stays as decompiled and
  oracle-tested; the stand-ins are the only new code, like the SDL3 driver. keys.c and the AVISaveOptionsFree shim
  already work this way.
- B: replace the ported platform functions (zpadPollDevices, winchanPump, zmovieDraw...) by SDL-native versions.
  Fewer Win32 semantics to emulate, but it cuts through faithful code and loses the A/B test against the real DLLs.

### P2. Where the stand-ins live and who can use them

- **A (recommended):** `src/platform/` (hosted C, i686 for now), one file per subsystem, a table
  `{dll, name, fn}` that loader.c consults before `GetProcAddress` (replacing kImportShims and bmxKeysImportShim).
  Slots in 1b get a named trap stub instead of the real function, so no unused DLL is loaded. The same stand-ins can
  be put into the **hook build's** import slots behind an env var (the hook DLL is already the game's `dinput.dll`
  proxy, src/hook/hook.c): the original exe then runs on SDL input/audio/movies, which gives side-by-side checks
  against the real DLLs in one process.
- B: native-only, no hook use. Simpler, but every check needs two processes and recorded logs.

End state: `bmxnative.exe` imports SDL3.dll, the UCRT and KERNEL32 (loader/CRT) only; a check in tools/check.sh
fails if dinput/dsound/winmm/avifil32/msvfw32/ddraw/advapi32/user32/gdi32 appear (llvm-objdump -p).

## 3. Subsystems

### 3.1 Input (DirectInput -> SDL3)

**Current path.** zpadDirectInputInitialize 0x493160 creates IDirectInput7 and enumerates the first attached
joystick (DIDEVTYPE_JOYSTICK, DIEDFL_ATTACHEDONLY), once. contrlerCreateDevices 0x408030 creates two devices of kind
2 (keyboard + pad); zpadSetupDirectInput 0x493310 creates the keyboard (c_dfDIKeyboard, cooperative flags 6:
foreground, non-exclusive) and the pad (c_dfDIJoystick, flags 5: foreground, exclusive), sets the X/Y range -127..127
(DIPH_BYOFFSET) and a device-wide dead zone of 1000 (10%) (the second dead zone call passes DIPROP_RANGE by mistake
and fails), reads the product name/GUID and picks a button map from the profiles at 0x561f2c ("Gravis GrIP"/"Gravis
GamePad Pro"; "Microsoft SideWinder game pad"/"(Auto Detect)" plus one GUID; else the default map 0x561ea8).
zpadPollDevices 0x493910 per poll: joystick GetDeviceState(DIJOYSTATE): **only lX/lY and rgbButtons are read**
(lX/lY > 0 into inputs 4..7 scaled by 1/127; buttons through the map into inputs 8..21); the keyboard (read once per
poll) ORs 1.0 into inputs through the key map (0x561e68 player 1; ngifRunFrame 0x45b8a0 replaces it at each level
load with one of four presets 0x55bb98, chosen in Options, 0x6952eb). Then each input > 0 sets a bit (kPadInputBits): 4 left 0x4,
5 right 0x8, 6 up 0x1, 7 down 0x2, 8..11 face 0x10..0x80, 14/15/16/17 shoulders 0x100/0x200/0x400/0x800, 12 0x10000,
13 0x20000, 20 0x4000, 21 0x8000. **The game sees only bits**: contrlerPoll 0x4080f0 copies the device's analogue
floats (inputs 0..3) which the PC code never writes, so they are always 0. The POV hat (d-pad, DIJOYSTATE+0x20), the
other axes and triggers on an axis are ignored. No hot-plug: the pad is chosen at start; it is always flagged
connected; a lost pad just reads zero.

Default pad map (DI button index -> input): 3->8, 0->9, 2->10, 1->11, 4->14, 6->16, 5->15, 7->17, 8->21, 9->20. With
the usual DirectInput button order this means (unmeasured, to verify in slice S5): **DualSense/DS4** ("Wireless
Controller", DI order Square, Cross, Circle, Triangle, L1, R1, L2, R2, Create, Options, L3, R3, PS, pad): Triangle 8,
Square 9, Circle 10, Cross 11, L1 14, R1 15, L2 16, R2 17, Create 21, Options 20. **Xbox via DirectInput** (A, B,
X, Y, LB, RB, Back, Start, LS, RS; triggers share the Z axis): Y 8, A 9, X 10, B 11, LB 14, RB 15, Back 16, Start
17, LS 21, RS 20. Inputs 12/13 (Esc/Enter bits) come only from the keyboard. The playtest note (DualSense steers
with the left stick, d-pad dead) is exactly this: the d-pad is the POV, which nothing reads.

**Cut.** The DirectInputCreateEx slot 0x699588 (thunk 0x4afc58): a stand-in IDirectInput7 /
IDirectInputDevice7A, as keys.c does today. pcinput.c is untouched; the stand-in fills DIJOYSTATE and the 256-byte
DIK state from SDL. `BMX_KEYS` becomes a source of that same stand-in (scripted keyboard state) instead of a separate
DirectInput.

**P3. Gamepad buttons: which DI index each SDL button reports**
- **A (recommended):** open the pad as SDL_Gamepad and report buttons in the order DirectInput would for that
  device family: tables for XInput-class (Xbox) and PlayStation (DS4/DualSense) pads, SDL_Joystick raw button order
  for everything else (generic HID pads, where SDL's order follows the HID report like DI). Same buttons as the
  original for the pads people use; the profile lookup works because getDeviceInfo reports SDL's product name and a
  DI-style guidProduct built from VID/PID (`PIDVID` in Data1, "PIDVID" in Data4), so the SideWinder GUID still matches.
- B: SDL_Joystick raw indices for all devices. Matches DI for plain HID, but SDL's HIDAPI/XInput backends number
  Xbox and PlayStation pads differently from DirectInput.
- C: map by position (south button = jump etc.). Nicer, but not what the original did: an option at most (P8).

**P4. Stick -> lX/lY**
- **A (recommended):** emulate DI's range/dead zone: SDL axis -32768..32767 -> -127..127 with a 10% dead zone and
  DI's linear rescale of the remainder, rounded like DI. The game only tests `> 0`, so what matters is the threshold
  at which a direction bit appears (about 10% + one step); calibrate it against real DirectInput with the dual-read
  check (S5).
- B: plain threshold at 10%. Same bits within the noise of a real stick, but not measured.

**P5. Which pad, hot-plug**
- **A (recommended):** vanilla: the first pad present at start, no hot-plug; "Compatibility" option (PLAN backlog)
  for hot-plug / pick a pad later.
- B: always hot-plug (behaviour change: a pad connected later starts working).

**P6. Keyboard**
- **A (recommended):** DIK code = PC set-1 scancode (+0x80 for extended keys) from SDL_Scancode via a fixed table;
  both are physical positions, so layouts (AZERTY, QWERTZ) behave as under DI. State from SDL_GetKeyboardState,
  zeroed while the window is not focused (DI foreground: GetDeviceState fails, the game reads nothing). Acquire /
  Unacquire on WM_ACTIVATE (zrwOnActivate 0x493830) stay no-ops returning OK.
- B: SDL keycodes (layout-dependent). Changes which key is "E" on AZERTY. No.

**P7. D-pad steering (opt-in, the playtest note)**
- **A (recommended):** in the stand-in pad's GetDeviceState, when the option is on, a pressed d-pad direction sets
  lX/lY to -127/+127 on that axis (d-pad wins over the stick on that axis; stick still works). The game sees a full
  stick deflection, i.e. the same direction bits 0x1..0x8, in menus and riding alike. No change to pcinput.c or the
  game's tables; default **on** since 2026-10-31 (PLAN.md Decisions, exception). It changes nothing the simulation does with its inputs (replays record the
  controller state after the mapping, BMX_TRACE_POLL in contrlerPoll), so it is tagged input-only and not recorded.
- **A2 (`[input] stick_8way = 1`, default **on** since 2026-10-31):** a thumbstick almost never gives a pure down / up / left /
  right (the game's 10% per-axis square dead zone: a pure "down" needs |lX| inside it), so tricks that need a pure
  direction (the backflip) fail. After the check has compared the vanilla state, the left stick (lX / lY) is
  replaced: inside a **radial** dead zone (radius 3277 = 10% of 32767, measured on the raw SDL vector, before the
  DirectInput cooking, which would already have clipped the drifting axis) lX = lY = 0; outside, the angle snaps to
  the nearest of 8 directions (45 degree sectors centred on the axes and diagonals, boundaries at 22.5 + k * 45) and
  lX / lY are -127 / +127 on the active axes, 0 on the other. Pad only, keyboard untouched; with `dpad_steering`
  on and a d-pad direction held, the snap is skipped and the d-pad acts as in A (it wins). Input-only, not
  recorded. Unit tests: sectors and boundaries, dead zone, no-option = vanilla, d-pad interplay.
- B: report the d-pad as four extra DI buttons and add {button, 4..7} pairs to the button map. Needs a modified copy
  of the game's map data; the profile lookup could replace it.
- C: d-pad as keyboard arrows. Mixes devices; the second player's map has no arrows.

**P8. Rebinding and other input options (seams only)**
- Vanilla already has action binding per player (PlayerSetup.bind, contrler.c padMask) and four keyboard presets
  (Options). **Recommended** extras, all opt-in at the stand-in: a physical remap table (SDL button -> DI index,
  SDL scancode -> DIK) loaded from the config file; position-based pad layout (P3-C); hot-plug (P5); rumble none
  (the game has no force feedback). Prompts (button glyphs) are menu graphics: later.

### 3.2 Audio (DirectSound + winmm timer -> SDL3)

**Current path.** bmxSoundOpen 0x44dda0: winchanInit 0x46d940 loads DSOUND.DLL, DirectSoundCreate, sets the primary
buffer format (22050 Hz 16-bit stereo, DSSCL_PRIORITY then back to NORMAL), creates one looping 16-bit stereo
secondary buffer of two 50 ms chunks (2 x 4408 bytes) for the game's own software mixer (ported, bit-exact), and
winstrm.c creates one looping secondary buffer per stream (music/speech: PCM 8/16-bit, mono/stereo, 11025..44100 Hz,
CTRLPAN|CTRLVOLUME|CTRLFREQUENCY, default 64 KB; ADPCM is expanded by the game). A winmm periodic timer (16 ms,
`timeSetEvent(16, period, 0x450020, 0, TIME_PERIODIC)`) runs winchanPump 0x46e010 at most every 4 ms on **winmm's
timer thread**, concurrently with the game thread (guarded by the plain flag gWinchanLock and winstrm's critical
sections): GetStatus / GetCurrentPosition / Lock / copy from the ring / Unlock, Restore on DSERR_BUFFERLOST. Streams
use Play(LOOPING), Stop, SetCurrentPosition(0), SetVolume (1/100 dB, table of 128), SetPan ((pan) * 78). Mixing of
the secondary buffers, volume, pan and resampling to the device rate is DirectSound's (on Vista+ a software
emulation in dsound.dll; the primary SetFormat is accepted but ignored).

**Cut.** LoadLibraryA/GetProcAddress/FreeLibrary slots: `"DSOUND.DLL"` / `"DirectSoundCreate"` return a stand-in
(other names fall through to the CRT path, which is dead). Stand-in IDirectSound + IDirectSoundBuffer per winds.h.
The CRITICAL_SECTION slots get SDL_Mutex shims (the game's 24-byte CRITICAL_SECTION in its data holds our pointer).
The five winmm timer slots get an SDL thread.

**P9. Mixing model**
- **A (recommended):** own mixer in one SDL_AudioStream (get-callback): every playing buffer keeps a play cursor in
  bytes that advances by the frames the device actually pulled (times buffer rate / device rate), so
  GetCurrentPosition, GetStatus and the wrap behave like a DS ring; per buffer: convert to float, resample, apply DS
  volume (`10^(vol/2000)`) and DS pan (attenuate only the far channel by |pan|/100 dB), sum, clamp. Deterministic
  and unit-testable offline.
- B: one SDL_AudioStream per DS buffer and let SDL mix (SDL_SetAudioStreamGain). Less code, but no pan, and the play
  cursor has to be inferred from queue sizes (coarse, races with SDL's mixer thread).

**P9 note: the game's own mixer resamples with a zero-order hold.** Besides our DirectSound mixer (P9-A / P10-A: output rate against buffer rate), the game has a software mixer of its own, winchanMix (winchan.c), that mixes the effects into a 22,050 Hz ring that one DirectSound buffer plays. It reads `data[position]` and ignores the 12 bit fraction of the position, so an 11,025 or 16,000 Hz effect is held for 2 or 1.4 output frames and images (grit). `[audio] smooth_resample = N` in bmxnative.ini (P19): 0 the original, 1 linear, 2 cubic (the default since 2026-10-31, the user's call; PLAN.md Decisions) (4 point Hermite / Catmull-Rom). It attaches at a seam, not inside the faithful port: src/platform/smoothmix.c is a native-only copy of winchanMix that differs only in the sample read (gains, envelopes, flanger, echo, the x87 rounding, clamp and halving are the same), and the native loader writes a jump over the ported winchanMix's entry when the option is on (the hook DLL and the oracle tests never see it). Neighbours follow the position logic: clamp for a one shot, wrap by length - loopStart for a loop, mirror at length - 1 (and at loopStart running backwards) for a ping-pong. tests/platform links the real winchanMix and checks that at fraction 0 all three modes are bit identical to it, the swap is absent at 0, the loop and ping-pong edges, 8 bit and stereo, and that a 1 kHz sine at 11,025 Hz played at 22,050 Hz has its image at 10,025 Hz attenuated (measured: original 852, linear 121, cubic 7 against a 5939 tone).

**P10. Resampler**
- **A (recommended):** SDL3's resampler per buffer (inside P9-A, via a private SDL_AudioStream per buffer
  feeding the mixer). DirectSound's own resampler is unspecified, so neither choice is bit-exact against Windows
  output; verify by recording (S7).
- B: own linear interpolation (simple, deterministic across SDL versions, a bit duller).

**P11. The 16 ms timer**
- **A (recommended):** an SDL thread at SDL_THREAD_PRIORITY_TIME_CRITICAL with absolute 16 ms deadlines
  (SDL_DelayPrecise), like winmm's timer thread; timeBeginPeriod/EndPeriod/GetDevCaps return {1, 1000000}/OK.
  Keeps the original's concurrency (the pump on another thread), which the ported code was written for.
- B: run the pump from the game loop (no thread). Removes the races, but changes when audio is refilled (a long
  frame would starve the 100 ms ring): a behaviour change.

`BMX_MUTE` then becomes "don't open the SDL audio device" (no WASAPI code in main.c).

### 3.3 Movies (Video for Windows + waveOut -> own decoder + SDL3)

**Current path.** zrwPlayMovie 0x48bed0 (startup ACCLAIM/ZAXIS/INTRO, Cool Stuff's movies and credits): zmovieOpen
0x4a06f0 (AVIFileOpenA, AVIFileGetStream per stream), zmovieScanStreams 0x4a0400 (AVIStreamInfoA, ReadFormat,
SampleToTime, GetFrameOpen(NULL) = the stream's own format, DrawDibOpen), zmoviePlay 0x4a09d0 (GetTickCount clock,
GetClientRect), a loop that every 40 ms (timeGetTime) calls zmovieUpdate 0x4a0a90 (GetDC on the window,
AVIStreamTimeToSample, AVIStreamGetFrame, DrawDibDraw stretched to the client rect, ReleaseDC), polls the pads (any
newly pressed button aborts), Sleep(1), and on MM_WOM_DONE refills audio (AVIStreamRead into 0x20-byte-header
buffers, waveOutWrite). Files (all seven checked): Cinepak (`cvid`) 320x240 24-bit, 15 fps; audio plain PCM 16-bit:
mono 37800 Hz (acclaim, amish, intro, zaxis), mono 38700 Hz (mirra, nyquist), stereo 44100 Hz (credits). Natively
today the real VfW draws with GDI into the SDL-claimed window (docs/NATIVE.md "Session, movies and two-player").

**Cut.** The 18 AVIFIL32 + 3 MSVFW32 slots, the 9 waveOut/sndPlaySound slots, GetDC/ReleaseDC/GetClientRect.
zmovie.c stays as is.

**P12. Decoder**
- **A (recommended):** own RIFF/AVI reader (idx1 index, the subset of AVIStream* the game calls, with VfW's
  time<->sample rounding) and own Cinepak decoder to a bottom-up 24-bit DIB like iccvid's output. ~1000 lines, no
  dependency, portable.
- B: FFmpeg (libavformat/libavcodec). Robust, but a large LGPL dependency for seven 320x240 files, and its Cinepak
  YUV->RGB rounding is not iccvid's.

**P13. Presentation of DrawDibDraw**
- **A (recommended):** DrawDibDraw uploads the frame and draws it, nearest-filtered, into the driver's mode-size
  target at the requested rect, then presents through the driver (letterboxed like D4); GetDC returns a dummy handle
  that only DrawDibDraw/ReleaseDC accept. 320x240 -> 640x480 is an exact 2x like DrawDib.
- B: SDL_Renderer on the window. Conflicts with the GPU device's claim on the window.
- C: keep GDI on Windows. Not standalone; and GDI over a D3D12 flip swapchain is fragile (black captures).

**P14. Movie audio**
- **A (recommended):** waveOutOpen -> an SDL_AudioStream at the file's rate (SDL resamples), waveOutWrite queues
  the buffer and remembers its byte end; when the device has consumed past it, mark WHDR_DONE and post MM_WOM_DONE
  (0x3bd) to the message queue (P15) so zrwPlayMovie refills exactly as before. Pause/Restart/Reset map to
  SDL_PauseAudioStreamDevice / resume / clear. sndPlaySoundA(NULL, 0) is a no-op.
- B: refill from a callback without messages. Bypasses zrwPlayMovie's protocol (changes ported behaviour).

### 3.4 Entry point, window, messages, device dialog

**Current path.** main.c does WinMain's job (0x48a460, never ported): globals at 0x697780.., `-r` parsing, a Win32
window (class 0x5611d4, style WS_OVERLAPPEDWINDOW 0xcf0000, CS_OWNDC, 640x480 client), zrwAttachPlugins 0x48b1a0
(-> zrwSelectDevice 0x489d70: DialogBoxParamA resource 0x68 unless `-r` gave w/h/depth), full screen = WS_POPUP,
zrwInitApp 0x48b450, then the PeekMessage loop with the frame limiter (`gRateC / gRateA > elapsed`, busy wait, no
Sleep). The SDL3 driver wraps the HWND (driver/gpu.c) and drops SDL's events at every present. zrwMainWndProc
0x48a800 is the window procedure. Note: main.c sets gHinstance 0x697784 to bmxnative.exe's module, which has no
dialog resource 0x68, so a run without `-r` probably gets -1 from DialogBoxParamA (zrwSelectDevice treats it as OK
and keeps the current mode): **untested**; all native runs used `-r640.480.16`.

**P15. Who owns the window and the message queue**
- **A (recommended):** SDL creates the window (SDL_CreateWindow, title from SetWindowTextA, size from the mode);
  gZrwWindow 0x697780 holds the SDL_Window pointer as an opaque handle (it only goes back into our stand-ins). A small
  message queue in src/platform replaces PeekMessageA/TranslateMessage/DispatchMessageA: SDL events become the
  messages zrwMainWndProc understands (focus -> WM_ACTIVATE, Esc key down -> WM_KEYDOWN with scan code 1, close ->
  WM_DESTROY -> PostQuitMessage -> WM_QUIT), plus MM_WOM_DONE from P14. main.c's loop pumps SDL instead of Win32.
  Removes USER32 and is what Mac/Linux need anyway.
- B: keep the Win32 window and message loop on Windows (as today), SDL wrapping the HWND. Least work now, but
  USER32 stays and the window code is redone for other platforms.

**P16. Device and mode selection (supersedes SDL3-DRIVER.md D6 for this step)**
- **A (recommended):** no dialog. Mode from the command line (`-r<w>.<h>.<depth>`, as now) or a config file;
  with neither, pick 640x480x16 full screen, which is what the original dialog preselects (zrwDialogInit selects
  640x480x16), i.e. the same as pressing Enter in the vanilla dialog. The ported dialog functions stay unused.
- B: rebuild the dialog with our own Win32 resource (Windows only; USER32 stays).
- C: an SDL-drawn chooser. Work for a screen most players skip.

**P17. Full screen / windowed / resolution (seams only)**
- Vanilla: as SDL3-DRIVER.md D4 (render at the mode size, scale with letterbox; "exclusive" modes do not change the
  display mode). Opt-in seams in the window module: windowed / borderless / exclusive, any window size, DPI
  awareness (vanilla is DPI-unaware: Windows bitmap-scales the 640x480 window), cursor capture. ShowCursor keeps
  Win32's display-counter semantics mapped to SDL_ShowCursor/HideCursor; ClipCursor(NULL) is a no-op.
- MessageBoxA -> SDL_ShowMessageBox with the same buttons and return codes (IDYES 6, IDNO 7, IDCANCEL 2, IDOK 1);
  keys.c's scripted answers move into it. OutputDebugStringA -> bmxLog; DebugBreak -> SDL_TriggerBreakpoint.
- **The quit prompt (the user's call, 2026-10-31, PLAN.md Decisions):** `[ui] confirm_quit` defaults to 0: the
  MessageBoxA stand-in answers Yes at once to exactly zrwConfirmQuit's box (text "Do you want to quit the game?",
  type 0x10124 = MB_YESNO | MB_ICONQUESTION | MB_DEFBUTTON2 | MB_SETFOREGROUND), so QUIT quits without a box;
  `= 1` shows it through SDL_ShowMessageBox as the original did. Any other text or type (asserts, errors) still
  shows. It sits at the MessageBoxA seam (src/platform/window.c), not in zrwConfirmQuit.

### 3.5 Files, saves, registry

**Current path.** Game files through the CRT (our crt.c `rwfopen` on the host's fopen); the sound banks/streams
through CreateFileA/ReadFile/SetFilePointer/CloseHandle; movies through AVIFileOpenA. All relative to the working
directory, backslash separated, case-insensitive in practice (`bin\movie\ACCLAIM.AVI` vs `acclaim.avi`,
`SFXCOM.ZSD` vs `sfxcom.zsd`). Save: `.\save\SAVE.DAT`. No registry, no INI (1b).

**P18. Paths**
- **A (recommended):** one resolver, `bmxPathResolve(const char *gamePath)`, used by crt.c's fopen, the
  CreateFileA shim and the AVI reader. Vanilla: identity (working directory = install). Seams (opt-in): asset folder
  from the config file / command line (run from anywhere); `save\` redirected to SDL_GetPrefPath (user profile,
  copied from the install on first use); later: `\` -> `/` and case-insensitive lookup for Mac/Linux.
- B: chdir to the install folder and leave paths alone. No seam for profile saves.

The CreateFileA/ReadFile shims map to SDL_IOStream (or stdio); the HANDLE is our pointer; only the flags winload/winstrm
pass need handling (GENERIC_READ, OPEN_EXISTING, FILE_BEGIN/CURRENT).

**P19. Config file**
- **A (recommended):** one small text file (`bmxnative.ini`, next to the exe by default, in the pref path when
  profile saves are on) for the mode, the options of this document (off by default except the two input conveniences P7 A/A2) and the remap tables. Options
  that change gameplay must also go into replays (PLAN); none in this document do.
- B: command line / environment only (as the dev aids today).

  **Defaults (2026-11-01, PLAN.md Decisions):** the enhancements are on unless the file sets them;
  `[general] vanilla = 1` gives the original's behaviour for every option the file does not set (bmxConfigOption):
  `width`/`height` 640 x 480, `widescreen`, `hud`, `menus`, `ui_filter`, `interpolate`, `full_draw_distance`,
  `smooth_resample`, `dpad_steering`, `stick_8way` 0 and `confirm_quit` 1. The values below in parentheses are the
  originals'; the enhanced defaults are `width = height = 0`, `widescreen = 1`, `hud = 1`, `menus = 2`,
  `ui_filter = 2`, `interpolate = 1`, `full_draw_distance = 1` (far clip and fog at their maximum, src/native/drawdist.c).

  The `[video]` keys (config.c `kKnownKeys`; all integers): `width`, `height` (default 640 x 480; `0` = the
  primary display's desktop size, docs/WIDESCREEN.md W1), `depth` (16), `fullscreen` (1; 0 = a window of width x
  height, rendered at that size); the widescreen options of docs/WIDESCREEN.md W9 (WS1-WS3): `widescreen` (0; 1
  installs the seams), `aspect` (1 hor+ keeping the 4:3 view inside, 0 stretch, 2 vert-), `fov_scale` (100, percent
  of both view axes, 10..400), `max_hfov` (0 = none; cap of the horizontal FOV in degrees), `hud` (0 centred 4:3, 2
  stretch; 1 = edge anchored, WS5; `hud_aspect` bounds it, aspect · 100, default 178), `menus` (0 pillarbox, 1 stretch, 2 pillarbox on a blurred, darkened fill: WS6), `movies` (0 pillarbox, 1
  stretch), `scale_filter` (0 linear letterbox, 1 nearest, 2 integer factor; the present, also without
  widescreen), `ui_filter` (0 the game's nearest sampling of 2D / HUD textures, 1 linear, 2 sharp bilinear; also
  without widescreen); the high frame rate options of docs/HIGHFPS.md H8 (HF1-HF3): `interpolate` (0; 1 draws
  between the 30 Hz ticks in levels, interpolated one tick behind; visual-only, the sleeping wait is used in that
  mode only), `max_fps` (0 = the display's refresh rate; draws per second with `interpolate = 1`), `vsync` (1; 0 =
  immediate presents, mailbox where immediate is missing; applies to every present). `-r<w>.<h>.<depth>` on the
  command line still wins over width / height / depth / fullscreen.

### 3.6 Timing

timeGetTime (0x699838) and GetTickCount (0x6995fc) -> milliseconds from SDL_GetTicks plus a fixed non-zero base
(main.c's limiter treats 0x67275c == 0 as "unset"; 32-bit wrap kept); Sleep -> SDL_Delay; QueryPerformance* ->
SDL_GetPerformanceCounter/Frequency (profiler only). The busy-wait frame limiter stays in vanilla; sleeping in it is
an opt-in (power) seam in main.c, not in ported code.

## 4. Implementation order (each slice testable)

| # | Slice | Size | Model | Verify |
|---|---|---|---|---|
| S1 | Import table rework (P2): `{dll, name, fn}` table, named trap stubs for the 64 dead slots, log which real DLLs are still loaded | ~150 lines | Sonnet | Scripted ProQuest / Freeride / movies runs as in NATIVE.md: no trap, identical logs; dead-slot traps never fire |
| S2 | Time + misc kernel32: timeGetTime, GetTickCount, Sleep, QPC/QPF, OutputDebugStringA, DebugBreak, critical sections | ~200 | Sonnet | Unit tests (monotonic, base, wrap); scripted runs; golden trace (docs/TRACING.md) of a scripted run still matches the hooked game's |
| S3 | winmm timer thread (P11) | ~150 | Sonnet, Opus review (threading) | Log pump intervals (mean 16 ms, jitter) native vs hooked; audio still plays (with real DirectSound) |
| S4 | File shims for the sound loader (CreateFileA/ReadFile/SetFilePointer/CloseHandle) + bmxPathResolve identity (P18) | ~200 | Sonnet | Hash of every byte the game reads per file, native vs hooked (trace log), equal |
| S5 | Input stand-in (P3-P6): keyboard + pad on SDL3, BMX_KEYS on top, `BMX_INPUT_CHECK=1` dual-read: real DirectInput opened alongside, every poll compares the 0x74-byte device blocks (and DIJOYSTATE / DIK state) and logs differences | ~700 | Opus (DI semantics, dead zone, button orders), Sonnet (DIK table) | User presses every key and pad button / sweeps the stick with an Xbox pad and the DualSense: zero mismatches in buttons, bits; stick threshold within one step. Also in the hook build (P2-A). |
| S6 | D-pad steering option (P7) + config file (P19) | ~150 | Sonnet | Option off: S5 check still clean; on: d-pad yields the same bits as the stick, golden trace with d-pad input equals one with stick input |
| S7 | DirectSound stand-in + SDL3 audio (P9, P10) | ~900 | Opus | (a) offline unit tests: lock/wrap/cursor/status against the real dsound.dll in a headless test process; (b) log every Lock/Unlock write + volume/pan/play/stop of a hooked session, replay it into our mixer and into real DirectSound, record both (WASAPI loopback), compare RMS / spectrum after alignment with a tolerance; (c) user listening test (menus, a run, speech, music changes) |
| S8 | Movies: AVI reader + Cinepak decoder (P12) | ~1000 | Opus (decoder exactness) | Windows-only test tool: for all frames of the 7 files, AVIStreamGetFrame (VfW/iccvid) vs ours byte-identical; StreamInfo/ReadFormat/TimeToSample/SampleToTime for every position equal; AVIStreamRead bytes equal |
| S9 | Movie output: DrawDib on the driver (P13), waveOut on SDL3 with MM_WOM_DONE (P14) | ~400 | Sonnet | DrawDibDraw into a memory DC (headless GDI) vs our GPU readback (BMX_SHOT) of the same frame at 640x480: pixel-identical; audio bytes queued equal AVIStreamRead's; intro/credits play to the end and abort on a key |
| S10 | SDL-owned window + message queue + message boxes + window misc (P15, P17), mode without dialog (P16) | ~500 | Opus (touches driver window ownership) | Scripted runs incl. QUIT (message box), Esc-quit before the key lock, alt-tab focus (input stops, like DI foreground), window close; full screen and windowed at 640x480 and 1024x768 |
| S11 (done, 4f) | Remove the remaining Win32 from main.c / keys.c / driver (WASAPI mute -> no audio device), import check in tools/check.sh | ~100 | Sonnet | `llvm-objdump -p bmxnative.exe`: only SDL3, UCRT, KERNEL32 (+ BCRYPT until an own SHA-256) |
| S12 | Opt-in seams: profile saves + asset folder (P18), hot-plug (P5), remap (P8), window modes (P17) | ~400 | Sonnet | Each option off: S1..S10 checks unchanged; on: feature test |

### 4a. Status of S1-S4 (2026-10-31, native-only, committed since)

Code: `src/platform/` (hosted C, SDL3, no windows.h): `imports.c` (the table), `kernel.c` (S2), `winmm.c` (S3),
`fileio.c` + `paths.c` (S4), `avi.c` (the AVISaveOptionsFree shim, moved out of loader.c), `standins.h` (the
stand-in prototypes), `platform.h`. It is part of the `bmxnative` target and excluded from tools/check.sh's hook
tidy list like src/native (tidy it with `-p build/native32`). Tests: `tests/platform/test_platform.c`, target
`bmxplatformtest` (preset native32; hosted, no game image; compares the stand-ins with the real kernel32 / winmm).
**The hook DLL is unchanged and does not use the stand-ins** (P2-A allows it; nothing in S1-S4 needs it, S5 will).

- **S1 done.** `loader.c fillImports` asks, per slot: `bmxStandInFind(dll, name)` (src/platform/imports.c; entries
  `{dll, name, fn}`, DLL name case-insensitive; keys.c adds its MessageBoxA through `bmxKeysStandIns` because
  only its definition knows the stdcall decoration), then `bmxImportIsDead` (the 64 slots of 1b: all of ADVAPI32 and DDRAW,
  49 KERNEL32, 9 USER32), else the real function. A dead slot gets a 16-byte stub (`push index; call deadSlot`) in a
  VirtualAlloc'd page: calling it logs `DEAD IMPORT called: KERNEL32.dll!HeapAlloc ..., caller <addr>` and exits
  with code 3. A DLL is loaded only when a slot needs the real function (or `bmxImportKeepsDll`: DINPUT.dll, since S5 only
  with BMX_INPUT_CHECK, which calls the real one; the game folder's copy may be the hook DLL, which must patch before
  our jumps).
  The log has one line per DLL and a summary; today: `137 slots filled: 23 stand-ins, 64 dead, 50 real; real DLLs
  still used: KERNEL32.dll USER32.dll WINMM.dll AVIFIL32.dll MSVFW32.dll` (KERNEL32: 3 real = LoadLibraryA /
  GetProcAddress / FreeLibrary; ADVAPI32, DDRAW and DINPUT are no longer loaded for the dead / stand-in slots).
  The stand-in count 23 = 11 (S2) + 5 (S3) + 4 (S4) + AVISaveOptionsFree + DirectInputCreateEx (S5, before: BMX_KEYS's) + MessageBoxA (BMX_KEYS).
- **S2 done** (`kernel.c`). timeGetTime and GetTickCount share one clock: milliseconds since process start plus a
  fixed base (0x36ee80 = 1 h of "uptime"; `BMX_TIME_BASE=<n>` overrides it, e.g. `0xfffff000` runs the limiter
  through the 32-bit wrap), 32-bit wrapping, never 0 at start. Resolution as Windows: timeGetTime 1 ms; GetTickCount
  floor(k * 15.625) ms (the 64 Hz system tick, steps of 15/16 ms; the movie clock sees what it saw). main.c's frame
  limiter now calls `bmxTimeGetTime` (it must share the clock with the game's own `gLastTick` writes). QPC / QPF are
  SDL's (the real ones on Windows: same base, 10 MHz). Sleep = SDL_Delay. Critical sections: the first word of the
  game's 24-byte CRITICAL_SECTION holds an SDL mutex (recursive); Enter/Leave on a zeroed section do nothing.
  OutputDebugStringA is **dropped** unless `BMX_DEBUG_OUTPUT=1` sends it to the log (it goes to a debugger on Windows;
  the game prints asset loads and every movie audio tick, thousands of lines; P17 said "-> bmxLog"). DebugBreak =
  SDL_TriggerBreakpoint (an int3 like the original; the trap handler treats it like any other).
- **S3 done** (`winmm.c`). timeGetDevCaps {1, 1000000}, timeBegin/EndPeriod OK and no effect, timeSetEvent starts an
  SDL thread (SDL_THREAD_PRIORITY_TIME_CRITICAL, name "bmx-timer", up to 8 events, one thread each) calling
  `proc(id, 0, user, 0, 0)` on absolute deadlines (resync instead of a burst when more than a period late; one-shot
  and periodic), timeKillEvent joins the thread (so the callback is not running when it returns; from the callback
  itself it only marks the event). The game's bmxSoundClose / reopen (movies, level loads) exercises kill + set
  many times. `BMX_TIMER_LOG=1` logs per event at kill (and at BMX_QUIT_FRAMES): calls, mean / min / max / sd of the
  interval, calls more than 2 ms late, mean callback time. Freeride run, real winchanPump: 4784 calls, mean 15.999 ms,
  sd 0.03, 0 late, callback 0.036 ms. The test program also runs real winmm and the stand-in side by side (3 s each): mean 16.003 vs 16.000 ms, sd 0.50 vs 0.11 ms.
- **S4 done** (`fileio.c`, `paths.c`). CreateFileA / ReadFile / SetFilePointer / CloseHandle on SDL_IOStream, read
  only (GENERIC_READ + OPEN_EXISTING; anything else is refused and logged); the HANDLE is a pointer to our record;
  Windows semantics kept (failed open = -1, read at the end = success with 0 bytes, seek failure = 0xFFFFFFFF).
  `bmxPathResolve` is the identity and is used by the CreateFileA shim and crt.c's fopen. SDL quirk: SDL's Windows
  `SDL_SeekIO` returns 0 (not -1) for a seek before the start, so failure is detected through the error string.
  `BMX_FILE_LOG=1` logs per file at close (reads, bytes, FNV-1a 64 of the bytes read in order). **`BMX_FILE_CHECK=1`
  (our addition) opens every file through the real kernel32 as well and repeats every call there, comparing
  results, byte counts, bytes and positions**: this replaces "hash of every byte read, native vs hooked" (the hook
  cannot log it) with an exact check against Windows in the same process; a game run (ProQuest into Vail Trails, with
  the music stream read on the timer thread) shows `filecheck ok` for all 8 files.
- Not done / deliberately left: LoadLibraryA / GetProcAddress / FreeLibrary stay real (S7), all window, movie
  and waveOut slots stay real (input: S5 below). The only stand-in that calls the real function when its option is
  off is BMX_KEYS's MessageBoxA.
- **S3 review (Opus, 2026-10-31)**, section 5 "Timer-thread concurrency" has the item-by-item notes. Fixed in
  winmm.c: (1) a timeKillEvent joining outside the lock left its slot looking live, so a concurrent timeSetEvent
  (reaping a fired one-shot) or a second kill of the same id could join the thread a second time and reuse the slot,
  whose new event the first kill then freed; a `joining` flag under the lock now keeps everyone else off the slot;
  (2) the thread id was written by the timer thread without the lock and read by other threads' kills: it is now
  set under the lock in timeSetEvent before the callback can run; (3) return codes measured against winmm: an
  unknown id, a killed id and a one-shot that already fired all give `TIMERR_NOCANDO` (97), ours gave 11 / 0 (the
  game ignores the value); (4) BMX_TIMER_LOG's lazy flag is read before any timer thread exists. New tests: two
  threads killing the same id (20 rounds, exactly one succeeds, nothing joined twice), the three return codes against
  the real winmm.

### 4b. Status of S5 and S6 (2026-10-31, native-only, committed since; the real-pad check needs the user)

Code: `src/platform/input.c` (the stand-in DirectInput and the conversions), `inputcheck.c` (BMX_INPUT_CHECK),
`diface.h` (the DirectInput 7 interfaces in our own types: src/platform still has no windows.h), `input.h`,
`config.c` (P19). keys.c keeps the BMX_KEYS parser and the MessageBoxA shim; its DirectInput stand-in is gone.
**The hook DLL is unchanged** (the dual-read runs in the native exe against the real dinput.dll in the same
process, so P2's hook use is not needed for it).

- **S5 done (code, unit tests, the unfocused half of the check); the focused check with real pads is the user's.**
  `DINPUT.dll!DirectInputCreateEx` -> `bmxStandDirectInputCreateEx` (all builds, not only BMX_KEYS). At the first
  call: the config, BMX_KEYS, then `SDL_InitSubSystem(SDL_INIT_GAMEPAD)` and the first joystick SDL lists (P5-A, no
  hot-plug; an unplugged pad is found again by its SDL GUID, as DirectInput re-acquires it). SDL's joystick and
  gamepad events are off (nobody pumps SDL's queue, the game's own message loop runs); the pad's `Poll` calls
  `SDL_UpdateJoysticks`. `SDL_HINT_JOYSTICK_ENHANCED_REPORTS=0` is set first: SDL's default switches a Bluetooth
  DualSense to its enhanced report, after which DirectInput (other programs, and our own check) sees no input until
  the pad is switched off. Devices (up to 8 objects): `SetDataFormat` (keeps dwDataSize; DIERR_ACQUIRED while
  acquired), `SetCooperativeLevel` (validates the flag pairs), `SetProperty` (DIPROP_RANGE / DEADZONE / SATURATION
  per axis or device-wide, sizes checked: the game's fourth call, DIPROP_RANGE with a DIPROPDWORD, fails with
  DIERR_INVALIDPARAM as it does in DirectInput), `Acquire` (fails with DIERR_OTHERAPPHASPRIO while the window has no
  focus; DI_OK, then S_FALSE when already acquired; "exclusive" does not shut out a second object of the same
  process: every pad object of the game acquires and reads the pad, see the fix below),
  `Unacquire`, `Poll` (S_FALSE: HID devices need no polling), `GetDeviceState` (DIERR_NOTACQUIRED / DIERR_INPUTLOST
  once after the focus or the pad went away, the buffer untouched on failure), `GetDeviceInfo` (SDL's name, the
  product GUID `PIDVID` / "PIDVID" from VID/PID, type 0x10404 like DirectInput's), Release. Focus = SDL's keyboard
  focus on the game window (SDL subclasses the HWND it wraps, driver/gpu.c, so it sees WM_ACTIVATE / WM_SETFOCUS).
  - Keyboard (P6-A): `SDL_GetKeyboardState`, each pressed scancode through a fixed SDL_Scancode -> DIK table (set-1
    scan code, + 0x80 for E0 keys; ISO `#` shares 0x2b with backslash as on Windows).
  - Pad (P3-A, P4-A): opened as SDL_Gamepad when SDL knows it. PS4 / PS5 types: DirectInput's HID order Square 0,
    Cross 1, Circle 2, Triangle 3, L1 4, R1 5, L2 6, R2 7, Create 8, Options 9, L3 10, R3 11, PS 12, touchpad 13,
    (DualSense) mute 14; L2 / R2 buttons = trigger > 0; axes X / Y left stick, Z / Rz right stick, Rx / Ry triggers.
    Xbox 360 / One types: A 0, B 1, X 2, Y 3, LB 4, RB 5, Back 6, Start 7, LS 8, RS 9, no guide; X / Y, Rx / Ry
    sticks, both triggers on Z. Everything else: SDL_Joystick raw buttons, axes in order into X, Y, Z, Rx, Ry, Rz,
    hat 0. POV 0 from the d-pad (hundredths of a degree, -1 centred), POVs 1-3 -1. Axis values: the raw HID value is
    recovered from SDL's (`r * 257 - 32768` for the Sony pads' 8-bit sticks under SDL's HIDAPI driver; `v + 32768`
    for 16-bit pads) and cooked like DirectInput: centre = (logical max + 1) / 2 (128), dead zone and saturation as fractions of
    each half (MulDiv-rounded, MulDiv checked bit-exact against kernel32's), the rest scaled linearly onto the half
    range with MulDiv. With the game's -127..127 and 10% that is: 8-bit stick 0 for raw 115..141, -1 at
    114, +1 at 142; 16-bit 0 within 3277 of 32768. This is Wine's dinput formula with the centre rounded up instead of
    down (fitted to the user's check logs, below).
  - BMX_KEYS: `bmxKeysScripted()` / `bmxKeysFillState()` (keys.c) are the keyboard; no pad is listed and the focus
    is ignored, so scripted runs behave exactly as before (ProQuest into Vail Trails: no trap, the frame at 1450 is
    byte-identical between runs).
  - **`BMX_INPUT_CHECK=1`** (inputcheck.c; off in a BMX_KEYS run): creates the real IDirectInput7 (the loader then
    keeps DINPUT.dll loaded, see `bmxImportKeepsDll`), finds its first joystick, and mirrors every call the game
    makes on a stand-in device on a real device for the same keyboard / joystick: the HRESULT of every call, then
    per read the DIK state, the 32 buttons, lX / lY. Strict (what pcinput.c reads, and the device block's bits are a
    function of exactly these): DIK & 0x80, buttons & 0x80, the sign of lX / lY. A difference that persists over two
    reads in a row is a `MISMATCH` line; a one-read difference is counted as transient (a key reaches SDL through
    the message queue, DirectInput directly). Informational: the other axes (beyond 1%) and the POVs (logged once
    each). The live log has every real key / button edge and every lX / lY sign change with our raw HID value, and
    the summary at exit (`bmxPlatformShutdown`, also at BMX_QUIT_FRAMES) the counts and, per stick direction, the
    largest distance from the centre DirectInput still reported as 0 and the smallest it reported non-zero, next to
    ours. Last line: `inputcheck: RESULT clean|MISMATCHES`.
  - Checked here (unfocused, with the user's DualSense attached): bmxplatformtest runs the game's exact pcinput.c
    sequence (keyboard and pad: formats, cooperative levels, the four SetProperty calls, GetDeviceInfo, Acquire,
    Poll, GetDeviceState, Unacquire) on the stand-in with BMX_INPUT_CHECK on and a hidden window: **0 HRESULT
    differences**; DirectInput's product name ("DualSense Wireless Controller") and product GUID
    (0ce6054c-0000-0000-0000-504944564944) equal ours; device type equal after the fix to 0x10404. Not checkable
    without the focus: the states themselves, the stick threshold, the second pad object's exclusive Acquire. That
    is the user's test (docs/NATIVE.md "Input on SDL3"). Xbox pads: no pad here, the table is from the usual
    DirectInput order (unverified).
- **S6 done (code and unit tests).** `bmxnative.ini` (P19, config.c): next to the exe, or `BMX_CONFIG=<path>`;
  `[section]` + `key = value`, `#` / `;` comments; read once, unknown keys logged; options off by default except dpad_steering and stick_8way.
  Option `[input] dpad_steering = 1` (P7-A): after the check has compared the vanilla state, a pressed d-pad
  direction sets lX / lY to the end of the range the game set (-127 / +127), over the stick on that axis; the log
  shows each d-pad change with the resulting lX / lY. Input-only (the game sees a full stick deflection, the same
  bits 0x1..0x8), not recorded in replays. Unit tests: left on a centred stick, up + right, no d-pad = no change. The
  "golden trace with d-pad equals one with the stick" part waits for native replay; with BMX_INPUT_CHECK on, the
  check still compares the vanilla values (the d-pad never shows as a mismatch).
- Decisions beyond the document: ENHANCED_REPORTS off (above); the check is in-process against the real dinput.dll,
  not in the hook build; transient vs persistent differences; Wine's cooking formula as the starting point (to be
  confirmed by the check); a lost pad is found again by SDL GUID; the d-pad log line; `bmxPlatformShutdown` replaces
  the two `bmxTimerShutdown` calls (main.c, driver/gpu.c).
- **The user's focused run (tests A and B, 2026-10-31) and two fixes.** Keyboard and pad buttons matched, the pad
  worked in the menus but was **dead in a level**: at the level load the game creates two more pad objects (player
  1 and 2, both foreground exclusive); ours refused the first one's Acquire (DIERR_OTHERAPPHASPRIO, because the other
  held the pad), while DirectInput gave DI_OK and then S_FALSE on every later call, and that object is the one the
  game reads in a level (1795 HRESULT differences, 0 reads). Fix: no exclusivity between objects of the process.
  Test: two exclusive (background, no focus needed) objects on the user's DualSense under BMX_INPUT_CHECK: both
  DI_OK, then S_FALSE, both read, 0 differences. Second, lX / lY were often DirectInput's + 1 (DI 39 / ours 40,
  threshold +14 vs +15 from our old centre 127): a search over centre (127, 127.5, 128, also on a 0..65535 scale),
  dead zone and output rounding against the 45 distinct logged pairs {raw, DirectInput} finds exactly one model with
  a single misfit: Wine's formula with centre 128 (raw 115..141 -> 0, 114 -> -1, 142 -> +1, 238 -> 108). The misfit,
  raw 146 -> 5, cannot come from any integer raw under it (145 -> 4, 146 -> 6) and was logged while the stick jumped 9
  steps between two polls, so the two sides most likely read different reports; the 44 other pairs are unit-test data.
  To settle such cases the check now also compares on **still** reads (our raw value and DirectInput's value both
  unchanged since the previous read: both certainly saw the same report) and prints, per 8-bit stick axis, the
  table raw:DirectInput of every still value seen; it logs a repeated HRESULT difference only three times and allows
  3000 lines (the dead-pad run had used up the old 400).
- Open: the user's rerun of test A (expect `still stick: N reads, 0 with a different lX / lY` and no HRESULT
  differences after the level load), an Xbox pad if one is at hand; DirectInput reports lRx / lRy as 32767 with the
  DualSense's triggers at rest while ours give 0 (informational, the game does not read them: DirectInput's axis
  assignment for the triggers is not what we assumed); SDL's HIDAPI driver may set the DualSense's light bar / player LED on open (cosmetic,
  DirectInput never did).

S1-S6 are done apart from the user's real-pad check.

### 4c. Status of S7 (2026-10-31, native-only, committed since; the listening test is the user's)

Code: `src/platform/dsound.c` (the stand-ins and the mixer), `soundcheck.c` (BMX_SOUND_CHECK), `dsface.h` (IDirectSound /
IDirectSoundBuffer in full, WAVEFORMATEX, DSBUFFERDESC, DSBCAPS, the HRESULTs, in our own types), `sound.h` (test and
check hooks). winchan.c / winstrm.c / winload.c are unchanged; **the hook DLL is unchanged** (the dual check runs in the
native exe against the real dsound.dll in the same process, as S5's did).

What the game calls (inventory from src/bmx/pc/zge/sound and bmxsound.c; nothing else in src/ touches DirectSound):
KERNEL32 `LoadLibraryA("DSOUND.DLL")`, `GetProcAddress(.., "DirectSoundCreate")`, `FreeLibrary` (winchanInit 0x46d940,
winchanProbeDevice 0x46f380, winchanShutdown 0x46df20); `DirectSoundCreate(NULL, ..)`; IDirectSound
SetCooperativeLevel (NORMAL, PRIORITY around the primary SetFormat, NORMAL), CreateSoundBuffer, Release; the primary
buffer: SetFormat (22050 Hz 16-bit stereo), Play(LOOPING), Stop, Release; secondary buffers (the mixer's 2 x 50 ms ring,
8816 bytes at 22050 Hz 16-bit stereo; one per stream, 44100 Hz mono 16-bit 256 KB for music, 64 KB by default):
GetStatus, GetCurrentPosition, Lock, Unlock, Restore, Play(LOOPING), Stop, SetCurrentPosition(0), SetVolume, SetPan,
Release. No DuplicateSoundBuffer, SetFrequency, GetCaps or QueryInterface; they are implemented anyway (cheap, tested).

- **The cut.** `LoadLibraryA` returns a stand-in module for `DSOUND.DLL` (any other name is refused and logged: the
  game asks for no other), `GetProcAddress` gives `bmxStandDirectSoundCreate` for it, `FreeLibrary` accepts it. KERNEL32 now
  has no real slot left: `137 slots filled: 26 stand-ins, 64 dead, 47 real; real DLLs still used: USER32.dll WINMM.dll
  AVIFIL32.dll MSVFW32.dll`.
- **Mixer (P9-A, P10-A).** One SDL audio stream (float stereo at the device's own rate, 48000 Hz here; WASAPI) whose
  get-callback mixes 10 ms quanta (`SDL_HINT_AUDIO_DEVICE_SAMPLE_FRAMES` = rate / 100, so the callback, and the cursors,
  step every 10 ms like dsound.dll's mixer). Per quantum and playing buffer: the buffer advances by quantum * frequency /
  rate frames, the new frames go through the buffer's own SDL_AudioStream (format, channels, SDL's resampler), then
  volume and pan, sum, clamp to +-1. If the default device cannot be opened, SDL's dummy driver (silent, cursors run).
- **Cursors** (measured on this machine, Windows 11, a 48 kHz endpoint; deterministic across runs): after k mixes since
  Play the write cursor is `ceil(k * quantum * frequency / rate)` frames past the start plus the resampler's lookahead
  (33 frames when the buffer is resampled up, 63 for 96 kHz into 48 kHz, 0 at the device rate); the play cursor is the
  write cursor minus `floor(30 ms of frames)`, capped at half the buffer, never before the start. Before the first mix
  both are at the start; a stopped buffer reports its play cursor for both; SetCurrentPosition while playing restarts
  the count there without the lookahead (the resampler keeps its history); a non-looping buffer stops at the mix that
  reaches its end, status 0, position 0. The frames up to the write cursor are the ones the mixer has read, so the
  game's writes between the play and write cursors are not heard, as on Windows. For the game's ring this means steps of
  880 / 884 bytes every 10 ms and a write lead of 2644 bytes; the first play-cursor move 20-30 ms after Play.
- **API semantics** measured and copied: Lock (offset at or past the end, 0 bytes or more than the buffer = INVALIDPARAM
  with the outputs zeroed; ENTIREBUFFER starts at the offset and wraps; FROMWRITECURSOR; unaligned offsets allowed;
  overlapping locks refused, also between a buffer and its duplicates); Unlock matches the lock by its first pointer only
  (sizes are not checked; a wrong second pointer fails but releases the lock; without a second pointer the second part
  stays locked); SetCurrentPosition rounds down to a frame; volume -10000..0, pan +-10000, frequency 100..200000 or 0,
  CONTROLUNAVAIL without the CTRL flag; Play's reserved / priority / unknown flags = INVALIDPARAM, Play on a playing
  buffer only changes the looping flag (status 5 -> 1); a failed CreateSoundBuffer leaves `*out` alone; sizes round up to
  the block; the caps add LOCSOFTWARE. Primary buffer: one object per device, 32768 bytes, caps flags 9, default format
  22050 Hz 8-bit stereo, SetFormat needs PRIORITY (PRIOLEVELNEEDED otherwise) and checks block align / bytes per second,
  is stored for GetFormat and otherwise ignored (dsound.dll mixes at the endpoint's format too), Play needs LOOPING,
  GetCurrentPosition / Lock need WRITEPRIMARY, SetCurrentPosition = INVALIDCALL. Duplicates share data and locks, start
  stopped at 0 with the original's volume.
- **Volume and pan law** (measured through the session's peak meter, which reads before the session mute): the
  attenuation (-volume, plus the pan on the far side) is applied in 0.1 dB steps, `max(1, (a + 5) / 10 - 1)` steps for a
  in 1/100 dB (1..24 -> 0.1 dB, 25..34 -> 0.2 dB, ..., 995..1004 -> 9.9 dB), the gain is truncated to 1/65536 (so -96 dB
  and below is silence); mono buffers play at full level on both sides; the sum clips at 1.0. 8-bit is (v - 128) / 128.
- **Focus** (measured: a buffer without DSBCAPS_GLOBALFOCUS in a process whose window is not in front is silent in
  dsound.dll, with GLOBALFOCUS it is heard): ours mute such buffers while SDL's keyboard focus is not on the game
  window; the cursors run on. Scripted (BMX_KEYS) runs are unfocused, hence silent, like the original would be.
- **BMX_MUTE** keeps the device open and mixes as usual but outputs silence (the cursors and the callback timing stay
  those of an audible run); the waveOut stand-in is silent as well, and main.c's WASAPI session mute is gone (S11, 4f).
  This replaces "don't open the SDL audio device" from P11's note.
- **`BMX_SOUND_CHECK=1`** (soundcheck.c): the real IDirectSound is created next to ours and every call is repeated on a
  real object made the same way: HRESULTs and Release counts, caps and format at creation, status, lock regions (offset
  and both sizes), volume / pan / frequency read back, the primary format, the steady write lead are strict (`DIFFERENCE`
  lines, `soundcheck: RESULT clean|DIFFERENCES` at exit); how far our play cursor is from the real one and the cursor
  step sizes between reads are reported per buffer. The real buffers never get the game's bytes (Lock / Unlock are
  mirrored, the data not copied), so a check run is not heard twice and can stay on during listening.
- **Tests** (`bmxplatformtest`, against the real dsound.dll in the same process, hidden window, this process's session
  muted): the API script (193 results: cooperative levels, the primary buffer, invalid descriptions, every Lock / Unlock
  case above, positions, volume / pan / frequency ranges, Play flags, status, refcounts, QueryInterface, duplicates)
  identical; the cursor sequences of 12 plays (the game's ring from 0 and 1000, half size, non-looping, 48 kHz, the
  44.1 kHz streams 8 / 16 bit, a tiny buffer, 11025 / 16000 / 96000 / 8000 Hz) identical after every mix, our offline
  mixer against dsound.dll in real time; SetCurrentPosition while playing and Stop identical; the mixer offline (16-bit
  stereo exact, volume / pan gains, -10000 silent, unfocused silent with the cursor running, GLOBALFOCUS heard, 8-bit
  mono, a 1 kHz sine's RMS through the resampler within 1.5%, the non-looping end); real time through our SDL device:
  steps every 10.0 ms and the lead 2644 bytes as dsound.dll's, and the peak level of a GLOBALFOCUS tone at four volume /
  pan settings equal to dsound.dll's (meter, 4 digits).
- **Game runs** (`native-run2`, `BMX_MUTE=1`, `BMX_KEYS`, `BMX_SOUND_CHECK=1`): ProQuest into Vail Trails and riding (96 s,
  17950 calls compared), and ProQuest, end run, exit to the main menu, Freeride into a level (156 s, 28012 calls, the
  music stream destroyed and created again): `RESULT clean`, 0 differences, every steady write lead equal, our play
  cursor equal to dsound.dll's at almost every read (the stream's: always; the mixer ring's: at most one 10 ms quantum
  apart, the two mixers wake on the same WASAPI period), identical cursor-step histograms, 0 resampler underruns, the
  pump at 15.999 ms.
- Decisions beyond the document: the cursor model and gain law copied from measurements rather than "advance by what
  the device pulled"; focus muting; BMX_MUTE outputs silence with the device open; the check in-process and silent;
  LoadLibraryA refuses other DLLs; mixing at the device's rate in 10 ms quanta; 24 / 32-bit buffers refused with
  BADFORMAT (dsound.dll takes them; the game has none).
- Not reproduced (unmeasurable or irrelevant here): dsound.dll's resampler (ours is SDL's; the 1 kHz test only checks
  the level), the absolute output latency after the cursor (ours sends a quantum to SDL when it is mixed), the timing
  of the first mix after Play within the 10 ms period, the lookahead for other downsampling ratios (assumed
  `ceil(32 * ratio) - 1`), dsound.dll's behaviour on a 44.1 kHz endpoint (not available here), BUFFERLOST (never).
- Open: the user's listening test (docs/NATIVE.md "DirectSound on SDL3"), including whether the original also goes
  silent behind the QUIT message box (ours does: the box takes the focus) and on alt-tab.

### 4d. Status of S8 and S9 (2026-10-31, native-only, committed since; the watch test is the user's)

Code: `src/platform/avi.c` (the AVIFIL32 stand-ins: RIFF/AVI reader), `cinepak.c` / `cinepak.h` (the decoder),
`avi.h` (AVIERR codes), `waveout.c` (WINMM waveOut on SDL3 audio), `src/driver/movie.c` (DrawDib, GetDC / ReleaseDC
/ GetClientRect on the driver); `gpu.c` got `drvGpuPresentMovie` (present without counting a shown frame). zmovie.c
and zrwPlayMovie are unchanged; **the hook DLL is unchanged**. Test tool: `tests/platform/test_movie.c`, target
`bmxmovietest` (Windows only, links vfw32: compares with avifil32 / iccvid / DrawDib in the same process).

What the game calls (from the import table and zge/zmovie.c, zrwzgl.c): AVIFIL32 AVIFileInit / Exit / OpenA (mode
0) / GetStream (type 0, by index) / Release, AVIStreamInfoA (0x8c bytes), ReadFormat (size query, then the bytes),
Start, Length, TimeToSample, SampleToTime, GetFrameOpen (NULL: the stream's own format), GetFrame, GetFrameClose,
Read (audio, 32 KB at a time), Release, AVISaveOptionsFree (the old shim); AVIStreamAddRef is imported, never
called. MSVFW32 DrawDibOpen / DrawDibDraw (whole frame into the client rectangle, flags 0, DDF_BACKGROUND for other
video streams) / DrawDibClose. USER32 GetDC / ReleaseDC / GetClientRect (only zmovie.c). WINMM waveOutOpen
(WAVE_MAPPER, CALLBACK_WINDOW), Prepare / Unprepare / Write (16 buffers of 32 KB), Pause / Restart / Reset /
Close, sndPlaySoundA(NULL, 0) (a retry when the open fails). The 7 files: Cinepak 320x240 15 fps (scale/rate
10000/150000, credits 1/15), two strips per frame, key frames with a 0x10 and a 0x11 strip; audio PCM 16-bit, mono
37800 / 38700 Hz in 1 s chunks, credits stereo 44100 Hz in 1/15 s chunks inside `rec ` lists; idx1 offsets relative
to `movi`; trailing zero bytes after the RIFF (ignored).

- **The cut.** 18 AVIFIL32, 3 MSVFW32, 9 WINMM (waveOut, sndPlaySoundA) and 3 USER32 slots are stand-ins. The
  loader now says `137 slots filled: 58 stand-ins, 64 dead, 15 real; real DLLs still used: USER32.dll` (WINMM: 15
  stand-ins, 0 real). During a movie the process has no avifil32, msvfw32, iccvid, wdmaud.drv or msacm32 loaded
  (module list of the 32-bit process, credits playing); WINMM.DLL itself stays loaded because SDL3.dll imports it.
- **AVI reader (P12).** The header list (avih, strl: strh, strf), the movi position and idx1 at open, per stream the
  chunks (offset, size, flags) and, for fixed-size samples, each chunk's first sample; data read on demand through
  SDL_IOStream (`bmxPathResolve` path). Everything the game sees is avifil32's, measured with bmxmovietest:
  - AVISTREAMINFOA: the strh fields (the frame rectangle as four int16 in a 56-byte strh, four int32 in a 64-byte
    one), dwSuggestedBufferSize = the largest chunk of the stream (not strh's), szName = `<file name as passed>
    Video #1` / `Audio #1`; only the fixed part and the name up to its terminator are written (the rest of szName
    keeps the caller's bytes).
  - TimeToSample: -1 for a negative time; `time * rate / (scale * 1000)` in 64-bit integers, rounded down when
    rate / scale < 1000 and up otherwise, an overflow of 31 bits gives -1; then clamped to [start, start + length].
    SampleToTime: the sample clamped first, then the same rounding the other way. (Wine's float formulas are off by
    one at exact multiples; the real one is integer.)
  - AVIStreamRead: a start before the stream fails with BADPARAM and leaves the counts alone; else they are zeroed;
    past the end ERROR for a 0-byte buffer, BUFFERTOOSMALL for less than a sample, FILEREAD otherwise. Audio:
    samples -1 = the rest of the chunk, 0 = as many as fit, another negative count = no limit; with a buffer clipped
    to the stream end and the buffer (a negative size: BUFFERTOOSMALL; less than a sample: 0 bytes, OK), without
    one only to size / sample size (not to the end), not at all for size 0. Reads cross chunks. Video: one frame
    per call; with a buffer a negative size is BUFFERTOOSMALL, 0 is ERROR, a too small one BUFFERTOOSMALL with the
    frame's size in the byte count.
  - ReadFormat: copies what fits and returns BUFFERTOOSMALL when the buffer is smaller than the format (size 0:
    OK); the size out is always the format's. GetStream past the last stream: NODATA. Reference counts as
    avifil32's (a stream and a GetFrame object hold the file). A missing file: FILEOPEN.
  - GetFrame: decodes from the last key frame at or before the frame (or on from the frame in the buffer when that
    is nearer) into one persistent 24-bit bottom-up DIB (header + bits, biSizeImage 230400) whose pointer is
    returned; NULL outside the stream; a 0-byte chunk keeps the picture.
- **Cinepak decoder.** The usual bitstream (frame header, strips of 4x4 blocks, V1 / V4 codebooks with full and
  selective updates, vector chunks 0x30 / 0x31 / 0x32), the colour conversion R = Y + 2V, G = Y - U/2 - V,
  B = Y + 2U clamped (C's truncating U/2), which is iccvid's exactly, and two **iccvid quirks**, found from the
  frames that differed and then measured with hand-made frames through ICDecompress:
  1. A selective V1 codebook update whose last flag word is zero overwrites one more entry with the chunk's first
     four bytes as luma and zero chroma: the entry after the last updated one when the word before is not zero,
     else entry 32 * (G - 1) + 1 for the last word G (400 random flag-word sets, all fit). V4 updates do not. In
     the movies this shows as single wrong-looking blocks for a frame or so (acclaim.avi frame 35, nyquist.avi frame
     232: 108 frames of acclaim / intro / nyquist differed before the fix).
  2. A strip copies the codebooks of the strip above it (frame flag bit 0 clear) only when it is an inter strip
     (0x11); a key strip (0x10) keeps its own from the previous frame. ffmpeg copies for both.
  Greyscale codebook chunks (0x24..0x27) are decoded as luma with zero chroma; iccvid differs in some cases there
  (not pursued: no movie of the game has one).
- **Exactness of the decoder:** byte-identical to iccvid for **every frame of all seven files** (6467 frames in
  order, plus 2800 random positions with backward jumps and positions outside the stream), and for 3000 random
  hand-made colour frames (1 to 3 strips of random heights, key and inter strips, full codebooks of 0..256 entries,
  selective updates with zero, single-bit, all-ones and random flag words, all three vector chunk kinds). So no
  tolerance was needed; the maximum per-channel difference is 0.
- **waveOut (P14).** One SDL audio stream per open device in the file's own format (SDL resamples to the device);
  its get-callback (SDL's audio thread) hands the queued buffers over in order and, when a buffer's last byte has
  gone, sets WHDR_DONE (clears WHDR_INQUEUE) and posts MM_WOM_DONE (wParam the device, lParam the header) to the
  window, as winmm's CALLBACK_WINDOW does; MM_WOM_OPEN / MM_WOM_CLOSE are posted too. Until the message queue is ours
  (S10) the messages go through the real PostMessageA, so zrwPlayMovie's PeekMessage loop gets them exactly as
  before. Pause / Restart stop and resume the callback; Reset returns every queued buffer done (each with its
  MM_WOM_DONE) and drops what SDL still holds; Unprepare / Close of a queued buffer: WAVERR_STILLPLAYING; Write of an
  unprepared one: WAVERR_UNPREPARED. BMX_MUTE: the bytes are consumed at the same pace, SDL gets silence. The
  close log line has the bytes written / played / dropped with FNV-1a 64 hashes; avi.c logs the same for the bytes
  AVIStreamRead returned.
- **DrawDib (P13).** GetDC gives a token DC, ReleaseDC takes it back; GetClientRect gives the presented size (the
  mode's in full screen, the window's client size when windowed: what the original's client rectangle was under
  D3D7). DrawDibDraw converts the DIB to RGBA, uploads it and blits it with nearest filtering into a target of the
  presented size at the requested rectangle (a new target starts black), then presents the target through the
  driver (letterboxed, D4) without counting a shown frame, so BMX_SHOT_FRAME / BMX_KEYS / BMX_QUIT_FRAMES numbers do
  not move during movies (as before). Only 24-bit uncompressed DIBs and the token DC (logged once otherwise).
  `BMX_MOVIE_SHOT=<file %d.bmp>` + `BMX_MOVIE_SHOT_FRAME=<n,...>` save the target after the n-th DrawDibDraw and log
  which movie frame it showed.
- **BMX_KEYS `t<ms>` events** (keys.c): a key at a time since the start, for movies, during which no frame is
  shown (`t4000:up` aborts the first startup movie).

Checks (all 2026-10-31):

- `bmxmovietest <game>\bin\movie` (about 7 s): 18.8 million comparisons, 0 differences: per file the open / GetStream
  results (also by type and past the end, and a missing file), AVIFileRelease / AddRef / Release counts; per stream
  StreamInfo (all 0x8c bytes, from a prefilled buffer), Start / Length, ReadFormat (size query, then every buffer
  size from 0 to the format size + 8), TimeToSample for every millisecond from -3 s to 3 s past the end plus
  extremes (INT32_MIN .. INT32_MAX), SampleToTime for every sample from -3000 to 3000 past the end plus extremes;
  audio AVIStreamRead in the game's pattern from start to end (bytes and FNV equal) and 4000 random reads per stream
  (starts around both ends, counts -1 / 0 / random, NULL and small buffers; result, both counts and the whole buffer);
  video Read of every frame with a large, a NULL and a too small buffer; GetFrame as above; the synthetic iccvid
  frames.
- DrawDib: `bmxmovietest --drawdib <file> <frame> <shot.bmp>` draws the real frame with the real DrawDibDraw into a
  640x480 DIB section (memory DC) as zmovieDraw calls it and compares it with our GPU readback (BMX_MOVIE_SHOT) of
  the same frame in the game: **0 of 307200 pixels differ** for 7 frames (acclaim 18 and 181, zaxis 146, intro 652,
  credits 192, 1706 and 3160); DrawDib's 2x is plain pixel doubling (also 0 differences).
- waveOut: `bmxplatformtest` runs the movie's protocol and its error cases (open, early write, prepare twice,
  pause, four writes, a write of a queued buffer, no MM_WOM_DONE while paused, restart, the four MM_WOM_DONE in
  order, two long buffers, unprepare / close while queued, reset, unprepare all, close) on the real winmm and on
  ours with a hidden window: every result, every header's flags and every message (kind, device, header) equal.
- Game runs (`native-run2`, `BMX_MUTE=1`, `BMX_KEYS`): the three startup movies to the end, the intro from Cool
  Stuff's MOVIES list to the end (the device open 105.0 s, 243 buffers), the credits to the end (220.9 s, 1190
  buffers); the bytes written to waveOut equal the bytes AVIStreamRead returned and the bytes played (same FNV-1a 64;
  and the same hash as real avifil32's reads in bmxmovietest: e.g. credits 38984400 bytes, 59d756f13803cada); every
  startup movie and the credits (30 s in) abort on a key (`Movie : Key Abort`; reset drops the rest: played +
  dropped = written); no trap, the save restored each time.
- `bmxplatformtest`: 262055 checks, 0 failures. Twice (of three runs) S7's real-time check of the first tone failed
  with an average step of 10.58 / 10.77 ms against 10 +- 0.5 (dsound.c and that test are unchanged; it runs before the
  waveOut test); the third run passed. A timing flake of the machine, to watch.

Decisions beyond the document: no FFmpeg-style tolerance (exact); iccvid's quirks copied (they are the original
picture); GetClientRect returns the presented size instead of asking Windows (in full screen the original's client
area was the mode under D3D7's exclusive mode, while main.c's window only changes its style and keeps its size); movie
frames are presented but not counted as shown frames; BMX_MOVIE_SHOT instead of BMX_SHOT for movie frames; the
waveOut messages still go through the real PostMessageA until S10; BMX_KEYS got `t` events for the abort checks.

Open: the user's watch test (docs/NATIVE.md "Movies on our decoder and SDL3"): picture, sound, lip sync, abort.
main.c's WASAPI session mute (BMX_MUTE) became redundant for the movies (both stand-ins output silence); removed in
S11 (4f).

### 4e. Status of S10 (2026-10-31, native-only, committed since; the focus test is the user's)

Code: `src/platform/window.c` (the window, the message queue, the USER32 stand-ins, message boxes), the
DialogBoxParamA stand-in in `src/driver/device.c` (it needs the driver's mode list), `src/native/main.c` (WinMain's
job on our stand-ins), `[video]` / `[ui]` keys in config.c. zrwzgl.c, pcinput.c, ngapp.c and zmovie.c are unchanged;
**the hook DLL is unchanged**.

- **The cut.** All 28 USER32 slots: 15 stand-ins (PeekMessageA, TranslateMessage, DispatchMessageA, DefWindowProcA,
  AdjustWindowRect, ClipCursor, PostQuitMessage, ShowCursor, SetWindowTextA, GetWindowLongA, MessageBoxA,
  DialogBoxParamA; GetDC / ReleaseDC / GetClientRect from S9), 13 dead (the 9 of WinMain / the D3D7 driver, plus the
  dialog procedure's EndDialog, GetDlgItem, SendMessageA, SetFocus). The loader says `137 slots filled: 69
  stand-ins, 68 dead, 0 real; real DLLs still used: none`, and `llvm-objdump -p bmxnative.exe` lists SDL3.dll,
  bcrypt.dll, ole32.dll, KERNEL32.dll and the UCRT (no USER32, no WINMM).
- **Entry (WinMain 0x48a460).** It stays unported (int3); main.c follows it step by step, now on our stand-ins: the
  globals at 0x697780 (memset 0x34, title "RenderWare Application", 640 x 480), `-r` parsed in place exactly like
  the original (InitApp gets the edited text), the window (RegisterClassA + CreateWindowExA -> bmxWindowCreate),
  gHinstance, zrwAttachPlugins (RwEngineOpen with the window, the device selection, RwEngineStart: the GPU device
  claims the SDL window), the full screen style, ShowWindow + UpdateWindow (bmxWindowShow), zrwInitApp, the
  PeekMessage loop with the busy-wait frame limiter, `_exit(-1)`. The "already running" box (hPrevInstance != 0)
  never happens on Win32 and is left out. A config `[video] fullscreen = 0` without `-r` sets the window size
  (gWidth / gHeight, 0x697798) before the window is made, which is also the size the game renders at.
- **The `-r` mode pick bug (original, kept).** zrwSelectDevice stores the wanted width into the mode it is
  comparing (0x489e2a `MOV [ESP+0x10],EAX`) before the compare, so only height, depth and the full screen flag are
  checked and the last match wins. On this machine `-r640.480.16` gives 640x480x16 only because no other 480-line
  mode is listed; `-r1024.768.16` gives **1366x768x16** (logged as `driver: mode 11 of 35: 1366x768x16 full screen`).
  The port is faithful (ported code, vanilla); the config path picks the exact mode, so `[video]` is the way to get
  1024x768. A `BMX_FIX` in zrwSelectDevice would be the alternative if wanted.
- **No device dialog (P16).** Without `-r`, zrwSelectDevice calls DialogBoxParamA(0x68): the stand-in sets
  gZglVideoMode (0x672758) and returns IDOK. `[video] fullscreen = 1` (default) picks the full screen mode `width`
  x `height` x `depth` exactly (default 640 x 480 x 16, the dialog's own preselection, so the default equals Enter
  in the vanilla dialog); `fullscreen = 0` the windowed mode 0; a size not in the list falls back to the
  preselection (logged). The dialog procedure is never run. The chosen mode is logged at device start.
- **The window (P15, P17).** SDL_CreateWindow (hidden, resizable) at the client size; the handle the game keeps in
  gZrwWindow is an opaque token (the SDL_Window pointer), which only our stand-ins and the driver take back. The
  procedure gets WM_GETMINMAXINFO (its size lock becomes SDL's maximum / minimum size; AdjustWindowRect is the
  identity, SDL sizes are client sizes) and WM_CREATE at creation. Full screen = SDL's desktop full screen (D4:
  the mode is rendered at its size and letterboxed, the display mode is not changed); in a BMX_KEYS run a borderless
  window of the same size instead, so a scripted run never covers the user's screen. bmxWindowShow shows it and pumps
  at once, so WM_ACTIVATE arrives during "ShowWindow" as on Windows (before zrwInitApp: DirectInput is not up yet,
  the call does nothing, as in the original). ShowCursor keeps the display counter (hidden below 0); ClipCursor(NULL)
  is a no-op; SetWindowTextA sets SDL's title; GetWindowLongA(GWL_HINSTANCE) gives gHinstance (DirectInputCreateEx,
  and the real one under BMX_INPUT_CHECK, gets the module handle as before).
- **The message queue.** PeekMessageA pumps SDL (at most once per millisecond: the main loop polls it in its busy
  wait) and turns SDL's events into the procedure's messages: focus gained / lost -> WM_ACTIVATE WA_ACTIVE /
  WA_INACTIVE, sent at once (SDL's keyboard focus, which the DirectInput and DirectSound stand-ins already used, is
  updated by then); a close request (close box, Alt+F4) -> WM_CLOSE, sent; DefWindowProcA(WM_CLOSE) does
  DestroyWindow's part: WM_ACTIVATE inactive, WM_DESTROY (the procedure's ClipCursor + PostQuitMessage), WM_NCDESTROY,
  the window hidden; key events -> posted WM_KEYDOWN / WM_KEYUP (WM_SYSKEY* with Alt or F10) with Windows' lParam
  (scan code from the DIK table, extended bit, previous state); in a BMX_KEYS run the script's key edges instead.
  bmxWindowPost (any thread; a spin lock) takes waveout.c's MM_WOM_OPEN / DONE / CLOSE, so zrwPlayMovie's loop refills
  exactly as before. Posted messages come before key messages; WM_QUIT comes when nothing else is queued, whatever
  the filter (as documented for PeekMessage). The driver no longer flushes SDL's events at every present.
- **Esc-quit is unreachable in the shipped game.** zrwMainWndProc quits on WM_KEYDOWN Esc (the only 0x80 entry of the
  key table 0x54dce8) until gZrwKeysLocked (0x6977b0) is set, and ngifInitialize sets it (zrwSetParameter(0x14)) at
  its start, inside zrwInitApp, before the first message is ever pumped (the startup movies' loop or the main loop).
  So no Esc reaches the procedure unlocked; ours behaves the same (checked below). Not a port difference.
- **Message boxes.** MessageBoxA -> SDL_ShowMessageBox: the buttons of the MB_ type in Windows' order (OK, OK /
  Cancel, Abort / Retry / Ignore, Yes / No / Cancel, Yes / No, Retry / Cancel, Cancel / Try Again / Continue) with
  Windows' return codes, MB_DEFBUTTONn as the Enter default, Cancel (or OK alone) on Esc, the icon (hand -> error,
  exclamation -> warning, else information), the owner as parent; a box closed without a button gives the Esc button
  (or the default). A BMX_KEYS run answers without a window as keys.c did (Yes / No -> Yes, Yes / No / Cancel ->
  Cancel, else OK; keys.c's shim is gone). The quit prompt: P17's `[ui] confirm_quit` note above. SDL's box is
  modal: the timer and audio threads run on; SDL's focus (hence the DirectSound focus mute) follows it live, the
  procedure's WM_ACTIVATE pair comes after the box closes.
- **Dev checks.** BMX_INPUT_CHECK and BMX_SOUND_CHECK hand the window to the real dinput.dll / dsound.dll:
  `bmxWindowNative` gives them the HWND behind the token.

Checks (all 2026-10-31, from `native-run2`, `BMX_MUTE=1`, the save restored after each run, all without a trap or a
dead import):

- ProQuest into Vail Trails and riding (`-r640.480.16`, frame 1500 shows the rider at 1:50); QUIT from the main menu
  with the default config (`window: quit prompt answered Yes without a box`, exit code -1) and with
  `confirm_quit = 1` (`BMX_KEYS: message box "Do you want to quit the game?" answered 6`, exit -1); window close
  (WM_CLOSE posted to the window from outside, as the close box does: `window: destroyed (WM_CLOSE -> WM_DESTROY),
  WM_QUIT posted`, exit -1); movies on (`t3000:esc` aborts ACCLAIM through DirectInput (`Movie : Key Abort`, no
  quit), ZAXIS and INTRO play to the end with 292 MM_WOM_DONE through our queue and the same bytes as S9's run
  (INTRO 243 buffers, 7938000 bytes), Esc in the main menu then: no quit); Esc in a level opens PAUSE; full screen
  1024x768x16 from the config (exact mode 5), windowed 640x480 and 1024x768 from the config (mode 0, the window and
  the camera raster at that size), all riding at frame 1500; `-r1024.768.16` (the pick bug above).
- `BMX_SOUND_CHECK=1` ProQuest run: `soundcheck: 13292 calls compared, 0 differences`, `RESULT clean`.
  BMX_INPUT_CHECK is off in scripted runs and needs the focus: the user's test (below); its cooperative-level path
  with a real HWND is in bmxplatformtest.
- `bmxplatformtest`: 262192 checks, 0 failures; new: the waveOut script now reads our queue (still equal to winmm's:
  61 results, flags and messages); message boxes (quit prompt skipped with the option off, shown with it on: Yes /
  No, default No, no Esc button, answers Yes / No / closed; other texts and styles and the assert box shown;
  scripted answers); the queue (filters, order, WM_QUIT last and only once, 100 posts from another thread in order);
  the window (WM_GETMINMAXINFO then WM_CREATE, the size lock, the HWND, the title, GWL_HINSTANCE, the cursor counter,
  AdjustWindowRect, focus -> WM_ACTIVATE, the script's keys as WM_KEYDOWN with lParam 0x001e0001 / 0x01480001,
  close -> WM_CLOSE, WM_ACTIVATE 0, WM_DESTROY, WM_NCDESTROY, WM_QUIT; nothing for a destroyed window).
  `bmxmovietest`: 18826111 checks, 0 failures.
- The user's test: `native-run\focustest.ps1` (docs/NATIVE.md "Window and messages on SDL3").

Decisions beyond the document: the token is the SDL_Window pointer; the dialog stand-in lives in the driver; a
windowed config sets the window and render size; BMX_KEYS runs get a borderless window for "full screen" and are put
at the bottom of the z order through the real SetWindowPos (loaded at run time, no import); the script's keys also
become WM_KEYDOWN / WM_KEYUP; PeekMessage pumps SDL at most once per millisecond; focus by SDL's keyboard focus (no
WA_CLICKACTIVE: the procedure treats it as a deactivation, and the game's poll re-acquires anyway); log lines for
focus changes, the close and the chosen mode; the `-r` pick bug kept.

### 4f. Status of S11 (2026-10-31, native-only, committed since; the focus test is the user's)

The import table of `bmxnative.exe` is now SDL3.dll, bcrypt.dll, KERNEL32.dll and the UCRT's api-ms-win-crt-* sets
(13 DLLs; no ole32, no WINMM, no USER32 of ours). **The hook DLL is unchanged.**

- **main.c.** No Win32 call left, and no `windows.h`: `main(argc, argv)` builds WinMain's lpCmdLine from argv
  (joined by single spaces, an argument with a blank or an empty one re-quoted) instead of GetCommandLineA;
  gHinstance is the linker's `__ImageBase` (the exe's own base is its HINSTANCE, what GetModuleHandleA(NULL) gave);
  the end is `_exit(-1)` like the original's, not ExitProcess. The WASAPI session mute (ole32 / mmdevapi) is gone.
  Reasoning: BMX_MUTE is carried by the audio stand-ins (dsound.c's mixer and waveout.c output silence, 4c / 4d),
  so the session mute added nothing. Its "unmute a muted session" half is not kept either: Windows remembers an
  app's mute per exe, but only the old build set it, and every old non-BMX_MUTE run cleared it again; **a machine
  whose last run was an old BMX_MUTE=1 run may still have bmxnative.exe muted in the Windows volume mixer: unmute it
  there once** (the new build never changes the session).
- **keys.c** has its own DIK constants (12, the keys the script names); **driver/gpu.c** has no `windows.h`
  (`MAX_PATH` -> a 1024 byte buffer, BMX_QUIT_FRAMES exits through `_Exit(0)`); CMakeLists no longer links winmm
  and ole32 for bmxnative (nothing imported them; bmxplatformtest still links ole32 / dinput for the real-DLL
  comparisons).
- **Kept Win32 (on purpose):** window.c's real SetWindowPos for BMX_KEYS runs (dev aid, loaded at run time through
  bmxRealProc, no import); the dev checks BMX_INPUT_CHECK / BMX_SOUND_CHECK / BMX_FILE_CHECK and bmxplatformtest,
  which load real DLLs; loader.c / trap.c / range.S / crt.c (VirtualProtect, the vectored handler, LoadLibrary /
  GetProcAddress, the .orig section: KERNEL32); bcrypt for the SHA-256 of NgBMX.exe until X1b (docs/PORTING.md), which
  stopped reading the exe. USER32.dll is in the process
  because SDL3.dll imports it.
- **Import check (tools/check.sh).** After the build, for each of `build/native64/OpenFreestyleBMX.exe` and `build/native32/OpenFreestyleBMX.exe`
  that exists (originally native32's `bmxnative.exe` only), `llvm-objdump -p` lists the imported DLLs (`OBJDUMP` overrides the tool) and any outside this
  allowlist fails the gate (also when no DLL name can be read):

  | DLL | Why |
  |---|---|
  | `SDL3.dll` | window, input, audio, GPU: the platform layer |
  | `KERNEL32.dll` | the loader / trap / range machinery and the CRT layer (VirtualProtect, the vectored exception handler, LoadLibrary / GetProcAddress for the dev checks and bmxRealProc, threads, files) |
  | `api-ms-win-crt-*.dll` | the UCRT's forwarder sets that the mingw-w64 CRT imports (stdio, heap, string, ...) |

  Tested: the real exe passes; a fake extra `USER32.dll` in the listing fails; an empty listing fails.
- **Held key at focus gain (the S10 focus test's open point, fixed).** DirectInput reads the keyboard's real state, so
  a key held at activation (the Enter that started the run: DIK 0x1c for 0.18 s in `focustest-fs640.log`, 178
  mismatches) reads as down at once. SDL3 3.4.18 does not resync its keyboard state on focus gain (the user's log is
  the evidence; SDL also drops a release for a key it thinks is up, so an SDL-side state could never end the key).
  Now `bmxInputFocusGained()` (input.c, called by window.c's `setActive` before WM_ACTIVATE is sent, i.e. during
  ShowWindow, before DirectInput exists) asks the OS which keys are down and remembers them as a seed; every
  keyboard read ORs the seeded keys into the SDL state and retires a seed when SDL reports the key (a new press, SDL
  tracks it from then on) or the OS says it was released (checked per read, only for seeded keys); focus lost
  forgets the seed. The OS probe is **Windows-only**: `GetAsyncKeyState` through `MapVirtualKeyW(MAPVK_VSC_TO_VK_EX)`
  (the DIK is the set-1 scan code, 0x80 for E0 keys), both loaded from USER32 at run time (no import);
  `bmxInputSetKeyProbe` replaces it (tests; another platform would add its own). Like DirectInput it posts no
  WM_KEYDOWN for the held key, and a scripted (BMX_KEYS) keyboard does not use the seed. Unit-tested in
  bmxplatformtest (`testHeldKeys`: seeding, release retires, SDL takes over, focus lost forgets, unmapped DIKs never
  seeded). The real focused behaviour is the user's test (`focustest.ps1`: hold Enter while the game starts, then
  check that `inputcheck:` reports no mismatch for DIK 0x1c).
- **The x87 RwMatrixMultiply routine (0x54b038, _rwcseg) is ported**: `rwMatrixMultiplyX87` in rw/math.c, a BMX_ADDR
  function whose body is `bmxX87MatrixMultiply` (compat/x87.h), the original's instruction sequence in inline x87 asm
  (same loop over three rows of the rotation part, the position row with b's position added, same additions
  order), so the result follows the x87 control word the camera update sets (24 bit, round down) exactly as the
  original's did; the non-hook fallback is plain float arithmetic. Oracle: test_rwmath.c "rwMatrixMultiplyX87
  (camera mode)" and "(default mode)" (random and edge matrices, huge values, NaN / infinity / denormals,
  out aliasing a or b), 0 differences. It is no longer in kTextKeep (docs/NATIVE.md).
- **kTextKeep after S11.** The incremental-link thunks (0x401005..0x402617) stay: function pointers in the data
  point at them and they jump to the function's own address (ported or trap), independent of dinput. The RpWorldDestroy
  stub (0x4ba9c0) stays: its 4 bytes are the entry the stub's `mov eax, [esp+4]` hands to the ported body, with
  nothing to port (a hook jump would overlap the next function). The dinput tables (0x4afc60..0x4b0f20) are only read
  by the real DirectInput, i.e. under BMX_INPUT_CHECK (our stand-in takes just the format's data size): they are now
  copied only when BMX_INPUT_CHECK is set (checked: a ProQuest ride without them has no trap); in normal runs the
  range is int3 like the rest of `.text`.
- **Checks** (2026-10-31, `native-run2`, `BMX_MUTE=1`, `BMX_SOUND_CHECK=1`, the save restored after each run, no trap,
  no dead import): ProQuest ride (`-r640.480.16`, 1600 frames, `soundcheck: 7300 calls compared, 0 differences`,
  `RESULT clean`); QUIT from the main menu (`window: quit prompt answered Yes without a box`, exit -1, soundcheck
  clean); movies on (`t3000:esc` aborts ACCLAIM, ZAXIS and INTRO play to the end with the same hashes as in 4d:
  INTRO 243 buffers, 7938000 bytes). `bmxplatformtest` 0 failures (its "tick step" checks of the GetTickCount
  stand-in are timing based and fail on a loaded machine: re-run), `bmxmovietest` 18826111 checks, 0 failures.
  Hook build, `tests/oracle/run.sh -k rwMatrixMultiplyX87`, void_eax / selfref / extend audits clean (argcount's
  findings are the existing ones).

Decisions beyond the document: the seed is kept in the platform layer (not in the device) because the activation
comes before DirectInput exists; `__ImageBase` for the HINSTANCE; `_exit` / `_Exit` for ExitProcess; the dinput tables
only under BMX_INPUT_CHECK.

## 5. Windows behaviour that is hard to reproduce

- **DirectInput's view of a pad**: button numbering comes from the HID driver per device (and Steam Input can present
  a DualSense as an Xbox 360 pad); the dead zone/saturation rescale is DI's own integer math. P3/P4 approximate it per
  family; only the dual-read (S5) proves it, per pad model.
- **DirectSound's mixer** (software since Vista): its resampler, volume curve rounding, pan law and play-cursor
  granularity are undocumented. S7 measured the cursors (exact model), the volume / pan law (0.1 dB steps, 1/65536
  truncation) and the focus muting in-process (4c); the resampler remains SDL's, so the mixed output matches within a
  tolerance only. The bytes the game writes are bit-exact by construction.
- **Timer-thread concurrency**: winchanPump runs on winmm's thread while the game thread changes channels (plain
  flag, no lock, in winchan.c). Races exist in the original too; P11-A keeps the same shape but not the same
  interleavings. Audio is not part of the deterministic simulation, so golden traces are unaffected.
  **Review of S3 (src/platform/winmm.c, Opus, 2026-10-31)**; the four fixes are listed in 4a.
  (a) timeKillEvent's join cannot deadlock the game: bmxSoundClose kills the timer first and holds no lock or
  critical section (the game thread takes winstrm's sections only inside winstrm calls), and the callback takes only
  winstrm's sections and the gWinchanLock flag. Without TIME_KILL_SYNCHRONOUS winmm may still run one callback after
  timeKillEvent returns; the join rules that out, a subset of the original's interleavings (the pump never touches
  a closed winchan). (b) The handshake holds: `finished` / `detached` / `joining` / `used` change only under the
  lock; a detached slot is touched only by its own thread, which frees it under the lock as its last act; reap()
  joins only finished, non-detached, non-joining one-shots; the double join through a concurrent kill or reap was
  the real bug (fixed). (c) The statistics are written only by the timer thread and read after SDL_WaitThread
  (a join orders them) or by the thread itself (self-kill): no race. (d) The FP environment equals the original's:
  winmm's timer thread is a new Windows thread as well, so both start with Windows' default x87 control word 0x27f
  (53-bit, exceptions masked) and MXCSR 0x1f80; a D3D7 change of the x87 precision would only affect the thread that
  created the device. bmxLog is two vfprintf calls on UCRT streams, each locked per stream (lines can interleave
  between threads, never tear); crt.c's malloc is the UCRT's (thread-safe). (e) Cost: the callback averages 0.015 to
  0.036 ms; SDL_DelayPrecise sleeps and spins only the last part of each 16 ms: acceptable, a power seam could use
  SDL_DelayNS later. (f) gWinchanLock stays a plain flag as in the original (aligned 32-bit stores are atomic on
  x86; the ported code reads it once per pump).
- **Timer resolution**: timeGetTime has 1 ms resolution only while the multimedia timer holds the system timer at
  1 ms (the game calls timeEndPeriod right after timeSetEvent); GetTickCount steps ~15.6 ms (movie clock). SDL clocks
  are finer: the frame limiter and movie frame choice can differ by a tick. Not part of the simulation (fixed tick).
- **iccvid's YUV->RGB rounding** and DrawDib's stretch for non-integer ratios: match by testing (S8/S9), only
  2x is checked pixel-exact. Done (4d): the conversion is exact (R = Y + 2V, G = Y - U/2 - V, B = Y + 2U, clamped);
  iccvid's two bitstream quirks (the zero-tail selective V1 update, no codebook copy for key strips) are copied;
  greyscale codebook chunks are not matched in every case (no movie of the game has one). DrawDib's 2x is plain
  pixel doubling; other ratios are our nearest-neighbour blit, unchecked (the game draws only 2x at 640x480).
- **Exclusive full screen**: D3D7 changed the display mode; SDL3-DRIVER.md D4 already decided not to.
- **DPI virtualisation** of the DPI-unaware original window, and CW_USEDEFAULT placement: cosmetic.
- **x87 control word**: MSVC's CRT starts at 53-bit precision; D3D7 may switch the x87 to 24-bit at device
  creation unless the app passed DDSCL_FPUPRESERVE/FPUSETUP. To check once in the hooked original (read the control
  word outside a camera update): if D3D7 left it at 24-bit, the ported x87 matrix multiply (0x54b038, S11) and the
  golden-trace baseline depend on D3D7, not on the game.
- **Case-insensitive, backslash paths** and a working directory equal to the install folder: free on Windows,
  P18's resolver for other platforms.
- **Modal message boxes** keep the timer thread (audio) running while the game thread waits; SDL_ShowMessageBox is
  modal the same way on Windows; on other platforms it must not block the audio thread either.
