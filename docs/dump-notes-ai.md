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

# Found: real AI mechanics

Everything below was decoded from the listing and is citable. This is the
first material actually usable for a 1:1 AI.

## The RNG

`0x8025ca64` and `0x8026bcc4` inline the same generator. It is two generators
combined, not the single LCG usually assumed:

    LCG   : [r13-25800] = [r13-25800] * 0x41C64E6D + 12345
    LFSR  : x = [r13-25796]; if (x & 1) x ^= 0x00011020; x >>= 1; [r13-25796] = x
    out   : (LCG ^ LFSR) & 0xFFFF

`0x41C64E6D` is built with `lis 0x41C6` + `addi 0x4E6D`, which is why searching
`mulli` for LCG multipliers found nothing.

## AI timers: min + rand % (max - min)

Object A (see below) carries countdown timers in frames, each re-rolled from
its own min/max pair when it reaches zero. Two are decoded:

| timer | min | max | re-roll site |
| --- | --- | --- | --- |
| `[A+0x110]` | `[A+0x28]` | `[A+0x2C]` | `0x8026bd14` |
| `[A+0x118]` | `[A+0x54]` | `[A+0x58]` | `0x8026bd98` |

The decrement-and-fire shape at `0x8026bd18`:

    lwz 3, 280(30)      ; [A+0x118]
    addic. 0, 3, -1
    stw 0, 280(30)
    bt 1, .+120         ; still > 0 -> done

This is not a fixed cooldown with jitter. It is a uniform draw over a
per-tank frame range taken from the parameter record.

## Only one enemy may act per frame, chosen from a random offset

`0x8025ca64` draws a 16-bit random, reduces it modulo the tank count
`[this+8]`, and starts scanning the tank array `[this+40]..[this+44]` at that
index, wrapping around. For each live tank it reads `[A+0x118]`, and among
those equal to 1 -- meaning "expires this frame" -- it lets only the first one
through, bumping the rest to 2:

    lwz 4, 280(5)       ; [A+0x118]
    cmpwi 4, 1
    bf 2, .+24          ; not about to fire -> next
    addi 7, 7, 1        ; count
    cmpwi 7, 1
    bf 1, .+12          ; already let one through -> leave this one alone
    addi 0, 4, 1
    stw 0, 280(5)       ; push back by one frame

`0x8025cb20` is the twin of this function for the other timer, and it carries
a different quota. It scans identically from a random origin but tests
`[A+0x110]` at `0x8025cb98` and compares the running count against **2** at
`0x8025cba8`, where the `[A+0x118]` pass compares against **1** at
`0x8025caec`. So the two timers are throttled differently:

| timer | tested at | max tanks firing per frame |
| --- | --- | --- |
| `[A+0x118]` | `0x8025cadc` | 1 |
| `[A+0x110]` | `0x8025cb98` | 2 |

So enemies are staggered, and which one wins is decided by a random scan
origin rather than array order. Reproducing this needs the RNG call order to
match, because the same seed feeds both this and the timer re-rolls.

## Two parameter objects, not one

`0x80268d2c` builds a holder with two sub-objects and the type index:

    stw 4, 12(3)      ; [holder+0x0C] = type
    lwz 3, 4(3)       ; A = [holder+4]
    bl 0x8026bfd4     ; A fills itself from the record, by type
    bl 0x8026be50
    lwz 3, 8(30)      ; B = [holder+8]
    bl 0x80269d84     ; GetTankParams(B, type)  -- the 16 fields already traced
    bl 0x80269c7c

B is the previously traced stats object. A is separate, fetches the 168-byte
record itself (`mulli 168` over `[[r13-25008]+0x34]`, 21 iterations of an
8-byte copy at `0x8026c058`) and stores the type at `[A+0x7C]`. A is read at
only two sites, both in the tank TU: `0x8025cad4` and `0x8025cb90`.

## Teal and Green are special-cased by type

At `0x8026c440`, after extrapolating a point `pos + f31 * dir` on all three
axes, the code dispatches on `[A+0x7C]`:

    cmpwi 0, 3 -> r9 = 1      ; type 3 = Teal
    cmpwi 0, 7 -> r9 = 2      ; type 7 = Green

No other type gets a branch here. Record order is Player 0, Brown 1, Ash 2,
Teal 3, Red 4, Yellow 5, Purple 6, Green 7, White 8, Black 9
(`docs/tnkgameparam.md`), and `[A+0x7C]` is the same index used for the
`mulli 168` lookup, so the mapping is sound.

## Threshold test on a dot product

`0x8026cdb0` scales an input by 0.711111 (`0x8026cdfc`) and later computes a
two-component dot product with `ps_mul` / `ps_madd` / `ps_sum0` at
`0x8026ceec..0x8026cf00`, comparing the result against f31 at `0x8026cf04`.
`0x8026d0fc` uses the same 0.711111 constant alongside the literal 22, which
is the map width. Not yet interpreted, but it is a genuine geometric test.

## Consequence for our implementation

Every quantity above is in frames and driven by a shared RNG whose call order
matters. Our simulation runs on a variable `float dt` with `rand()`, so it
cannot reproduce this timing even in principle. A fixed 60 Hz tick with the
RNG state held explicitly is a prerequisite for the 1:1 AI, independent of any
netcode consideration.

## The AI frame order

`0x802569c0` runs the three passes back to back on the same tank manager
`[r30+0x50]`, which fixes both the ordering and the RNG consumption order:

    lwz 3, 80(30) ; bl 0x8025cb20    ; stagger [A+0x110], quota 2, 1 RNG draw
    lwz 3, 80(30) ; bl 0x8025ca64    ; stagger [A+0x118], quota 1, 1 RNG draw
    lwz 3, 80(30) ; bl 0x8025d710    ; per-tank update

`0x8025d710` is a plain walk over the tank list calling the virtual at
vtable+0x18 on each live tank (`0x8025d744..0x8025d750`); it draws no random
numbers and arbitrates nothing.

The two mechanisms fit together cleanly. A stagger pass tests for `== 1`,
meaning "expires on this frame's decrement", and pushes surplus tanks to 2.
The per-tank update then decrements: a tank left at 1 reaches 0 and fires,
while a tank bumped to 2 reaches 1 and becomes a candidate again next frame.
So no action is lost, only deferred, and the deferral order is randomised by
the scan origin.

Reproducing this requires the same order: both stagger passes, in that
order, before any tank updates, with exactly one RNG draw each.

---

# Le tick par frame du contrôleur AI — 0x8026bb3c

Décodé instruction par instruction et recoupé deux fois (extraction Claude,
réfutation agy), puis les branchements re-vérifiés à la main sur le binaire.
`r3`/`r30` = objet A. Table complète des verdicts : `re/VERDICTS.md`.

## Trois compteurs, pas deux

On en connaissait deux. Il y en a **trois**, tous décrémentés dans ce tick :

| compteur | callee sur expiration | VMA du `bl`  | garde      | rechargement |
|----------|-----------------------|--------------|------------|--------------|
| `[A+0x10C]` | `0x8026c7d4`       | `0x8026bb98` | aucune     | fixe, `[A+0x24]` |
| `[A+0x110]` | `0x8026c730`       | `0x8026bcb4` | `[A+0x70]` | aléatoire, bornes `[A+0x28]`/`[A+0x2C]` |
| `[A+0x118]` | `0x8026c5ac`       | `0x8026bd38` | `[A+0x74]` | aléatoire, bornes `[A+0x54]`/`[A+0x58]` |

`[A+0x10C]` est le nouveau : il se recharge par un simple `lwz 0, 36(r30)` —
**une valeur fixe, sans tirage RNG**, contrairement aux deux autres.

## La forme exacte du décrément

Identique aux trois, ex. pour `[A+0x110]` :

    8026bc90  lwz    r3, 272(r30)     ; charge le compteur
    8026bc94  addic. r0, r3, -1       ; r0 = r3-1, positionne CR0
    8026bc98  stw    r0, 272(r30)     ; ré-écrit AVANT de brancher
    8026bc9c  bt     CR0[GT], .+124   ; si r3-1 > 0, on saute l'action

L'action part donc quand `r3 - 1 <= 0`, c'est-à-dire **quand le compteur valait
1 et tombe à 0**. Cohérent avec les deux passes d'étalement déjà documentées :
un char laissé à 1 agit à la frame suivante, un char poussé à 2 devient candidat
la frame d'après.

## Les gardes sont des verrous, pas des autorisations

    8026bca4  cmpwi r0, 0            ; r0 = [A+0x70]
    8026bca8  bf    CR0[EQ], .+16    ; si != 0, saute l'appel

L'appel n'a lieu **que si le champ vaut 0**. `[A+0x70]` et `[A+0x74]` sont donc
des drapeaux d'inhibition (occupé / interdit), pas des drapeaux d'activation.
Se tromper de polarité ici ferait tirer les chars exactement quand ils ne doivent pas.

## Deux drapeaux remis à zéro à chaque frame

    8026bb88  stb r7, 264(r3)   ; [A+0x108] <- 0
    8026bb8c  stb r7, 276(r3)   ; [A+0x114] <- 0

Remis à zéro en tête de tick, repositionnés par les callees. Ce sont des
requêtes valables une frame. `0x8026c7bc` écrit 1 dans `[A+0x108]`, `0x8026c718`
écrit 1 dans `[A+0x114]`.

Note d'honnêteté : que `[A+0x108]` soit « le drapeau de tir » et `[A+0x114]`
« le drapeau de mine » n'est **pas** prouvé par le code. Le décodage est certain,
l'interprétation gameplay ne l'est pas — l'auditeur l'a explicitement rejetée.
Ce qui est certain : ce sont deux requêtes booléennes d'une frame, l'une posée
par le callee de `[A+0x110]`, l'autre par celui de `[A+0x118]`.

## Un interrupteur global au-dessus des trois

    8026bc7c  lwz    r3, 284(r30)      ; [A+0x11C] -> objet lié
    8026bc80  lwz    r3, 408(r3)       ; [+0x198]
    8026bc88  rlwinm. r0, r0, 0, 30, 30 ; isole le bit 1
    8026bc8c  bf     CR0[EQ], .+272    ; si posé, saute TOUTE la section timers

Toute la logique de décision est court-circuitée quand ce bit est posé.

## Fin de tick

    8026bda8  bl 0x8026c280            ; sélecteur d'action, appelé sans condition

Contrairement aux trois callees ci-dessus, `0x8026c280` tourne à **chaque** frame.

## Ce qui reste non prouvé

`0x8026c730` et `0x8026c5ac` portent des noms de travail (« tir », « mine »)
venant d'une seule source non recoupée. Les 33 claims gameplay de `0x8026c730`
ont toutes été rejetées comme non forcées par le code. Ne rien écrire dans `src/`
sur la foi de ces noms.

# Ce que font vraiment les trois callees de timer

Les noms restés en suspens sont maintenant tranchés en suivant les `bl` dans les
callees indirects — ce qu'une fonction fait se décide chez ses appelés, pas dans
sa propre arithmétique. Rapports bruts : `re/callee_*.md`.

## `0x8026c730` — tir. PROUVÉ.

    8026c768  add  r4, r5, r4      ; indexe le tableau global par l'id du char
    8026c76c  lwz  r4, ...         ; nombre de balles actives de ce char
    8026c778  bl   0x8026d2d4      ; si sous la limite -> spawn
    8026c7bc  stb  r0, 264(r31)    ; sinon [A+0x108] <- 1

Le spawner `0x8026d2d4` calcule la position de sortie en `Pos + f1 * Forward`
(`fmadds` en `0x8026d304` et `0x8026d310`), f1 étant la longueur de canon passée
en argument — donc la balle naît à la bouche du canon, pas au centre du char.

Il y a bien une **limite de balles simultanées par char**, lue dans un tableau
global indexé par l'id. Quand la limite est atteinte le tir n'est pas perdu :
`[A+0x108]` passe à 1, ce qui reporte l'intention sur une frame suivante.

## `0x8026c5ac` — pose de mine. PROBABLE, pas prouvé.

Faisceau d'indices concordants :

- l'entité créée reçoit le **type 2** (`li r7, 2` en `0x802692ac`, écrit en
  `0x802693ac`), là où la balle passe par un chemin différent ;
- elle naît en `A[0x64..0x6C]`, le **centre du char**, pas à la bouche du canon ;
- garde d'espacement en `0x8026c608` : si l'objet le plus proche est à une
  distance `<= A[0x50]`, la fonction abandonne — empêche d'empiler les poses ;
- deux probabilités distinctes selon la proximité d'une autre entité
  (`0x8026c6a4`) : tirage RNG dans `[0, 100)` comparé à `A[0x60]` si recouvrement,
  à `A[0x5C]` sinon.

Ce qui manque pour prouver : la valeur d'énumération derrière le type 2, et
l'identité des tableaux globaux `0x80456C68` et `0x80456C78`. Tant que ce n'est
pas établi, ne pas câbler « mine » en dur dans `src/`.

## `0x8026c7d4` — NON TRANCHÉ.

Il faut d'abord `0x800e82e0`, `0x800e829c`, `0x8002ff8c`, `0x800e7fe8`
(désassemblés dans `re/asm/`, l'analyse reste à faire).

# Correction : `0x80269288` n'est pas deux fonctions

Une lecture antérieure en faisait un « sélecteur de direction par raycast 4 voies »,
une autre « le spawner de projectile ». Les deux sont des vues partielles de la
**même** fonction de 418 instructions.

`tools/dol.py fn` annonce « 95 instructions jusqu'au prochain blr/tail-branch »,
mais ce prétendu bord est un `b .+988` en `0x80269400` qui saute vers
`0x802697dc` — **à l'intérieur** de la fonction. Le seul `blr` du bloc est en
`0x8026990c`. La mesure « en avant jusqu'au blr » compte donc un saut interne
comme une fin de fonction et tronque.

Conséquence pratique : sur toute fonction contenant un `b` en avant non
conditionnel, croiser la borne avec un scan des `blr` avant de découper un
listing. Un listing tronqué produit des lectures qui semblent cohérentes et sont
fausses.

# Constantes flottantes du pool sda2

Résolues depuis r2 = 0x8045EF00, valeurs brutes, sans interprétation :

| accès       | VMA          | valeur    |
|-------------|--------------|-----------|
| `r2-17912`  | `0x8045a908` | 0.7111111 |
| `r2-17908`  | `0x8045a90c` | 22.0      |
| `r2-17904`  | `0x8045a910` | 176.0     |
| `r2-18008`  | `0x8045a8a8` | 0.0       |

22.0 est la largeur de grille. 176 = 22 x 8. 0.7111111 vaut exactement 32/45 ;
son rôle n'est pas établi et ne doit pas être deviné.
