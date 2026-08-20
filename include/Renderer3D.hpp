#pragma once

#include "Common.hpp"
#include <string>
#include <map>

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

    // Textures decoded out of the game's own BRRES files by tools/rip_textures.py.
    // Absent on a fresh clone, since they are Nintendo's and stay out of git, so
    // every draw falls back to the flat colours when a lookup misses.
    void LoadRippedTextures();
    const Texture2D* Tex(const std::string& key) const;
    void DrawTexCube(Vector3 pos, Vector3 size, const std::string& key, Color tint);

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

    std::map<std::string, Texture2D> m_tex;
    Model m_cube;
    bool m_cubeReady = false;
};
