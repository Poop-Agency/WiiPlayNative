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

bool Level::LoadMission(int missionNumber, bool is2Player) {
    m_currentMission = missionNumber;
    int missionIdx = missionNumber - 1;
    if (missionIdx < 0) missionIdx = 0;
    
    // Original mission files are numbered 00 to 29 or more (with variants)
    std::string prefix = is2Player ? "TnkMapData_P2_" : "TnkMapData_P1_";
    int fileIdx = missionIdx % 30; // 30 official core stages
    int variant = (missionIdx / 30) % 2;

    std::ostringstream ss;
    ss << "assets/maps/" << prefix << std::setfill('0') << std::setw(2) << fileIdx << "_" << variant << ".bin";
    
    if (LoadFromBinary(ss.str())) {
        return true;
    }

    // Fallback: try non-variant 1
    std::ostringstream ssFallback;
    ssFallback << "assets/maps/" << prefix << std::setfill('0') << std::setw(2) << fileIdx << "_1.bin";
    return LoadFromBinary(ssFallback.str());
}

bool Level::LoadFromBinary(const std::string& filepath) {
    std::ifstream file(filepath, std::ios::binary);
    if (!file.is_open()) {
        std::cerr << "Failed to open map file: " << filepath << std::endl;
        return false;
    }

    // Read header (16 bytes)
    uint32_t rawW = 0, rawH = 0, unk1 = 0, unk2 = 0;
    auto readBE32 = [](std::ifstream& f) -> uint32_t {
        unsigned char b[4];
        f.read(reinterpret_cast<char*>(b), 4);
        return (uint32_t(b[0]) << 24) | (uint32_t(b[1]) << 16) | (uint32_t(b[2]) << 8) | uint32_t(b[3]);
    };

    rawW = readBE32(file);
    rawH = readBE32(file);
    unk1 = readBE32(file);
    unk2 = readBE32(file);

    m_width = rawW;
    m_height = rawH;
    m_grid.assign(m_width * m_height, TileType::Empty);
    m_enemySpawns.clear();

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
                TankType eType = TankType::EnemyBrown;
                switch (tile) {
                    case TileType::SpawnEnemyBrown:  eType = TankType::EnemyBrown; break;
                    case TileType::SpawnEnemyAsh:    eType = TankType::EnemyAsh; break;
                    case TileType::SpawnEnemyTeal:   eType = TankType::EnemyTeal; break;
                    case TileType::SpawnEnemyYellow: eType = TankType::EnemyYellow; break;
                    case TileType::SpawnEnemyRed:    eType = TankType::EnemyRed; break;
                    case TileType::SpawnEnemyGreen:  eType = TankType::EnemyGreen; break;
                    case TileType::SpawnEnemyPurple: eType = TankType::EnemyPurple; break;
                    case TileType::SpawnEnemyWhite:  eType = TankType::EnemyWhite; break;
                    case TileType::SpawnEnemyBlack:  eType = TankType::EnemyBlack; break;
                    default:                         eType = TankType::EnemyBrown; break;
                }
                m_enemySpawns.push_back({ eType, wPos, x, y });
                m_grid[idx] = TileType::Empty;
            } else {
                m_grid[idx] = tile;
            }
        }
    }

    std::cout << "Loaded map: " << filepath << " (" << m_width << "x" << m_height 
              << ") with " << m_enemySpawns.size() << " enemies." << std::endl;
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

    // Check bounds
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

    // Check neighboring grid cells
    int centerGx, centerGy;
    WorldToGrid(pos, centerGx, centerGy);

    for (int dy = -1; dy <= 1; ++dy) {
        for (int dx = -1; dx <= 1; ++dx) {
            int gx = centerGx + dx;
            int gy = centerGy + dy;

            if (IsSolid(gx, gy) || IsHole(gx, gy)) {
                Vector2 tileCenter = GridToWorld(gx, gy);
                float halfCell = CELL_SIZE * 0.5f;

                // Find closest point on AABB to circle
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
                    // Center inside block: push out along smallest axis
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

// 2D DDA Raycaster for bullet trajectory and ricochet reflection
bool Level::Raycast(Vector2 start, Vector2 dir, float maxDist, 
                     Vector2& outHitPoint, Vector2& outNormal, 
                     int& outTileX, int& outTileY, bool ignoreHoles) const 
{
    float len = std::sqrt(dir.x * dir.x + dir.y * dir.y);
    if (len < 0.0001f) return false;
    Vector2 rayDir = { dir.x / len, dir.y / len };

    float halfArenaW = (m_width * 0.5f) * CELL_SIZE;
    float halfArenaH = (m_height * 0.5f) * CELL_SIZE;

    // First check bounds collision (outer wooden table border)
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

    // Grid DDA
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

    // If we hit outer boundary wall
    if (tBound < maxDist) {
        outHitPoint = { start.x + rayDir.x * tBound, start.y + rayDir.y * tBound };
        outNormal = boundNormal;
        outTileX = -1;
        outTileY = -1;
        return true;
    }

    return false;
}
