#include "Bullet.hpp"
#include "Level.hpp"
#include "Tank.hpp"
#include "Particle.hpp"
#include <cmath>

BulletManager::BulletManager()
    : m_nextId(1)
{
    m_bullets.reserve(100);
}

BulletManager::~BulletManager() {}

void BulletManager::Reset() {
    m_bullets.clear();
}

int BulletManager::CountActiveBulletsForOwner(uint32_t ownerId) const {
    int count = 0;
    for (const auto& b : m_bullets) {
        if (b.active && b.ownerId == ownerId) {
            ++count;
        }
    }
    return count;
}

void BulletManager::SpawnBullet(uint32_t ownerId, Vector2 pos, Vector2 dir, float speed, int bounces, bool isRocket, Color color) {
    float len = Vector2Length(dir);
    if (len < 0.0001f) dir = { 1.0f, 0.0f };
    else dir = { dir.x / len, dir.y / len };

    Bullet b;
    b.id = m_nextId++;
    b.ownerId = ownerId;
    b.position = pos;
    b.velocity = { dir.x * speed, dir.y * speed };
    b.speed = speed;
    b.bouncesLeft = bounces;
    b.isRocket = isRocket;
    b.lifetime = 10.0f;
    b.active = true;
    b.color = color;
    b.trailTimer = 0.0f;

    m_bullets.push_back(b);
}

void BulletManager::Update(float dt, Level& level, std::vector<Tank>& tanks, ParticleManager& particles, bool isServer) {
    for (size_t i = 0; i < m_bullets.size(); ++i) {
        Bullet& b = m_bullets[i];
        if (!b.active) continue;

        b.lifetime -= dt;
        if (b.lifetime <= 0.0f) {
            b.active = false;
            continue;
        }

        // Trail recording
        b.trailTimer += dt;
        if (b.trailTimer >= 0.02f) {
            b.trailTimer = 0.0f;
            b.trail.push_back(b.position);
            if (b.trail.size() > 12) {
                b.trail.erase(b.trail.begin());
            }
        }

        // Continuous collision raycasting
        float moveDist = b.speed * dt;
        Vector2 moveDir = { b.velocity.x / b.speed, b.velocity.y / b.speed };

        Vector2 hitPoint, hitNormal;
        int hitTileX, hitTileY;

        if (level.Raycast(b.position, moveDir, moveDist, hitPoint, hitNormal, hitTileX, hitTileY, true)) {
            // Reached an obstacle or border
            Vector3 hitPos3D = { hitPoint.x, 0.4f, hitPoint.y };
            Vector3 norm3D = { hitNormal.x, 0.0f, hitNormal.y };

            if (hitTileX >= 0 && hitTileY >= 0 && level.IsDestructible(hitTileX, hitTileY)) {
                // Destroy cork block
                level.DestroyBlock(hitTileX, hitTileY);
                Vector2 blockCenter = level.GridToWorld(hitTileX, hitTileY);
                particles.AddBlockDebris({ blockCenter.x, 0.5f, blockCenter.y }, { 210, 160, 100, 255 });
                b.active = false;
            } else if (b.isRocket) {
                // Rocket explodes on any impact
                particles.AddExplosion(hitPos3D, 2.5f);
                b.active = false;
            } else if (b.bouncesLeft > 0) {
                // Ricochet bounce!
                --b.bouncesLeft;
                b.position = { hitPoint.x + hitNormal.x * 0.05f, hitPoint.y + hitNormal.y * 0.05f };

                // Reflect velocity: v' = v - 2(v.n)n
                float dot = b.velocity.x * hitNormal.x + b.velocity.y * hitNormal.y;
                b.velocity.x -= 2.0f * dot * hitNormal.x;
                b.velocity.y -= 2.0f * dot * hitNormal.y;

                particles.AddRicochetSparks(hitPos3D, norm3D);
            } else {
                // Out of bounces, dissolve in sparks
                particles.AddRicochetSparks(hitPos3D, norm3D);
                b.active = false;
            }
        } else {
            // Normal motion
            b.position.x += b.velocity.x * dt;
            b.position.y += b.velocity.y * dt;
        }

        if (!b.active) continue;

        // Check collision with tanks
        for (auto& tank : tanks) {
            if (!tank.IsAlive()) continue;

            float dist = Vector2Distance(b.position, tank.GetPosition());
            if (dist < TANK_RADIUS + BULLET_RADIUS) {
                tank.TakeDamage(particles);
                b.active = false;
                break;
            }
        }
    }

    // Bullet-on-bullet collision (canceling each other out)
    for (size_t i = 0; i < m_bullets.size(); ++i) {
        if (!m_bullets[i].active) continue;
        for (size_t j = i + 1; j < m_bullets.size(); ++j) {
            if (!m_bullets[j].active) continue;

            float dist = Vector2Distance(m_bullets[i].position, m_bullets[j].position);
            if (dist < BULLET_RADIUS * 2.5f) {
                Vector3 midPoint = {
                    (m_bullets[i].position.x + m_bullets[j].position.x) * 0.5f,
                    0.4f,
                    (m_bullets[i].position.y + m_bullets[j].position.y) * 0.5f
                };
                particles.AddRicochetSparks(midPoint, { 0, 1, 0 });
                m_bullets[i].active = false;
                m_bullets[j].active = false;
                break;
            }
        }
    }

    // Remove inactive bullets
    for (size_t i = 0; i < m_bullets.size(); ) {
        if (!m_bullets[i].active) {
            m_bullets[i] = m_bullets.back();
            m_bullets.pop_back();
        } else {
            ++i;
        }
    }
}
