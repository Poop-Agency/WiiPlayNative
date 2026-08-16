import struct
import sys
import os

def parse_game_param(filepath):
    with open(filepath, 'rb') as f:
        data = f.read()
    
    words = struct.unpack(f'>{len(data)//4}I', data)
    missions = []
    for i in range(100):
        start = 421 + i * 22
        end = start + 22
        missions.append(words[start:end])
        
    return missions

def parse_map_file(filepath):
    with open(filepath, 'rb') as f:
        data = f.read()
    
    header = struct.unpack('>4I', data[:16])
    width, height = header[0], header[1]
    
    tiles = struct.unpack(f'>{width*height}I', data[16:])
    
    enemy_counts = [0] * 9
    for tile in tiles:
        if 400 <= tile <= 408:
            enemy_counts[tile - 400] += 1
            
    return enemy_counts

def analyze_columns(missions):
    print("=== Column Analysis ===")
    for col in range(22):
        values = [m[col] for m in missions]
        min_val = min(values)
        max_val = max(values)
        distinct = len(set(values))
        is_monotonic_inc = all(values[i] <= values[i+1] for i in range(len(values)-1))
        is_monotonic_dec = all(values[i] >= values[i+1] for i in range(len(values)-1))
        
        corr_inc = "inc" if is_monotonic_inc else "dec" if is_monotonic_dec else "no"
        print(f"Col {col:2}: range {min_val:4}..{max_val:4}, {distinct:3} distinct, monotonic={corr_inc}")
    print()

def main():
    if len(sys.argv) < 3:
        print("Usage: python tools/mission_table.py <param.bin> <map_dir>")
        return
        
    param_path = sys.argv[1]
    map_dir = sys.argv[2]
    
    missions = parse_game_param(param_path)
    
    print("=== First 5 missions ===")
    for i in range(5):
        print(missions[i])
    print()
    
    analyze_columns(missions)
    
    # Check map capacities
    map_counts = {}
    for p in [1, 2]:
        for i in range(30):
            for layout in [0, 1]:
                filepath = os.path.join(map_dir, f'TnkMapData_P{p}_{i:02d}_{layout}.bin')
                if os.path.exists(filepath):
                    map_counts[(p, i, layout)] = parse_map_file(filepath)
    
    # Are all maps exactly [1,1,1,1,1,1,1,1,0]?
    all_same = True
    for k, counts in map_counts.items():
        if counts != [1,1,1,1,1,1,1,1,0]:
            all_same = False
            # print(f"Map {k} has different counts: {counts}")
    print(f"All 120 maps have exactly [1,1,1,1,1,1,1,1,0] spawn points: {all_same}")
    
    # Test hypothesis
    # If a group of 8 is per-enemy-type counts then:
    # 1. their sum should be that mission's total enemy count
    # 2. that sum should grow roughly with mission number
    # 3. no per-type count should exceed what the corresponding map can actually spawn
    
    print("=== Hypothesis Test ===")
    matches = 0
    refuted = 0
    
    for i, m in enumerate(missions):
        p = m[0] + 1
        map_idx = m[18]
        g1 = m[2:10]
        g2 = m[10:18]
        
        m0_caps = map_counts.get((p, map_idx, 0), [0]*9)
        m1_caps = map_counts.get((p, map_idx, 1), [0]*9)
        
        valid = True
        for t in range(8):
            if g1[t] > m0_caps[t] or g2[t] > m1_caps[t]:
                valid = False
                break
                
        if valid:
            matches += 1
        else:
            refuted += 1
            
        if i == 1:
            print(f"Mission {i}: map P{p}_{map_idx:02d}, Brown requested g1={g1[0]}, g2={g2[0]}. Map caps: m0={m0_caps[0]}, m1={m1_caps[0]}")
        if i == 4:
            print(f"Mission {i}: map P{p}_{map_idx:02d}, Ash requested g1={g1[1]}, g2={g2[1]}. Map caps: m0={m0_caps[1]}, m1={m1_caps[1]}")
            
    print(f"Match rate: {matches} of 100 missions match exactly.")
    
    # Check sum growth
    sums_g1 = [sum(m[2:10]) for m in missions]
    print(f"First 10 g1 sums: {sums_g1[:10]}")
    print(f"Last 10 g1 sums: {sums_g1[-10:]}")
    
    # Full dump
    print("\n=== Full Dump ===")
    for i, m in enumerate(missions):
        if i < 5:  # Keep output small, user wants first 5 only in reply
            print(f"M{i:02d}: {m}")

if __name__ == '__main__':
    main()
