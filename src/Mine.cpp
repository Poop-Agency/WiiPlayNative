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
    m.age = 0.0f;
    m.fuse = 0.0f;
    m.fuseLit = false;
    m.proxArmed = false;
    m.beepTimer = 0.0f;
    m.beepRate = 1.0f;
    m.active = true;
    m.detonated = false;
    m.flashTimer = 0.0f;

    m_mines.push_back(m);
    return true;
}

bool MineManager::DetonateAt(Vector2 point, float radius) {
    bool any = false;
    for (auto& m : m_mines) {
        if (!m.active || m.detonated) continue;
        if (Vector2Distance(point, m.position) >= radius + MINE_RADIUS) continue;
        m.fuseLit = true;
        m.fuse = 0.0f;
        any = true;
    }
    return any;
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

        m.age += dt;

        // Mine::checkCollisions 0x80267484 runs one proximity query per frame:
        // until +0xD2 is set it asks for tanks within 90 px, and only afterwards
        // for tanks within 70 px. So a tank must first enter the arming ring,
        // then close to the trigger ring on a later frame.
        float queryRadius = m.proxArmed ? MINE_TRIGGER_RADIUS : MINE_ARM_RADIUS;
        bool tankNearby = false;
        for (const auto& tank : tanks) {
            if (!tank.IsAlive()) continue;
            if (Vector2Distance(m.position, tank.GetPosition()) >= queryRadius) continue;
            tankNearby = true;
            if (m.proxArmed) {
                // +0xCD -> +0xC0 = 20 frames, +0xD3 = 1
                if (!m.fuseLit) {
                    m.fuseLit = true;
                    m.fuse = MINE_TRIGGER_FUSE;
                }
            } else {
                m.proxArmed = true;
            }
            break;
        }

        // Chain detonation: another mine inside the summed 12 px radii.
        for (size_t j = 0; j < m_mines.size() && !m.fuseLit; ++j) {
            if (j == i || !m_mines[j].active || m_mines[j].detonated) continue;
            if (Vector2Distance(m.position, m_mines[j].position) < MINE_RADIUS * 2.0f) {
                m.fuseLit = true;
                m.fuse = 0.0f;
            }
        }

        // Self-arming: +0xA0 reaches 480 frames, then a 120 frame fuse.
        if (!m.fuseLit && m.age >= MINE_ARM_TIME) {
            m.fuseLit = true;
            m.fuse = MINE_FUSE_TIME;
        }

        // Dynamic beep frequency
        m.beepRate = tankNearby ? 6.0f : (1.0f + (m.age / MINE_LIFETIME) * 3.0f);
        m.beepTimer += dt * m.beepRate;
        if (m.beepTimer >= 1.0f) {
            m.beepTimer = 0.0f;
            m.flashTimer = 0.12f;
        }

        if (m.flashTimer > 0.0f) {
            m.flashTimer -= dt;
        }

        if (m.fuseLit) {
            m.fuse -= dt;
            if (m.fuse <= 0.0f) {
                DetonateMine(i, level, tanks, particles);
            }
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
