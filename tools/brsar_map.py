#!/usr/bin/env python3
"""Resolve each Wii Play Tanks sound name to the PCM sample it actually plays.

The sound effects are not stored as one wave per name. 115 of the 116 entries in
this archive are soundType 1, a sequence: the name points at a label in an RSEQ,
the sequence selects a program in an RBNK, and the program's region table names
the WAVE index that finally holds the samples. Numbering the extracted waves in
name order therefore cannot work, which is why the existing RP_TNK_SE_*.wav
filenames are wrong.

  python3 tools/brsar_map.py            resolve every label
  python3 tools/brsar_map.py --verify   compare against ear-identified clues
"""
import sys, struct
from pathlib import Path

sys.path.insert(0, str(Path(__file__).parent))
from brsar_extract import parse

BRSAR = "/mnt/stockage/ROM/WII/wii_play_extracted/files/Sound/RPTnkScene/rp_Tnk_sound.brsar"

# Sequence opcodes that carry a fixed number of trailing bytes. Anything not
# listed and below 0xE0 takes one byte; 0xE0..0xEF take two.
VLQ_OPS = {0x80, 0x81}          # wait, prg
OFF_OPS = {0x89, 0x8A}          # jump, call  (3-byte offset)


def blocks(b, hdr):
    out = {}
    for i in range(b.u16(hdr + 0x0E)):
        bo, bs = b.u32(hdr + 0x10 + i * 8), b.u32(hdr + 0x14 + i * 8)
        out[b.d[hdr + bo:hdr + bo + 4]] = (hdr + bo, bs)
    return out


def labels(b, hdr):
    """Yield (name, offset-from-seq-base) for every label in an RSEQ."""
    blk = blocks(b, hdr)
    # LABL is a plain u32 count followed by u32 offsets, not 8-byte DataRefs.
    lb = blk[b"LABL"][0] + 8
    for i in range(b.u32(lb)):
        o = lb + b.u32(lb + 4 + i * 4)
        ln = b.u32(o + 4)
        yield b.d[o + 8:o + 8 + ln].decode("ascii", "replace"), b.u32(o)


def vlq(d, p):
    v = 0
    while True:
        c = d[p]; p += 1
        v = (v << 7) | (c & 0x7F)
        if not c & 0x80:
            return v, p


def walk(d, base, start, limit=400):
    """Run the sequence far enough to collect the programs and notes it plays.

    Follows one level of open-track and jump so a wrapper label that only opens a
    track still reaches the notes. Returns (programs, notes).
    """
    progs, notes, seen = [], [], set()
    todo = [start]
    while todo and limit > 0:
        p = todo.pop(0)
        if p in seen:
            continue
        seen.add(p)
        while limit > 0:
            limit -= 1
            if base + p >= len(d):
                break
            op = d[base + p]; p += 1
            if op < 0x80:                       # note on: velocity + VLQ length
                notes.append(op)
                p += 1
                _, p = vlq(d, base + p)
                p -= base
            elif op in VLQ_OPS:
                v, np = vlq(d, base + p)
                p = np - base
                if op == 0x81:
                    progs.append(v)
            elif op in (0x88,):                 # open track: track + 3-byte off
                trk = d[base + p]
                off = int.from_bytes(d[base + p + 1:base + p + 4], "big")
                p += 4
                todo.append(off)
            elif op in OFF_OPS:
                off = int.from_bytes(d[base + p:base + p + 3], "big")
                p += 3
                todo.append(off)
                if op == 0x89:
                    break
            elif op == 0xA0 or op == 0xA1:      # random / variable prefix
                continue                        # next byte is the real command
            elif op == 0xFE:
                p += 2
            elif op in (0xFD, 0xFF):
                break
            elif 0xE0 <= op <= 0xEF:
                p += 2
            elif op == 0xFC:
                continue
            else:
                p += 1
    return progs, notes


def bank_regions(b, hdr):
    """program -> [(keyLo, keyHi, waveIndex)] from an RBNK instrument table.

    Each table entry and each subregion is an 8-byte reference: u8 refType,
    u8 dataType, u16 reserved, s32 offset relative to the start of the DATA
    block payload. dataType 1 is a leaf whose first word is the wave index,
    2 a key-range table, 3 a dense index table.
    """
    blk = blocks(b, hdr)
    if b"DATA" not in blk:
        return {}
    base = blk[b"DATA"][0] + 8
    out = {}
    for i in range(b.u32(base)):
        e = base + 4 + i * 8
        off = b.s32(e + 4)
        if off > 0:
            out[i] = leaf(b, base, base + off, b.u8(e + 1))
    return out


def leaf(b, base, o, typ, lo=0, hi=127, depth=0):
    if depth > 4:
        return []
    if typ == 1:
        return [(lo, hi, b.s32(o))]
    if typ == 2:                                # key-range table
        n = b.u8(o)
        keys = [b.u8(o + 1 + k) for k in range(n)]
        tbl = o + 1 + n
        tbl += (-(tbl - base)) % 4
        res, prev = [], lo
        for k in range(n):
            e = tbl + k * 8
            off = b.s32(e + 4)
            if off > 0:
                res += leaf(b, base, base + off, b.u8(e + 1), prev, keys[k], depth + 1)
            prev = keys[k] + 1
        return res
    if typ == 3:                                # dense index table
        a, z = b.u8(o), b.u8(o + 1)
        res = []
        for k in range(z - a + 1):
            e = o + 4 + k * 8
            off = b.s32(e + 4)
            if off > 0:
                res += leaf(b, base, base + off, b.u8(e + 1), lo, hi, depth + 1)
        return res
    return []


CLUES = {0: "player firing", 4: "rocket exploding on wall", 7: "ricochet",
         9: "mine planted", 10: "mine beep", 12: "tank rolling"}
WEAK = {2: "rocket sound 1?", 3: "rocket sound 2?", 11: "mine timer?"}


def main(verify=False):
    b, names, banknames, files = parse(BRSAR)
    seq_hdr = files[10]["hdr"]
    blk = blocks(b, seq_hdr)
    dblk, dsz = blk[b"DATA"]
    seq_base = dblk + b.u32(dblk + 8)          # dataOffset is from block start
    regions = bank_regions(b, files[21]["hdr"])

    rows = []
    for name, off in labels(b, seq_hdr):
        progs, notes = walk(b.d, seq_base, off)
        waves = []
        for pr in progs:
            for lo, hi, wi in regions.get(pr, []):
                if not notes or any(lo <= n <= hi for n in notes):
                    if wi not in waves:
                        waves.append(wi)
        rows.append((name, off, progs, notes, waves))

    print("%-34s %5s %-10s %-10s %s" % ("name", "off", "prg", "note", "wave"))
    for name, off, progs, notes, waves in sorted(rows):
        print("%-34s %5d %-10s %-10s %s" % (
            name, off,
            ",".join(map(str, progs)) or "-",
            ",".join(map(str, notes)) or "-",
            ",".join(map(str, waves)) or "UNRESOLVED"))

    unres = [r[0] for r in rows if not r[4]]
    print("\nresolved %d / %d" % (len(rows) - len(unres), len(rows)))
    if unres:
        print("unresolved:", ", ".join(unres))

    if verify:
        print("\n-- against the ear-identified clues --")
        byw = {}
        for name, off, progs, notes, waves in rows:
            for w in waves:
                byw.setdefault(w, []).append(name)
        for w, desc in sorted({**CLUES, **WEAK}.items()):
            tag = "clue" if w in CLUES else "weak"
            got = byw.get(w, [])
            print("wave %-3d %-4s %-28s -> %s" % (
                w, tag, desc, ", ".join(got) if got else "NOTHING RESOLVES HERE"))


if __name__ == "__main__":
    main("--verify" in sys.argv)
