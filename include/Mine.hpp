#pragma once

#include "Common.hpp"
#include <vector>

class Level;
class Tank;
class ParticleManager;

struct Mine {
    uint32_t id;
    uint32_t ownerId;
    Vector2 position;
    float timer;
    float armTimer;
    float beepTimer;
    float beepRate;
    bool active;
    bool detonated;
    float flashTimer;
};

class MineManager {
public:
    MineManager();
    ~MineManager();

    void Reset();
    bool PlantMine(uint32_t ownerId, Vector2 pos);
    void DetonateMine(size_t index, Level& level, std::vector<Tank>& tanks, ParticleManager& particles);
    void Update(float dt, Level& level, std::vector<Tank>& tanks, ParticleManager& particles, bool isServer);

    std::vector<Mine>& GetMines() { return m_mines; }
    const std::vector<Mine>& GetMines() const { return m_mines; }

    int CountActiveMinesForOwner(uint32_t ownerId) const;

private:
    std::vector<Mine> m_mines;
    uint32_t m_nextId;
};
