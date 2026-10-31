# Platform layer: Windows APIs -> SDL3 (P4, "Windows first")

Status 2026-10-31: **decided; S1-S6 implemented** (sections 4a, 4b; S5's check with real pads is the user's). The user took the recommendation for every decision
P1..P19 (A unless said otherwise). Order: S1-S4 (done, S3 reviewed), input + d-pad (S5, S6: done), then the rest.
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
types), driver/gpu.c (wraps the game's HWND for SDL), range.S (the .orig section). The loader/trap/range parts are
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
| S11 | Remove the remaining Win32 from main.c / keys.c / driver (WASAPI mute -> no audio device), import check in tools/check.sh | ~100 | Sonnet | `llvm-objdump -p bmxnative.exe`: only SDL3, UCRT, KERNEL32 (+ BCRYPT until an own SHA-256) |
| S12 | Opt-in seams: profile saves + asset folder (P18), hot-plug (P5), remap (P8), window modes (P17) | ~400 | Sonnet | Each option off: S1..S10 checks unchanged; on: feature test |

### 4a. Status of S1-S4 (2026-10-31, native-only, not committed)

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

### 4b. Status of S5 and S6 (2026-10-31, native-only, not committed; the real-pad check needs the user)

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

S1-S6 are done apart from the user's real-pad check. S7 and S8 are the large ones.

## 5. Windows behaviour that is hard to reproduce

- **DirectInput's view of a pad**: button numbering comes from the HID driver per device (and Steam Input can present
  a DualSense as an Xbox 360 pad); the dead zone/saturation rescale is DI's own integer math. P3/P4 approximate it per
  family; only the dual-read (S5) proves it, per pad model.
- **DirectSound's mixer** (software since Vista): its resampler, volume curve rounding, pan law and play-cursor
  granularity (about 10 ms steps) are undocumented. The bytes the game writes are bit-exact by construction; the mixed
  output can only be matched within a tolerance.
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
  2x is checked pixel-exact.
- **Exclusive full screen**: D3D7 changed the display mode; SDL3-DRIVER.md D4 already decided not to.
- **DPI virtualisation** of the DPI-unaware original window, and CW_USEDEFAULT placement: cosmetic.
- **x87 control word**: MSVC's CRT starts at 53-bit precision; D3D7 may switch the x87 to 24-bit at device
  creation unless the app passed DDSCL_FPUPRESERVE/FPUSETUP. To check once in the hooked original (read the control
  word outside a camera update): if D3D7 left it at 24-bit, the kept x87 code (0x54b038) and the golden-trace
  baseline depend on D3D7, not on the game.
- **Case-insensitive, backslash paths** and a working directory equal to the install folder: free on Windows,
  P18's resolver for other platforms.
- **Modal message boxes** keep the timer thread (audio) running while the game thread waits; SDL_ShowMessageBox is
  modal the same way on Windows; on other platforms it must not block the audio thread either.
