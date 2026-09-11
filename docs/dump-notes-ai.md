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
    out   : ((LCG ^ LFSR) >> 4) & 0xFFFF   (rlwinm 3,0,28,16,31 at 0x8026bcfc)

Earlier notes had `& 0xFFFF` without the shift. Running the tick on a Dolphin
dump settled it: all 55 fire-timer reloads in 2000 frames match the shifted
form and none match the other (`tools/test_oracle_timers.cpp`).

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

The reload runs on every expiry, whether or not the guard (`[A+0x70]`,
`[A+0x74]`) let the callee run: the guard's `bf` only skips the `bl`. A zero
span still draws, and `divw` then `mullw` by that zero leaves the raw draw,
so a tank without mines reloads `[A+0x118]` to up to 65535 frames.

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

## Les gardes sont des recharges, pas des drapeaux

    8026bca4  cmpwi r0, 0            ; r0 = [A+0x70]
    8026bca8  bf    CR0[EQ], .+16    ; si != 0, saute l'appel

L'appel n'a lieu **que si le champ vaut 0**. Se tromper de polarité ici ferait
tirer les chars exactement quand ils ne doivent pas.

Et ce ne sont pas des booléens. Le sélecteur d'action les entretient comme des
compteurs de frames, décrémentés puis bornés à zéro :

    8026c508  lwz   r3, 8(r30)      ; [A+0x8] = champ 36, le cooldown de tir
    8026c510  stw   r3, 112(r30)    ; [A+0x70] <- champ 36
    8026c514  stw   r0, 120(r30)    ; [A+0x78] <- champ 41
    8026c518  lwz   r3, 112(r30)
    8026c51c  addic. r0, r3, -1
    8026c520  stw   r0, 112(r30)
    8026c524  bf    CR0[LT], .+12   ; negatif -> on borne a 0
    8026c530  lwz   r3, 116(r30)    ; meme traitement pour [A+0x74]
    8026c538  stw   r0, 116(r30)

Donc `[A+0x70]` est la **recharge de l'arme**, rechargée depuis le champ 36, et la
garde du timer de tir signifie simplement « le canon est encore en train de se
recharger ». Deux cadences distinctes se superposent : le timer `[A+0x110]` décide
*quand envisager* de tirer, `[A+0x70]` décide *si l'arme est prête*.

Le site de rechargement de `[A+0x74]` n'est pas encore localisé — il est décrémenté
ici mais rechargé ailleurs, probablement par le callee de pose de mine.

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

## `0x8026c5ac` — pose de mine. Confirmé par recoupement.

Faisceau d'indices concordants :

- l'entité créée reçoit le **type 2** (`li r7, 2` en `0x802692ac`, écrit en
  `0x802693ac`), là où la balle passe par un chemin différent ;
- elle naît en `A[0x64..0x6C]`, le **centre du char**, pas à la bouche du canon ;
- garde d'espacement en `0x8026c608` : si l'objet le plus proche est à une
  distance `<= A[0x50]`, la fonction abandonne — empêche d'empiler les poses ;
- deux probabilités distinctes selon la proximité d'une autre entité
  (`0x8026c6a4`) : tirage RNG dans `[0, 100)` comparé à `A[0x60]` si recouvrement,
  à `A[0x5C]` sinon.

L'énumération derrière le type 2 reste non identifiée, donc le désassemblage seul
s'arrêtait à « probable ». Ce qui tranche vient d'ailleurs : les bornes du timer
`[A+0x118]` sont les champs 4 et 3 du record, et ces deux champs sont non nuls
**exactement** pour Joueur, Yellow, Purple, White et Black — la liste des chars
qui posent des mines. Les cinq autres ont des bornes à zéro. Un tel alignement
n'arrive pas par accident.

## `0x8026c7d4` — re-visée avec dispersion.

    8026cad8  bl 0x8002ff8c   ; matrice de rotation depuis des angles d'Euler,
                              ; le lacet = tirage RNG mis a l'echelle par [A+0x1C]
    8026cae8  bl 0x800e7fe8   ; applique la matrice au vecteur normalise vers la
                              ; cible, resultat dans [A+0x80]

Rien n'est alloué, aucun projectile n'est créé, aucun compteur ni cooldown n'est
touché : la seule sortie est le vecteur `[A+0x80]`. C'est le seul des trois
callees à tourner sans garde et à se recharger sur une valeur fixe (champ 39),
donc à cadence constante par char.

`[A+0x1C]` est le champ 28 du record. Ses valeurs par char reproduisent le
comportement connu du jeu — Brown 170 le plus dispersé, Teal 0 parfaitement
droit, Black 5 le plus précis. Reste non établi : l'unité d'angle.

# Correction : `0x80269288` n'est pas deux fonctions

Une lecture antérieure en faisait un « sélecteur de direction par raycast 4 voies »,
une autre « le spawner de projectile ». Les deux sont des vues partielles de la
**même** fonction de 418 instructions.

`tools/dol.py fn` annonce « 95 instructions jusqu'au prochain blr/tail-branch »,
mais ce prétendu bord est un `b .+988` en `0x80269400` qui saute vers
`0x802697dc` — **à l'intérieur** de la fonction. Le seul `blr` du bloc est en
`0x8026990c`. La mesure « en avant jusqu'au blr » compte donc un saut interne
comme une fin de fonction et tronque.

Le listing, lui, était complet : il est découpé sur la borne `end`, pas sur cette
mesure. Aucune analyse n'a donc tourné sur du code tronqué. Le dégât est ailleurs :
ce « 95 » a servi d'argument pour conclure que deux lectures divergentes visaient
deux fonctions distinctes. Elles décrivaient les deux moitiés d'une seule.

Corrigé dans `tools/dol.py` (commit 17a7902) : un terminateur ne compte que si
aucun branchement antérieur ne porte au-delà. Cas épinglé dans `selftest`.

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

## La tourelle ne se cale pas d'un coup (champ 38)

Le champ 38 du record est recopié en `A+0x20` (`0x8026c104 lfs`, `0x8026c1d4
stfs`). Le tick le lit six fois, en `0x8026bbac`, `0x8026bbc4`, `0x8026bbd8`
puis `0x8026bbf4`, `0x8026bc0c`, `0x8026bc20` :

    P+ = A[0x8C..0x94] + A[0x98..0xA0] * s     (fmadds, trois composantes)
    P- = A[0x8C..0x94] - A[0x98..0xA0] * s     (fnmsubs)

les deux normalisés par `0x800e829c`. `A+0x8C` est une direction unitaire,
`A+0x98` le vecteur perpendiculaire que `0x8026bc78` recalcule à partir d'elle et
d'un vecteur constant en `0x80453528`. Donc `P±` sont les deux directions
obtenues en tournant de `±atan(s)`.

Suivent deux appels à `0x801b6ab0(out, a, b, c)`, qui compare `dot(a,b)` et
`dot(a,c)` en paired-single et recopie dans `out` celui des deux qui gagne
(`fcmpo` puis `cror 2,1,2`, donc « b si `dot(a,b) >= dot(a,c)` ») :

1. `0x8026bc44` : `pick = plus_proche_de(A+0x80, P-, P+)` — le pas dans le bon sens.
2. `0x8026bc58` : `A+0x8C = plus_proche_de(A+0x8C, pick, A+0x80)` — la cible si
   elle est déjà dans le cône, sinon le pas.

C'est une rotation vers la cible bornée à un cône par frame. `A+0x80` est la
direction visée : le callee de visée l'écrit en `0x8026cae8`, après avoir tiré
l'écart uniformément dans `[-champ 28, +champ 28]` (`0x8026ca60`..`0x8026cac0`,
LCG XOR LFSR, `& 0x7FFFFF` puis `* 1.19209e-07`) et construit la rotation
correspondante. Donc `A+0x8C` est bien la tourelle, pas la caisse.

`s` est la tangente de l'angle, pas l'angle :

| char | champ 38 | °/frame | °/s |
|------|----------|---------|-----|
| Joueur, Teal | 0.05 | 2.862 | 171.7 |
| Purple, White, Black | 0.03 | 1.718 | 103.1 |
| Red, Yellow, Green | 0.02 | 1.146 | 68.7 |
| Brown, Ash | 0.01 | 0.573 | 34.4 |

Un demi-tour de tourelle prend 315 frames au Brown, soit 5,2 s. C'est la
deuxième moitié de son imprécision, l'autre étant les 60 frames de visée figée
du champ 39 : le canon n'a simplement pas le temps de suivre.

Le record du joueur porte le champ lui aussi, à 0.05.

## Le tir n'est gardé par aucun test de mur (0x8026c280..0x8026c5a8)

Le contrôleur d'IA a été lu en entier. La détente est un simple compteur :

| Adresse | Ce qu'elle fait |
|---|---|
| `0x8026c3a0` | recharge `[A+0x74]` depuis `[A+0x40]` et `[A+0x78]` depuis `[A+0x48]` |
| `0x8026c3cc` | tire le délai `[A+0x110]` dans le LCG `[r13-25800]` xor le LFSR `[r13-25796]` |
| `0x8026c534` | décrémente `[A+0x74]`, borné à 0 |
| `0x8026c548` | `lwz 0,120(30)` puis `cmpwi 0,0` et `bf CR0[GT]` vers `0x8026c578` — pas de `cror` devant, donc la chute est `[A+0x78] > 0` |
| `0x8026c564` | `bl 0x80268d04`, qui ne fait que basculer le bit 4 de `[r3+0x10]` : c'est l'entrée « bouton tir », pas le spawn de l'obus |
| `0x8026c568` | décrémente `[A+0x78]` |

Rien sur ce chemin n'interroge le terrain. Le pointeur du gestionnaire de blocs
`[r13-25032]` n'apparaît qu'une fois dans tout le contrôleur, en `0x8026c61c`,
dans la routine de mine. Les deux aides de visée `0x802639b4` et `0x80263ba4`
passent par `[r13-25016]`, et la routine de mine indexe ce même pointeur en
`[[r13-25016]+8] + (index+2)*4` : c'est la table des entités, pas la géométrie.
La valeur de retour de l'aide de visée est morte — `0x8026c4f8` recharge `r3`
juste après l'appel.

Conclusion : le jeu d'origine tire au rythme du compteur, mur ou pas, et son
propre ricochet peut lui revenir dessus.

Le drapeau `[A+0x114]` sélectionne laquelle des deux aides de visée tourne
(`lbz` en `0x8026c330`, `bt CR0[EQ]` vers `0x8026c42c`). Il est mis à 1 en
`0x8026c714` en queue de la routine de mine, quand `0x80269288` rend non nul, et
remis à 0 en `0x8026c5c4` et `0x8026bee4`.

**Ce que fait notre code à la place.** `AIManager::ShotIsClear` (src/AI.cpp)
refuse la détente tant que le canon lui-même ne tient pas la solution. C'est une
divergence assumée, pas une extraction : notre tourelle pivote lentement
(`turretSlewTan`) alors que la décision de tir se prenait sur la solution visée,
si bien que le char tirait dans le mur d'à côté puis se prenait le retour.

## La décision de déplacement est une machine à trois états (0x8026995c..0x80269b10)

Le corps ne tourne qu'à l'expiration de `[M+0x1FC]`. Il ne lit aucun des
paramètres de distance lui-même : il vide deux tableaux de candidats, laisse deux
collecteurs les remplir, puis choisit un état.

1. `0x80269974..0x802699d4` : boucle de 4 tours, borne immédiate 4 (pas
   `[M+0x40]`), pas de 44 octets (`addi 6,6,44` en `0x802699d0`). Elle remet à
   zéro le tableau `M+0x44`, en écrivant le vecteur nul de `0x80453510`.
   `0x80269964` met le compte `[M+0xF4]` à zéro.
2. `0x802699e0` : `bl 0x80262134` avec `r3 = [r13-25024]`, `r4 = M`.
3. `0x802699fc..0x80269a40` : même boucle de 4 tours pour le tableau `M+0xF8`,
   pas de 28 octets. `0x802699ec` met le compte `[M+0x168]` à zéro.
4. `0x80269a4c` : `bl 0x802666b4` avec `r3 = [r13-24992]`, `r4 = M`.

Puis le choix, rangé dans `[M+0x200]` :

| Condition | État |
|---|---|
| `[M+0x1F9] != 0` (`lbz` en `0x80269a50`) | 2, écrit en `0x80269a64` |
| sinon `[M+0xF4] > 0` (`bt CR0[GT]` en `0x80269a90`) | 1, écrit en `0x80269aa8` |
| sinon `[M+0x168] > 0` (`bf CR0[GT]` en `0x80269a9c`) | 1 |
| sinon | 0, écrit en `0x80269ad4` |

Les trois bras appellent `0x80268d04` avec `r3 = [[M+0x208]+0x198]`, `r4 = 2`,
`r5 = 1` pour les états 2 et 1 et `r5 = 0` pour l'état 0 — mais seulement si
`[M+0x3C]` (champ 19) est non nul (`bt CR0[EQ]` en `0x80269a6c` et `0x80269ab0`).
`0x80268d04` est la même bascule d'entrée que la détente, avec un autre index :
`r4 = 1` pour le tir, `r4 = 2` ici. Le champ 19 vaut 1 pour Player, Ash, Teal,
Yellow, Purple et White, 0 pour Red et Black, donc il ouvre une seconde voie
d'entrée pour ces chars-là seulement.

Le corps n'interroge ni le terrain (`[r13-25032]`, `0x801bfd44`) ni la table
d'entités (`[r13-25016]`). Les distances des champs 15..18 doivent donc être lues
un cran plus bas, dans les deux collecteurs, qui reçoivent M en `r4`.

**Ouvert** : ce que remplissent `0x80262134` (via `[r13-25024]`, entrées de 44
octets) et `0x802666b4` (via `[r13-24992]`, entrées de 28 octets), et ce que fait
l'état 0/1/2 en aval.

### L'état choisi appelle un des trois gestionnaires de déplacement

`0x80269ae8` relit `[M+0x200]`, fait `mulli 0,0,12`, `add 4,29,0`,
`addi 12,4,524` et appelle `0x800b2150` — le thunk PTMF de CodeWarrior, qui prend
en `r12` un descripteur de 12 octets. Donc `M+0x20C + 12*état` est un
pointeur-sur-membre, `r3 = M`.

Les trois descripteurs sont recopiés en `0x8026b184..0x8026b1c0` depuis la table
statique `0x803764F8`, chargée en `0x8026b118` (`lis 31,0x8037` / `addi
31,31,25848`). Contenu de la table :

| état | descripteur | fonction |
|---|---|---|
| 0 | `0x803764F8` `00000000 ffffffff 8026a75c` | `0x8026a75c..0x8026ad50`, 382 instructions |
| 1 | `0x80376504` `00000000 ffffffff 8026a300` | `0x8026a300..0x8026a758`, 279 instructions |
| 2 | `0x80376510` `00000000 ffffffff 8026a1c8` | `0x8026a1c8..0x8026a2fc`, 78 instructions |

Le `ffffffff` en deuxième mot est la marque d'un appel non virtuel, donc ce sont
bien ces trois adresses-là qui tournent.

Juste après le gestionnaire, `0x80269b04` normalise le vecteur `M+0x1DC`
(`bl 0x800e829c`) et `0x80269b20` fait son produit vectoriel avec la constante
`0x80453510` pour ranger la perpendiculaire en `M+0x1E8` (`bl 0x800e8344`). Le
gestionnaire écrit donc une direction dans `M+0x1DC` : c'est la direction de
déplacement choisie.

Au passage, `0x8026b13c..0x8026b154` : le vptr de M est `0x8037651C`, et l'objet
IA A est un membre alloué à part — `li 3,320` puis `bl 0x800a39f8`, constructeur
`0x8026bb00`, rangé en `M+0x204`.

### État 2 : l'esquive (`0x8026a1c8..0x8026a2fc`)

`r31 = M`.

1. `0x8026a1f8..0x8026a234` : boucle de `[M+0x40]` tours (champ 21, 4 partout ;
   la borne est vraiment lue en `0x8026a1e4`, contrairement aux boucles du tick
   qui ont un 4 immédiat). Elle additionne des vec3 pris en `M+0x17C + 12*i`
   (`lfs 0,380(4)` / `384(4)` / `388(4)`, `addi 4,4,12`) dans un accumulateur de
   pile. C'est donc un tableau de 4 vecteurs d'évitement.
2. `0x8026a23c` : `bl 0x800e82e0` rend la longueur de la somme. Si elle est non
   nulle, `0x8026a254` normalise et le résultat va en `M+0x1DC` ; sinon
   `0x8026a274` recopie tel quel le premier vecteur `M+0x17C`. Dans les deux cas
   `M+0x1DC` est la direction de déplacement, celle que le tick normalise ensuite.
3. `0x8026a294` : `[M+0x1F8] = 1`.
4. `0x8026a298` : si le compte `[M+0x168]` est nul, `[M+0x1F9] = 0` et retour —
   l'état 2 se désarme.
5. Sinon `0x8026a2ac..0x8026a2d8` parcourt les 4 entrées de 28 octets du tableau
   B (`M+0xF8`, `addi 3,3,28`) et lit le flottant à `+0x0C` de chaque entrée
   (`lfs 1,260(3)` au premier tour). `fcmpo` contre la constante sda2
   `0x8045b9b0` = 16, `bf CR0[LT]` : le drapeau local ne passe à 1 que pour une
   entrée **strictement inférieure à 16**. Si aucune ne l'est, `0x8026a2e8`
   remet `[M+0x1F9]` à 0.

Donc l'état 2 est l'esquive : le char part le long de la somme des vecteurs
d'évitement et y reste tant qu'une entrée du tableau B est sous 16. L'unité de ce
16 n'est pas encore fixée — 16 px est un demi-bloc, mais ça peut aussi être un
compte à rebours en frames.

### Les deux collecteurs : obus et mines

`0x802699e0` appelle `0x80262134..0x80262498` avec `r3 = [r13-25024]`, `r4 = M`.
`0x80269a4c` appelle `0x802666b4..0x80266938` avec `r3 = [r13-24992]`, `r4 = M`.

Les deux gestionnaires sont des objets de 64 octets construits par `operator new`
en `0x800a39f8` : `[r13-25024]` en `0x802628e0`, vtable `0x80375BE0`, rangé en
`0x80262910` ; `[r13-24992]` en `0x80266cb0`, vtable `0x80375E18`, rangé en
`0x80266cdc`.

`[r13-24992]` est le gestionnaire de **mines** : le code de bloc l'interroge en
`0x80260b58`, dans la branche de proximité de mine déjà identifiée en
`0x80260b38`. `[r13-25024]` est donc celui des **obus**, ce que confirme la forme
des entrées : celles du tableau A portent une vitesse, celles du tableau B non, et
une mine ne bouge pas.

#### Sélection de la portée

Les deux collecteurs ont exactement la même structure. Pour chaque objet du pool,
un appel virtuel `vtable+0x98` (`lwz 12,152(12)` puis `bctrl`, en `0x8026226c` et
`0x802667d4`) renvoie un état, et cet état choisit la portée :

| | retour != 0 | retour == 0 |
|---|---|---|
| obus, `0x8026227c` `bf CR0[EQ]` | `M+0x2C` = champ 16 = 120 partout | `M+0x30` = champ 18 |
| mines, `0x802667e4` `bf CR0[EQ]` | `M+0x34` = champ 15 = 120, Yellow 130 | `M+0x38` = champ 17 |

Champ 18 : Player 60, Ash/Teal/Red/Yellow/White 40, Purple 60, Black 100,
Brown/Green 0. Champ 17 : Player 120, Yellow/Purple/White 160, et **0** pour Ash,
Teal, Red et Black — ces quatre-là ignorent les mines dans l'état « retour 0 ».

Gardes et acceptation :

- Si les deux portées valent 0, le scan est sauté d'emblée (`0x802621d4` puis
  `0x80262240`/`0x80262248` pour les obus, `0x802667ac`..`0x802667b4` pour les
  mines). Brown et Green tombent là-dedans.
- Si la portée retenue vaut 0, l'objet est sauté (`0x8026228c` `fcmpu 0,29,28`
  puis `bt`, et `0x802667f4` `fcmpu 0,31,27` puis `bt`).
- Distance : `bl 0x800e82e0` rend la longueur, puis `fcmpo` contre la portée et
  `bf CR0[LT]` rejette (`0x80262350`/`0x80262354`, `0x80266858`/`0x8026685c`).
  Accepté seulement si `distance < portée`.
- Obus seulement : `0x80262370` `ps_sum0` termine un produit scalaire en
  paired-single, `0x80262374` le compare à 0 et `0x80262378` `bf CR0[GT]` rejette.
  Un obus qui s'éloigne n'est pas retenu.

#### L'insertion

Deux aides séparées, appelées avec M :

| tableau | aide | compte | plafond |
|---|---|---|---|
| A, obus, 44 octets | `0x80269068..0x80269284` | `M+0xF4` (`lwz 7,244(3)`) | 4, forcé en `0x80269278` |
| B, mines, 28 octets | `0x80268ec8..0x80269064` | `M+0x168` (`lwz 4,360(3)`) | 4, forcé en `0x8026905c` |

Les deux font `addi 0,compte,1`, `cmpwi 0,4`, `bclr` sur GT puis réécrivent 4 :
le compte sature à 4, il ne déborde pas.
