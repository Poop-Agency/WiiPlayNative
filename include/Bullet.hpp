#pragma once

#include "Common.hpp"
#include <vector>
#include "Audio.hpp"

class Level;
class Tank;
class ParticleManager;
class MineManager;

struct Bullet {
    uint32_t id;
    uint32_t ownerId;
    Vector2 position;
    Vector2 velocity;
    float speed;
    int bouncesLeft;
    bool isRocket;
    float lifetime;
    bool active;
    bool leftOwner;   // false until the shell has cleared the tank that fired it
    Color color;

    // Trail effect
    std::vector<Vector2> trail;
    float trailTimer;
};

class BulletManager {
public:
    BulletManager();
    ~BulletManager();

    std::vector<SoundType> audioEvents;

    void Reset();
    void SpawnBullet(uint32_t ownerId, Vector2 pos, Vector2 dir, float speed, int bounces, bool isRocket, Color color);
    void Update(float dt, Level& level, std::vector<Tank>& tanks, MineManager& mines, ParticleManager& particles, bool isServer);
    
    std::vector<Bullet>& GetBullets() { return m_bullets; }
    const std::vector<Bullet>& GetBullets() const { return m_bullets; }

    int CountActiveBulletsForOwner(uint32_t ownerId) const;

private:
    std::vector<Bullet> m_bullets;
    uint32_t m_nextId;
};
