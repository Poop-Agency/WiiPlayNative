#!/usr/bin/env python3
"""Extract DSP-ADPCM waves out of a NW4R .brsar (RSAR v1.1) into WAV files.

vgmstream only opens standalone RWSD/RWAR; this archive stores its samples as
raw DSP blobs referenced by RBNK WAVE tables, so we walk the container by hand.
"""
import struct, sys, wave as wavemod
from pathlib import Path

# ---------- little binary helpers ----------------------------------------
class Buf:
    def __init__(self, data): self.d = data
    def u8(self, o):  return self.d[o]
    def s8(self, o):  return struct.unpack_from(">b", self.d, o)[0]
    def u16(self, o): return struct.unpack_from(">H", self.d, o)[0]
    def s16(self, o): return struct.unpack_from(">h", self.d, o)[0]
    def u32(self, o): return struct.unpack_from(">I", self.d, o)[0]
    def s32(self, o): return struct.unpack_from(">i", self.d, o)[0]


# ---------- Nintendo DSP ADPCM --------------------------------------------
def dsp_decode(data, coef, nsamples):
    """8-byte frames: 1 header byte (scale|index) + 14 nibbles = 14 samples."""
    out = bytearray()
    yn1 = yn2 = 0
    pos = 0
    done = 0
    while done < nsamples and pos + 1 <= len(data):
        ps = data[pos]; pos += 1
        scale = 1 << (ps & 0x0F)
        ci = (ps >> 4) & 0x0F
        c1, c2 = coef[ci * 2], coef[ci * 2 + 1]
        for i in range(14):
            if done >= nsamples or pos >= len(data):
                break
            byte = data[pos + (i >> 1)]
            nib = (byte >> 4) if (i & 1) == 0 else (byte & 0x0F)
            if nib >= 8:
                nib -= 16
            s = (nib * scale << 11) + 1024 + c1 * yn1 + c2 * yn2
            s >>= 11
            s = -32768 if s < -32768 else (32767 if s > 32767 else s)
            yn2, yn1 = yn1, s
            out += struct.pack("<h", s)
            done += 1
        pos += 7
    return bytes(out)


def nibbles_to_samples(n):
    frames, rem = divmod(n, 16)
    return frames * 14 + (rem - 2 if rem > 2 else 0)


# ---------- RSAR walk ------------------------------------------------------
def parse(path):
    b = Buf(Path(path).read_bytes())
    assert b.d[:4] == b"RSAR", "not an RSAR"
    SYMB, INFO = b.u32(0x10), b.u32(0x18)
    ib, sb = INFO + 8, SYMB + 8

    strtab = sb + b.u32(sb)
    names = []
    for i in range(b.u32(strtab)):
        o = sb + b.u32(strtab + 4 + i * 4)
        names.append(b.d[o:b.d.index(b"\0", o)].decode())

    ref = lambda o: ib + b.s32(o + 4)
    table = lambda o: [ref(o + 4 + i * 8) for i in range(b.u32(o))]
    snd_t, bnk_t, ply_t, fil_t, grp_t, _ = [ref(ib + i * 8) for i in range(6)]

    # bank id -> symbol name
    banknames = {}
    for o in table(bnk_t):
        sid, fid = b.s32(o), b.s32(o + 4)
        if 0 <= sid < len(names):
            banknames[fid] = names[sid]

    # file id -> absolute header / wave-blob location
    files = {}
    for g in table(grp_t):
        gh, gw = b.s32(g + 0x10), b.s32(g + 0x18)
        if b.s32(g + 4) == 0:
            continue
        for it in table(ref(g + 0x20)):
            files[b.s32(it)] = dict(hdr=gh + b.s32(it + 4), hsz=b.s32(it + 8),
                                    wav=gw + b.s32(it + 12), wsz=b.s32(it + 16))
    return b, names, banknames, files


def wave_entries(b, hdr):
    """Yield (index, WaveInfo abs offset) for an RBNK/RWSD header."""
    nblocks = b.u16(hdr + 0x0E)
    for i in range(nblocks):
        bo, bs = b.u32(hdr + 0x10 + i * 8), b.u32(hdr + 0x14 + i * 8)
        if b.d[hdr + bo:hdr + bo + 4] != b"WAVE":
            continue
        base = hdr + bo + 8              # DataRef offsets are relative to here
        n = b.u32(base)
        for k in range(n):
            yield k, base + b.s32(base + 4 + k * 8 + 4)


def extract_wave(b, w, blob_base, blob_size):
    fmt      = b.u8(w)
    looped   = b.u8(w + 1)
    nch      = b.u8(w + 2)
    rate     = (b.u8(w + 3) << 16) | b.u16(w + 4)
    loop_beg = b.u32(w + 8)
    loop_end = b.u32(w + 12)
    chtab    = w + b.u32(w + 16)   # plain u32 array, one entry per channel
    dataloc  = blob_base + b.u32(w + 20)

    nsamples = nibbles_to_samples(loop_end) if fmt == 2 else loop_end
    chans = []
    for c in range(nch):
        ci = w + b.u32(chtab + c * 4)
        cdata = dataloc + b.u32(ci)
        adpcm = w + b.u32(ci + 4)
        if fmt == 2:
            coef = [b.s16(adpcm + j * 2) for j in range(16)]
            nbytes = (nsamples + 13) // 14 * 8
            pcm = dsp_decode(b.d[cdata:cdata + nbytes], coef, nsamples)
        elif fmt == 1:
            pcm = b"".join(struct.pack("<h", b.s16(cdata + j * 2)) for j in range(nsamples))
        else:                                    # PCM8
            pcm = b"".join(struct.pack("<h", b.s8(cdata + j) << 8) for j in range(nsamples))
        chans.append(pcm)
    return rate, nch, nsamples, looped, loop_beg, chans


def write_wav(path, rate, chans):
    n = min(len(c) for c in chans) // 2
    if len(chans) == 1:
        data = chans[0][:n * 2]
    else:                                        # interleave
        data = bytearray()
        for i in range(n):
            for c in chans:
                data += c[i * 2:i * 2 + 2]
        data = bytes(data)
    with wavemod.open(str(path), "wb") as f:
        f.setnchannels(len(chans)); f.setsampwidth(2); f.setframerate(rate)
        f.writeframes(data)


def main(src, outdir):
    b, names, banknames, files = parse(src)
    out = Path(outdir); out.mkdir(parents=True, exist_ok=True)
    total = 0
    for fid, v in sorted(files.items()):
        magic = b.d[v["hdr"]:v["hdr"] + 4]
        if magic not in (b"RBNK", b"RWSD") or v["wsz"] == 0:
            continue
        bank = banknames.get(fid, "file%02d" % fid)
        bdir = out / bank
        bdir.mkdir(exist_ok=True)
        print("%s  (file%d, %s, %d bytes of samples)" % (bank, fid, magic.decode(), v["wsz"]))
        for idx, w in wave_entries(b, v["hdr"]):
            try:
                rate, nch, ns, looped, lb, chans = extract_wave(b, w, v["wav"], v["wsz"])
                if ns <= 0 or not chans or not chans[0]:
                    print("   [%02d] empty, skipped" % idx); continue
                name = bdir / ("%02d_%dHz_%.2fs%s.wav" % (idx, rate, ns / rate, "_loop" if looped else ""))
                write_wav(name, rate, chans)
                total += 1
                print("   [%02d] %5dHz %dch %7d smp %6.2fs %s" % (idx, rate, nch, ns, ns / rate, name.name))
            except Exception as e:
                print("   [%02d] FAILED: %s" % (idx, e))
    print("\n%d wav written under %s" % (total, out))


if __name__ == "__main__":
    main(sys.argv[1], sys.argv[2])
