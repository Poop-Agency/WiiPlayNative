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
        // Direct LOS is totally clear!
        outAimPos = targetPos;
        return true;
    }
    return false;
}

bool AIManager::FindBankShot(const Tank& enemy, Vector2 targetPos, const Level& level, int maxBounces, Vector2& outAimPos) {
    Vector2 myPos = enemy.GetPosition();
    const int NUM_ANGLES = 48;

    for (int i = 0; i < NUM_ANGLES; ++i) {
        float angle = (i * (2.0f * PI / NUM_ANGLES));
        Vector2 curPos = myPos;
        Vector2 curDir = { std::cos(angle), std::sin(angle) };

        for (int bounce = 0; bounce <= maxBounces; ++bounce) {
            Vector2 hitPoint, hitNormal;
            int hitTileX, hitTileY;

            bool hit = level.Raycast(curPos, curDir, 40.0f, hitPoint, hitNormal, hitTileX, hitTileY, true);
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

    // Only some tanks lead the target, and the binary agrees with the list that
    // was already here. The action selector extrapolates pos + f31 * dir for
    // everyone at 0x8026c474, then picks an aim mode by type right after: type 3
    // Teal and type 9 Black take mode 1 (0x8026c464, 0x8026c49c), type 7 Green
    // takes mode 2 (0x8026c48c), and every other type falls through on mode 0.
    //
    // Reading mode 1 as "lead the shot" is OURS. The branch structure is forced
    // by the code; what each mode means downstream is not. Green's mode 2 is the
    // odd one out and is not handled here at all yet.
    Vector2 predictedPlayerPos = playerPos;
    if (enemy.GetConfig().isRocket || enemy.GetType() == TankType::EnemyBlack || enemy.GetType() == TankType::EnemyTeal) {
        // The 1.5 lead distance is still ours: f31 comes in as an argument at
        // 0x8026c294 and its caller has not been read yet.
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

    // Aim error, from 0x8026c7d4. That callee fires on its own countdown
    // [A+0x10C], reloaded from field 39 (0x8026bb9c lwz / 0x8026bba0 stw) with a
    // fixed value and no gate, so each tank re-aims on a constant beat: Brown
    // every 60 frames, Teal every 8.
    //
    // The error itself: f31 = [A+0x1C] (0x8026c804), f30 = -f31 (0x8026c80c), then
    // a uniform draw between the two (0x8026caa8..0x8026cac0, the RNG normalised by
    // 2^-23) scaled by 0.711111 (0x8026cacc, sda2 0x8045a908) and handed to the
    // rotation builder at 0x8002ff8c as a yaw. A full circle is 65536 of those
    // units: 0x8002faa8 subtracts 65536 until the angle is in range.
    //
    // So the spread is +/- field 28, and the record makes the tanks behave as they
    // do on screen: Brown 170 the wildest, Teal 0 dead straight, Black 5 deadly.
    //
    // Open: at 65536 units per turn this puts Brown at only +/- 0.66 degrees, which
    // feels tighter than Brown plays. The mechanism is proven, the unit chain into
    // the table lookup at 0x8002fad0 is not fully unpicked. Worth measuring against
    // the real game before trusting the magnitude.
    // The turret does not track continuously. [A+0x10C] counts down and only on
    // expiry does 0x8026c7d4 recompute a heading; between beats the tank keeps
    // aiming where it last decided. Recomputing every frame is what made these
    // tanks play like an aimbot -- they were re-solving the intercept 60 times a
    // second against a target the original only looks at every reaimFrames.
    const TankConfig& aimCfg = enemy.GetConfig();
    state.aimTimer -= dt;
    if (state.aimTimer <= 0.0f) {
        state.aimTimer = aimCfg.reaimFrames / 60.0f;

        // Aim error, drawn fresh on each beat and held until the next one.
        if (aimCfg.aimSpread > 0.0f) {
            float draw = ((rand() / (float)RAND_MAX) * 2.0f - 1.0f) * aimCfg.aimSpread;
            state.aimError = draw * 0.711111f * (2.0f * PI / 65536.0f);
        } else {
            state.aimError = 0.0f;
        }

        Vector2 me = enemy.GetPosition();
        Vector2 d = { enemy.aimTarget.x - me.x, enemy.aimTarget.y - me.y };
        float c = std::cos(state.aimError), sn = std::sin(state.aimError);
        state.heldAim = { me.x + d.x * c - d.y * sn, me.y + d.x * sn + d.y * c };
        state.hasAim = true;
    }
    if (state.hasAim) enemy.aimTarget = state.heldAim;

    if (canShoot && state.shootTimer <= 0.0f) {
        enemy.shootRequested = true;

        // Cooldowns come from TnkGameParam.bin (col 37, frames at 60 Hz), with a small
        // jitter so a pack of identical tanks does not fire in lockstep.
        float cooldown = enemy.GetConfig().shootCooldown;

        if (enemy.GetType() == TankType::EnemyGreen) {
            // Green fires its two shots back to back, then waits out the full cooldown
            state.burstCount++;
            if (state.burstCount < 2) {
                state.shootTimer = 0.15f;
            } else {
                state.burstCount = 0;
                state.shootTimer = cooldown;
            }
        } else {
            // The original does not jitter a cooldown. Its fire timer [A+0x110]
            // is re-rolled as min + rand % (max - min) in frames, from record
            // fields 35 and 34 (reload at 0x8026bd14, bounds read at 0x8026bd50).
            // Brown waits 30-45 frames between decisions, Black 5-10.
            const TankConfig& cfg = enemy.GetConfig();
            int span = cfg.fireDecisionMax - cfg.fireDecisionMin;
            int frames = cfg.fireDecisionMin + (span > 0 ? rand() % span : 0);
            state.shootTimer = frames / 60.0f;
        }
    }

    // 2. MOVEMENT & NAVIGATION
    state.moveTimer -= dt;
    Vector2 moveDir = { 0.0f, 0.0f };

    // Check bullet dodging first (Elite tanks: Black, Teal, Purple, White)
    if (enemy.GetType() == TankType::EnemyBlack || enemy.GetType() == TankType::EnemyTeal || enemy.GetType() == TankType::EnemyWhite) {
        Vector2 dodge = FindDodgeVector(enemy, bullets);
        if (Vector2Length(dodge) > 0.1f) {
            moveDir = dodge;
            state.moveTimer = 0.4f;
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
                // Flee from players and lay mines
                if (closestDist < 7.0f) {
                    moveDir = { myPos.x - playerPos.x, myPos.y - playerPos.y };
                } else if (state.moveTimer <= 0.0f) {
                    float randAngle = (rand() % 360) * DEG2RAD;
                    moveDir = { std::cos(randAngle), std::sin(randAngle) };
                    state.moveTimer = 2.0f + (rand() % 100) / 50.0f;
                }
                break;

            case TankType::EnemyTeal:
            case TankType::EnemyBlack:
            case TankType::EnemyPurple:
            case TankType::EnemyWhite:
                // Flanking & approach
                if (closestDist > 8.0f) {
                    moveDir = { playerPos.x - myPos.x, playerPos.y - myPos.y };
                } else if (state.moveTimer <= 0.0f) {
                    float randAngle = (rand() % 360) * DEG2RAD;
                    moveDir = { std::cos(randAngle), std::sin(randAngle) };
                    state.moveTimer = 1.5f + (rand() % 100) / 60.0f;
                }
                break;

            case TankType::EnemyAsh:
            case TankType::EnemyRed:
            default:
                // Random wander
                if (state.moveTimer <= 0.0f) {
                    float randAngle = (rand() % 360) * DEG2RAD;
                    state.moveTarget = { std::cos(randAngle), std::sin(randAngle) };
                    state.moveTimer = 2.5f + (rand() % 100) / 40.0f;
                }
                moveDir = state.moveTarget;
                break;
        }
    }

    // Mine laying was hardcoded to the Yellow case, so three of the four tanks
    // that carry mines never laid one. Drive it off the stat instead: field 2 is
    // the allowance, non-zero for Yellow 4, Purple 2, White 2, Black 2.
    //
    // The spacing is no longer ours. The original re-rolls timer [A+0x118] as
    // min + rand % (max - min) frames from fields 4 and 3 (reload at 0x8026bd98,
    // bounds read at 0x8026bd50), i.e. a decision every 40-60 frames. Those two
    // fields are non-zero for exactly the tanks that carry mines, which is what
    // pins 0x8026c5ac as the mine callee.
    //
    // The range gate is no longer ours either. At 0x8026c5fc the original loads
    // A[0x50] and compares it against a distance, and the polarity is easy to get
    // backwards: `cror 2,0,2` folds LT into EQ, so the `bt` at 0x8026c608 leaves
    // when A[0x50] <= dist. The mine is laid only while something is CLOSER than
    // A[0x50]. It is a proximity requirement, not a spacing guard.
    //
    // A[0x50] is field 5, a flat 100 for every tank. Stored distances are pixels,
    // as the speeds are, so 100 px is 100 * CELL_SIZE / 32 world units.
    //
    // Expiry does not lay a mine by itself: a percentage is rolled against
    // A[0x5C] or A[0x60] (0x8026c6ac and 0x8026c6c8), the RNG scaled to [0,100)
    // at 0x8026c68c. Yellow rolls 50, the other three 3 or 5, which is why Yellow
    // fills a map with mines and Black leaves one now and then.
    //
    // Still ours: which of the two chances applies. The original picks on the
    // boolean returned by 0x80261c14 (tested at 0x8026c680); until that function
    // is read, take the lower, commoner one.
    const TankConfig& mineCfg = enemy.GetConfig();
    if (mineCfg.maxMines > 0 && mineCfg.mineDecisionMax > 0) {
        state.mineTimer -= dt;
        if (state.mineTimer <= 0.0f) {
            float rangeWorld = mineCfg.mineRangePx * CELL_SIZE / 32.0f;
            if (closestDist < rangeWorld &&
                (rand() % 100) <= (int)mineCfg.mineChanceFar) {
                enemy.mineRequested = true;
            }
            int span = mineCfg.mineDecisionMax - mineCfg.mineDecisionMin;
            state.mineTimer = (mineCfg.mineDecisionMin + (span > 0 ? rand() % span : 0)) / 60.0f;
        }
    }

    float moveLen = Vector2Length(moveDir);
    if (moveLen > 0.01f) {
        enemy.moveInput = { moveDir.x / moveLen, moveDir.y / moveLen };
    } else {
        enemy.moveInput = { 0, 0 };
    }
}
