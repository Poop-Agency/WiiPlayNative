#pragma once

#include "Common.hpp"
#include <vector>

enum class ParticleType {
    Smoke,
    Spark,
    Debris,
    Fireball,
    Shockwave
};

struct Particle {
    Vector3 position;
    Vector3 velocity;
    Color color;
    float size;
    float startSize;
    float life;
    float maxLife;
    ParticleType type;
};

struct TreadMark {
    Vector2 leftPos;
    Vector2 rightPos;
    float angle;
    Color color;
    float alpha;
};

class ParticleManager {
public:
    ParticleManager();
    ~ParticleManager();

    void Reset();
    void Update(float dt);

    void AddMuzzleFlash(Vector3 pos, Vector3 dir, Color color);
    void AddRicochetSparks(Vector3 pos, Vector3 normal);
    void AddBlockDebris(Vector3 pos, Color blockColor);
    void AddExplosion(Vector3 pos, float radius = 2.5f);
    void AddTreadMark(Vector2 centerPos, float angle, float width, Color color);

    const std::vector<Particle>& GetParticles() const { return m_particles; }
    const std::vector<TreadMark>& GetTreadMarks() const { return m_treadMarks; }

private:
    std::vector<Particle> m_particles;
    std::vector<TreadMark> m_treadMarks;
};
