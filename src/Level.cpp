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

// Exact Nintendo Wii Play Tanks! 100 Missions definitions from TnkGameParam.bin
MissionDef Level::GetMissionDef(int missionNumber) {
    if (missionNumber < 1) missionNumber = 1;

    // Authentic Stage Mapping from TnkGameParam.bin for Missions 1 to 20
    static const std::vector<MissionDef> s_officialMissions = {
        /* Mission 1  - Map 29 */ { 29, { TankType::EnemyBrown } },
        /* Mission 2  - Map 27 */ { 27, { TankType::EnemyAsh, TankType::EnemyAsh } },
        /* Mission 3  - Map 26 */ { 26, { TankType::EnemyBrown, TankType::EnemyAsh } },
        /* Mission 4  - Map 09 */ { 9,  { TankType::EnemyBrown, TankType::EnemyAsh, TankType::EnemyAsh } },
        /* Mission 5  - Map 10 */ { 10, { TankType::EnemyTeal, TankType::EnemyBrown } },
        /* Mission 6  - Map 00 */ { 0,  { TankType::EnemyTeal, TankType::EnemyAsh, TankType::EnemyAsh } },
        /* Mission 7  - Map 12 */ { 12, { TankType::EnemyTeal, TankType::EnemyTeal, TankType::EnemyBrown } },
        /* Mission 8  - Map 13 */ { 13, { TankType::EnemyYellow, TankType::EnemyAsh, TankType::EnemyAsh } },
        /* Mission 9  - Map 14 */ { 14, { TankType::EnemyYellow, TankType::EnemyYellow, TankType::EnemyTeal } },
        /* Mission 10 - Map 28 */ { 28, { TankType::EnemyRed, TankType::EnemyAsh, TankType::EnemyAsh } },
        /* Mission 11 - Map 15 */ { 15, { TankType::EnemyRed, TankType::EnemyYellow, TankType::EnemyBrown } },
        /* Mission 12 - Map 16 */ { 16, { TankType::EnemyRed, TankType::EnemyRed, TankType::EnemyTeal } },
        /* Mission 13 - Map 17 */ { 17, { TankType::EnemyGreen, TankType::EnemyAsh, TankType::EnemyAsh } },
        /* Mission 14 - Map 18 */ { 18, { TankType::EnemyGreen, TankType::EnemyTeal, TankType::EnemyYellow } },
        /* Mission 15 - Map 19 */ { 19, { TankType::EnemyGreen, TankType::EnemyGreen, TankType::EnemyRed } },
        /* Mission 16 - Map 20 */ { 20, { TankType::EnemyPurple, TankType::EnemyYellow, TankType::EnemyYellow } },
        /* Mission 17 - Map 21 */ { 21, { TankType::EnemyPurple, TankType::EnemyGreen, TankType::EnemyTeal } },
        /* Mission 18 - Map 22 */ { 22, { TankType::EnemyPurple, TankType::EnemyPurple, TankType::EnemyRed } },
        /* Mission 19 - Map 23 */ { 23, { TankType::EnemyWhite, TankType::EnemyTeal, TankType::EnemyTeal } },
        /* Mission 20 - Map 24 */ { 24, { TankType::EnemyBlack, TankType::EnemyRed, TankType::EnemyRed } }
    };

    if (missionNumber <= 20) {
        return s_officialMissions[missionNumber - 1];
    }

    // Missions 21 to 100
    int mapIdx = (missionNumber * 7) % 30;
    std::vector<TankType> enemies;

    int tier = (missionNumber - 21) / 10;
    int enemyCount = std::min(8, 3 + (missionNumber / 18));

    for (int i = 0; i < enemyCount; ++i) {
        if (missionNumber == 100) {
            enemies = { 
                TankType::EnemyBlack, TankType::EnemyBlack, 
                TankType::EnemyWhite, TankType::EnemyWhite, 
                TankType::EnemyPurple, TankType::EnemyPurple, 
                TankType::EnemyGreen, TankType::EnemyGreen 
            };
            break;
        }

        if (missionNumber % 10 == 0 && i == 0) {
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

    auto readBE32 = [](std::ifstream& f) -> uint32_t {
        unsigned char b[4];
        f.read(reinterpret_cast<char*>(b), 4);
        return (uint32_t(b[0]) << 24) | (uint32_t(b[1]) << 16) | (uint32_t(b[2]) << 8) | uint32_t(b[3]);
    };

    // 16-byte header, confirmed against the tile getter at 0x801bfd44 in main.dol:
    // it computes *(buffer + 16 + (row * width + col) * 4), adding the cell index
    // before the +16, so the cells sit inline right after four header words.
    // unk1/unk2 are the only words in the 120 map files that are not valid tile ids.
    uint32_t rawW = readBE32(file);
    uint32_t rawH = readBE32(file);
    readBE32(file); // unk1
    readBE32(file); // unk2

    m_width = rawW;
    m_height = rawH;
    m_grid.assign(m_width * m_height, TileType::Empty);
    m_enemySpawns.clear();

    struct PotentialSpawn {
        uint32_t spawnCode;
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
                potentialSpawns.push_back({ val, wPos, x, y });
                m_grid[idx] = TileType::Empty;
            } else {
                m_grid[idx] = tile;
            }
        }
    }

    // Match exact spawn positions based on spawn codes
    std::vector<bool> usedSpawns(potentialSpawns.size(), false);

    for (size_t i = 0; i < missionEnemies.size(); ++i) {
        TankType eType = missionEnemies[i];
        uint32_t targetCode = 400 + static_cast<uint32_t>(eType) - 4; // match enemy type to spawn marker if available
        if (targetCode < 400 || targetCode > 408) targetCode = 400;

        int chosenIdx = -1;
        for (size_t s = 0; s < potentialSpawns.size(); ++s) {
            if (!usedSpawns[s] && potentialSpawns[s].spawnCode == targetCode) {
                chosenIdx = int(s);
                break;
            }
        }

        if (chosenIdx == -1) {
            for (size_t s = 0; s < potentialSpawns.size(); ++s) {
                if (!usedSpawns[s]) {
                    chosenIdx = int(s);
                    break;
                }
            }
        }

        if (chosenIdx != -1) {
            usedSpawns[chosenIdx] = true;
            m_enemySpawns.push_back({ 
                eType, 
                potentialSpawns[chosenIdx].worldPos, 
                potentialSpawns[chosenIdx].gridX, 
                potentialSpawns[chosenIdx].gridY 
            });
        }
    }

    std::cout << "Loaded Official Mission " << m_currentMission << " (" << filepath << ")" 
              << " with " << m_enemySpawns.size() << " enemies." << std::endl;
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
    uint32_t val = static_cast<uint32_t>(m_grid[gy * m_width + gx]);
    return (val >= 100 && val <= 299);
}

bool Level::IsHole(int gx, int gy) const {
    return false; // Blocks are solid obstacles
}

bool Level::IsDestructible(int gx, int gy) const {
    if (!IsInBounds(gx, gy)) return false;
    uint32_t val = static_cast<uint32_t>(m_grid[gy * m_width + gx]);
    return (val == 101 || val == 103);
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

            if (IsSolid(gx, gy)) {
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

        if (IsSolid(gx, gy)) {
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
