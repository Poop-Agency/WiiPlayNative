#!/usr/bin/env python3
"""Read and disassemble Wii Play's main.dol (stripped PowerPC, no symbols).

  python3 tools/dol.py map                    section table
  python3 tools/dol.py dis 0x800a1234 [n]     disassemble n instructions at a VMA
  python3 tools/dol.py fn 0x800a1234          bracket the function containing a VMA
  python3 tools/dol.py find u32 0x66          VMAs of a 32-bit immediate/word
  python3 tools/dol.py find f32 1.25          VMAs of a float constant
  python3 tools/dol.py xref 0x803247c0        instructions whose lis/addi pair builds that VMA
  python3 tools/dol.py selftest               check the paired-single decoding

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
        return [_ps(struct.unpack_from(">I", chunk, i * 4)[0]) or l
                for i, l in enumerate(lines)]
    out = []
    for i in range(0, len(chunk), 4):
        q = subprocess.run(["llvm-mc", "--disassemble", "--triple=powerpc", "--mcpu=750"],
                           input=" ".join("0x%02x" % b for b in chunk[i:i + 4]),
                           capture_output=True, text=True)
        got = [l.strip() for l in q.stdout.splitlines() if l.strip() and not l.startswith("\t.")]
        w = struct.unpack_from(">I", chunk, i)[0]
        out.append(_ps(w) or (got[0] if got else "<bad>"))
    return out


# ---------------------------------------------------------------------------
# Broadway / Gekko paired-single decoding.
#
# llvm-mc has no Gekko target: primary opcode 4 and 56/57/60/61 are the
# paired-single space on this core, but upstream PowerPC reads them as VSX/lq
# forms, so every ps_* word comes out as `lq` or `<bad>`. Game vector math is
# built almost entirely from these, so an undecoded ps_* word is not a cosmetic
# gap -- it silently hides the arithmetic. Decode them here and override llvm.
#
# A-form operands live at fixed bit positions (big-endian bit 0 = MSB):
#   D = 21..25   A = 16..20   B = 11..15   C = 6..10   XO5 = 1..5   Rc = 0
# XO10 (bits 21..30) selects the non-arithmetic forms. Checking XO10 first is
# unambiguous: every XO10 opcode below reduces to an XO5 value that is absent
# from the XO5 table (528 -> 16, 40/72/136/264 -> 8, 1014 -> 22, 6/7 -> 6/7,
# 0/32/64/96 -> 0), so the two tables cannot collide.
# ---------------------------------------------------------------------------

# XO5 -> (mnemonic, operand shape)
#   ab  = D, A, B      ac  = D, A, C      acb = D, A, C, B      b = D, B
_PS_XO5 = {
    10: ("ps_sum0", "acb"), 11: ("ps_sum1", "acb"),
    12: ("ps_muls0", "ac"), 13: ("ps_muls1", "ac"),
    14: ("ps_madds0", "acb"), 15: ("ps_madds1", "acb"),
    18: ("ps_div", "ab"), 20: ("ps_sub", "ab"), 21: ("ps_add", "ab"),
    23: ("ps_sel", "acb"), 24: ("ps_res", "b"), 25: ("ps_mul", "ac"),
    26: ("ps_rsqrte", "b"),
    28: ("ps_msub", "acb"), 29: ("ps_madd", "acb"),
    30: ("ps_nmsub", "acb"), 31: ("ps_nmadd", "acb"),
}

_PS_XO10 = {
    0: ("ps_cmpu0", "cmp"), 32: ("ps_cmpo0", "cmp"),
    64: ("ps_cmpu1", "cmp"), 96: ("ps_cmpo1", "cmp"),
    40: ("ps_neg", "b"), 72: ("ps_mr", "b"),
    136: ("ps_nabs", "b"), 264: ("ps_abs", "b"),
    528: ("ps_merge00", "ab"), 560: ("ps_merge01", "ab"),
    592: ("ps_merge10", "ab"), 624: ("ps_merge11", "ab"),
    6: ("psq_lx", "qx"), 38: ("psq_lux", "qx"),
    7: ("psq_stx", "qx"), 39: ("psq_stux", "qx"),
    1014: ("dcbz_l", "ab_gpr"),
}

# primary opcode -> mnemonic, for the quantized load/store immediate forms
_PS_QMEM = {56: "psq_l", 57: "psq_lu", 60: "psq_st", 61: "psq_stu"}


def _ps(w):
    """Decode one paired-single word, or return None if it is not one."""
    op = w >> 26
    if op in _PS_QMEM:
        # | opcd(6) | D(5) | A(5) | W(1) | I(3) | d(12) |
        d = w & 0xFFF
        if d & 0x800:
            d -= 0x1000
        return "%s f%d, %d(r%d), %d, %d" % (
            _PS_QMEM[op], (w >> 21) & 0x1F, d, (w >> 16) & 0x1F,
            (w >> 15) & 1, (w >> 12) & 7)
    if op != 4:
        return None

    D, A, B, C = (w >> 21) & 0x1F, (w >> 16) & 0x1F, (w >> 11) & 0x1F, (w >> 6) & 0x1F
    rc = "." if w & 1 else ""

    ent = _PS_XO10.get((w >> 1) & 0x3FF)
    if ent is None:
        ent = _PS_XO5.get((w >> 1) & 0x1F)
        if ent is None:
            return None
    name, shape = ent

    if shape == "ab":
        return "%s%s f%d, f%d, f%d" % (name, rc, D, A, B)
    if shape == "ac":
        return "%s%s f%d, f%d, f%d" % (name, rc, D, A, C)
    if shape == "acb":
        return "%s%s f%d, f%d, f%d, f%d" % (name, rc, D, A, C, B)
    if shape == "b":
        return "%s%s f%d, f%d" % (name, rc, D, B)
    if shape == "cmp":
        return "%s cr%d, f%d, f%d" % (name, D >> 2, A, B)
    if shape == "qx":
        return "%s f%d, r%d, r%d, %d, %d" % (name, D, A, B, (w >> 10) & 1, (w >> 7) & 7)
    if shape == "ab_gpr":
        return "%s r%d, r%d" % (name, A, B)
    return None


def selftest():
    """VEC3Length at 0x800e82e0 has known semantics, so it pins the decoding.

    x^2 and y^2 come from one psq_l plus a ps_mul, z^2 is folded in with a
    ps_madd, and ps_sum0 adds the two halves of the pair together. Any bit-field
    slip in _ps shows up here as a wrong register number or a missing mnemonic.
    """
    want = [
        (0xE0030000, "psq_l f0, 0(r3), 0, 0"),
        (0x10000032, "ps_mul f0, f0, f0"),
        (0x1021007A, "ps_madd f1, f1, f1, f0"),
        (0x10210014, "ps_sum0 f1, f1, f0, f0"),
    ]
    for w, exp in want:
        got = _ps(w)
        assert got == exp, "0x%08x -> %r, expected %r" % (w, got, exp)
    assert _ps(0xC0230008) is None, "lfs must not be claimed by the ps decoder"
    assert _ps(0x4E800020) is None, "blr must not be claimed by the ps decoder"
    print("ps decoding OK (%d words)" % len(want))


# r2 is the read-only small-data base, set in __init_registers at 0x8006330c.
# Float constants live below it, so `lfs fN, -0x1234(r2)` is a literal.
SDA2 = 0x8045EF00


def _f32(vma):
    d, off = to_off(vma)
    return struct.unpack_from(">f", d, off)[0]


def dis(vma, count=40):
    d, off = to_off(vma)
    raw = d[off:off + count * 4]
    for i, line in enumerate(_decode(raw)):
        a = vma + i * 4
        w = struct.unpack_from(">I", raw, i * 4)[0]
        note = ""
        if (w >> 26) in (48, 52) and ((w >> 16) & 0x1F) == 2:
            disp = w & 0xFFFF
            disp -= 0x10000 if disp & 0x8000 else 0
            try:
                note = "   ; = %g" % _f32(SDA2 + disp)
            except SystemExit:
                pass
        elif (w >> 26) == 18:
            t = w & 0x03FFFFFC
            t -= 0x04000000 if t & 0x02000000 else 0
            note = "   ; -> %08x" % ((a + t) if not (w & 2) else t)
        print("%08x  %08x  %-34s%s" % (a, w, line, note))


def _w32(vma):
    d, off = to_off(vma)
    return struct.unpack_from(">I", d, off)[0]


def bounds(vma):
    """Start and end of the function containing a VMA.

    A Metrowerks prologue is `stwu r1, -N(r1)` reached right after a blr or an
    unconditional branch, which is enough to bracket a function here.
    """
    p = vma
    while p > 0x80004000:
        if (_w32(p) >> 16) == 0x9421 and (_w32(p - 4) == 0x4E800020 or (_w32(p - 4) >> 26) == 18):
            break
        p -= 4
    e = vma
    while _w32(e) != 0x4E800020:
        e += 4
    return p, e


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
    elif sys.argv[1] == "fn":
        q = int(sys.argv[2], 0)
        a, b = bounds(q)
        print("start %08x  end %08x  (%d instructions)" % (a, b, (b - a) // 4 + 1))
        # The walk-back only recognises `stwu r1, -N(r1)` as a prologue, but
        # Metrowerks schedules loads ahead of it, so a function can begin with
        # an lfs/lwz and the walk sails past into the previous function. Say so
        # instead of reporting a confident wrong start.
        if a != q and (_w32(q) >> 16) != 0x9421:
            print("  warning: start is a guess -- %08x is not itself a prologue," % q)
            print("  and a function may begin with a scheduled load before `stwu`.")
        n = 0
        while n < 2000:
            w = _w32(q + n * 4)
            n += 1
            if w == 0x4E800020 or ((w >> 26) == 18 and not (w & 1)):
                break
        print("  forward from %08x: %d instructions to the next blr/tail-branch" % (q, n))
    elif sys.argv[1] == "dis":
        dis(int(sys.argv[2], 0), int(sys.argv[3]) if len(sys.argv) > 3 else 40)
    elif sys.argv[1] == "find":
        find(sys.argv[2], sys.argv[3])
    elif sys.argv[1] == "xref":
        xref(int(sys.argv[2], 0))
    elif sys.argv[1] == "selftest":
        selftest()
