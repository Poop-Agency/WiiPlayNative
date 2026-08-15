#include "Level.hpp"
#include <fstream>
#include <iostream>
#include <sstream>
#include <iomanip>
#include <algorithm>

Level::Level()
    : m_width(GRID_WIDTH)
    , m_height(GRID_HEIGHT)
    , m_currentMission(1)
    , m_player1Spawn{0.0f, 0.0f}
    , m_player2Spawn{0.0f, 0.0f}
{
    m_grid.resize(m_width * m_height, TileType::Empty);
}

Level::~Level() {}

void Level::Reset() {
    std::fill(m_grid.begin(), m_grid.end(), TileType::Empty);
    m_enemySpawns.clear();
    m_player1Spawn = { -ARENA_HALF_W + CELL_SIZE * 2, 0.0f };
    m_player2Spawn = { -ARENA_HALF_W + CELL_SIZE * 2, CELL_SIZE * 2 };
}

// Authentic Wii Play Tanks! 100 Missions definitions
MissionDef Level::GetMissionDef(int missionNumber) {
    if (missionNumber < 1) missionNumber = 1;

    // Missions 1 to 20 (Fixed classic campaign progression)
    static const std::vector<MissionDef> s_classicMissions = {
        /* Mission 1  */ { 0,  { TankType::EnemyBrown } },
        /* Mission 2  */ { 1,  { TankType::EnemyAsh, TankType::EnemyAsh } },
        /* Mission 3  */ { 2,  { TankType::EnemyBrown, TankType::EnemyAsh } },
        /* Mission 4  */ { 3,  { TankType::EnemyBrown, TankType::EnemyAsh, TankType::EnemyAsh } },
        /* Mission 5  */ { 4,  { TankType::EnemyTeal, TankType::EnemyBrown } },
        /* Mission 6  */ { 5,  { TankType::EnemyTeal, TankType::EnemyAsh, TankType::EnemyAsh } },
        /* Mission 7  */ { 6,  { TankType::EnemyTeal, TankType::EnemyTeal, TankType::EnemyBrown } },
        /* Mission 8  */ { 7,  { TankType::EnemyYellow, TankType::EnemyAsh, TankType::EnemyAsh } },
        /* Mission 9  */ { 8,  { TankType::EnemyYellow, TankType::EnemyYellow, TankType::EnemyTeal } },
        /* Mission 10 */ { 9,  { TankType::EnemyRed, TankType::EnemyAsh, TankType::EnemyAsh } },
        /* Mission 11 */ { 10, { TankType::EnemyRed, TankType::EnemyYellow, TankType::EnemyBrown } },
        /* Mission 12 */ { 11, { TankType::EnemyRed, TankType::EnemyRed, TankType::EnemyTeal } },
        /* Mission 13 */ { 12, { TankType::EnemyGreen, TankType::EnemyAsh, TankType::EnemyAsh } },
        /* Mission 14 */ { 13, { TankType::EnemyGreen, TankType::EnemyTeal, TankType::EnemyYellow } },
        /* Mission 15 */ { 14, { TankType::EnemyGreen, TankType::EnemyGreen, TankType::EnemyRed } },
        /* Mission 16 */ { 15, { TankType::EnemyPurple, TankType::EnemyYellow, TankType::EnemyYellow } },
        /* Mission 17 */ { 16, { TankType::EnemyPurple, TankType::EnemyGreen, TankType::EnemyTeal } },
        /* Mission 18 */ { 17, { TankType::EnemyPurple, TankType::EnemyPurple, TankType::EnemyRed } },
        /* Mission 19 */ { 18, { TankType::EnemyWhite, TankType::EnemyTeal, TankType::EnemyTeal } },
        /* Mission 20 */ { 19, { TankType::EnemyBlack, TankType::EnemyRed, TankType::EnemyRed } }
    };

    if (missionNumber <= 20) {
        return s_classicMissions[missionNumber - 1];
    }

    // Missions 21 to 100 (Escalating difficulties across the 30 map layouts)
    int mapIdx = (missionNumber - 1) % 30;
    std::vector<TankType> enemies;

    int tier = (missionNumber - 21) / 10; // 0 to 7
    int enemyCount = std::min(8, 3 + (missionNumber / 18));

    // Fill with escalating high-tier enemies
    for (int i = 0; i < enemyCount; ++i) {
        if (missionNumber == 100) {
            // Ultimate Mission 100 Boss
            enemies = { 
                TankType::EnemyBlack, TankType::EnemyBlack, 
                TankType::EnemyWhite, TankType::EnemyWhite, 
                TankType::EnemyPurple, TankType::EnemyPurple, 
                TankType::EnemyGreen, TankType::EnemyGreen 
            };
            break;
        }

        if (missionNumber % 10 == 0 && i == 0) {
            // Every 10 missions: Boss Black tank
            enemies.push_back(TankType::EnemyBlack);
            continue;
        }

        int randChoice = (missionNumber * 7 + i * 13) % 100;
        if (tier >= 5 && randChoice > 75) {
            enemies.push_back(TankType::EnemyWhite);
        } else if (tier >= 4 && randChoice > 55) {
            enemies.push_back(TankType::EnemyPurple);
        } else if (tier >= 3 && randChoice > 40) {
            enemies.push_back(TankType::EnemyGreen);
        } else if (tier >= 2 && randChoice > 25) {
            enemies.push_back(TankType::EnemyRed);
        } else if (tier >= 1 && randChoice > 15) {
            enemies.push_back(TankType::EnemyYellow);
        } else {
            enemies.push_back(TankType::EnemyTeal);
        }
    }

    return { mapIdx, enemies };
}

bool Level::LoadMission(int missionNumber, bool is2Player) {
    m_currentMission = missionNumber;
    MissionDef def = GetMissionDef(missionNumber);

    std::string prefix = is2Player ? "TnkMapData_P2_" : "TnkMapData_P1_";
    
    std::ostringstream ss;
    ss << "assets/maps/" << prefix << std::setfill('0') << std::setw(2) << def.mapIndex << "_1.bin";
    
    if (LoadFromBinary(ss.str(), def.enemies)) {
        return true;
    }

    // Fallback try non-variant 0
    std::ostringstream ssFallback;
    ssFallback << "assets/maps/" << prefix << std::setfill('0') << std::setw(2) << def.mapIndex << "_0.bin";
    return LoadFromBinary(ssFallback.str(), def.enemies);
}

bool Level::LoadFromBinary(const std::string& filepath, const std::vector<TankType>& missionEnemies) {
    std::ifstream file(filepath, std::ios::binary);
    if (!file.is_open()) {
        std::cerr << "Failed to open map file: " << filepath << std::endl;
        return false;
    }

    // Read header (16 bytes)
    auto readBE32 = [](std::ifstream& f) -> uint32_t {
        unsigned char b[4];
        f.read(reinterpret_cast<char*>(b), 4);
        return (uint32_t(b[0]) << 24) | (uint32_t(b[1]) << 16) | (uint32_t(b[2]) << 8) | uint32_t(b[3]);
    };

    uint32_t rawW = readBE32(file);
    uint32_t rawH = readBE32(file);
    readBE32(file); // unk1
    readBE32(file); // unk2

    m_width = rawW;
    m_height = rawH;
    m_grid.assign(m_width * m_height, TileType::Empty);
    m_enemySpawns.clear();

    struct PotentialSpawn {
        Vector2 worldPos;
        int gridX;
        int gridY;
    };
    std::vector<PotentialSpawn> potentialSpawns;

    for (int y = 0; y < m_height; ++y) {
        for (int x = 0; x < m_width; ++x) {
            uint32_t val = readBE32(file);
            TileType tile = static_cast<TileType>(val);
            int idx = y * m_width + x;

            Vector2 wPos = GridToWorld(x, y);

            if (tile == TileType::SpawnP1) {
                m_player1Spawn = wPos;
                m_grid[idx] = TileType::Empty;
            } else if (tile == TileType::SpawnP2) {
                m_player2Spawn = wPos;
                m_grid[idx] = TileType::Empty;
            } else if (val >= 400 && val <= 408) {
                potentialSpawns.push_back({ wPos, x, y });
                m_grid[idx] = TileType::Empty;
            } else {
                m_grid[idx] = tile;
            }
        }
    }

    // Assign exactly the enemies required for this mission into available map spawn slots
    for (size_t i = 0; i < missionEnemies.size(); ++i) {
        TankType eType = missionEnemies[i];
        if (i < potentialSpawns.size()) {
            m_enemySpawns.push_back({ eType, potentialSpawns[i].worldPos, potentialSpawns[i].gridX, potentialSpawns[i].gridY });
        } else {
            // Fallback spawn across table if more enemies than map markers
            float offset = float(i) * 3.0f;
            Vector2 fallbackPos = { ARENA_HALF_W - 4.0f, -ARENA_HALF_H + 4.0f + offset };
            m_enemySpawns.push_back({ eType, fallbackPos, m_width - 3, 3 + int(i) });
        }
    }

    std::cout << "Loaded Mission " << m_currentMission << ": Map " << filepath 
              << " (" << m_width << "x" << m_height << ") with " 
              << m_enemySpawns.size() << " authentic enemy tanks." << std::endl;
    return true;
}

bool Level::IsInBounds(int gx, int gy) const {
    return gx >= 0 && gx < m_width && gy >= 0 && gy < m_height;
}

TileType Level::GetTile(int gx, int gy) const {
    if (!IsInBounds(gx, gy)) return TileType::SolidBlock;
    return m_grid[gy * m_width + gx];
}

void Level::SetTile(int gx, int gy, TileType type) {
    if (IsInBounds(gx, gy)) {
        m_grid[gy * m_width + gx] = type;
    }
}

bool Level::IsSolid(int gx, int gy) const {
    if (!IsInBounds(gx, gy)) return true;
    TileType t = m_grid[gy * m_width + gx];
    return t == TileType::CorkBlock || t == TileType::SolidBlock || 
           t == TileType::BlockVariant1 || t == TileType::BlockVariant2;
}

bool Level::IsHole(int gx, int gy) const {
    if (!IsInBounds(gx, gy)) return false;
    uint32_t val = static_cast<uint32_t>(m_grid[gy * m_width + gx]);
    return (val >= 200 && val <= 207);
}

bool Level::IsDestructible(int gx, int gy) const {
    if (!IsInBounds(gx, gy)) return false;
    TileType t = m_grid[gy * m_width + gx];
    return t == TileType::CorkBlock || t == TileType::BlockVariant1;
}

bool Level::DestroyBlock(int gx, int gy) {
    if (IsDestructible(gx, gy)) {
        m_grid[gy * m_width + gx] = TileType::Empty;
        return true;
    }
    return false;
}

Vector2 Level::GridToWorld(int gx, int gy) const {
    float x = (gx - m_width * 0.5f + 0.5f) * CELL_SIZE;
    float y = (gy - m_height * 0.5f + 0.5f) * CELL_SIZE;
    return { x, y };
}

void Level::WorldToGrid(Vector2 worldPos, int& outGx, int& outGy) const {
    outGx = static_cast<int>(std::floor(worldPos.x / CELL_SIZE + m_width * 0.5f));
    outGy = static_cast<int>(std::floor(worldPos.y / CELL_SIZE + m_height * 0.5f));
}

bool Level::CheckTankCollision(Vector2 pos, float radius, Vector2& outPushback) const {
    outPushback = { 0.0f, 0.0f };
    bool collided = false;

    float halfArenaW = (m_width * 0.5f) * CELL_SIZE;
    float halfArenaH = (m_height * 0.5f) * CELL_SIZE;

    if (pos.x - radius < -halfArenaW) {
        outPushback.x += (-halfArenaW) - (pos.x - radius);
        collided = true;
    }
    if (pos.x + radius > halfArenaW) {
        outPushback.x += (halfArenaW) - (pos.x + radius);
        collided = true;
    }
    if (pos.y - radius < -halfArenaH) {
        outPushback.y += (-halfArenaH) - (pos.y - radius);
        collided = true;
    }
    if (pos.y + radius > halfArenaH) {
        outPushback.y += (halfArenaH) - (pos.y + radius);
        collided = true;
    }

    int centerGx, centerGy;
    WorldToGrid(pos, centerGx, centerGy);

    for (int dy = -1; dy <= 1; ++dy) {
        for (int dx = -1; dx <= 1; ++dx) {
            int gx = centerGx + dx;
            int gy = centerGy + dy;

            if (IsSolid(gx, gy) || IsHole(gx, gy)) {
                Vector2 tileCenter = GridToWorld(gx, gy);
                float halfCell = CELL_SIZE * 0.5f;

                float closestX = std::clamp(pos.x, tileCenter.x - halfCell, tileCenter.x + halfCell);
                float closestY = std::clamp(pos.y, tileCenter.y - halfCell, tileCenter.y + halfCell);

                float distX = pos.x - closestX;
                float distY = pos.y - closestY;
                float distSq = distX * distX + distY * distY;

                if (distSq < radius * radius && distSq > 0.0001f) {
                    float dist = std::sqrt(distSq);
                    float overlap = radius - dist;
                    outPushback.x += (distX / dist) * overlap;
                    outPushback.y += (distY / dist) * overlap;
                    collided = true;
                } else if (distSq <= 0.0001f) {
                    float pushX = (pos.x >= tileCenter.x) ? (tileCenter.x + halfCell + radius - pos.x) : (tileCenter.x - halfCell - radius - pos.x);
                    float pushY = (pos.y >= tileCenter.y) ? (tileCenter.y + halfCell + radius - pos.y) : (tileCenter.y - halfCell - radius - pos.y);
                    if (std::abs(pushX) < std::abs(pushY)) {
                        outPushback.x += pushX;
                    } else {
                        outPushback.y += pushY;
                    }
                    collided = true;
                }
            }
        }
    }

    return collided;
}

bool Level::Raycast(Vector2 start, Vector2 dir, float maxDist, 
                     Vector2& outHitPoint, Vector2& outNormal, 
                     int& outTileX, int& outTileY, bool ignoreHoles) const 
{
    float len = std::sqrt(dir.x * dir.x + dir.y * dir.y);
    if (len < 0.0001f) return false;
    Vector2 rayDir = { dir.x / len, dir.y / len };

    float halfArenaW = (m_width * 0.5f) * CELL_SIZE;
    float halfArenaH = (m_height * 0.5f) * CELL_SIZE;

    float tBound = maxDist;
    Vector2 boundNormal = { 0.0f, 0.0f };

    if (rayDir.x > 0.0f) {
        float t = (halfArenaW - start.x) / rayDir.x;
        if (t > 0.0f && t < tBound) { tBound = t; boundNormal = { -1.0f, 0.0f }; }
    } else if (rayDir.x < 0.0f) {
        float t = (-halfArenaW - start.x) / rayDir.x;
        if (t > 0.0f && t < tBound) { tBound = t; boundNormal = { 1.0f, 0.0f }; }
    }

    if (rayDir.y > 0.0f) {
        float t = (halfArenaH - start.y) / rayDir.y;
        if (t > 0.0f && t < tBound) { tBound = t; boundNormal = { 0.0f, -1.0f }; }
    } else if (rayDir.y < 0.0f) {
        float t = (-halfArenaH - start.y) / rayDir.y;
        if (t > 0.0f && t < tBound) { tBound = t; boundNormal = { 0.0f, 1.0f }; }
    }

    int gx, gy;
    WorldToGrid(start, gx, gy);

    int stepX = (rayDir.x >= 0) ? 1 : -1;
    int stepY = (rayDir.y >= 0) ? 1 : -1;

    Vector2 cellCenter = GridToWorld(gx, gy);
    float halfCell = CELL_SIZE * 0.5f;

    float nextVoxelBoundaryX = cellCenter.x + stepX * halfCell;
    float nextVoxelBoundaryY = cellCenter.y + stepY * halfCell;

    float tMaxX = (rayDir.x != 0.0f) ? (nextVoxelBoundaryX - start.x) / rayDir.x : 1e30f;
    float tMaxY = (rayDir.y != 0.0f) ? (nextVoxelBoundaryY - start.y) / rayDir.y : 1e30f;

    float tDeltaX = (rayDir.x != 0.0f) ? std::abs(CELL_SIZE / rayDir.x) : 1e30f;
    float tDeltaY = (rayDir.y != 0.0f) ? std::abs(CELL_SIZE / rayDir.y) : 1e30f;

    float t = 0.0f;
    Vector2 hitNormal = { 0.0f, 0.0f };

    while (t < std::min(maxDist, tBound)) {
        if (tMaxX < tMaxY) {
            t = tMaxX;
            tMaxX += tDeltaX;
            gx += stepX;
            hitNormal = { -float(stepX), 0.0f };
        } else {
            t = tMaxY;
            tMaxY += tDeltaY;
            gy += stepY;
            hitNormal = { 0.0f, -float(stepY) };
        }

        if (t >= maxDist || t >= tBound) break;

        if (IsSolid(gx, gy) || (!ignoreHoles && IsHole(gx, gy))) {
            outHitPoint = { start.x + rayDir.x * t, start.y + rayDir.y * t };
            outNormal = hitNormal;
            outTileX = gx;
            outTileY = gy;
            return true;
        }
    }

    if (tBound < maxDist) {
        outHitPoint = { start.x + rayDir.x * tBound, start.y + rayDir.y * tBound };
        outNormal = boundNormal;
        outTileX = -1;
        outTileY = -1;
        return true;
    }

    return false;
}
