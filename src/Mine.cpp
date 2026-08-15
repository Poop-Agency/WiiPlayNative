#include "Mine.hpp"
#include "Level.hpp"
#include "Tank.hpp"
#include "Particle.hpp"
#include <cmath>

MineManager::MineManager()
    : m_nextId(1)
{
    m_mines.reserve(30);
}

MineManager::~MineManager() {}

void MineManager::Reset() {
    m_mines.clear();
}

int MineManager::CountActiveMinesForOwner(uint32_t ownerId) const {
    int count = 0;
    for (const auto& m : m_mines) {
        if (m.active && m.ownerId == ownerId) {
            ++count;
        }
    }
    return count;
}

bool MineManager::PlantMine(uint32_t ownerId, Vector2 pos) {
    Mine m;
    m.id = m_nextId++;
    m.ownerId = ownerId;
    m.position = pos;
    m.timer = MINE_LIFETIME;
    m.armTimer = 0.6f;
    m.beepTimer = 0.0f;
    m.beepRate = 1.0f;
    m.active = true;
    m.detonated = false;
    m.flashTimer = 0.0f;

    m_mines.push_back(m);
    return true;
}

void MineManager::DetonateMine(size_t index, Level& level, std::vector<Tank>& tanks, ParticleManager& particles) {
    if (index >= m_mines.size() || !m_mines[index].active || m_mines[index].detonated) return;

    m_mines[index].detonated = true;
    m_mines[index].active = false;
    Vector2 minePos = m_mines[index].position;

    // Spawn massive explosion
    Vector3 minePos3D = { minePos.x, 0.3f, minePos.y };
    particles.AddExplosion(minePos3D, MINE_BLAST_RADIUS);

    // Damage all tanks within blast radius
    for (auto& tank : tanks) {
        if (!tank.IsAlive()) continue;
        float dist = Vector2Distance(minePos, tank.GetPosition());
        if (dist < MINE_BLAST_RADIUS) {
            tank.TakeDamage(particles);
        }
    }

    // Destroy surrounding cork blocks in 3x3 grid around explosion
    int centerGx, centerGy;
    level.WorldToGrid(minePos, centerGx, centerGy);

    for (int dy = -1; dy <= 1; ++dy) {
        for (int dx = -1; dx <= 1; ++dx) {
            int gx = centerGx + dx;
            int gy = centerGy + dy;
            if (level.IsDestructible(gx, gy)) {
                Vector2 blockWorld = level.GridToWorld(gx, gy);
                if (Vector2Distance(minePos, blockWorld) <= MINE_BLAST_RADIUS * 1.1f) {
                    level.DestroyBlock(gx, gy);
                    particles.AddBlockDebris({ blockWorld.x, 0.5f, blockWorld.y }, { 210, 160, 100, 255 });
                }
            }
        }
    }

    // Chain reaction on neighboring mines
    for (size_t i = 0; i < m_mines.size(); ++i) {
        if (i == index || !m_mines[i].active || m_mines[i].detonated) continue;
        float dist = Vector2Distance(minePos, m_mines[i].position);
        if (dist < MINE_BLAST_RADIUS) {
            DetonateMine(i, level, tanks, particles);
        }
    }
}

void MineManager::Update(float dt, Level& level, std::vector<Tank>& tanks, ParticleManager& particles, bool isServer) {
    for (size_t i = 0; i < m_mines.size(); ++i) {
        Mine& m = m_mines[i];
        if (!m.active || m.detonated) continue;

        if (m.armTimer > 0.0f) {
            m.armTimer -= dt;
        }

        m.timer -= dt;

        // Check tank proximity
        bool tankNearby = false;
        for (const auto& tank : tanks) {
            if (!tank.IsAlive()) continue;
            float dist = Vector2Distance(m.position, tank.GetPosition());
            
            // Immediate detonation on contact if armed
            if (m.armTimer <= 0.0f && dist < TANK_RADIUS + MINE_RADIUS * 0.5f) {
                DetonateMine(i, level, tanks, particles);
                break;
            }

            if (dist < 3.0f) {
                tankNearby = true;
            }
        }

        if (!m.active || m.detonated) continue;

        // Dynamic beep frequency
        m.beepRate = tankNearby ? 6.0f : (1.0f + (1.0f - (m.timer / MINE_LIFETIME)) * 3.0f);
        m.beepTimer += dt * m.beepRate;
        if (m.beepTimer >= 1.0f) {
            m.beepTimer = 0.0f;
            m.flashTimer = 0.12f;
        }

        if (m.flashTimer > 0.0f) {
            m.flashTimer -= dt;
        }

        if (m.timer <= 0.0f) {
            DetonateMine(i, level, tanks, particles);
        }
    }

    // Clean up detonated mines
    for (size_t i = 0; i < m_mines.size(); ) {
        if (m_mines[i].detonated) {
            m_mines[i] = m_mines.back();
            m_mines.pop_back();
        } else {
            ++i;
        }
    }
}
