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
| 3 | i | 60 | 0 | 0 | 0 | 0 | 60 | 60 | 0 | 60 | 60 | unknown |
| 4 | i | 40 | 0 | 0 | 0 | 0 | 40 | 40 | 0 | 40 | 40 | unknown |
| 5 | f | 100 | 100 | 100 | 100 | 100 | 100 | 100 | 100 | 100 | 100 | unknown, constant |
| 6 | f | 100 | 0 | 0 | 0 | 0 | 50 | 5 | 0 | 5 | 5 | unknown |
| 7 | f | 10 | 0 | 0 | 0 | 0 | 50 | 3 | 0 | 3 | 3 | unknown |
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
| 28 | f | 40 | 170 | 40 | 0 | 40 | 40 | 40 | 80 | 40 | 5 | unknown |
| 29 | i | 5 | 1 | 1 | 1 | 3 | 1 | 5 | 2 | 5 | 3 | max shells |
| 30 | f | 5 | 5 | 5 | 5 | 5 | 5 | 5 | 5 | 5 | 5 | unknown, constant |
| 31 | f | 20 | 20 | 20 | 20 | 20 | 20 | 20 | 20 | 20 | 20 | unknown, constant |
| 32 | f | 20 | 20 | 20 | 20 | 20 | 20 | 20 | 20 | 20 | 20 | unknown, constant |
| 33 | i | 1 | 1 | 1 | 0 | 1 | 1 | 1 | 2 | 1 | 0 | ricochets |
| 34 | i | 45 | 45 | 45 | 10 | 10 | 45 | 10 | 10 | 10 | 10 | unknown |
| 35 | i | 30 | 30 | 30 | 5 | 5 | 30 | 5 | 5 | 5 | 5 | unknown |
| 36 | i | 6 | 300 | 180 | 180 | 30 | 180 | 30 | 60 | 30 | 60 | shot cooldown frames |
| 37 | f | 3 | 3 | 3 | 6 | 3 | 3 | 3 | 6 | 3 | 6 | shell speed px/frame |
| 38 | f | 0.05 | 0.01 | 0.01 | 0.05 | 0.02 | 0.02 | 0.03 | 0.02 | 0.03 | 0.03 | unknown |
| 39 | i | 4 | 60 | 45 | 8 | 20 | 30 | 20 | 30 | 30 | 20 | unknown |
| 40 | f | 70 | 70 | 70 | 70 | 70 | 70 | 70 | 70 | 70 | 70 | unknown, constant |
| 41 | i | 5 | 60 | 10 | 20 | 5 | 10 | 5 | 5 | 5 | 10 | unknown |

## Status

Seven fields are identified and in use. The rest are listed with their
real values but no proven meaning; do not wire one into gameplay without
first proving it from the binary. Fields 9, 19, 34, 35, 38, 39 and 41 vary
per tank type and are the plausible AI parameters, but none is proven.
