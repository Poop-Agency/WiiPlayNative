import argparse
import sys
import os
import struct

EXPECTED_COUNTS = {
    0: 33256,
    101: 240, 102: 426, 103: 136, 104: 76, 105: 44, 106: 20, 107: 32,
    200: 916, 201: 748, 202: 878, 203: 488, 204: 188, 205: 48, 206: 116, 207: 8,
    300: 120,
    301: 60,
    400: 120, 401: 120, 402: 120, 403: 120, 404: 120, 405: 120, 406: 120, 407: 120
}

def get_expected_filenames():
    names = []
    # The retail disc ships 30 single-player stages and 30 versus stages, each
    # in a 16-wide (_0) and a 22-wide (_1) variant. That is 120 files, not 60
    # P1 stages: TnkMapData_P1_30..59 do not exist anywhere, on disc or here.
    for player in ("P1", "P2"):
        for i in range(30):
            names.append(f"TnkMapData_{player}_{i:02d}_0.bin")
            names.append(f"TnkMapData_{player}_{i:02d}_1.bin")
    return names

def read_map(filepath):
    with open(filepath, 'rb') as f:
        data = f.read()
    if len(data) < 16:
        raise ValueError(f"File {filepath} too small")
    width, height, unk1, unk2 = struct.unpack(">IIII", data[:16])
    expected_size = 16 + width * height * 4
    if len(data) != expected_size:
        raise ValueError(f"File {filepath} has invalid size: {len(data)} != expected {expected_size}")
    
    tiles = []
    for i in range(width * height):
        offset = 16 + i * 4
        tile_id, = struct.unpack(">I", data[offset:offset+4])
        tiles.append(tile_id)
        
    return width, height, tiles

def mode_verify(maps_dir, retail_dir):
    expected_names = get_expected_filenames()
    
    mismatches = 0
    missing = 0
    
    maps_files = set()
    if os.path.exists(maps_dir):
        maps_files = set(f for f in os.listdir(maps_dir) if f.endswith('.bin'))
        
    for name in expected_names:
        map_path = os.path.join(maps_dir, name)
        retail_path = os.path.join(retail_dir, name)
        
        if not os.path.exists(map_path):
            print(f"MISSING in maps_dir: {name}")
            missing += 1
            continue
            
        try:
            with open(map_path, 'rb') as f:
                map_data = f.read()
            with open(retail_path, 'rb') as f:
                retail_data = f.read()
                
            if map_data != retail_data:
                print(f"DIFFERS: {name}")
                mismatches += 1
        except Exception as e:
            print(f"Error checking {name}: {e}")
            mismatches += 1
            
    extra = 0
    for f in maps_files:
        if f not in expected_names:
            print(f"EXTRA in maps_dir: {f}")
            extra += 1
            
    print(f"Summary: {missing} missing, {mismatches} differ, {extra} extra.")
    if missing > 0 or mismatches > 0 or extra > 0:
        return 1
    return 0

def mode_census(maps_dir):
    expected_names = get_expected_filenames()
    counts = {}
    
    for name in expected_names:
        map_path = os.path.join(maps_dir, name)
        if not os.path.exists(map_path):
            print(f"File {map_path} missing.")
            return 1
        
        width, height, tiles = read_map(map_path)
        for tile in tiles:
            counts[tile] = counts.get(tile, 0) + 1
            
    print(f"{'ID':>5} | {'Actual':>8} | {'Expected':>8} | Status")
    print("-" * 40)
    
    all_keys = set(counts.keys()) | set(EXPECTED_COUNTS.keys())
    has_drift = False
    
    for k in sorted(list(all_keys)):
        actual = counts.get(k, 0)
        expected = EXPECTED_COUNTS.get(k, 0)
        status = "OK" if actual == expected else "DRIFT"
        if status == "DRIFT":
            has_drift = True
        print(f"{k:>5} | {actual:>8} | {expected:>8} | {status}")
        
    if has_drift:
        print("Census failed: Drift detected!")
        return 1
    else:
        print("Census passed: All counts match exactly.")
        return 0

def tile_to_char(tile_id):
    if tile_id == 0:
        return '.'
    if 101 <= tile_id <= 107:
        return 'c'
    if 200 <= tile_id <= 207:
        return '#'
    if tile_id == 300:
        return '1'
    if tile_id == 301:
        return '2'
    if 400 <= tile_id <= 407:
        return str(tile_id - 400)
    return '?'
    
def mode_show(maps_dir, nn_str):
    nn = int(nn_str)
    name0 = f"TnkMapData_P1_{nn:02d}_0.bin"
    name1 = f"TnkMapData_P1_{nn:02d}_1.bin"
    
    path0 = os.path.join(maps_dir, name0)
    path1 = os.path.join(maps_dir, name1)
    
    w0, h0, tiles0 = read_map(path0)
    w1, h1, tiles1 = read_map(path1)
    
    enemies0 = sorted(list(set(t for t in tiles0 if 400 <= t <= 407)))
    enemies1 = sorted(list(set(t for t in tiles1 if 400 <= t <= 407)))
    
    print(f"Map {nn:02d} side-by-side:")
    print(f"{name0:<30} {name1}")
    print(f"Enemies: {enemies0} {' '*(30 - len(str(enemies0)) - 9)} Enemies: {enemies1}")
    print()
    
    for y in range(max(h0, h1)):
        row0 = ""
        if y < h0:
            row0 = "".join(tile_to_char(tiles0[y*w0 + x]) for x in range(w0))
        row0 = row0.ljust(w0)
        
        row1 = ""
        if y < h1:
            row1 = "".join(tile_to_char(tiles1[y*w1 + x]) for x in range(w1))
            
        print(f"{row0}      {row1}")
        
    print("\nLegend:")
    print("  . : Empty (0)")
    print("  c : Cork (101-107)")
    print("  # : Solid (200-207)")
    print("  1 : P1 Spawn (300)")
    print("  2 : P2 Spawn (301)")
    print(" 0-7: Enemy Spawn (400-407)")
    return 0

def mode_selftest(maps_dir):
    expected_names = get_expected_filenames()
    counts = {}
    
    print("Running selftest...")
    
    for i, name in enumerate(expected_names):
        map_path = os.path.join(maps_dir, name)
        assert os.path.exists(map_path), f"File {map_path} does not exist"
        
        with open(map_path, 'rb') as f:
            data = f.read()
        assert len(data) >= 16, f"File {map_path} is too small"
        
        w, h, u1, u2 = struct.unpack(">IIII", data[:16])
        assert h == 17, f"File {map_path} height is {h}, expected 17"
        if name.endswith("_0.bin"):
            assert w == 16, f"File {map_path} width is {w}, expected 16"
        else:
            assert w == 22, f"File {map_path} width is {w}, expected 22"
            
        expected_size = 16 + w * h * 4
        assert len(data) == expected_size, f"File {map_path} size {len(data)} != {expected_size}"
        
        tiles = []
        for j in range(w * h):
            offset = 16 + j * 4
            tile_id, = struct.unpack(">I", data[offset:offset+4])
            tiles.append(tile_id)
            
        for tile in tiles:
            counts[tile] = counts.get(tile, 0) + 1
            
    print("PASS: Header parsing and file dimensions match expectations (16x17 and 22x17).")
    
    for k, v in EXPECTED_COUNTS.items():
        assert counts.get(k, 0) == v, f"Count drift for ID {k}: expected {v}, got {counts.get(k, 0)}"
    for k in counts.keys():
        assert k in EXPECTED_COUNTS, f"Unexpected ID {k} found with count {counts[k]}"
        
    print("PASS: Full census matches exactly with expected totals.")
    print("Selftest completed successfully.")
    return 0

def main():
    parser = argparse.ArgumentParser(description="Wii Tanks map check tool")
    parser.add_argument("--maps-dir", default="assets/maps", help="Path to assets/maps")
    parser.add_argument("--retail-dir", default="/mnt/stockage/ROM/WII/extracted_tnk_common/MapData", help="Path to retail MapData")
    
    parser.add_argument("--verify", action="store_true", help="Verify maps byte-for-byte")
    parser.add_argument("--census", action="store_true", help="Verify tile counts")
    parser.add_argument("--show", type=str, metavar="NN", help="Show map NN side-by-side")
    parser.add_argument("--selftest", action="store_true", help="Run self tests")
    
    args = parser.parse_args()
    
    modes_selected = sum([bool(args.verify), bool(args.census), bool(args.show), bool(args.selftest)])
    if modes_selected == 0:
        parser.print_help()
        return 1
    
    if args.verify:
        sys.exit(mode_verify(args.maps_dir, args.retail_dir))
    if args.census:
        sys.exit(mode_census(args.maps_dir))
    if args.show is not None:
        sys.exit(mode_show(args.maps_dir, args.show))
    if args.selftest:
        sys.exit(mode_selftest(args.maps_dir))
        
if __name__ == "__main__":
    main()
