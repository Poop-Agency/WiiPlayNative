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
constexpr float BULLET_SPEED_NORMAL = 9.5f;
constexpr float BULLET_SPEED_FAST = 16.0f;
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
    EnemyBrown,     // Stationary, 1 bullet, slow, no bounce
    EnemyAsh,       // Slow wanderer, 1 bullet
    EnemyTeal,      // Slowest mover, fast rocket, fires every 3 s
    EnemyYellow,    // Fast, places mines, flees
    EnemyRed,       // Mobile, 3 bullets, rapid 0.5 s bank shots
    EnemyGreen,     // Stationary, rapid-fire, 2-bounce ricochets
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
    float shootCooldown;   // seconds between shots, from TnkGameParam.bin col 37 (frames/60)
};

// Speeds below are TnkGameParam.bin col 23 scaled by 2.5, the factor that maps the
// player's stored 1.8 onto the 4.5 this build already used. Bullet counts and mine
// counts already matched the file. Ricochet counts are NOT in that table -- Brown 0
// and Black 0 come from observed original behaviour, everything else is unchanged.

inline TankConfig GetTankConfig(TankType type) {
    switch (type) {
        case TankType::Player1:
            return { type, "Player 1 (Blue)", { 50, 120, 220, 255 }, { 30, 30, 30, 255 }, { 70, 140, 240, 255 }, 4.5f, 5.0f, 5, 1, BULLET_SPEED_NORMAL, 2, false, false, 0, 0.10f };
        case TankType::Player2:
            return { type, "Player 2 (Red)", { 220, 50, 50, 255 }, { 30, 30, 30, 255 }, { 240, 70, 70, 255 }, 4.5f, 5.0f, 5, 1, BULLET_SPEED_NORMAL, 2, false, false, 0, 0.10f };
        case TankType::Player3:
            return { type, "Player 3 (Green)", { 50, 200, 70, 255 }, { 30, 30, 30, 255 }, { 70, 220, 90, 255 }, 4.5f, 5.0f, 5, 1, BULLET_SPEED_NORMAL, 2, false, false, 0, 0.10f };
        case TankType::Player4:
            return { type, "Player 4 (Yellow)", { 230, 200, 40, 255 }, { 30, 30, 30, 255 }, { 250, 220, 60, 255 }, 4.5f, 5.0f, 5, 1, BULLET_SPEED_NORMAL, 2, false, false, 0, 0.10f };
        
        case TankType::EnemyBrown:
            return { type, "Brown Tank", { 160, 110, 70, 255 }, { 60, 50, 40, 255 }, { 180, 130, 90, 255 }, 0.0f, 2.5f, 1, 0, BULLET_SPEED_NORMAL * 0.85f, 0, false, false, 100, 5.00f };
        case TankType::EnemyAsh:
            return { type, "Ash Tank", { 160, 160, 160, 255 }, { 50, 50, 50, 255 }, { 180, 180, 180, 255 }, 3.0f, 3.0f, 1, 1, BULLET_SPEED_NORMAL * 0.85f, 0, false, false, 200, 3.00f };
        case TankType::EnemyTeal:
            return { type, "Teal Tank", { 40, 190, 190, 255 }, { 30, 60, 60, 255 }, { 60, 210, 210, 255 }, 2.5f, 5.0f, 1, 1, BULLET_SPEED_FAST, 0, true, false, 300, 3.00f };
        case TankType::EnemyYellow:
            return { type, "Yellow Tank", { 230, 210, 50, 255 }, { 60, 60, 20, 255 }, { 250, 230, 70, 255 }, 4.5f, 4.5f, 1, 1, BULLET_SPEED_NORMAL, 4, false, false, 400, 3.00f };
        case TankType::EnemyRed:
            return { type, "Red Tank", { 210, 50, 50, 255 }, { 50, 20, 20, 255 }, { 230, 70, 70, 255 }, 3.0f, 3.5f, 3, 1, BULLET_SPEED_NORMAL * 1.15f, 0, false, false, 500, 0.50f };
        case TankType::EnemyGreen:
            return { type, "Green Tank", { 50, 180, 60, 255 }, { 20, 50, 20, 255 }, { 70, 200, 80, 255 }, 0.0f, 4.0f, 2, 2, BULLET_SPEED_FAST * 1.05f, 0, true, false, 600, 1.00f };
        case TankType::EnemyPurple:
            return { type, "Purple Tank", { 170, 60, 200, 255 }, { 40, 20, 50, 255 }, { 190, 80, 220, 255 }, 4.5f, 4.0f, 5, 1, BULLET_SPEED_NORMAL, 2, false, false, 700, 0.50f };
        case TankType::EnemyWhite:
            return { type, "White Tank", { 240, 240, 245, 255 }, { 80, 80, 80, 255 }, { 255, 255, 255, 255 }, 3.0f, 4.0f, 5, 1, BULLET_SPEED_FAST, 2, false, true, 800, 0.50f };
        case TankType::EnemyBlack:
            return { type, "Black Tank", { 35, 35, 40, 255 }, { 15, 15, 15, 255 }, { 55, 55, 60, 255 }, 6.0f, 5.5f, 3, 0, BULLET_SPEED_FAST, 2, true, false, 1000, 1.00f };
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
