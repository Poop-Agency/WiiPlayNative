#!/usr/bin/env python3
"""Read and disassemble Wii Play's main.dol (stripped PowerPC, no symbols).

  python3 tools/dol.py map                    section table
  python3 tools/dol.py dis 0x800a1234 [n]     disassemble n instructions at a VMA
  python3 tools/dol.py find u32 0x66          VMAs of a 32-bit immediate/word
  python3 tools/dol.py find f32 1.25          VMAs of a float constant
  python3 tools/dol.py xref 0x803247c0        instructions whose lis/addi pair builds that VMA

Disassembly shells out to llvm-mc; binutils here is built x86-64 only.
"""
import struct, subprocess, sys
from pathlib import Path

DOL = Path("/mnt/stockage/ROM/WII/wii_play_extracted/sys/main.dol")


def sections():
    d = DOL.read_bytes()
    out = []
    for i in range(18):
        off = struct.unpack_from(">I", d, i * 4)[0]
        vma = struct.unpack_from(">I", d, 0x48 + i * 4)[0]
        size = struct.unpack_from(">I", d, 0x90 + i * 4)[0]
        if size:
            out.append(("text%d" % i if i < 7 else "data%d" % i, off, vma, size))
    return d, out


def to_off(vma):
    d, secs = sections()
    for _, off, base, size in secs:
        if base <= vma < base + size:
            return d, off + (vma - base)
    raise SystemExit("VMA 0x%08x is not in any section" % vma)


def _decode(chunk):
    """Disassemble a byte string, one instruction per 4-byte word.

    mcpu=750 is the Broadway core: without it llvm-mc reads VSX forms that the
    hardware does not have, swallows 8 bytes for one opcode, and every mnemonic
    after it is off by a word. Any residual mismatch falls back to per-word
    decoding so the listing can never silently misalign.
    """
    hexs = " ".join("0x%02x" % b for b in chunk)
    p = subprocess.run(["llvm-mc", "--disassemble", "--triple=powerpc", "--mcpu=750"],
                       input=hexs, capture_output=True, text=True)
    lines = [l.strip() for l in p.stdout.splitlines() if l.strip() and not l.startswith("\t.")]
    if len(lines) == len(chunk) // 4:
        return lines
    out = []
    for i in range(0, len(chunk), 4):
        q = subprocess.run(["llvm-mc", "--disassemble", "--triple=powerpc", "--mcpu=750"],
                           input=" ".join("0x%02x" % b for b in chunk[i:i + 4]),
                           capture_output=True, text=True)
        got = [l.strip() for l in q.stdout.splitlines() if l.strip() and not l.startswith("\t.")]
        out.append(got[0] if got else "<bad>")
    return out


def dis(vma, count=40):
    d, off = to_off(vma)
    raw = d[off:off + count * 4]
    for i, line in enumerate(_decode(raw)):
        print("%08x  %08x  %s" % (vma + i * 4, struct.unpack_from(">I", raw, i * 4)[0], line))


def find(kind, value):
    d, secs = sections()
    if kind == "f32":
        needle = struct.pack(">f", float(value))
    else:
        needle = struct.pack(">I", int(value, 0) if isinstance(value, str) else value)
    for name, off, vma, size in secs:
        blob = d[off:off + size]
        start = 0
        while True:
            i = blob.find(needle, start)
            if i < 0:
                break
            if i % 4 == 0:
                print("%-7s %08x" % (name, vma + i))
            start = i + 1


def xref(target):
    """lis rX, hi / addi rX, rX, lo pairs that materialise `target`."""
    d, secs = sections()
    hi, lo = (target >> 16) & 0xFFFF, target & 0xFFFF
    if lo & 0x8000:
        hi = (hi + 1) & 0xFFFF
    for name, off, vma, size in secs:
        if not name.startswith("text"):
            continue
        blob = d[off:off + size]
        for i in range(0, size - 8, 4):
            w1 = struct.unpack_from(">I", blob, i)[0]
            if (w1 >> 26) != 15 or (w1 & 0xFFFF) != hi:      # lis
                continue
            reg = (w1 >> 21) & 0x1F
            for j in (i + 4, i + 8, i + 12):
                if j + 4 > size:
                    break
                w2 = struct.unpack_from(">I", blob, j)[0]
                op, rt, ra, imm = w2 >> 26, (w2 >> 21) & 0x1F, (w2 >> 16) & 0x1F, w2 & 0xFFFF
                if ra == reg and imm == lo and op in (14, 32, 36, 48, 52):
                    print("%08x  (%s +%d)" % (vma + i, {14: "addi", 32: "lwz", 36: "stw",
                                                        48: "lfs", 52: "stfs"}[op], j - i))
                    break


if __name__ == "__main__":
    if len(sys.argv) < 2 or sys.argv[1] == "map":
        _, secs = sections()
        print("%-7s %-10s %-10s %s" % ("sec", "fileoff", "vma", "size"))
        for n, o, v, s in secs:
            print("%-7s %-10s %-10s %s" % (n, hex(o), hex(v), hex(s)))
    elif sys.argv[1] == "dis":
        dis(int(sys.argv[2], 0), int(sys.argv[3]) if len(sys.argv) > 3 else 40)
    elif sys.argv[1] == "find":
        find(sys.argv[2], sys.argv[3])
    elif sys.argv[1] == "xref":
        xref(int(sys.argv[2], 0))
