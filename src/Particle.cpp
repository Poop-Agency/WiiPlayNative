#include "Particle.hpp"
#include <cstdlib>
#include <algorithm>

ParticleManager::ParticleManager() {
    m_particles.reserve(1000);
    m_treadMarks.reserve(500);
}

ParticleManager::~ParticleManager() {}

void ParticleManager::Reset() {
    m_particles.clear();
    m_treadMarks.clear();
}

void ParticleManager::Update(float dt) {
    // Update active particles
    for (size_t i = 0; i < m_particles.size(); ) {
        Particle& p = m_particles[i];
        p.life -= dt;
        if (p.life <= 0.0f) {
            m_particles[i] = m_particles.back();
            m_particles.pop_back();
            continue;
        }

        float progress = 1.0f - (p.life / p.maxLife);

        if (p.type == ParticleType::Debris) {
            p.velocity.y -= 25.0f * dt; // gravity
            p.position.x += p.velocity.x * dt;
            p.position.y += p.velocity.y * dt;
            p.position.z += p.velocity.z * dt;
            if (p.position.y < 0.1f) {
                p.position.y = 0.1f;
                p.velocity.y *= -0.4f;
                p.velocity.x *= 0.7f;
                p.velocity.z *= 0.7f;
            }
        } else if (p.type == ParticleType::Fireball || p.type == ParticleType::Smoke) {
            p.position.x += p.velocity.x * dt;
            p.position.y += p.velocity.y * dt;
            p.position.z += p.velocity.z * dt;
            p.velocity.x *= 0.94f;
            p.velocity.z *= 0.94f;
            p.velocity.y += 1.5f * dt; // buoyancy
            p.size = p.startSize * (1.0f + progress * 1.5f);
            p.color.a = static_cast<unsigned char>(255 * (1.0f - progress));
        } else if (p.type == ParticleType::Spark) {
            p.position.x += p.velocity.x * dt;
            p.position.y += p.velocity.y * dt;
            p.position.z += p.velocity.z * dt;
            p.velocity.x *= 0.90f;
            p.velocity.y -= 15.0f * dt;
            p.velocity.z *= 0.90f;
            p.color.a = static_cast<unsigned char>(255 * (1.0f - progress));
        } else if (p.type == ParticleType::Shockwave) {
            p.size = p.startSize + progress * 6.0f;
            p.color.a = static_cast<unsigned char>(200 * (1.0f - progress));
        }

        ++i;
    }

    // Limit maximum tread marks
    if (m_treadMarks.size() > 400) {
        m_treadMarks.erase(m_treadMarks.begin(), m_treadMarks.begin() + 100);
    }
}

void ParticleManager::AddMuzzleFlash(Vector3 pos, Vector3 dir, Color color) {
    for (int i = 0; i < 8; ++i) {
        float speed = 2.0f + (rand() % 100) / 30.0f;
        float spreadX = ((rand() % 100) - 50) / 100.0f;
        float spreadY = ((rand() % 100) - 50) / 100.0f;
        float spreadZ = ((rand() % 100) - 50) / 100.0f;

        Vector3 vel = {
            dir.x * speed + spreadX,
            dir.y * speed + spreadY + 0.5f,
            dir.z * speed + spreadZ
        };

        m_particles.push_back({
            pos, vel, { 255, 200, 100, 255 }, 0.25f, 0.25f, 0.15f, 0.15f, ParticleType::Fireball
        });
    }

    for (int i = 0; i < 6; ++i) {
        Vector3 vel = {
            dir.x * 1.5f + ((rand() % 100) - 50) / 100.0f,
            0.5f + (rand() % 50) / 100.0f,
            dir.z * 1.5f + ((rand() % 100) - 50) / 100.0f
        };
        m_particles.push_back({
            pos, vel, { 180, 180, 180, 180 }, 0.3f, 0.3f, 0.4f, 0.4f, ParticleType::Smoke
        });
    }
}

void ParticleManager::AddRicochetSparks(Vector3 pos, Vector3 normal) {
    for (int i = 0; i < 15; ++i) {
        float speed = 4.0f + (rand() % 100) / 15.0f;
        float rx = ((rand() % 100) - 50) / 50.0f;
        float ry = (rand() % 100) / 70.0f;
        float rz = ((rand() % 100) - 50) / 50.0f;

        Vector3 vel = {
            normal.x * speed + rx * 2.0f,
            normal.y * speed + ry * 3.0f + 1.0f,
            normal.z * speed + rz * 2.0f
        };

        m_particles.push_back({
            pos, vel, { 255, 230, 120, 255 }, 0.12f, 0.12f, 0.25f, 0.25f, ParticleType::Spark
        });
    }
}

void ParticleManager::AddBlockDebris(Vector3 pos, Color blockColor) {
    for (int i = 0; i < 24; ++i) {
        float speed = 3.0f + (rand() % 100) / 20.0f;
        float rx = ((rand() % 200) - 100) / 50.0f;
        float ry = 3.0f + (rand() % 100) / 20.0f;
        float rz = ((rand() % 200) - 100) / 50.0f;

        Color c = blockColor;
        c.r = static_cast<unsigned char>(std::clamp(int(c.r) + (rand() % 40 - 20), 0, 255));
        c.g = static_cast<unsigned char>(std::clamp(int(c.g) + (rand() % 40 - 20), 0, 255));
        c.b = static_cast<unsigned char>(std::clamp(int(c.b) + (rand() % 40 - 20), 0, 255));

        m_particles.push_back({
            pos, { rx, ry, rz }, c, 0.28f, 0.28f, 0.9f, 0.9f, ParticleType::Debris
        });
    }
}

void ParticleManager::AddExplosion(Vector3 pos, float radius) {
    // Shockwave ring
    m_particles.push_back({
        { pos.x, 0.05f, pos.z }, { 0, 0, 0 }, { 255, 220, 150, 220 }, 0.5f, 0.5f, 0.4f, 0.4f, ParticleType::Shockwave
    });

    // Fireball puffs
    for (int i = 0; i < 25; ++i) {
        float speed = 2.0f + (rand() % 100) / 20.0f;
        float angle = (rand() % 360) * DEG2RAD;
        float pitch = ((rand() % 100) - 20) / 100.0f;

        Vector3 vel = {
            std::cos(angle) * speed,
            1.5f + pitch * 3.0f,
            std::sin(angle) * speed
        };

        Color fireColor = (rand() % 2 == 0) ? Color{ 255, 120, 30, 255 } : Color{ 255, 210, 50, 255 };
        float pSize = (radius * 0.4f) + (rand() % 50) / 100.0f;

        m_particles.push_back({
            pos, vel, fireColor, pSize, pSize, 0.45f + (rand() % 30) / 100.0f, 0.55f, ParticleType::Fireball
        });
    }

    // Heavy black smoke
    for (int i = 0; i < 18; ++i) {
        float speed = 1.0f + (rand() % 100) / 35.0f;
        float angle = (rand() % 360) * DEG2RAD;

        Vector3 vel = {
            std::cos(angle) * speed,
            2.0f + (rand() % 100) / 30.0f,
            std::sin(angle) * speed
        };

        unsigned char grey = static_cast<unsigned char>(40 + rand() % 50);
        m_particles.push_back({
            pos, vel, { grey, grey, grey, 200 }, 0.6f, 0.6f, 0.9f, 0.9f, ParticleType::Smoke
        });
    }
}

void ParticleManager::AddTreadMark(Vector2 centerPos, float angle, float width, Color color) {
    float perpAngle = angle + PI * 0.5f;
    float halfW = width * 0.5f;

    Vector2 left = {
        centerPos.x + std::cos(perpAngle) * halfW,
        centerPos.y + std::sin(perpAngle) * halfW
    };
    Vector2 right = {
        centerPos.x - std::cos(perpAngle) * halfW,
        centerPos.y - std::sin(perpAngle) * halfW
    };

    m_treadMarks.push_back({ left, right, angle, color, 0.45f });
}
