#include "Level.hpp"
#include <fstream>
#include <iostream>
#include <sstream>
#include <iomanip>
#include <algorithm>
#include <cstdlib>

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

#include "MissionTable.inc"

// The mission table is extracted, not invented: assets/param/TnkGameParam.bin
// offset 1684, 100 rows of 88 bytes.  tools/mission_table.py regenerates
// MissionTable.inc and lists the main.dol addresses that pin every field --
// 0x80265bdc for the base and stride, 0x80265444/0x8026545c for the map range,
// 0x80265cb8 for the slot encoding.
MissionDef Level::GetMissionDef(int missionNumber) {
    if (missionNumber < 1) missionNumber = 1;
    if (missionNumber > 100) missionNumber = 100;
    const MissionRow& row = kMissionTable[missionNumber - 1];

    // 0x80265470: equal bounds are used as-is, otherwise the game draws in
    // [lo, hi] inclusive (0x802654b0 computes hi - lo + 1).
    int mapIndex = row.mapLo;
    if (row.mapHi != row.mapLo) {
        mapIndex = row.mapLo + rand() % (row.mapHi - row.mapLo + 1);
    }

    std::vector<EnemySlotDef> enemies;
    for (int i = 0; i < 8; ++i) {
        int v = row.slot[i];
        if (v == 0) continue; // 0x80265cc0: a zero slot is empty
        // 0x80265cc4: below 10 the value is the record type itself; at or above
        // 10 it is a lo/hi pair of digits and the type is drawn between them.
        int type = v < 10 ? v : (v / 10) + rand() % (v % 10 - v / 10 + 1);
        enemies.push_back({ i, kRecordType[type] });
    }

    return { mapIndex, row.bonus != 0, enemies };
}

bool Level::LoadMission(int missionNumber, bool is2Player) {
    m_currentMission = missionNumber;
    m_currentDef = GetMissionDef(missionNumber);
    const MissionDef& def = m_currentDef;

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

bool Level::LoadFromBinary(const std::string& filepath, const std::vector<EnemySlotDef>& missionEnemies) {
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

    // Slot index i is spawn marker 400 + i; every map file carries all eight
    // exactly once, so each enemy lands on the tile its row selected.
    for (const EnemySlotDef& e : missionEnemies) {
        uint32_t targetCode = 400 + static_cast<uint32_t>(e.slot);
        for (const PotentialSpawn& s : potentialSpawns) {
            if (s.spawnCode != targetCode) continue;
            m_enemySpawns.push_back({ e.type, s.worldPos, s.gridX, s.gridY });
            break;
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
    // Tanks has no hole tile. The field builder at 0x80265e68 accepts exactly two
    // tile ranges, 100..107 and 200..207, and hands both to the same block create
    // call; every other value is skipped. So both families are solid obstacles.
    return false;
}

bool Level::IsDestructible(int gx, int gy) const {
    if (!IsInBounds(gx, gy)) return false;
    uint32_t val = static_cast<uint32_t>(m_grid[gy * m_width + gx]);
    // The builder packs the tile as ((id % 100) << 4) for the 100 family and
    // (((id - 200) << 4) | 1) for the 200 family. The constructor at 0x802616a8
    // stores the low two bits in +0xB0, and the block only runs its mine
    // proximity query when +0xB0 is zero (0x80260b38), which is what sets +0x148
    // and makes Block::break fire (0x80260f8c). So the 100 family breaks to a
    // mine and the 200 family never breaks. Nothing anywhere lets a shell set
    // +0x148, which is why gunfire leaves cork standing.
    return val >= 101 && val <= 107;
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
