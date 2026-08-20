# TnkGameParam.bin tank records

10 records of 42 words, starting at word 1 (word 0 is a header, value 3).
Record order is fixed by field 22, whose values reproduce the known speeds
exactly. Types come from the lwz/lfs mix in the loader's copy loop at
0x80269e18 and agree with the bit patterns of all 342 non-zero fields.

Speeds are pixels per frame at 60 Hz; one block is 32 px and one cell.

| fld | ty | Player | Brown | Ash | Teal | Red | Yellow | Purple | Green | White | Black | meaning |
|---|---|---|---|---|---|---|---|---|---|---|---|---|
| 0 | i | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 1 | 0 | stealth flag |
| 1 | f | 100 | 100 | 100 | 100 | 100 | 100 | 200 | 100 | 200 | 200 | unknown |
| 2 | i | 2 | 0 | 0 | 0 | 0 | 4 | 2 | 0 | 2 | 2 | max mines |
| 3 | i | 60 | 0 | 0 | 0 | 0 | 60 | 60 | 0 | 60 | 60 | mine timer max, frames -> A+0x58 |
| 4 | i | 40 | 0 | 0 | 0 | 0 | 40 | 40 | 0 | 40 | 40 | mine timer min, frames -> A+0x54 |
| 5 | f | 100 | 100 | 100 | 100 | 100 | 100 | 100 | 100 | 100 | 100 | mine range, px -> A+0x50 |
| 6 | f | 100 | 0 | 0 | 0 | 0 | 50 | 5 | 0 | 5 | 5 | mine chance near, % -> A+0x60 |
| 7 | f | 10 | 0 | 0 | 0 | 0 | 50 | 3 | 0 | 3 | 3 | mine chance far, % -> A+0x5C |
| 8 | i | 6 | 6 | 6 | 6 | 6 | 6 | 6 | 6 | 6 | 6 | unknown, constant |
| 9 | i | 1 | 3 | 3 | 3 | 3 | 1 | 1 | 3 | 1 | 1 | unknown |
| 10 | f | 0.3 | 0 | 0.3 | 0.3 | 0.3 | 0.3 | 0.3 | 0 | 0.3 | 0.3 | unknown |
| 11 | f | 0.6 | 0 | 0.6 | 0.6 | 0.6 | 0.6 | 0.6 | 0 | 0.6 | 0.6 | unknown |
| 12 | f | 30 | 0 | 30 | 30 | 30 | 30 | 30 | 0 | 30 | 30 | unknown |
| 13 | i | 10 | 0 | 15 | 10 | 10 | 15 | 10 | 0 | 10 | 10 | unknown |
| 14 | i | 5 | 0 | 10 | 5 | 5 | 10 | 5 | 0 | 5 | 5 | unknown |
| 15 | f | 120 | 0 | 120 | 120 | 120 | 130 | 120 | 0 | 120 | 120 | unknown |
| 16 | f | 120 | 0 | 120 | 120 | 120 | 120 | 120 | 0 | 120 | 120 | unknown |
| 17 | f | 120 | 0 | 0 | 0 | 0 | 160 | 160 | 0 | 160 | 0 | unknown |
| 18 | f | 60 | 0 | 40 | 40 | 40 | 40 | 60 | 0 | 40 | 100 | unknown |
| 19 | i | 1 | 0 | 1 | 1 | 0 | 1 | 1 | 0 | 1 | 0 | unknown |
| 20 | f | 0.1 | 0 | 0.03 | -0.1 | 0.2 | 0 | 0.1 | 0 | 0.1 | 0.2 | unknown |
| 21 | i | 4 | 4 | 4 | 4 | 4 | 4 | 4 | 4 | 4 | 4 | unknown, constant |
| 22 | f | 1.8 | 0 | 1.2 | 1 | 1.2 | 1.8 | 1.8 | 0 | 1.2 | 2.4 | speed px/frame |
| 23 | f | 0.12 | 0.12 | 0.12 | 0.12 | 0.12 | 0.12 | 0.12 | 0.12 | 0.12 | 0.12 | unknown, constant |
| 24 | f | 0.85 | 0.85 | 0.85 | 0.85 | 0.85 | 0.85 | 0.85 | 0.85 | 0.85 | 0.85 | unknown, constant |
| 25 | f | 0.08 | 0.08 | 0.08 | 0.2 | 0.08 | 0.08 | 0.08 | 0.08 | 0.08 | 0.06 | unknown |
| 26 | f | 10 | 10 | 10 | 10 | 10 | 10 | 10 | 10 | 10 | 5 | unknown |
| 27 | i | 50 | 30 | 30 | 30 | 50 | 30 | 50 | 50 | 50 | 50 | unknown |
| 28 | f | 40 | 170 | 40 | 0 | 40 | 40 | 40 | 80 | 40 | 5 | aim spread -> A+0x1C |
| 29 | i | 5 | 1 | 1 | 1 | 3 | 1 | 5 | 2 | 5 | 3 | max shells |
| 30 | f | 5 | 5 | 5 | 5 | 5 | 5 | 5 | 5 | 5 | 5 | unknown, constant |
| 31 | f | 20 | 20 | 20 | 20 | 20 | 20 | 20 | 20 | 20 | 20 | unknown, constant |
| 32 | f | 20 | 20 | 20 | 20 | 20 | 20 | 20 | 20 | 20 | 20 | unknown, constant |
| 33 | i | 1 | 1 | 1 | 0 | 1 | 1 | 1 | 2 | 1 | 0 | ricochets |
| 34 | i | 45 | 45 | 45 | 10 | 10 | 45 | 10 | 10 | 10 | 10 | fire timer max, frames -> A+0x2C |
| 35 | i | 30 | 30 | 30 | 5 | 5 | 30 | 5 | 5 | 5 | 5 | fire timer min, frames -> A+0x28 |
| 36 | i | 6 | 300 | 180 | 180 | 30 | 180 | 30 | 60 | 30 | 60 | shot cooldown frames |
| 37 | f | 3 | 3 | 3 | 6 | 3 | 3 | 3 | 6 | 3 | 6 | shell speed px/frame |
| 38 | f | 0.05 | 0.01 | 0.01 | 0.05 | 0.02 | 0.02 | 0.03 | 0.02 | 0.03 | 0.03 | unknown |
| 39 | i | 4 | 60 | 45 | 8 | 20 | 30 | 20 | 30 | 30 | 20 | re-aim period, frames -> A+0x24 |
| 40 | f | 70 | 70 | 70 | 70 | 70 | 70 | 70 | 70 | 70 | 70 | unknown, constant |
| 41 | i | 5 | 60 | 10 | 20 | 5 | 10 | 5 | 5 | 5 | 10 | unknown |

## Status

Seven fields are identified and in use. The rest are listed with their
real values but no proven meaning; do not wire one into gameplay without
first proving it from the binary. Fields 9, 19, 34, 35, 38, 39 and 41 vary
per tank type and are the plausible AI parameters, but none is proven.

## Six champs identifiés par leur destination dans l'objet AI

Le constructeur `0x8026bfd4` recopie le record dans l'objet AI A. Chaque ligne
ci-dessous a été vérifiée à la main : le `lfs`/`lwz` depuis la pile, le
`stfs`/`stw` vers A, et l'indice `k = (offset_pile - 8) / 4`.

| champ | -> A     | rôle                                        | preuve                            |
|-------|----------|---------------------------------------------|-----------------------------------|
| 28    | `0x1C`   | dispersion de visée                         | `8026c0dc` lfs / `8026c1d0` stfs  |
| 39    | `0x24`   | période de re-visée, frames (rechargement fixe de `[A+0x10C]`) | `8026c108` lwz / `8026c1d8` stw |
| 35    | `0x28`   | borne min du timer de tir `[A+0x110]`       | `8026c0f8` lwz / `8026c1dc` stw   |
| 34    | `0x2C`   | borne max du timer de tir                   | `8026c0f4` lwz / `8026c1e0` stw   |
| 4     | `0x54`   | borne min du timer de mine `[A+0x118]`      | `8026c07c` lwz / `8026c208` stw   |
| 5     | `0x50`   | portée de pose de mine, px                  | `8026c080` lfs / `8026c204` stfs  |
| 7     | `0x5C`   | probabilité de pose, loin, %                | `8026c088` lfs / `8026c210` stfs  |
| 6     | `0x60`   | probabilité de pose, près, %                | `8026c084` lfs / `8026c214` stfs  |
| 3     | `0x58`   | borne max du timer de mine                  | `8026c078` lwz / `8026c20c` stw   |

### Pourquoi ces rôles tiennent

**Champ 28, dispersion.** `0x8026c7d4` construit une matrice de rotation
(`0x8002ff8c`) dont l'angle de lacet est un tirage RNG mis à l'échelle par
`[A+0x1C]`, puis fait tourner le vecteur normalisé vers la cible
(`0x800e7fe8`). Les valeurs collent au comportement connu du jeu : Brown 170,
de loin le plus imprécis ; Teal 0, tir parfaitement droit ; Black 5, le plus
précis ; Green 80, qui vise par ricochet.

**Champs 3 et 4, mines.** Ils sont non nuls exactement pour Joueur, Yellow,
Purple, White et Black — c'est-à-dire précisément la liste des chars qui posent
des mines, établie par ailleurs (champ 2). Un char sans mine a des bornes à zéro.
C'est ce recoupement, et non le désassemblage seul, qui fait passer
`0x8026c5ac` de « probable » à sûr.

**Champ 39, re-visée.** Brown 60 (une re-visée par seconde, pataud), Teal 8
(très réactif), Joueur 4. Cohérent avec la réactivité perçue de chaque char.

Unités : ces six champs sont des compteurs de frames à 60 Hz, sauf le 28 dont
l'échelle angulaire n'est pas encore établie.

### La pose de mine en entier

Le timer `[A+0x118]` qui expire ne pose pas de mine : il ouvre une tentative.

1. **Portée.** `0x8026c5fc` charge `A[0x50]` et le compare à une distance. Attention
   à la polarité : `cror 2,0,2` replie LT dans EQ, donc le `bt` de `0x8026c608`
   sort quand `A[0x50] <= dist`. On ne pose que si quelque chose est **plus près**
   que `A[0x50]`. C'est une exigence de proximité, pas une garde d'espacement.
2. **Tirage.** Le RNG est ramené dans `[0, 100)` en `0x8026c68c`, puis comparé à
   `A[0x60]` (`0x8026c6ac`) ou à `A[0x5C]` (`0x8026c6c8`).
3. **Lequel des deux.** Le booléen rendu par `0x80261c14`, testé en `0x8026c680`.
   Non nul -> `A[0x60]`, nul -> `A[0x5C]`. Ce que teste cette fonction n'est pas
   encore établi.

Yellow tire 50 des deux côtés, les trois autres 3 ou 5 : c'est ce qui fait que
Yellow tapisse la carte et que Black lâche une mine de loin en loin.


## La table complète, dérivée mécaniquement

`tools/fieldmap.py` la reconstruit depuis le désassemblage de `0x8026bfd4` au lieu
de la lire d'un rapport. Le principe : le constructeur recopie le record de 168
octets sur sa propre pile, puis déplace les champs de là vers l'objet. Chaque
`stfs`/`stw rN, D(r3)` est donc une ligne, dès qu'on sait de quel emplacement de
pile `rN` a été chargé pour la dernière fois. Registres entiers et flottants sont
suivis séparément, et le type se déduit de `lwz` contre `lfs`.

Régénérer :

    python3 tools/fieldmap.py

| fld | -> A   | type  | load     | store | sens |
|-----|--------|-------|----------|---------- |------|
|   1 | 0x4C   | float | 8026c070 | 8026c200 | ? |
|   2 | 0x44   | int   | 8026c074 | 8026c1f8 | mines (fld 2, connu) |
|   3 | 0x58   | int   | 8026c078 | 8026c20c | timer mine max |
|   4 | 0x54   | int   | 8026c07c | 8026c208 | timer mine min |
|   5 | 0x50   | float | 8026c080 | 8026c204 | portee mine, px |
|   6 | 0x60   | float | 8026c084 | 8026c214 | proba mine pres, % |
|   7 | 0x5C   | float | 8026c088 | 8026c210 | proba mine loin, % |
|   8 | 0x40   | int   | 8026c08c | 8026c1f4 | ? |
|   9 | 0x48   | int   | 8026c090 | 8026c1fc | ? |
|  28 | 0x1C   | float | 8026c0dc | 8026c1d0 | dispersion de visee |
|  29 | 0xC    | int   | 8026c0e0 | 8026c1c0 | balles simultanees (fld 29, connu) |
|  30 | 0x30   | float | 8026c0e4 | 8026c1e4 | ? |
|  31 | 0x38   | float | 8026c0e8 | 8026c1ec | ? |
|  32 | 0x34   | float | 8026c0ec | 8026c1e8 | ? |
|  33 | 0x10   | int   | 8026c0f0 | 8026c1c4 | ricochets (fld 33, connu) |
|  34 | 0x2C   | int   | 8026c0f4 | 8026c1e0 | timer tir max |
|  35 | 0x28   | int   | 8026c0f8 | 8026c1dc | timer tir min |
|  36 | 0x8    | int   | 8026c0fc | 8026c1bc | cooldown de tir, frames (fld 36, connu) |
|  37 | 0x14   | float | 8026c100 | 8026c1c8 | vitesse d obus (fld 37, connu) |
|  38 | 0x20   | float | 8026c104 | 8026c1d4 | ? |
|  39 | 0x24   | int   | 8026c108 | 8026c1d8 | periode de re-visee |
|  40 | 0x3C   | float | 8026c10c | 8026c1f0 | ? |
|  41 | 0x18   | int   | 8026c110 | 8026c1cc | ? |

Les 42 champs sont tous chargés depuis la pile, mais seuls 23 sont écrits dans
l'objet AI. Les autres sont consommés ailleurs — ils partent vers le char plutôt
que vers son contrôleur. Le champ 22, la vitesse, en fait partie : elle n'entre
jamais dans l'objet AI.

Cette table reproduit sans y toucher les neuf lignes qui avaient été vérifiées à
la main une par une (champs 3, 4, 5, 6, 7, 28, 34, 35, 39). C'est une confirmation
indépendante : elle sort du binaire, pas d'un rapport.
