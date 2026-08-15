#include "GameState.hpp"
#include "Network.hpp"
#include <iostream>

GameState::GameState()
    : m_screen(GameScreen::Title)
    , m_mode(GameMode::CampaignSingle)
    , m_currentMission(1)
    , m_missionTimer(0.0f)
    , m_stateTimer(0.0f)
    , m_missionAnnounced(false)
{
}

GameState::~GameState() {}

void GameState::Init() {
    m_audioManager.Init();
    m_playerScores.push_back({ 0, "Player 1", 0, 0, 0, { 50, 120, 220, 255 } });
    m_playerScores.push_back({ 1, "Player 2", 0, 0, 0, { 220, 50, 50, 255 } });
}

void GameState::Reset() {
    m_tanks.clear();
    m_bulletManager.Reset();
    m_mineManager.Reset();
    m_particleManager.Reset();
    m_aiManager.Reset();
    m_level.Reset();
}

void GameState::StartMission(int missionNumber, bool is2Player) {
    m_currentMission = missionNumber;
    Reset();

    if (!m_level.LoadMission(missionNumber, is2Player)) {
        std::cerr << "Could not load mission " << missionNumber << ", starting level 1 fallback" << std::endl;
        m_level.LoadMission(1, is2Player);
    }

    // Spawn Player 1
    Tank p1(0, TankType::Player1, m_level.GetPlayer1Spawn());
    p1.isHuman = true;
    m_tanks.push_back(p1);

    // Spawn Player 2 if Coop or 2P
    if (is2Player || m_mode == GameMode::CampaignCoop || m_mode == GameMode::PvPDeathmatch) {
        Tank p2(1, TankType::Player2, m_level.GetPlayer2Spawn());
        p2.isHuman = true;
        m_tanks.push_back(p2);
    }

    // Spawn Enemy Tanks
    uint32_t enemyId = 10;
    for (const auto& spawn : m_level.GetEnemySpawns()) {
        Tank enemy(enemyId++, spawn.type, spawn.worldPos);
        enemy.isHuman = false;
        m_tanks.push_back(enemy);
    }

    m_screen = GameScreen::StageIntro;
    m_stateTimer = 1.8f;
    m_missionTimer = 0.0f;
    m_missionAnnounced = true;
    m_audioManager.Play(SoundType::MissionStart);
}

void GameState::NextMission() {
    StartMission(m_currentMission + 1, (m_mode != GameMode::CampaignSingle));
}

void GameState::RestartMission() {
    StartMission(m_currentMission, (m_mode != GameMode::CampaignSingle));
}

int GameState::CountLivingEnemies() const {
    int count = 0;
    for (const auto& t : m_tanks) {
        if (!t.isHuman && t.IsAlive()) {
            ++count;
        }
    }
    return count;
}

int GameState::CountLivingPlayers() const {
    int count = 0;
    for (const auto& t : m_tanks) {
        if (t.isHuman && t.IsAlive()) {
            ++count;
        }
    }
    return count;
}

bool GameState::IsMissionComplete() const {
    return (CountLivingEnemies() == 0);
}

bool GameState::IsMissionFailed() const {
    return (CountLivingPlayers() == 0);
}

void GameState::AddScore(uint32_t playerId, int points) {
    for (auto& p : m_playerScores) {
        if (p.id == playerId) {
            p.score += points;
            p.kills++;
            break;
        }
    }
}

void GameState::ProcessLocalInput(Camera3D& camera, uint32_t localPlayerId) {
    Tank* localTank = nullptr;
    for (auto& t : m_tanks) {
        if (t.GetId() == localPlayerId) {
            localTank = &t;
            break;
        }
    }

    if (!localTank || !localTank->IsAlive()) return;

    // Movement Input (WASD / ZQSD / Arrows)
    Vector2 move = { 0.0f, 0.0f };
    if (IsKeyDown(KEY_W) || IsKeyDown(KEY_Z) || IsKeyDown(KEY_UP)) move.y -= 1.0f;
    if (IsKeyDown(KEY_S) || IsKeyDown(KEY_DOWN)) move.y += 1.0f;
    if (IsKeyDown(KEY_A) || IsKeyDown(KEY_Q) || IsKeyDown(KEY_LEFT)) move.x -= 1.0f;
    if (IsKeyDown(KEY_D) || IsKeyDown(KEY_RIGHT)) move.x += 1.0f;

    // Gamepad Left Stick
    if (IsGamepadAvailable(0)) {
        float stickX = GetGamepadAxisMovement(0, GAMEPAD_AXIS_LEFT_X);
        float stickY = GetGamepadAxisMovement(0, GAMEPAD_AXIS_LEFT_Y);
        if (std::abs(stickX) > 0.15f) move.x = stickX;
        if (std::abs(stickY) > 0.15f) move.y = stickY;
    }

    float moveLen = Vector2Length(move);
    if (moveLen > 0.01f) {
        localTank->moveInput = { move.x / moveLen, move.y / moveLen };
    } else {
        localTank->moveInput = { 0.0f, 0.0f };
    }

    // Aiming with Mouse Raycast onto Y=0 Plane
    Ray mouseRay = GetScreenToWorldRay(GetMousePosition(), camera);
    if (std::abs(mouseRay.direction.y) > 0.001f) {
        float t = (0.35f - mouseRay.position.y) / mouseRay.direction.y;
        if (t > 0.0f) {
            localTank->aimTarget = {
                mouseRay.position.x + mouseRay.direction.x * t,
                mouseRay.position.z + mouseRay.direction.z * t
            };
        }
    }

    // Gamepad Right Stick Aiming
    if (IsGamepadAvailable(0)) {
        float rStickX = GetGamepadAxisMovement(0, GAMEPAD_AXIS_RIGHT_X);
        float rStickY = GetGamepadAxisMovement(0, GAMEPAD_AXIS_RIGHT_Y);
        if (std::abs(rStickX) > 0.2f || std::abs(rStickY) > 0.2f) {
            localTank->aimTarget = {
                localTank->GetPosition().x + rStickX * 10.0f,
                localTank->GetPosition().y + rStickY * 10.0f
            };
        }
    }

    // Shoot Request
    localTank->shootRequested = IsMouseButtonPressed(MOUSE_BUTTON_LEFT) || 
                                 IsKeyPressed(KEY_J) || 
                                 (IsGamepadAvailable(0) && IsGamepadButtonPressed(0, GAMEPAD_BUTTON_RIGHT_TRIGGER_1));

    // Mine Request
    localTank->mineRequested = IsMouseButtonPressed(MOUSE_BUTTON_RIGHT) || 
                                IsKeyPressed(KEY_SPACE) || 
                                IsKeyPressed(KEY_K) || 
                                (IsGamepadAvailable(0) && IsGamepadButtonPressed(0, GAMEPAD_BUTTON_LEFT_TRIGGER_1));
}

void GameState::Update(float dt, NetworkManager* network) {
    if (m_screen == GameScreen::StageIntro) {
        m_stateTimer -= dt;
        if (m_stateTimer <= 0.0f) {
            m_screen = GameScreen::Playing;
        }
        return;
    }

    if (m_screen == GameScreen::Playing) {
        m_missionTimer += dt;

        // Process tank actions
        for (auto& tank : m_tanks) {
            if (!tank.IsAlive()) continue;

            if (tank.shootRequested) {
                if (tank.Shoot(m_bulletManager, m_particleManager)) {
                    if (tank.GetConfig().isRocket) {
                        m_audioManager.Play(SoundType::ShootRocket);
                    } else {
                        m_audioManager.Play(SoundType::ShootNormal);
                    }
                }
            }

            if (tank.mineRequested) {
                if (tank.PlantMine(m_mineManager)) {
                    m_audioManager.Play(SoundType::MinePlant);
                }
            }

            tank.Update(dt, m_level, m_particleManager);
        }

        // Update AI for enemies
        m_aiManager.Update(dt, m_tanks, m_level, m_bulletManager, m_mineManager);

        // Update Bullets & Mines
        m_bulletManager.Update(dt, m_level, m_tanks, m_particleManager, true);
        m_mineManager.Update(dt, m_level, m_tanks, m_particleManager, true);

        // Update Particles
        m_particleManager.Update(dt);

        // Win / Loss conditions
        if (IsMissionComplete()) {
            m_screen = GameScreen::Victory;
            m_stateTimer = 2.5f;
            m_audioManager.Play(SoundType::Victory);
        } else if (IsMissionFailed()) {
            m_screen = GameScreen::GameOver;
            m_stateTimer = 3.5f;
            m_audioManager.Play(SoundType::GameOver);
        }
    } else if (m_screen == GameScreen::Victory) {
        m_particleManager.Update(dt);
        m_stateTimer -= dt;
        if (m_stateTimer <= 0.0f) {
            NextMission();
        }
    } else if (m_screen == GameScreen::GameOver) {
        m_particleManager.Update(dt);
        m_stateTimer -= dt;
        if (m_stateTimer <= 0.0f) {
            RestartMission();
        }
    }
}
