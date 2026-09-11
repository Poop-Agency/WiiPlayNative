#pragma once

#include <cstdint>
#include <string>
#include <vector>
#include <array>
#include <cmath>
#include "raylib.h"
#include "raymath.h"

// Grid dimensions (16:9 widescreen standard)
constexpr int GRID_WIDTH = 22;
constexpr int GRID_HEIGHT = 17;
constexpr float CELL_SIZE = 2.0f;

// World bounds calculated from grid
constexpr float ARENA_WIDTH = GRID_WIDTH * CELL_SIZE;
constexpr float ARENA_HEIGHT = GRID_HEIGHT * CELL_SIZE;
constexpr float ARENA_HALF_W = ARENA_WIDTH * 0.5f;
constexpr float ARENA_HALF_H = ARENA_HEIGHT * 0.5f;

// One original pixel in world units. A block is 32 px wide and one grid cell,
// so CELL_SIZE / 32 converts every radius main.dol stores in pixels.
constexpr float PX = CELL_SIZE / 32.0f;          // 0.0625

// Collision radii, all read out of main.dol. Each game object keeps its radius
// in field +0x90 and its class' collide method adds its own hard-coded radius
// to the other object's +0x90, so the contact distance is the sum of the two.
//   tank   15 px  Tank::collide  0x8025a3a4 (lfs 15.0, sdata2 0x8045a6e4)
//   shell   6 px  Shell::check   0x80262f80/0x80262fa4/0x8026303c
//   mine   12 px  Mine::collide  0x80267110 (lfs 12.0, sdata2 0x8045a82c)
constexpr float TANK_RADIUS = 15.0f * PX;        // 0.9375
constexpr float TANK_HEIGHT = 0.7f;
constexpr float BULLET_RADIUS = 6.0f * PX;       // 0.375
constexpr float MINE_RADIUS = 12.0f * PX;        // 0.75

// The muzzle is the `cannon` bone of G3D/tnk_tank.brres, not a DOL constant.
// Clamped to the hull radius so the barrel tip can never sit inside a block:
// that is what makes a point-blank shot burst on the wall instead of spawning
// past it. Marked inferred -- the model would give the exact bone offset.
constexpr float BARREL_LENGTH = TANK_RADIUS;

constexpr float BULLET_SPEED_NORMAL = 11.25f;   // field 37 = 3 px/frame at 60 Hz
constexpr float BULLET_SPEED_FAST = BULLET_SPEED_NORMAL * 2.0f;   // TnkGameParam field 37: 6 vs 3

// Mine timing and radii, from Mine::checkCollisions 0x80267484 and the mine
// think function 0x802679f8. Frame counts are at 60 Hz.
//   +0xA0 reaches 480  -> arm, then +0xC0 = 120 frame fuse   (8 s + 2 s)
//   tank within 90 px  -> +0xD2 armed          (lfs 90, 0x802674d8)
//   tank within 70 px  -> +0xC0 = 20 frame fuse (lfs 70, 0x8026751c)
//   blast radius +0xC8 = 80 px                  (stfs 80,  0x80267984)
constexpr float MINE_ARM_RADIUS = 90.0f * PX;        // 5.625
constexpr float MINE_TRIGGER_RADIUS = 70.0f * PX;    // 4.375
constexpr float MINE_BLAST_RADIUS = 80.0f * PX;      // 5.0
constexpr float MINE_ARM_TIME = 480.0f / 60.0f;      // 8.0 s
constexpr float MINE_FUSE_TIME = 120.0f / 60.0f;     // 2.0 s
constexpr float MINE_TRIGGER_FUSE = 20.0f / 60.0f;   // 0.333 s
constexpr float MINE_LIFETIME = MINE_ARM_TIME + MINE_FUSE_TIME;   // 10.0 s

// Guards on the pixel conversion: if CELL_SIZE or PX ever drifts these stop the
// build instead of silently rescaling every collision in the game.
static_assert(PX * 32.0f == CELL_SIZE, "one block must be 32 original pixels");
static_assert(TANK_RADIUS == 0.9375f && BULLET_RADIUS == 0.375f && MINE_RADIUS == 0.75f,
              "collision radii must stay at the 15/6/12 px values read from main.dol");
static_assert(BARREL_LENGTH <= TANK_RADIUS,
              "muzzle must stay inside the hull or point-blank shots spawn past the wall");
static_assert(MINE_LIFETIME == 10.0f, "mine must live 480 + 120 frames at 60 Hz");

// Game tiles and IDs matching original Nintendo data
enum class TileType : uint32_t {
    Empty = 0,
    // Two block families.  The field builder at 0x80265e68 accepts exactly two
    // tile ranges and packs each one as (index << 4) | family: 0x80265f38 does
    // `addi 0,3,-100; slwi 5,0,4` for 100..107 (family bit clear) and 0x80265ff0
    // does `addi 0,3,-200; slwi 0,0,4; ori 5,0,1` for 200..207 (family bit set).
    // So the high nibble is an index 0..7, not a height 1..8 -- an earlier note
    // here said 1..8 and was wrong.  Both families end on the same virtual call
    // (vtable+0x4C on the block manager at [r13-25032]).
    //
    // 101..107 are the cork blocks a mine blast breaks; 201..207 never break.
    CorkBlock = 101,
    CorkBlock2 = 102,
    CorkBlock3 = 103,
    CorkBlock4 = 104,
    CorkBlock5 = 105,
    CorkBlock6 = 106,
    CorkBlock7 = 107,
    // Index 0 exists only in the 200 family: tile 100 appears in no map, 200 in
    // twelve of them, and map 27 (mission 4) is made of nothing else.
    //
    // PARTLY OURS.  The tile id and the (index << 4) | family packing above come
    // straight out of the binary.  That index 0 of that family is a HOLE and not
    // a block does not: it rests on assets/ripped/tnk_block/hole.png and
    // floor_lower.png existing at all, on map 27 being 100% tile 200, and on the
    // player reporting mission 4 as a field of holes.  The virtual call that
    // would settle it forwards through a pool allocator we did not follow.
    Hole = 200,
    SolidBlock1 = 201,
    SolidBlock2 = 202,
    SolidBlock3 = 203,
    SpawnP1 = 300,
    SpawnP2 = 301,
    SpawnEnemyBrown = 400,
    SpawnEnemyAsh = 401,
    SpawnEnemyTeal = 402,
    SpawnEnemyYellow = 403,
    SpawnEnemyRed = 404,
    SpawnEnemyGreen = 405,
    SpawnEnemyPurple = 406,
    SpawnEnemyWhite = 407,
    SpawnEnemyBlack = 408
};

// Tank Types
enum class TankType {
    Player1,
    Player2,
    Player3,
    Player4,
    EnemyBrown,     // Stationary, 1 bullet, 1 ricochet, fires every 5 s
    EnemyAsh,       // Slow wanderer, 1 bullet
    EnemyTeal,      // Slowest mover, fast rocket, fires every 3 s
    EnemyYellow,    // Fast, places mines, flees
    EnemyRed,       // Mobile, 3 bullets, rapid 0.5 s bank shots
    EnemyGreen,     // Stationary, 2 bullets, 2 ricochets, fast shells
    EnemyPurple,    // Mobile assault, 5 bullets, drops mines
    EnemyWhite,     // Invisible stealth assassin, fast bullets
    EnemyBlack      // Elite commander, agile dodging, rockets, mines
};

// Tank configuration properties
struct TankConfig {
    TankType type;
    std::string name;
    Color bodyColor;
    Color treadColor;
    Color turretColor;
    float maxSpeed;
    float turnSpeed;
    int maxBullets;
    int maxBounces;
    float bulletSpeed;
    int maxMines;
    bool isRocket;
    bool hasStealth;
    int pointValue;
    float shootCooldown;   // seconds between shots, TnkGameParam.bin field 36 (frames/60)

    // The AI cadence fields. These are the enemy controller's own timers, not the
    // weapon's: field 36 above says how fast the gun recovers, these say how often
    // the tank decides anything. All are frame counts at 60 Hz except aimSpread.
    // Proven in main.dol by following the record through the AI constructor at
    // 0x8026bfd4, load site then store site, in docs/tnkgameparam.md.
    float aimSpread;       // fld 28 -> A+0x1C   0x8026c0dc lfs / 0x8026c1d0 stfs
    int   reaimFrames;     // fld 39 -> A+0x24   0x8026c108 lwz / 0x8026c1d8 stw
    int   fireDecisionMin; // fld 35 -> A+0x28   0x8026c0f8 lwz / 0x8026c1dc stw
    int   fireDecisionMax; // fld 34 -> A+0x2C   0x8026c0f4 lwz / 0x8026c1e0 stw
    int   mineDecisionMin; // fld 4  -> A+0x54   0x8026c07c lwz / 0x8026c208 stw
    int   mineDecisionMax; // fld 3  -> A+0x58   0x8026c078 lwz / 0x8026c20c stw
    float mineRangePx;     // fld 5  -> A+0x50   0x8026c080 lfs / 0x8026c204 stfs
    float mineChanceFar;   // fld 7  -> A+0x5C   0x8026c088 lfs / 0x8026c210 stfs
    float mineChanceNear;  // fld 6  -> A+0x60   0x8026c084 lfs / 0x8026c214 stfs

    // How far the barrel may swing in one frame, as the tangent of the angle.
    // fld 38 -> A+0x20 (0x8026c104 lfs / 0x8026c1d4 stfs).  The tick reads it at
    // 0x8026bbac and 0x8026bbf4 to build fwd +- s * right, normalises both, and
    // calls 0x801b6ab0 twice; that helper keeps whichever of its last two
    // arguments has the larger dot product with its second, so the pair is a
    // rotate-toward clamped to the cone.  The target direction it turns toward
    // is A+0x80, written at 0x8026cae8 by the aim callee, so this is the turret
    // and not the hull.  Every record carries it, the player's included, but a
    // player's barrel ignores it: it turned 155 deg in 29 frames between two
    // dumps, past the 83 deg this cap allows, while its A+0x8C stayed put.
    float turretSlewTan;   // fld 38 -> A+0x20   0x8026c104 lfs / 0x8026c1d4 stfs

    // How far the tank looks for something to dodge, in original pixels.
    //
    // The movement object M gets these from the record at 0x80269d84: field 18 ->
    // M+0x30 (0x80269e60 lfs / 0x80269f94 stfs) and field 17 -> M+0x38
    // (0x80269e5c lfs / 0x80269f9c stfs).  The shell collector 0x80262134 and the
    // mine collector 0x802666b4 each read one of two ranges depending on a
    // virtual call at vtable+0x98 on the tracked object: non-zero takes field 16
    // or 15, both a flat 120, and zero takes these.  We only model the zero case,
    // because what that virtual call reports is not read yet.
    //
    // Acceptance is `distance < range` (0x80262350 fcmpo + 0x80262354 bf CR0[LT],
    // and 0x80266858/0x8026685c for mines), and for shells also a paired-single
    // dot product greater than zero at 0x80262374, so a shell already moving away
    // is not tracked.  A zero range means the tank does not scan at all, which is
    // how Brown and Green never dodge and how Ash, Teal, Red and Black ignore
    // mines.
    float shellTrackPx;    // fld 18 -> M+0x30   0x80269e60 lfs / 0x80269f94 stfs
    float mineTrackPx;     // fld 17 -> M+0x38   0x80269e5c lfs / 0x80269f9c stfs
};

// Every stat below comes from TnkGameParam.bin. The record layout is proven by the
// loader at 0x80269de8 in main.dol: mulli r4,r4,168 then a 21 x 2-word copy, so each
// tank is 42 words, and lwz/lfs there fix which columns are ints and which are floats.
// Word 0 of the file is a header; the tank records run from word 1, so field j of
// tank t is word 1 + t*42 + j. Field types come from the lwz/lfs mix in that copy
// and agree with the bit patterns in all 342 non-zero fields.
//   fld 0  stealth (set for White alone)     fld 2  mines
//   fld 22 speed      fld 29 bullets         fld 33 ricochets
//   fld 36 shot cooldown in frames at 60 Hz  fld 37 shell speed
//
// The stored speeds are pixels per frame at 60 Hz, which three measurements taken
// frame by frame on the real game confirm against a 736 px arena: the player's 1.8
// crosses it in 6.8 s (measured 7), a shell's 3 in 4.09 s (measured 4), a rocket's
// 6 in 2.04 s (measured 2). A block is 32 px and one cell, so CELL_SIZE 2.0 makes
// a pixel 0.0625 world units and a stored unit 3.75 world units per second.

inline TankConfig GetTankConfig(TankType type) {
    switch (type) {
        case TankType::Player1:
            return { type, "Player 1 (Blue)", { 50, 120, 220, 255 }, { 30, 30, 30, 255 }, { 70, 140, 240, 255 }, 6.75f, 5.0f, 5, 1, BULLET_SPEED_NORMAL, 2, false, false, 0, 0.10f, 40.0f, 4, 30, 45, 40, 60, 100.0f, 10.0f, 100.0f, 0.05f, 60.0f, 120.0f };
        case TankType::Player2:
            return { type, "Player 2 (Red)", { 220, 50, 50, 255 }, { 30, 30, 30, 255 }, { 240, 70, 70, 255 }, 6.75f, 5.0f, 5, 1, BULLET_SPEED_NORMAL, 2, false, false, 0, 0.10f, 40.0f, 4, 30, 45, 40, 60, 100.0f, 10.0f, 100.0f, 0.05f, 60.0f, 120.0f };
        case TankType::Player3:
            return { type, "Player 3 (Green)", { 50, 200, 70, 255 }, { 30, 30, 30, 255 }, { 70, 220, 90, 255 }, 6.75f, 5.0f, 5, 1, BULLET_SPEED_NORMAL, 2, false, false, 0, 0.10f, 40.0f, 4, 30, 45, 40, 60, 100.0f, 10.0f, 100.0f, 0.05f, 60.0f, 120.0f };
        case TankType::Player4:
            return { type, "Player 4 (Yellow)", { 230, 200, 40, 255 }, { 30, 30, 30, 255 }, { 250, 220, 60, 255 }, 6.75f, 5.0f, 5, 1, BULLET_SPEED_NORMAL, 2, false, false, 0, 0.10f, 40.0f, 4, 30, 45, 40, 60, 100.0f, 10.0f, 100.0f, 0.05f, 60.0f, 120.0f };
        
        case TankType::EnemyBrown:
            return { type, "Brown Tank", { 160, 110, 70, 255 }, { 60, 50, 40, 255 }, { 180, 130, 90, 255 }, 0.0f, 2.5f, 1, 1, BULLET_SPEED_NORMAL, 0, false, false, 100, 5.00f, 170.0f, 60, 30, 45, 0, 0, 100.0f, 0.0f, 0.0f, 0.01f, 0.0f, 0.0f };
        case TankType::EnemyAsh:
            return { type, "Ash Tank", { 160, 160, 160, 255 }, { 50, 50, 50, 255 }, { 180, 180, 180, 255 }, 4.5f, 3.0f, 1, 1, BULLET_SPEED_NORMAL, 0, false, false, 200, 3.00f, 40.0f, 45, 30, 45, 0, 0, 100.0f, 0.0f, 0.0f, 0.01f, 40.0f, 0.0f };
        case TankType::EnemyTeal:
            return { type, "Teal Tank", { 40, 190, 190, 255 }, { 30, 60, 60, 255 }, { 60, 210, 210, 255 }, 3.75f, 5.0f, 1, 0, BULLET_SPEED_FAST, 0, true, false, 300, 3.00f, 0.0f, 8, 5, 10, 0, 0, 100.0f, 0.0f, 0.0f, 0.05f, 40.0f, 0.0f };
        case TankType::EnemyYellow:
            return { type, "Yellow Tank", { 230, 210, 50, 255 }, { 60, 60, 20, 255 }, { 250, 230, 70, 255 }, 6.75f, 4.5f, 1, 1, BULLET_SPEED_NORMAL, 4, false, false, 400, 3.00f, 40.0f, 30, 30, 45, 40, 60, 100.0f, 50.0f, 50.0f, 0.02f, 40.0f, 160.0f };
        case TankType::EnemyRed:
            return { type, "Red Tank", { 210, 50, 50, 255 }, { 50, 20, 20, 255 }, { 230, 70, 70, 255 }, 4.5f, 3.5f, 3, 1, BULLET_SPEED_NORMAL, 0, false, false, 500, 0.50f, 40.0f, 20, 5, 10, 0, 0, 100.0f, 0.0f, 0.0f, 0.02f, 40.0f, 0.0f };
        case TankType::EnemyGreen:
            return { type, "Green Tank", { 50, 180, 60, 255 }, { 20, 50, 20, 255 }, { 70, 200, 80, 255 }, 0.0f, 4.0f, 2, 2, BULLET_SPEED_FAST, 0, true, false, 600, 1.00f, 80.0f, 30, 5, 10, 0, 0, 100.0f, 0.0f, 0.0f, 0.02f, 0.0f, 0.0f };
        case TankType::EnemyPurple:
            return { type, "Purple Tank", { 170, 60, 200, 255 }, { 40, 20, 50, 255 }, { 190, 80, 220, 255 }, 6.75f, 4.0f, 5, 1, BULLET_SPEED_NORMAL, 2, false, false, 700, 0.50f, 40.0f, 20, 5, 10, 40, 60, 100.0f, 3.0f, 5.0f, 0.03f, 60.0f, 160.0f };
        case TankType::EnemyWhite:
            return { type, "White Tank", { 240, 240, 245, 255 }, { 80, 80, 80, 255 }, { 255, 255, 255, 255 }, 4.5f, 4.0f, 5, 1, BULLET_SPEED_FAST, 2, false, true, 800, 0.50f, 40.0f, 30, 5, 10, 40, 60, 100.0f, 3.0f, 5.0f, 0.03f, 40.0f, 160.0f };
        case TankType::EnemyBlack:
            return { type, "Black Tank", { 35, 35, 40, 255 }, { 15, 15, 15, 255 }, { 55, 55, 60, 255 }, 9.0f, 5.5f, 3, 0, BULLET_SPEED_FAST, 2, true, false, 1000, 1.00f, 5.0f, 20, 5, 10, 40, 60, 100.0f, 3.0f, 5.0f, 0.03f, 100.0f, 0.0f };
    }
    return { TankType::EnemyBrown, "Unknown", WHITE, BLACK, WHITE, 2.0f, 2.0f, 1, 1, 8.0f, 0, false, false, 100, 1.0f };
}

// Game states
enum class GameScreen {
    Title,
    MissionSelect,
    LobbyMultiplayer,
    Playing,
    StageIntro,
    GameOver,
    Victory
};

enum class GameMode {
    CampaignSingle,
    CampaignCoop,
    PvPDeathmatch
};
