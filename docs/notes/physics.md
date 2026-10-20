# physics.c notes

Findings from the first analysis pass (2026-10-19). Treat them as leads to verify against the
disassembly, not as facts.

## Rider state

- Riders: array of `PhysRider` (0xf70 bytes) at `0x691f40`. The player count byte is at `0x69621c`.
- Layout: `tools/structs/physics.spec`, generated into `src/bmx/com/action/physics_types.h`. Add fields
  to the spec, never by hand, and regenerate:
  `python3 -I tools/structs/gen_struct.py tools/structs/physics.spec src/bmx/com/action/physics_types.h BMX_COM_ACTION_PHYSICS_TYPES_H '"zge/zmath.h"'`
- State id at `+0x3dc`, surface kind at `+0x3dd`, contact pointer at `+0x488`.
- Contact record size by surface kind:
  - kinds 0, 4–8, 10, 11, 13 and 14: 0x88 bytes
  - kinds 1, 2, 3 and 9: 0x38 bytes
  - kind 12: 500 bytes
- Raycast result: 0x44 bytes, global at `0x56c128`.

## State machine

- Per-state callback tables, 7 entries each:
  - exit at `0x557898`
  - enter at `0x5578b4`
  - update A/B at `0x5578d0` and `0x5578ec`
  - per-frame at `0x557908` and `0x557924`
- `0x420cb0` is set-state. Its `from == 0xff` path calls the pointer at `0x5578a0`.
- Names leaked by asserts and labels: `0x422e50` bmxphysStateInitAir, `0x425940` bmxphysUpdatePositionRoll,
  `0x428df0` bmxphysUpdatePositionAir.
- Tunables: `.data` `0x5577f0–0x557890` are mutable globals, so use `ORIG_VAR`, not literals. `.rdata`
  floats `0x54d01c–0x54d19c` are constants. Check every operand's width (float or qword) per use.

## Helpers (not yet ported; candidate names for zmath.h)

| Address | Name |
|---|---|
| 0x481120 | zvec3Zero |
| 0x481130 | zvec3Add |
| 0x481210 | zvec3Scale |
| 0x4812f0 | zvec3Negate |
| 0x481390 | zvec3RotateY |
| 0x481540 | zvec3Dot (returns double) |
| 0x4815b0 | edge cross |
| 0x481640 | matrix transform point |
| 0x481880 | identity |
| 0x481930, 0x481a00, 0x481ad0 | rotate X/Y/Z |
| 0x481ba0 | rotate by Euler angles |
| 0x481c60 | copy |
| 0x481cf0 | multiply |
| 0x481e90 | transpose |
| 0x480e60 | zmathAsin |
| 0x464b50 | zactorGetPartPosition |

## Not physics

- `0x41f930` / `0x41f960` show and hide HUD text.
- `0x41fa20` registers the debug-draw callback (`0x40252c`, stored with `BMX_FNPTR`).
- `0x4203a0` is the debug line-draw callback.
