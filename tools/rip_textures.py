#!/usr/bin/env python3
"""Decode every TEX0 in the unpacked Tanks BRRES files to assets/ripped/."""
import sys
from pathlib import Path
from PIL import Image

sys.path.insert(0, str(Path(__file__).parent))
from wiiassets import brres_textures

SRC = Path(sys.argv[1] if len(sys.argv) > 1 else "/tmp/tnk/G3D")
OUT = Path(__file__).parent.parent / "assets" / "ripped"

n = 0
for f in sorted(SRC.glob("*.brres")):
    for name, w, h, fmt, px in brres_textures(f.read_bytes()):
        d = OUT / f.stem
        d.mkdir(parents=True, exist_ok=True)
        im = Image.new("RGBA", (w, h))
        im.putdata(px)
        im.save(d / (name + ".png"))
        n += 1
print("wrote %d textures to %s" % (n, OUT))
