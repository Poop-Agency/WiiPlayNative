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

// Tank and physical constants
constexpr float TANK_RADIUS = 0.75f;
constexpr float TANK_HEIGHT = 0.7f;
constexpr float BULLET_RADIUS = 0.18f;
constexpr float BARREL_LENGTH = 1.25f;   // longer than TANK_RADIUS: muzzle can overlap a wall
constexpr float BULLET_SPEED_NORMAL = 11.25f;   // field 37 = 3 px/frame at 60 Hz
constexpr float BULLET_SPEED_FAST = BULLET_SPEED_NORMAL * 2.0f;   // TnkGameParam field 37: 6 vs 3
constexpr float MINE_RADIUS = 0.45f;
constexpr float MINE_BLAST_RADIUS = 3.2f;
constexpr float MINE_LIFETIME = 10.0f;

// Game tiles and IDs matching original Nintendo data
enum class TileType : uint32_t {
    Empty = 0,
    CorkBlock = 101,      // Destructible cork wall
    SolidBlock = 102,     // Indestructible stone wall
    BlockVariant1 = 103,
    BlockVariant2 = 104,
    Hole = 200,           // Trench/hole (bullets fly over, tanks cannot pass)
    Hole2 = 201,
    Hole3 = 202,
    Hole4 = 203,
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
            return { type, "Player 1 (Blue)", { 50, 120, 220, 255 }, { 30, 30, 30, 255 }, { 70, 140, 240, 255 }, 6.75f, 5.0f, 5, 1, BULLET_SPEED_NORMAL, 2, false, false, 0, 0.10f };
        case TankType::Player2:
            return { type, "Player 2 (Red)", { 220, 50, 50, 255 }, { 30, 30, 30, 255 }, { 240, 70, 70, 255 }, 6.75f, 5.0f, 5, 1, BULLET_SPEED_NORMAL, 2, false, false, 0, 0.10f };
        case TankType::Player3:
            return { type, "Player 3 (Green)", { 50, 200, 70, 255 }, { 30, 30, 30, 255 }, { 70, 220, 90, 255 }, 6.75f, 5.0f, 5, 1, BULLET_SPEED_NORMAL, 2, false, false, 0, 0.10f };
        case TankType::Player4:
            return { type, "Player 4 (Yellow)", { 230, 200, 40, 255 }, { 30, 30, 30, 255 }, { 250, 220, 60, 255 }, 6.75f, 5.0f, 5, 1, BULLET_SPEED_NORMAL, 2, false, false, 0, 0.10f };
        
        case TankType::EnemyBrown:
            return { type, "Brown Tank", { 160, 110, 70, 255 }, { 60, 50, 40, 255 }, { 180, 130, 90, 255 }, 0.0f, 2.5f, 1, 1, BULLET_SPEED_NORMAL, 0, false, false, 100, 5.00f };
        case TankType::EnemyAsh:
            return { type, "Ash Tank", { 160, 160, 160, 255 }, { 50, 50, 50, 255 }, { 180, 180, 180, 255 }, 4.5f, 3.0f, 1, 1, BULLET_SPEED_NORMAL, 0, false, false, 200, 3.00f };
        case TankType::EnemyTeal:
            return { type, "Teal Tank", { 40, 190, 190, 255 }, { 30, 60, 60, 255 }, { 60, 210, 210, 255 }, 3.75f, 5.0f, 1, 0, BULLET_SPEED_FAST, 0, true, false, 300, 3.00f };
        case TankType::EnemyYellow:
            return { type, "Yellow Tank", { 230, 210, 50, 255 }, { 60, 60, 20, 255 }, { 250, 230, 70, 255 }, 6.75f, 4.5f, 1, 1, BULLET_SPEED_NORMAL, 4, false, false, 400, 3.00f };
        case TankType::EnemyRed:
            return { type, "Red Tank", { 210, 50, 50, 255 }, { 50, 20, 20, 255 }, { 230, 70, 70, 255 }, 4.5f, 3.5f, 3, 1, BULLET_SPEED_NORMAL, 0, false, false, 500, 0.50f };
        case TankType::EnemyGreen:
            return { type, "Green Tank", { 50, 180, 60, 255 }, { 20, 50, 20, 255 }, { 70, 200, 80, 255 }, 0.0f, 4.0f, 2, 2, BULLET_SPEED_FAST, 0, true, false, 600, 1.00f };
        case TankType::EnemyPurple:
            return { type, "Purple Tank", { 170, 60, 200, 255 }, { 40, 20, 50, 255 }, { 190, 80, 220, 255 }, 6.75f, 4.0f, 5, 1, BULLET_SPEED_NORMAL, 2, false, false, 700, 0.50f };
        case TankType::EnemyWhite:
            return { type, "White Tank", { 240, 240, 245, 255 }, { 80, 80, 80, 255 }, { 255, 255, 255, 255 }, 4.5f, 4.0f, 5, 1, BULLET_SPEED_FAST, 2, false, true, 800, 0.50f };
        case TankType::EnemyBlack:
            return { type, "Black Tank", { 35, 35, 40, 255 }, { 15, 15, 15, 255 }, { 55, 55, 60, 255 }, 9.0f, 5.5f, 3, 0, BULLET_SPEED_FAST, 2, true, false, 1000, 1.00f };
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
