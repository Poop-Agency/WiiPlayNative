# Wii Play: Tanks! Native (PPC Source Port)

An authentic, native C++20 source port and static decompilation project of **Wii Play: Tanks!** (*Chars d'assaut*) with online multiplayer support via ENet UDP, 3D modern rendering via Raylib, authentic physical parameters, and all 100 official Nintendo missions.

---

## Features

- **Authentic Mission Loader**: Directly parses original binary stage maps (`TnkMapData_P1_*.bin` & `TnkMapData_P2_*.bin`) extracted from `main.dol` and `common.carc`.
- **Accurate Physics & Ballistics**:
  - Continuous DDA raycasting for bullet trajectories.
  - Authentic 1-bounce / 2-bounce ricochet reflections.
  - Dynamic block destruction (cork destructible blocks & solid stone obstacles).
  - Mid-air bullet collisions canceling each other out.
  - Timed proximity landmines with chain-reaction detonations.
- **Complete AI Suite (9 Enemy Tank Types)**:
  - **Brown**: Stationary turret with direct line of sight.
  - **Ash**: Slow wanderer with single shots.
  - **Teal**: Fast scout firing high-velocity rockets.
  - **Yellow**: Evasive runner that plants mines in chokepoints.
  - **Red**: Ricochet marksman calculating 1-bounce bank shots off walls.
  - **Green**: Rapid-fire machine gun sniper calculating 2-bounce bank shots.
  - **Purple**: Mobile assault unit with 5-bullet salvos and mine traps.
  - **White**: Cloaked stealth assassin with active camouflage.
  - **Black**: Elite commander with predictive leading aim and bullet dodging.
- **Low-Latency Online Multiplayer (ENet UDP)**:
  - Host/Client architecture.
  - **Coop Campaign Mode**: 2 to 4 players teaming up against all missions.
  - **PvP Deathmatch Mode**: Arena combat.
- **Graphics & Controls**:
  - 3D Table arena, lighting, tank models with independent rotating turrets and recoil animations.
  - 3 Camera modes (3D Isometric, Dynamic 3D, and Classic 2D Top-Down toggle via `[C]`).
  - Native Keyboard & Mouse aim (`WASD` / `ZQSD` + Cursor aim) and full Gamepad support.

---

## Building from Source

### Prerequisites (Arch / CachyOS / Ubuntu / Fedora)
- CMake 3.20+
- GCC / Clang (supporting C++20)
- Ninja
- `libenet` (`sudo pacman -S enet` or `sudo apt install libenet-dev`)

### Build instructions
```bash
git clone https://github.com/Poop-Agency/WiiPlayNative.git
cd WiiPlayNative
cmake -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build
```

### Running the game
```bash
./build/wii_tanks
```

---

## Controls

| Action | Keyboard / Mouse | Gamepad |
| :--- | :--- | :--- |
| **Move Tank** | `W/A/S/D` or `Z/Q/S/D` / Arrows | Left Analog Stick |
| **Aim Turret** | Mouse Cursor | Right Analog Stick |
| **Fire Bullet** | Left Mouse Button / `J` | Right Trigger / Button A |
| **Plant Mine** | Right Mouse Button / `Space` / `K` | Left Trigger / Button B |
| **Toggle Camera** | `C` | Select / Back |
| **Menu / Pause** | `Escape` | Start |

---

## Repository Structure

```
├── assets/
│   ├── maps/            # All 120 official binary level maps
│   ├── param/           # Original TnkGameParam.bin
│   └── textures/
├── include/
│   ├── Common.hpp       # Constants, tile types, tank specs
│   ├── Level.hpp        # Map loader, spatial grid, DDA raycaster
│   ├── Tank.hpp         # Tank entity and movement
│   ├── Bullet.hpp       # Projectile ballistics and ricochet
│   ├── Mine.hpp         # Landmines and proximity trigger
│   ├── AI.hpp           # 9 Enemy AI behaviors & ricochet solvers
│   ├── Particle.hpp     # Tread decals, sparks, debris, explosions
│   ├── Audio.hpp        # Procedural sound synthesis and FX
│   ├── Protocol.hpp     # ENet binary network protocol
│   ├── Network.hpp      # Host/Client replication manager
│   ├── Renderer3D.hpp   # 3D arena, tank models and HUD
│   ├── GameState.hpp    # Mission progression and state machine
│   └── Engine.hpp       # Main engine loop
├── src/                 # Implementation files
└── CMakeLists.txt
```
