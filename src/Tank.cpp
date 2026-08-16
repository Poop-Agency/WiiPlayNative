#include "Tank.hpp"
#include "Level.hpp"
#include "Bullet.hpp"
#include "Mine.hpp"
#include "Particle.hpp"
#include <cmath>
#include <algorithm>

static float NormalizeAngle(float angle) {
    while (angle > PI) angle -= 2.0f * PI;
    while (angle < -PI) angle += 2.0f * PI;
    return angle;
}

Tank::Tank(uint32_t id, TankType type, Vector2 spawnPos)
    : m_id(id)
    , m_type(type)
    , m_config(GetTankConfig(type))
    , m_position(spawnPos)
    , m_velocity{0.0f, 0.0f}
    , m_chassisAngle(0.0f)
    , m_targetChassisAngle(0.0f)
    , m_turretAngle(0.0f)
    , m_speed(0.0f)
    , m_isAlive(true)
    , m_lives(3)
    , m_shootCooldown(0.0f)
    , m_mineCooldown(0.0f)
    , m_recoil(0.0f)
    , m_stealthAlpha(1.0f)
    , m_treadDistance(0.0f)
    , m_lastTreadPos(spawnPos)
    , moveInput{0.0f, 0.0f}
    , aimTarget{0.0f, 0.0f}
    , shootRequested(false)
    , mineRequested(false)
    , isHuman(false)
{
}

Tank::~Tank() {}

void Tank::Respawn(Vector2 spawnPos) {
    m_position = spawnPos;
    m_velocity = { 0.0f, 0.0f };
    m_isAlive = true;
    m_shootCooldown = 0.0f;
    m_mineCooldown = 0.0f;
    m_recoil = 0.0f;
    m_stealthAlpha = 1.0f;
    m_lastTreadPos = spawnPos;
    m_treadDistance = 0.0f;
}

Vector2 Tank::GetBarrelTip() const {
    return {
        m_position.x + std::cos(m_turretAngle) * BARREL_LENGTH,
        m_position.y + std::sin(m_turretAngle) * BARREL_LENGTH
    };
}

void Tank::Update(float dt, Level& level, ParticleManager& particles) {
    if (!m_isAlive) return;

    if (m_shootCooldown > 0.0f) m_shootCooldown -= dt;
    if (m_mineCooldown > 0.0f) m_mineCooldown -= dt;

    // Recoil spring recovery
    if (m_recoil > 0.0f) {
        m_recoil = std::max(0.0f, m_recoil - 6.0f * dt);
    }

    // Stealth alpha
    if (m_config.hasStealth) {
        float targetAlpha = 0.05f;
        if (m_shootCooldown > 0.0f || Vector2Length(moveInput) > 0.1f) {
            targetAlpha = 0.6f;
        }
        m_stealthAlpha += (targetAlpha - m_stealthAlpha) * 4.0f * dt;
    } else {
        m_stealthAlpha = 1.0f;
    }

    // Turret aiming towards target
    Vector2 toTarget = { aimTarget.x - m_position.x, aimTarget.y - m_position.y };
    float targetTurretAngle = std::atan2(toTarget.y, toTarget.x);
    m_turretAngle = targetTurretAngle;

    // Chassis movement
    float inputLen = Vector2Length(moveInput);
    if (inputLen > 0.05f && m_config.maxSpeed > 0.0f) {
        Vector2 normInput = { moveInput.x / inputLen, moveInput.y / inputLen };
        m_targetChassisAngle = std::atan2(normInput.y, normInput.x);

        // Smoothly rotate chassis
        float angleDiff = NormalizeAngle(m_targetChassisAngle - m_chassisAngle);
        m_chassisAngle += angleDiff * std::min(1.0f, m_config.turnSpeed * dt);
        m_chassisAngle = NormalizeAngle(m_chassisAngle);

        // Accelerate along facing/input
        float targetSpeed = m_config.maxSpeed * std::min(1.0f, inputLen);
        m_speed += (targetSpeed - m_speed) * 10.0f * dt;

        m_velocity = {
            std::cos(m_chassisAngle) * m_speed,
            std::sin(m_chassisAngle) * m_speed
        };
    } else {
        m_speed = std::max(0.0f, m_speed - 15.0f * dt);
        m_velocity = {
            std::cos(m_chassisAngle) * m_speed,
            std::sin(m_chassisAngle) * m_speed
        };
    }

    // Move position
    m_position.x += m_velocity.x * dt;
    m_position.y += m_velocity.y * dt;

    // Level collision resolution
    Vector2 pushback;
    if (level.CheckTankCollision(m_position, TANK_RADIUS, pushback)) {
        m_position.x += pushback.x;
        m_position.y += pushback.y;
    }

    // Tread marks on table
    float distMoved = Vector2Distance(m_position, m_lastTreadPos);
    m_treadDistance += distMoved;
    m_lastTreadPos = m_position;

    if (m_treadDistance >= 0.7f && m_speed > 0.2f) {
        m_treadDistance = 0.0f;
        Color treadColor = m_config.treadColor;
        treadColor.a = static_cast<unsigned char>(100 * m_stealthAlpha);
        particles.AddTreadMark(m_position, m_chassisAngle, 1.1f, treadColor);
    }
}

bool Tank::Shoot(BulletManager& bullets, ParticleManager& particles) {
    if (!m_isAlive || m_shootCooldown > 0.0f) return false;

    int activeCount = bullets.CountActiveBulletsForOwner(m_id);
    if (activeCount >= m_config.maxBullets) return false;

    // BARREL_LENGTH is the hull radius, so the muzzle sits on the hull edge and
    // can never be inside a block.
    Vector2 barrelTip = GetBarrelTip();
    Vector2 shootDir = { std::cos(m_turretAngle), std::sin(m_turretAngle) };

    bullets.SpawnBullet(
        m_id,
        barrelTip,
        shootDir,
        m_config.bulletSpeed,
        m_config.maxBounces,
        m_config.isRocket,
        m_config.turretColor
    );

    // Muzzle flash & smoke particles
    Vector3 barrelTip3D = { barrelTip.x, 0.4f, barrelTip.y };
    Vector3 shootDir3D = { shootDir.x, 0.0f, shootDir.y };
    particles.AddMuzzleFlash(barrelTip3D, shootDir3D, m_config.turretColor);

    // Recoil
    m_recoil = 0.35f;
    m_shootCooldown = m_config.shootCooldown;   // TnkGameParam field 36, frames/60

    return true;
}

bool Tank::PlantMine(MineManager& mines) {
    if (!m_isAlive || m_mineCooldown > 0.0f) return false;

    int activeCount = mines.CountActiveMinesForOwner(m_id);
    if (activeCount >= m_config.maxMines) return false;

    // Plant mine at current tank position
    if (mines.PlantMine(m_id, m_position)) {
        m_mineCooldown = 0.6f;
        return true;
    }
    return false;
}

void Tank::TakeDamage(ParticleManager& particles) {
    if (!m_isAlive) return;

    m_isAlive = false;
    --m_lives;

    // Spawn massive tank destruction explosion
    Vector3 tankPos3D = { m_position.x, 0.4f, m_position.y };
    particles.AddExplosion(tankPos3D, 3.5f);
    particles.AddBlockDebris(tankPos3D, m_config.bodyColor);
}
