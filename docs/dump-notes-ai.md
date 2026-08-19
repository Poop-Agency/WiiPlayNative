# AI hunt in main.dol — what has been ruled out

Written so the dead ends are not walked twice. Every entry here was checked by
disassembling, not inferred. Addresses are VMAs in
`/mnt/stockage/ROM/WII/wii_play_extracted/sys/main.dol`.

## Refuted: functions previously labelled "AI"

**`0x8025a83c` — screen projection for HUD, not steering.** Earlier notes
called this "probably the steering, the biggest function in the tank TU". It is
not. It builds a point 11.0 units above the tank, hands it to the view manager
`[r13-25064]` through `0x80256e7c`, then divides the result by half of two u16
values read from a pair of tables 4 bytes apart at `[r13-32408]`/`[r13-32404]`,
both indexed by a view counter at `[r13-28440]`:

    lwz 0, -28440(13)      ; current view index
    addi 5, 13, -32408     ; u16 table A
    lhzx 5, 5, 0           ; A[index]
    addi 3, 13, -32404     ; u16 table B (A + 4)
    lhzx 0, 3, 0           ; B[index]
    rlwinm 5,5,31,16,31    ; A >> 1
    rlwinm 3,0,31,16,31    ; B >> 1
    lis 4, 17200 / lfd 3, -18448(2) / fsubs / fdivs

`0x43300000` plus the 2^52 constant is the Metrowerks unsigned-int-to-double
trick, so the halved values are converted and used as a divisor. Two u16s four
bytes apart, same index, halved, used to normalise a projected point: viewport
width and height, halved to the screen centre. This places a marker above a
tank. It is presentation.

**`0x8025bdc8`, `0x8025bcc4`, `0x8025bd54` — distance fade and effect placement.**
`0x8025bcc4` copies the global Vec3 at `0x80453510` into `[this+0x48..0x50]`,
measures the distance from it to the tank via `0x800e82e0`, and stores the
result in `[this+0xDC]` with a reference maximum in `[this+0xE0]`.
`0x8025bdc8` divides those two, clamps to `[0, 1]`, feeds the ratio to an object
at `[this+0x1CC]`, and — when the byte `[this+0x101]` is set — places an effect
at scale 1.6 through vtable slots `+0x78`/`+0x7C`/`+0x88`. A clamped
distance ratio driving a visual: LOD or fade.

**`0x8026a300` — camera framing.** A weighted sum over an array of 44-byte
elements at `[this+0x64]`, count `[this+0xF4]`, weights from a 4-float table at
`0x8033d798`, accumulated onto the same global point `0x80453510`. Only the x
and z components are blended, never y, so it is a horizontal average of
positions. Framing, not decision-making.

**`0x8025c614` — tank init.** Largest tank vtable override (slot `+0xc4`,
154 instructions). Writes `[this+0x90] = 16.0`, `[this+0x9C] = 9.0`, scale 1.6
into `[this+0x10..0x18]`, `[this+0xB4] = 11.0 * 1.6`, and copies a table from
`0x8045354c`. Note `[+0x90] = 16.0` here, while `Tank::collide` at `0x8025a3a4`
hardcodes 15.0 — the two numbers have not been reconciled.

## Retracted claim: field 22 is not confirmed as speed

An earlier note said "field 22 (speed) is read by the movement code at
`0x8025bd38`, confirming the mapping by usage". That is wrong. `0x8025bd38`
reads `[[[this+0x198]+8]+12]` — a float at offset 12 of a sub-object reached
through the holder at `+0x198`. It is not the 168-byte parameter record, which
is only ever indexed with `mulli ..., 168`. The field-22 mapping now rests on
the `GetTankParams` register trace alone and should be treated as unconfirmed.

## Where the AI is not

The tank vtable at `0x80375870` was enumerated and every entry measured
forward from its own address to the next `blr`. The tank's own overrides are
init, collision, bounding-box and presentation; the largest is the 154-instruction
initialiser above. **There is no large per-frame decision virtual in the tank
vtable**, which argues the enemy logic lives in a separate controller rather
than in a tank method.

`Tank::init` at `0x8025b010` does not fan the record out to sub-objects as
previously guessed. The block at `0x8025b1c4..0x8025b2c4` that looked like a
huge argument list is a by-value struct copy: every load from `r1+80..244` is
stored to exactly `+168` higher, which is the record size. All five
AI-candidate fields (34, 35, 38, 39, 41) appear only inside that copy.

`0x8026bfd4` (171 instructions) looks up the record itself by type index —
`mulli r4, r4, 168` over `[[r13-25008]+0x34]`, 21 iterations of an 8-byte copy —
and stores the type at `[this+0x7C]`. Unlike `Tank::init`, which receives the
record by value, this class fetches it. Its two callers are `0x80268d50` and
`0x80268db0`. Not yet identified; the most promising remaining thread.

The tilemap getter at `0x801bfd44` has only two callers, the field builder and
the spawn dispatch, so whatever the AI is, **it does not read the tilemap.**

## Proven support facts

- `0x800e82e0` is `VEC3Length`: `psq_l` loads (x, y), `ps_mul` squares the pair,
  `ps_madd` folds in z², `ps_sum0` adds the halves, then `frsqrte` with one
  Newton step. This is the selftest case in `tools/dol.py`.
- `0x80453510` is a fixed world point, used both as the LOD distance reference
  and as the camera blend origin.
- `[r13-25008]` holds the parameter blob; `+0x34` is the array base.

## Tool limits that produced wrong readings

- **llvm-mc cannot decode Broadway paired singles.** No `mcpu` value helps
  (`750cl`, `gekko`, `broadway` are all unrecognised). Before this was fixed,
  every `ps_*` word printed as `lq` or `<bad>`, silently hiding the arithmetic
  in exactly the float-heavy functions worth reading. `tools/dol.py` now decodes
  them itself; `python3 tools/dol.py selftest` pins it.
- **`bounds()` guesses the start.** It walks back to `stwu r1, -N(r1)`, but
  Metrowerks schedules loads ahead of the prologue, so a function beginning with
  an `lfs` makes the walk sail into the previous function. This is how
  `0x8025a018` was reported as a 392-instruction function when it is a 26-
  instruction bounding-box helper. `fn` now warns, and prints the forward
  distance to the next `blr`, which is the reliable measure.
