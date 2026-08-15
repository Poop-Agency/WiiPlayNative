#pragma once

#include "Common.hpp"
#include <string>

class GameState;
class Level;
class Tank;
class BulletManager;
class MineManager;
class ParticleManager;
class NetworkManager;

class Renderer3D {
public:
    Renderer3D();
    ~Renderer3D();

    void Init(int screenWidth, int screenHeight);
    void Render(GameState& gameState, NetworkManager* network = nullptr);
    void UpdateCamera(float dt, const Tank* playerTank);

    Camera3D& GetCamera() { return m_camera; }
    void ToggleCameraMode();

private:
    void DrawArena(const Level& level);
    void DrawBlocks(const Level& level);
    void DrawTanks(const std::vector<Tank>& tanks);
    void DrawSingleTank(const Tank& tank);
    void DrawBullets(const BulletManager& bullets);
    void DrawMines(const MineManager& mines);
    void DrawParticles(const ParticleManager& particles);
    void DrawTreadMarks(const ParticleManager& particles);

    void DrawHUD(GameState& gameState, NetworkManager* network);
    void DrawCrosshair(Vector2 aimTarget);
    void DrawTitleScreen(GameState& gameState);
    void DrawMissionSelect(GameState& gameState);
    void DrawMultiplayerLobby(GameState& gameState, NetworkManager* network);

    Camera3D m_camera;
    int m_cameraMode; // 0 = 3D Isometric, 1 = Free Orbit, 2 = 2D Top-Down
    float m_camDistance;
    float m_camPitch;
    float m_camYaw;

    int m_screenWidth;
    int m_screenHeight;
};
