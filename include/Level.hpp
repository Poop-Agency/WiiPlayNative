#pragma once

#include "Common.hpp"
#include <vector>
#include <string>

struct EnemySpawn {
    TankType type;
    Vector2 worldPos;
    int gridX;
    int gridY;
};

struct MissionDef {
    int mapIndex; // 0 to 29
    std::vector<TankType> enemies;
};

class Level {
public:
    Level();
    ~Level();

    bool LoadMission(int missionNumber, bool is2Player = false);
    bool LoadFromBinary(const std::string& filepath, const std::vector<TankType>& missionEnemies);
    void Reset();

    // Grid queries
    bool IsInBounds(int gx, int gy) const;
    TileType GetTile(int gx, int gy) const;
    void SetTile(int gx, int gy, TileType type);
    bool IsSolid(int gx, int gy) const;
    bool IsHole(int gx, int gy) const;
    bool IsDestructible(int gx, int gy) const;
    bool DestroyBlock(int gx, int gy);

    // Coordinate conversion
    Vector2 GridToWorld(int gx, int gy) const;
    void WorldToGrid(Vector2 worldPos, int& outGx, int& outGy) const;

    // Physics & Raycasting
    bool CheckTankCollision(Vector2 pos, float radius, Vector2& outPushback) const;
    bool Raycast(Vector2 start, Vector2 dir, float maxDist, 
                 Vector2& outHitPoint, Vector2& outNormal, 
                 int& outTileX, int& outTileY, bool ignoreHoles = true) const;

    // Spawns
    Vector2 GetPlayer1Spawn() const { return m_player1Spawn; }
    Vector2 GetPlayer2Spawn() const { return m_player2Spawn; }
    const std::vector<EnemySpawn>& GetEnemySpawns() const { return m_enemySpawns; }

    int GetWidth() const { return m_width; }
    int GetHeight() const { return m_height; }
    int GetMissionNumber() const { return m_currentMission; }

    static MissionDef GetMissionDef(int missionNumber);

private:
    int m_width;
    int m_height;
    int m_currentMission;
    std::vector<TileType> m_grid;
    Vector2 m_player1Spawn;
    Vector2 m_player2Spawn;
    std::vector<EnemySpawn> m_enemySpawns;
};
