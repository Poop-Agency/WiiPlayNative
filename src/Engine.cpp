#include "Engine.hpp"
#include <iostream>
#include <algorithm>

Engine::Engine()
    : m_width(1280)
    , m_height(720)
    , m_running(false)
{
}

Engine::~Engine() {
    Shutdown();
}

bool Engine::Init(int width, int height, const char* title) {
    m_width = width;
    m_height = height;

    SetConfigFlags(FLAG_MSAA_4X_HINT | FLAG_WINDOW_RESIZABLE | FLAG_VSYNC_HINT);
    InitWindow(m_width, m_height, title);
    SetTargetFPS(60);

    if (!IsWindowReady()) {
        std::cerr << "Failed to initialize Raylib window." << std::endl;
        return false;
    }

    m_gameState.Init();
    m_renderer.Init(m_width, m_height);
    m_network.Init();

    m_running = true;
    std::cout << "Wii Play Tanks! Native engine initialized successfully." << std::endl;
    return true;
}

void Engine::Shutdown() {
    if (!m_running) return;

    m_network.Shutdown();
    CloseWindow();
    m_running = false;
}

void Engine::ProcessInput() {
    if (m_gameState.GetScreen() == GameScreen::Playing || 
        m_gameState.GetScreen() == GameScreen::StageIntro) {
        
        m_gameState.ProcessLocalInput(m_renderer.GetCamera(), m_network.GetLocalPlayerId());

        // Send local input across network if connected as client
        if (m_network.GetRole() == NetworkRole::Client) {
            for (const auto& tank : m_gameState.GetTanks()) {
                if (tank.GetId() == m_network.GetLocalPlayerId()) {
                    PktPlayerInput inputPkt;
                    inputPkt.header.type = PacketType::PlayerInput;
                    inputPkt.playerId = static_cast<uint8_t>(tank.GetId());
                    inputPkt.moveX = tank.moveInput.x;
                    inputPkt.moveY = tank.moveInput.y;
                    inputPkt.aimX = tank.aimTarget.x;
                    inputPkt.aimY = tank.aimTarget.y;
                    inputPkt.shoot = tank.shootRequested ? 1 : 0;
                    inputPkt.mine = tank.mineRequested ? 1 : 0;
                    m_network.SendUnreliable(&inputPkt, sizeof(inputPkt));
                    break;
                }
            }
        }
    }

    if (IsKeyPressed(KEY_ESCAPE)) {
        if (m_gameState.GetScreen() == GameScreen::Playing || 
            m_gameState.GetScreen() == GameScreen::StageIntro ||
            m_gameState.GetScreen() == GameScreen::Victory ||
            m_gameState.GetScreen() == GameScreen::GameOver) {
            m_gameState.SetScreen(GameScreen::Title);
        }
    }
}

void Engine::Update(float dt) {
    // Clamp delta time to prevent physics anomalies on hitches
    dt = std::min(dt, 0.033f);

    m_network.Poll(m_gameState);
    m_gameState.Update(dt, &m_network);
    m_renderer.UpdateCamera(dt, nullptr);

    // If Server, broadcast state synchronization packets
    if (m_network.GetRole() == NetworkRole::Server) {
        for (const auto& tank : m_gameState.GetTanks()) {
            PktTankStateSync sync;
            sync.header.type = PacketType::TankStateSync;
            sync.tankId = static_cast<uint8_t>(tank.GetId());
            sync.posX = tank.GetPosition().x;
            sync.posY = tank.GetPosition().y;
            sync.chassisAngle = tank.GetChassisAngle();
            sync.turretAngle = tank.GetTurretAngle();
            sync.isAlive = tank.IsAlive() ? 1 : 0;
            sync.lives = static_cast<uint8_t>(tank.GetLives());
            m_network.BroadcastUnreliable(&sync, sizeof(sync));
        }
    }
}

void Engine::Render() {
    m_renderer.Render(m_gameState, &m_network);
}

void Engine::Run() {
    while (!WindowShouldClose() && m_running) {
        float dt = GetFrameTime();
        ProcessInput();
        Update(dt);
        Render();
    }
}
