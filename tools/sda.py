#!/usr/bin/env python3
"""Find every access to an SDA-relative global, and the code that writes it.

The AI code reaches its manager objects through r13/r2 rather than lis/addi
pairs, so `dol.py xref` cannot see them. Usage:

  python3 tools/sda.py 0x80456d10      # by absolute VMA
  python3 tools/sda.py -25008          # by r13 displacement
"""
import struct, sys
sys.path.insert(0, "tools")
import dol

SDA1 = 0x8045CEC0  # r13, from __init_registers @ 0x80006290
SDA2 = 0x8045EF00  # r2

# D-form opcodes; stores are what identify the owner.
OPC = {32: "lwz", 34: "lbz", 36: "stw", 37: "stwu", 38: "stb",
       40: "lhz", 44: "sth", 48: "lfs", 52: "stfs"}


def accesses(disp, base_reg):
    d, secs = dol.sections()
    imm = disp & 0xFFFF
    for name, off, vma, size in secs:
        if not name.startswith("text"):
            continue
        for i in range(0, size - 3, 4):
            w = struct.unpack_from(">I", d, off + i)[0]
            if (w & 0xFFFF) != imm:
                continue
            op = w >> 26
            if op not in OPC or ((w >> 16) & 0x1F) != base_reg:
                continue
            yield vma + i, OPC[op], (w >> 21) & 0x1F


def main():
    arg = int(sys.argv[1], 0)
    if arg >= 0x80000000:
        disp, base_reg, base = arg - SDA1, 13, SDA1
        if not -0x8000 <= disp < 0x8000:
            disp, base_reg, base = arg - SDA2, 2, SDA2
    else:
        disp, base_reg, base = arg, 13, SDA1

    print("global 0x%08X  =  r%d %+d\n" % (base + disp, base_reg, disp))
    writes = []
    reads = 0
    for vma, op, reg in accesses(disp, base_reg):
        if op.startswith("st"):
            writes.append((vma, op, reg))
        else:
            reads += 1
    for vma, op, reg in writes:
        start, _ = dol.bounds(vma)
        print("WRITE 0x%08X  %-4s r%d      (in function 0x%08X)" % (vma, op, reg, start))
    print("\n%d write(s), %d read(s)" % (len(writes), reads))


if __name__ == "__main__":
    main()
