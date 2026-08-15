#pragma once

#include "Common.hpp"
#include "Level.hpp"
#include "Tank.hpp"
#include "Bullet.hpp"
#include "Mine.hpp"
#include "Particle.hpp"
#include "AI.hpp"
#include "Audio.hpp"
#include <vector>
#include <string>

class NetworkManager;

struct PlayerScore {
    uint32_t id;
    std::string name;
    int score;
    int kills;
    int deaths;
    Color color;
};

class GameState {
public:
    GameState();
    ~GameState();

    void Init();
    void Reset();

    void StartMission(int missionNumber, bool is2Player = false);
    void NextMission();
    void RestartMission();

    void Update(float dt, NetworkManager* network = nullptr);
    void ProcessLocalInput(Camera3D& camera, uint32_t localPlayerId);

    // Queries
    int CountLivingEnemies() const;
    int CountLivingPlayers() const;
    bool IsMissionComplete() const;
    bool IsMissionFailed() const;

    // Components
    Level& GetLevel() { return m_level; }
    std::vector<Tank>& GetTanks() { return m_tanks; }
    BulletManager& GetBulletManager() { return m_bulletManager; }
    MineManager& GetMineManager() { return m_mineManager; }
    ParticleManager& GetParticleManager() { return m_particleManager; }
    AIManager& GetAIManager() { return m_aiManager; }
    AudioManager& GetAudioManager() { return m_audioManager; }

    // State & Scores
    GameScreen GetScreen() const { return m_screen; }
    void SetScreen(GameScreen screen) { m_screen = screen; }
    
    GameMode GetMode() const { return m_mode; }
    void SetMode(GameMode mode) { m_mode = mode; }

    int GetCurrentMission() const { return m_currentMission; }
    float GetMissionTimer() const { return m_missionTimer; }
    float GetStateTimer() const { return m_stateTimer; }

    std::vector<PlayerScore>& GetPlayerScores() { return m_playerScores; }
    void AddScore(uint32_t playerId, int points);

private:
    Level m_level;
    std::vector<Tank> m_tanks;
    BulletManager m_bulletManager;
    MineManager m_mineManager;
    ParticleManager m_particleManager;
    AIManager m_aiManager;
    AudioManager m_audioManager;

    GameScreen m_screen;
    GameMode m_mode;
    int m_currentMission;
    float m_missionTimer;
    float m_stateTimer;
    bool m_missionAnnounced;
    TankType m_dominantEnemy;

    std::vector<PlayerScore> m_playerScores;
};
