#!/usr/bin/env python3
"""Unpack Wii U8 archives and decode the GameCube/Wii TPL textures inside.

  python3 tools/wiiassets.py list   <archive.carc>
  python3 tools/wiiassets.py unpack <archive.carc> <outdir>
  python3 tools/wiiassets.py tpl    <file.tpl> <outdir>

The textures belong to the game and stay out of git; see assets/ripped/README.
"""
import struct, sys, os
from pathlib import Path
from PIL import Image

U8_MAGIC = 0x55AA382D
TPL_MAGIC = 0x0020AF30


# ---------------------------------------------------------------- U8 archives

def u8_nodes(d):
    magic, root, hdr_size, data_off = struct.unpack_from(">IIII", d, 0)
    if magic != U8_MAGIC:
        raise SystemExit("not a U8 archive (magic %08x)" % magic)
    count = struct.unpack_from(">I", d, root + 8)[0]
    strings = root + count * 12
    out = []
    for i in range(count):
        t, n1, n2, n3 = struct.unpack_from(">BBHI", d, root + i * 12)
        name_off = (n1 << 16) | n2
        off, size = struct.unpack_from(">II", d, root + i * 12 + 4)
        end = d.index(b"\0", strings + name_off)
        out.append((t, d[strings + name_off:end].decode("ascii", "replace"), off, size))
    return out


def u8_walk(d):
    """Yield (path, offset, size) for every file, rebuilding directory nesting."""
    nodes = u8_nodes(d)
    stack, out = [("", len(nodes))], []
    for i, (t, name, off, size) in enumerate(nodes):
        if i == 0:
            continue
        while len(stack) > 1 and i >= stack[-1][1]:
            stack.pop()
        if t == 1:
            stack.append((stack[-1][0] + name + "/", size))
        else:
            out.append((stack[-1][0] + name, off, size))
    return out


# ------------------------------------------------------------------- TPL/GX

def _rgb565(v):
    return ((v >> 11) * 255 // 31, ((v >> 5) & 63) * 255 // 63, (v & 31) * 255 // 31, 255)


def _rgb5a3(v):
    if v & 0x8000:                                    # opaque, 5 bits per channel
        return (((v >> 10) & 31) * 255 // 31, ((v >> 5) & 31) * 255 // 31,
                (v & 31) * 255 // 31, 255)
    return (((v >> 8) & 15) * 17, ((v >> 4) & 15) * 17, (v & 15) * 17,
            ((v >> 12) & 7) * 255 // 7)               # 4 bits per channel + 3 bit alpha


def _blocks(w, h, bw, bh):
    for by in range(0, h, bh):
        for bx in range(0, w, bw):
            yield bx, by


def _decode(fmt, data, w, h, pal):
    px = [(0, 0, 0, 0)] * (w * h)

    def put(x, y, c):
        if x < w and y < h:
            px[y * w + x] = c

    i = 0
    if fmt in (0, 8):                                             # I4 / C4, 8x8 nibbles
        for bx, by in _blocks(w, h, 8, 8):
            for y in range(8):
                for x in range(0, 8, 2):
                    b = data[i]; i += 1
                    for k, v in ((0, b >> 4), (1, b & 15)):
                        put(bx + x + k, by + y, pal[v] if pal else (v * 17,) * 3 + (255,))
    elif fmt in (1, 9):                                           # I8 / C8, 8x4
        for bx, by in _blocks(w, h, 8, 4):
            for y in range(4):
                for x in range(8):
                    v = data[i]; i += 1
                    put(bx + x, by + y, pal[v] if pal else (v, v, v, 255))
    elif fmt == 2:                                                # IA4, 8x4
        for bx, by in _blocks(w, h, 8, 4):
            for y in range(4):
                for x in range(8):
                    b = data[i]; i += 1
                    l = (b & 15) * 17
                    put(bx + x, by + y, (l, l, l, (b >> 4) * 17))
    elif fmt in (3, 4, 5, 10):                                    # IA8/RGB565/RGB5A3/C14X2, 4x4
        for bx, by in _blocks(w, h, 4, 4):
            for y in range(4):
                for x in range(4):
                    v = struct.unpack_from(">H", data, i)[0]; i += 2
                    if fmt == 3:
                        l = v & 0xFF
                        c = (l, l, l, v >> 8)
                    elif fmt == 4:
                        c = _rgb565(v)
                    elif fmt == 5:
                        c = _rgb5a3(v)
                    else:
                        c = pal[v & 0x3FFF] if pal else (0, 0, 0, 0)
                    put(bx + x, by + y, c)
    elif fmt == 6:                                                # RGBA32, 4x4, two planes
        for bx, by in _blocks(w, h, 4, 4):
            ar = data[i:i + 32]; gb = data[i + 32:i + 64]; i += 64
            for y in range(4):
                for x in range(4):
                    j = (y * 4 + x) * 2
                    put(bx + x, by + y, (gb[j], gb[j + 1], ar[j + 1], ar[j]))
    elif fmt == 14:                                               # CMPR: DXT1 in 8x8 tiles
        for bx, by in _blocks(w, h, 8, 8):
            for sy in (0, 4):
                for sx in (0, 4):
                    c0, c1 = struct.unpack_from(">HH", data, i)
                    bits = struct.unpack_from(">I", data, i + 4)[0]; i += 8
                    a, b = _rgb565(c0), _rgb565(c1)
                    if c0 > c1:
                        cols = [a, b,
                                tuple((2 * a[k] + b[k]) // 3 for k in range(3)) + (255,),
                                tuple((a[k] + 2 * b[k]) // 3 for k in range(3)) + (255,)]
                    else:
                        cols = [a, b,
                                tuple((a[k] + b[k]) // 2 for k in range(3)) + (255,),
                                (0, 0, 0, 0)]
                    for y in range(4):
                        for x in range(4):
                            sel = (bits >> (30 - 2 * (y * 4 + x))) & 3
                            put(bx + sx + x, by + sy + y, cols[sel])
    else:
        raise ValueError("unsupported TPL format %d" % fmt)
    return px


def tpl_images(d):
    magic, ntex, hdr = struct.unpack_from(">III", d, 0)
    if magic != TPL_MAGIC:
        raise SystemExit("not a TPL (magic %08x)" % magic)
    for n in range(ntex):
        img_off, pal_off = struct.unpack_from(">II", d, hdr + n * 8)
        pal = None
        if pal_off:
            cnt, _unpacked, pfmt, pdata = struct.unpack_from(">HBBI", d, pal_off)
            pal = []
            for k in range(cnt):
                v = struct.unpack_from(">H", d, pdata + k * 2)[0]
                pal.append({0: lambda x: ((x & 0xFF),) * 3 + (x >> 8,),
                            1: _rgb565, 2: _rgb5a3}[pfmt](v))
        h, w, fmt = struct.unpack_from(">HHI", d, img_off)
        data_off = struct.unpack_from(">I", d, img_off + 8)[0]
        yield n, w, h, fmt, _decode(fmt, d[data_off:], w, h, pal)


def write_tpl(path, outdir, stem):
    d = Path(path).read_bytes()
    made = []
    for n, w, h, fmt, px in tpl_images(d):
        im = Image.new("RGBA", (w, h))
        im.putdata(px)
        name = "%s.png" % stem if n == 0 else "%s_%d.png" % (stem, n)
        out = Path(outdir) / name
        out.parent.mkdir(parents=True, exist_ok=True)
        im.save(out)
        made.append((name, w, h, fmt))
    return made


if __name__ == "__main__":
    cmd = sys.argv[1] if len(sys.argv) > 1 else "list"
    if cmd == "list":
        for p, o, s in u8_walk(Path(sys.argv[2]).read_bytes()):
            print("%-58s %8d" % (p, s))
    elif cmd == "unpack":
        d = Path(sys.argv[2]).read_bytes()
        out = Path(sys.argv[3])
        for p, o, s in u8_walk(d):
            f = out / p
            f.parent.mkdir(parents=True, exist_ok=True)
            f.write_bytes(d[o:o + s])
        print("unpacked %d files to %s" % (len(u8_walk(d)), out))
    elif cmd == "tpl":
        for name, w, h, fmt in write_tpl(sys.argv[2], sys.argv[3],
                                         Path(sys.argv[2]).stem):
            print("%-40s %4dx%-4d fmt %d" % (name, w, h, fmt))


# ----------------------------------------------------------------- BRRES/TEX0

def brres_textures(d):
    """Yield (name, w, h, fmt, pixels) for every TEX0 section in a BRRES blob.

    TEX0 sections are found by scanning for the magic rather than by walking the
    resource-group tree: the groups nest differently between files here and the
    sections are self-describing anyway.
    """
    pos = 0
    while True:
        pos = d.find(b"TEX0", pos)
        if pos < 0:
            return
        try:
            data_off, name_off = struct.unpack_from(">II", d, pos + 0x10)
            w, h = struct.unpack_from(">HH", d, pos + 0x1C)
            fmt = struct.unpack_from(">I", d, pos + 0x20)[0]
            if not (0 < w <= 2048 and 0 < h <= 2048) or fmt > 14:
                pos += 4
                continue
            n_end = d.index(b"\0", pos + name_off)
            name = d[pos + name_off:n_end].decode("ascii", "replace")
            px = _decode(fmt, d[pos + data_off:], w, h, None)
        except Exception:
            pos += 4
            continue
        yield name or "tex_%06x" % pos, w, h, fmt, px
        pos += 4
