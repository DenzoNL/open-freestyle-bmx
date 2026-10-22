You are porting a slice of the Dave Mirra Freestyle BMX (PC, 2000) game from Ghidra output to clean C17. Every ported function is verified bit-for-bit against the original executable running in an emulator (Unicorn). You work in your own git worktree of the repo (your current working directory); other agents port other slices in parallel in their own worktrees.

Read these first, fully, and follow them exactly:
1. CONTRIBUTING.md — the porting contract (conventions, the x87 floating-point model, tests, style). The FP section is critical: translate FP code from the disassembly, `double` = value kept in an x87 register, `float` = value stored to memory.
2. Finished, verified examples: src/zge/zquat.c (+ .h; FP-heavy), src/zge/zmem.c, src/zge/zstring.c, src/zge/zlist.c (+ headers), src/zge/zmath.h, src/zge/zdebug.h, src/compat/orig.h (ORIG_FN, ORIG_VAR, BMX_FNPTR, BMX_ADDR), src/compat/crt.h.
3. Test framework + examples: tests/cases/dt.h, tests/cases/dt.c, tests/cases/test_zquat.c, tests/cases/test_zmem.c, tests/cases/dt_zmem.h.
4. Your work packet (absolute path given below). It lists every function with pseudo-C, call graph and port status, data refs, and full disassembly for FPU-using functions. It is large: read it in chunks (Read with offset/limit) as you go, module by module, rather than all at once. Per-function JSON bundles with full disassembly for every function: <repo>/work/bundles/<addr>.json — consult the disassembly whenever pseudo-C is ambiguous (calling convention, return width AX/AL, signedness, struct offsets, spills).

Shared resources live outside your worktree (they are gitignored): the original exe is <repo>/work/NgBMX.exe, bundles and packets are under <repo>/work/. Always export BMX_EXE=<repo>/work/NgBMX.exe for test runs. tools/rdata.py takes the exe path as first argument.

First, make sure your worktree contains the latest main: run `git log --oneline -1 main` and, if your HEAD is older, `git merge --ff-only main` (no commit needed). Do not copy files from the main checkout.

Commands (from your worktree root, always through the nix devshell; the default shell `nix develop` works):
  nix develop -c cmake --preset hook                 (once, creates build/hook)
  nix develop -c cmake --build --preset hook
  BMX_EXE=<repo>/work/NgBMX.exe nix develop -c tests/oracle/run.sh -k <name> [-v] [-n 2000]
  BMX_EXE=<repo>/work/NgBMX.exe nix develop -c tools/check.sh      (full gate: format, build, clang-tidy, all tests)
  nix develop -c clang-format -i <files>
  nix develop -c python3 -I tools/rdata.py <repo>/work/NgBMX.exe 0x54d048 ...
  python3 -I tools/progress.py        (updates data/functions.csv from BMX_ADDR annotations)

Task: port ALL functions in your packet with BMX_ADDR annotations, real names and types (infer struct layouts from offsets; name after behaviour; keep names the original leaks via assert strings or community labels where sensible), and write differential tests covering every function (tests/cases/test_<module>.c). Put code in the mirrored path of the original source file (e.g. C:\zaxis\Bmx\src\com\action\physics.c -> src/bmx/com/action/physics.c). The packet's file assignment for rows marked `affinity`/`edge` is a guess: if a function clearly belongs elsewhere, still port it but put it in the right file (or keep it with a `/* TODO(file): probably belongs to ... */` comment). Do not skip functions.

Struct layouts shared between slices are generated: edit the spec under tools/structs/ (add fields at their offsets; unknown gaps stay padding) and regenerate the header with tools/structs/gen_struct.py — never hand-edit generated headers. Parallel agents edit the same spec; add only what you need.

Game state: much of this code reads/writes global structs (rider, game, actor tables). Define proper struct types for what your slice touches (only the fields you can justify; use `uint8_t padN[...]` for unknown gaps and `_Static_assert` offsets/sizes you rely on), access exe globals through ORIG_VAR, and in test setup() populate state deterministically (randomised + edge values; zero-filled is a weak test). Callees outside your slice that are unported stay as ORIG_FN (declare them in the owning module's header under the mirrored path; create the header if needed). Stub logs: tests compile at -O0, so everything a stub records really is compared — pass every logged word through dt_canon() (emulated-stack pointers differ between the original and the port) and never log uninitialised bytes (struct padding, unset locals). For callees that need a loaded level / renderer / other missing state, use dt_stub() (tests/cases/dt.h) — it stubs the callee on both the original and our side for one iteration; never patch code by hand. A stub that replaces a ported callee must stay within that callee's real return range (e.g. only 0/1 for a boolean): clang may optimise the caller on what the callee can return, even across noinline. Seed with dt_seed/begin before drawing random values in setup(). If a function's test would need state that is impractical in the emulator (e.g. a live RenderWare device, files on disk, Win32 calls the harness traps), still port the function, write the best test you can (e.g. covering early-out/assert paths), and list it in your report.

Rules:
- Other agents run tests on this machine at the same time: never use pkill/killall on oracle.py or cmake; stop only processes you started, by PID.
- Before naming a function, run `grep -rn 0x<addr> src` and reuse a name that is already in use for that address.
- Tolerance 0 ULP. Never add tolerance or skip cases to make tests pass; find the real difference (the harness prints differing addresses and values; for FP it is almost always a spill/rounding or operand grouping you modelled wrong — re-read the disassembly).
- Do NOT run the game, Wine, or anything that plays audio or video. Only the emulator harness.
- Do NOT git commit or push. Leave all changes in your worktree.
- Do not modify tests/oracle/oracle.py or tools/ unless genuinely broken (then minimal fix + report). Shared files (src/compat/*, src/zge/*, tests/cases/dt.*, CONTRIBUTING.md, .clang-tidy) may only receive minimal additive edits (e.g. new ORIG_FN lines, a dt_init entry); never rewrite or reformat them — other agents edit them too.
- Keep the quality of the examples: concise comments saying what a function does and any surprise or original quirk; no narration.

Done means: every function in the packet ported with a DT_CASES entry, `tools/check.sh` passes end to end at 0 ULP, `python3 -I tools/progress.py` run. Then verify your tests have teeth: introduce ~3 deliberate small bugs in different functions, confirm each is caught, revert them.

Final report (under 400 words): functions ported / total; any not passing and why; functions relocated or misassigned (addr -> suggested file); struct types defined (names + what they are); test gaps (functions whose tests are weak and why); edits to shared files; harness/tool problems; hardest functions; approximate number of build/test iterations. Be factual; say plainly if something is unfinished.
