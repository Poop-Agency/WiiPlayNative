#include "AI.hpp"
#include "Tank.hpp"
#include "Level.hpp"
#include "Bullet.hpp"
#include "Mine.hpp"
#include <cmath>
#include <cstdlib>
#include <algorithm>

AIManager::AIManager() {}

AIManager::~AIManager() {}

void AIManager::Reset() {
    m_states.clear();
}

bool AIManager::FindDirectShot(const Tank& enemy, Vector2 targetPos, const Level& level, Vector2& outAimPos) {
    Vector2 myPos = enemy.GetPosition();
    Vector2 dir = { targetPos.x - myPos.x, targetPos.y - myPos.y };
    float dist = Vector2Length(dir);
    if (dist < 0.1f) return false;

    Vector2 normDir = { dir.x / dist, dir.y / dist };
    Vector2 hitPoint, hitNormal;
    int hitTileX, hitTileY;

    if (!level.Raycast(myPos, normDir, dist, hitPoint, hitNormal, hitTileX, hitTileY, true)) {
        // Direct Line of Sight is clear
        outAimPos = targetPos;
        return true;
    }
    return false;
}

bool AIManager::FindBankShot(const Tank& enemy, Vector2 targetPos, const Level& level, int maxBounces, Vector2& outAimPos) {
    Vector2 myPos = enemy.GetPosition();
    // Use 64 radial angles for high-precision ricochet calculations (Green tank snipes)
    const int NUM_ANGLES = (maxBounces >= 2) ? 64 : 48;

    for (int i = 0; i < NUM_ANGLES; ++i) {
        float angle = (i * (2.0f * PI / NUM_ANGLES));
        Vector2 curPos = myPos;
        Vector2 curDir = { std::cos(angle), std::sin(angle) };

        for (int bounce = 0; bounce <= maxBounces; ++bounce) {
            Vector2 hitPoint, hitNormal;
            int hitTileX, hitTileY;

            bool hit = level.Raycast(curPos, curDir, 45.0f, hitPoint, hitNormal, hitTileX, hitTileY, true);
            if (!hit) break;

            // Check if segment from curPos to hitPoint passes near targetPos
            Vector2 seg = { hitPoint.x - curPos.x, hitPoint.y - curPos.y };
            float segLen = Vector2Length(seg);
            if (segLen > 0.1f) {
                Vector2 segNorm = { seg.x / segLen, seg.y / segLen };
                Vector2 toTarget = { targetPos.x - curPos.x, targetPos.y - curPos.y };
                float proj = toTarget.x * segNorm.x + toTarget.y * segNorm.y;

                if (proj > 0.0f && proj < segLen) {
                    Vector2 closest = { curPos.x + segNorm.x * proj, curPos.y + segNorm.y * proj };
                    if (Vector2Distance(closest, targetPos) < TANK_RADIUS * 1.6f) {
                        // Found a valid bank shot!
                        outAimPos = { myPos.x + std::cos(angle) * 5.0f, myPos.y + std::sin(angle) * 5.0f };
                        return true;
                    }
                }
            }

            if (bounce == maxBounces) break;

            // Reflect ray for next bounce
            float dot = curDir.x * hitNormal.x + curDir.y * hitNormal.y;
            curDir.x -= 2.0f * dot * hitNormal.x;
            curDir.y -= 2.0f * dot * hitNormal.y;
            curPos = { hitPoint.x + hitNormal.x * 0.05f, hitPoint.y + hitNormal.y * 0.05f };
        }
    }

    return false;
}

Vector2 AIManager::FindDodgeVector(const Tank& enemy, const BulletManager& bullets) {
    Vector2 myPos = enemy.GetPosition();
    Vector2 dodgeVec = { 0.0f, 0.0f };

    for (const auto& b : bullets.GetBullets()) {
        if (!b.active || b.ownerId == enemy.GetId()) continue;

        Vector2 toTank = { myPos.x - b.position.x, myPos.y - b.position.y };
        float dist = Vector2Length(toTank);
        if (dist > 7.0f || dist < 0.1f) continue;

        Vector2 bulletDir = { b.velocity.x / b.speed, b.velocity.y / b.speed };
        float dot = toTank.x * bulletDir.x + toTank.y * bulletDir.y;

        // If bullet is flying towards this tank
        if (dot > 0.0f) {
            Vector2 closestPoint = { b.position.x + bulletDir.x * dot, b.position.y + bulletDir.y * dot };
            float crossDist = Vector2Distance(closestPoint, myPos);

            if (crossDist < TANK_RADIUS * 2.2f) {
                // Strafe perpendicular to bullet
                Vector2 perp1 = { -bulletDir.y, bulletDir.x };
                Vector2 perp2 = { bulletDir.y, -bulletDir.x };

                // Choose direction further from bullet
                Vector2 pos1 = { myPos.x + perp1.x, myPos.y + perp1.y };
                Vector2 pos2 = { myPos.x + perp2.x, myPos.y + perp2.y };

                if (Vector2Distance(pos1, b.position) > Vector2Distance(pos2, b.position)) {
                    dodgeVec = perp1;
                } else {
                    dodgeVec = perp2;
                }
                break;
            }
        }
    }

    return dodgeVec;
}

static Vector2 AvoidWalls(Vector2 pos, Vector2 desiredDir, const Level& level) {
    if (Vector2Length(desiredDir) < 0.01f) return desiredDir;

    Vector2 normDesired = { desiredDir.x / Vector2Length(desiredDir), desiredDir.y / Vector2Length(desiredDir) };
    Vector2 hitPoint, hitNormal;
    int hitX, hitY;

    // If clear ahead for 1.25 blocks, proceed directly
    if (!level.Raycast(pos, normDesired, 1.25f * CELL_SIZE, hitPoint, hitNormal, hitX, hitY, true)) {
        return normDesired;
    }

    // Otherwise evaluate lateral 45 and 90 degree corridor angles
    float baseAngle = std::atan2(normDesired.y, normDesired.x);
    float testOffsets[] = { 0.785f, -0.785f, 1.57f, -1.57f, 2.356f, -2.356f, 3.1415f };

    for (float off : testOffsets) {
        float testAngle = baseAngle + off;
        Vector2 testDir = { std::cos(testAngle), std::sin(testAngle) };
        if (!level.Raycast(pos, testDir, 1.0f * CELL_SIZE, hitPoint, hitNormal, hitX, hitY, true)) {
            return testDir;
        }
    }

    return normDesired;
}

void AIManager::Update(float dt, std::vector<Tank>& tanks, Level& level, 
                       const BulletManager& bullets, MineManager& mines) 
{
    if (m_states.size() < tanks.size()) {
        m_states.resize(tanks.size());
    }

    for (size_t i = 0; i < tanks.size(); ++i) {
        if (tanks[i].isHuman || !tanks[i].IsAlive()) continue;
        UpdateEnemy(tanks[i], m_states[i], dt, tanks, level, bullets, mines);
    }
}

void AIManager::UpdateEnemy(Tank& enemy, AIState& state, float dt, 
                            const std::vector<Tank>& tanks, Level& level, 
                            const BulletManager& bullets, MineManager& mines) 
{
    // Find closest living player tank
    const Tank* targetPlayer = nullptr;
    float closestDist = 1e9f;

    for (const auto& other : tanks) {
        if (other.isHuman && other.IsAlive()) {
            float d = Vector2Distance(enemy.GetPosition(), other.GetPosition());
            if (d < closestDist) {
                closestDist = d;
                targetPlayer = &other;
            }
        }
    }

    if (!targetPlayer) {
        enemy.moveInput = { 0, 0 };
        return;
    }

    Vector2 playerPos = targetPlayer->GetPosition();
    Vector2 myPos = enemy.GetPosition();

    // Lead target with velocity for rocket/elite tanks
    Vector2 predictedPlayerPos = playerPos;
    if (enemy.GetConfig().isRocket || enemy.GetType() == TankType::EnemyBlack || enemy.GetType() == TankType::EnemyTeal) {
        predictedPlayerPos = {
            playerPos.x + targetPlayer->moveInput.x * 1.5f,
            playerPos.y + targetPlayer->moveInput.y * 1.5f
        };
    }

    // 1. AIMING & SHOOTING
    state.shootTimer -= dt;
    enemy.shootRequested = false;
    enemy.mineRequested = false;

    Vector2 aimPos;
    bool canShoot = false;

    if (FindDirectShot(enemy, predictedPlayerPos, level, aimPos)) {
        enemy.aimTarget = aimPos;
        canShoot = true;
    } else if (enemy.GetConfig().maxBounces > 0) {
        // Calculate ricochet bank shot
        if (FindBankShot(enemy, predictedPlayerPos, level, enemy.GetConfig().maxBounces, aimPos)) {
            enemy.aimTarget = aimPos;
            canShoot = true;
        } else {
            enemy.aimTarget = predictedPlayerPos;
        }
    } else {
        enemy.aimTarget = predictedPlayerPos;
    }

    if (canShoot && state.shootTimer <= 0.0f) {
        enemy.shootRequested = true;

        // Cooldowns come from TnkGameParam.bin field 36 (frames at 60 Hz)
        float cooldown = enemy.GetConfig().shootCooldown;

        if (enemy.GetType() == TankType::EnemyGreen) {
            // Green sniper fires 2 rapid burst shots, then waits out the full cooldown
            state.burstCount++;
            if (state.burstCount < 2) {
                state.shootTimer = 0.15f;
            } else {
                state.burstCount = 0;
                state.shootTimer = cooldown;
            }
        } else {
            state.shootTimer = cooldown * (0.92f + (rand() % 100) / 600.0f);
        }
    }

    // 2. MOVEMENT & NAVIGATION
    state.moveTimer -= dt;
    Vector2 moveDir = { 0.0f, 0.0f };

    // Check bullet dodging (Elite dodging tanks: Black, Teal, White)
    if (enemy.GetType() == TankType::EnemyBlack || enemy.GetType() == TankType::EnemyTeal || enemy.GetType() == TankType::EnemyWhite) {
        Vector2 dodge = FindDodgeVector(enemy, bullets);
        if (Vector2Length(dodge) > 0.1f) {
            moveDir = dodge;
            state.moveTimer = 0.35f;
        }
    }

    if (Vector2Length(moveDir) < 0.1f) {
        switch (enemy.GetType()) {
            case TankType::EnemyBrown:
            case TankType::EnemyGreen:
                // Stationary turrets
                moveDir = { 0, 0 };
                break;

            case TankType::EnemyYellow:
                // Flee from players and plant mines
                if (closestDist < 7.0f) {
                    Vector2 away = { myPos.x - playerPos.x, myPos.y - playerPos.y };
                    moveDir = AvoidWalls(myPos, away, level);
                } else if (state.moveTimer <= 0.0f) {
                    float randAngle = (rand() % 8) * (PI * 0.25f);
                    Vector2 wander = { std::cos(randAngle), std::sin(randAngle) };
                    moveDir = AvoidWalls(myPos, wander, level);
                    state.moveTimer = 2.0f + (rand() % 100) / 50.0f;
                }
                break;

            case TankType::EnemyTeal:
            case TankType::EnemyBlack:
            case TankType::EnemyPurple:
            case TankType::EnemyWhite:
                // Flanking & pursuit
                if (closestDist > 7.5f) {
                    Vector2 towards = { playerPos.x - myPos.x, playerPos.y - myPos.y };
                    moveDir = AvoidWalls(myPos, towards, level);
                } else if (state.moveTimer <= 0.0f) {
                    float randAngle = (rand() % 8) * (PI * 0.25f);
                    Vector2 wander = { std::cos(randAngle), std::sin(randAngle) };
                    moveDir = AvoidWalls(myPos, wander, level);
                    state.moveTimer = 1.5f + (rand() % 100) / 60.0f;
                }
                break;

            case TankType::EnemyAsh:
            case TankType::EnemyRed:
            default:
                // Random corridor wander
                if (state.moveTimer <= 0.0f) {
                    float randAngle = (rand() % 8) * (PI * 0.25f);
                    Vector2 wander = { std::cos(randAngle), std::sin(randAngle) };
                    state.moveTarget = AvoidWalls(myPos, wander, level);
                    state.moveTimer = 2.5f + (rand() % 100) / 40.0f;
                }
                moveDir = state.moveTarget;
                break;
        }
    }

    // Mine laying from TnkGameParam.bin field 2:
    // Yellow 4, Purple 2, White 2, Black 2
    if (enemy.GetConfig().maxMines > 0) {
        state.mineTimer -= dt;
        if (state.mineTimer <= 0.0f && closestDist < 5.0f) {
            enemy.mineRequested = true;
            state.mineTimer = (enemy.GetType() == TankType::EnemyYellow) ? 3.0f : 4.0f;
        }
    }

    float moveLen = Vector2Length(moveDir);
    if (moveLen > 0.01f) {
        enemy.moveInput = { moveDir.x / moveLen, moveDir.y / moveLen };
    } else {
        enemy.moveInput = { 0, 0 };
    }
}
