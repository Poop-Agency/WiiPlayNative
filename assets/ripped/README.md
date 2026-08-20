# Ripped textures

Not in git, and not redistributable: these are Nintendo's assets, decoded from a
copy of Wii Play you own. The extractor is in the repo, the output is not.

Regenerate:

    python3 tools/wiiassets.py unpack \
        /mnt/stockage/ROM/WII/wii_play_extracted/files/Common/RPTnkScene/common.carc /tmp/tnk
    python3 tools/rip_textures.py

100 textures come out of the eight BRRES files under G3D/, in GX formats CMPR,
RGB565 and RGB5A3. Names are the originals: tnk_tank/tank_brown.png and so on.
