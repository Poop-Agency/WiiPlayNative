#!/usr/bin/env python3
"""Derive the TnkGameParam record -> AI object map from 0x8026bfd4 itself.

The constructor copies the 168-byte record onto its own stack, then moves fields
from there into the object. So every `stfs/stw rN, D(r3)` is one row, provided we
know which stack slot rN was last loaded from.

The copy loop pre-biases both pointers by -4 and steps 8 at a time, so record
field k lands at r1 + 8 + 4k, i.e. k = (slot - 8) / 4.

Reads the disassembly on stdin or from re/asm/8026bfd4.txt.
"""
import re, sys
from pathlib import Path

SRC = Path(sys.argv[1] if len(sys.argv) > 1 else "re/asm/8026bfd4.txt")

LOAD = re.compile(r"^([0-9a-f]{8})\s+[0-9a-f]{8}\s+(lwz|lfs)\s+(\d+), (-?\d+)\(1\)")
STORE = re.compile(r"^([0-9a-f]{8})\s+[0-9a-f]{8}\s+(stw|stfs)\s+(\d+), (-?\d+)\(3\)")

# General and float registers are separate files; lwz/stw use one, lfs/stfs the other.
src = {"i": {}, "f": {}}
rows = []
for line in SRC.read_text().splitlines():
    m = LOAD.match(line.strip())
    if m:
        vma, op, reg, slot = m.group(1), m.group(2), int(m.group(3)), int(m.group(4))
        src["i" if op == "lwz" else "f"][reg] = (slot, vma)
        continue
    m = STORE.match(line.strip())
    if m:
        vma, op, reg, dest = m.group(1), m.group(2), int(m.group(3)), int(m.group(4))
        bank = "i" if op == "stw" else "f"
        got = src[bank].get(reg)
        if not got:
            continue                        # value came from somewhere other than the record
        slot, lvma = got
        if slot < 8 or (slot - 8) % 4:
            continue
        rows.append(((slot - 8) // 4, dest, "int" if bank == "i" else "float", lvma, vma))

rows.sort()
print("| fld | -> A   | type  | load     | store    |")
print("|-----|--------|-------|----------|----------|")
seen = set()
for fld, dest, ty, lv, sv in rows:
    if (fld, dest) in seen:
        continue
    seen.add((fld, dest))
    print("| %3d | 0x%-4X | %-5s | %s | %s |" % (fld, dest, ty, lv, sv))
print("\n%d fields copied into the object, of 42 in the record." % len(seen))
