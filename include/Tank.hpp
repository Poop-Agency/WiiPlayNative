#pragma once

#include "Common.hpp"
#include <string>

class Level;
class BulletManager;
class MineManager;
class ParticleManager;

class Tank {
public:
    Tank(uint32_t id, TankType type, Vector2 spawnPos);
    ~Tank();

    void Update(float dt, Level& level, ParticleManager& particles);
    bool Shoot(BulletManager& bullets, ParticleManager& particles);
    bool PlantMine(MineManager& mines);
    void TakeDamage(ParticleManager& particles);
    void Respawn(Vector2 spawnPos);

    // Getters & Setters
    uint32_t GetId() const { return m_id; }
    TankType GetType() const { return m_type; }
    const TankConfig& GetConfig() const { return m_config; }
    
    Vector2 GetPosition() const { return m_position; }
    void SetPosition(Vector2 pos) { m_position = pos; }

    float GetChassisAngle() const { return m_chassisAngle; }
    void SetChassisAngle(float angle) { m_chassisAngle = angle; }

    float GetTurretAngle() const { return m_turretAngle; }
    void SetTurretAngle(float angle) { m_turretAngle = angle; }

    bool IsAlive() const { return m_isAlive; }
    void SetAlive(bool alive) { m_isAlive = alive; }

    int GetLives() const { return m_lives; }
    void SetLives(int lives) { m_lives = lives; }

    float GetStealthAlpha() const { return m_stealthAlpha; }
    float GetRecoil() const { return m_recoil; }

    Vector2 GetBarrelTip() const;

    // Controls
    Vector2 moveInput;      // -1.0 to 1.0 (X, Y)
    Vector2 aimTarget;      // Target world position
    bool shootRequested;
    bool mineRequested;
    bool isHuman;

private:
    uint32_t m_id;
    TankType m_type;
    TankConfig m_config;
    Vector2 m_position;
    Vector2 m_velocity;
    float m_chassisAngle;
    float m_targetChassisAngle;
    float m_turretAngle;
    float m_speed;
    bool m_isAlive;
    int m_lives;

    float m_shootCooldown;
    float m_mineCooldown;
    float m_recoil;
    float m_stealthAlpha;
    float m_treadDistance;
    Vector2 m_lastTreadPos;
};
