#include "Renderer3D.hpp"
#include "GameState.hpp"
#include "Network.hpp"
#include "rlgl.h"
#include <cmath>
#include <iostream>
#include <string>

Renderer3D::Renderer3D()
    : m_cameraMode(0)
    , m_camDistance(34.0f)
    , m_camPitch(56.0f)
    , m_camYaw(0.0f)
    , m_screenWidth(1280)
    , m_screenHeight(720)
{
}

Renderer3D::~Renderer3D() {}

void Renderer3D::Init(int screenWidth, int screenHeight) {
    m_screenWidth = screenWidth;
    m_screenHeight = screenHeight;

    m_camera.position = { 0.0f, 32.0f, 22.0f };
    m_camera.target = { 0.0f, 0.0f, 0.0f };
    m_camera.up = { 0.0f, 1.0f, 0.0f };
    m_camera.fovy = 45.0f;
    m_camera.projection = CAMERA_PERSPECTIVE;
}

void Renderer3D::ToggleCameraMode() {
    m_cameraMode = (m_cameraMode + 1) % 3;
    if (m_cameraMode == 0) {
        // 3D Isometric / Angled View (Default Wii Style)
        m_camera.position = { 0.0f, 32.0f, 22.0f };
        m_camera.target = { 0.0f, 0.0f, 0.0f };
        m_camera.fovy = 45.0f;
        m_camera.projection = CAMERA_PERSPECTIVE;
    } else if (m_cameraMode == 1) {
        // Dynamic 3D
        m_camera.position = { 0.0f, 24.0f, 18.0f };
        m_camera.target = { 0.0f, 0.0f, -2.0f };
        m_camera.fovy = 52.0f;
        m_camera.projection = CAMERA_PERSPECTIVE;
    } else if (m_cameraMode == 2) {
        // Classic 2D Top-Down View
        m_camera.position = { 0.0f, 40.0f, 0.001f };
        m_camera.target = { 0.0f, 0.0f, 0.0f };
        m_camera.fovy = 40.0f;
        m_camera.projection = CAMERA_PERSPECTIVE;
    }
}

void Renderer3D::UpdateCamera(float dt, const Tank* playerTank) {
    if (IsKeyPressed(KEY_C)) {
        ToggleCameraMode();
    }
}

void Renderer3D::Render(GameState& gameState, NetworkManager* network) {
    BeginDrawing();
    ClearBackground({ 28, 30, 36, 255 });

    if (gameState.GetScreen() == GameScreen::Title) {
        DrawTitleScreen(gameState);
        EndDrawing();
        return;
    }

    if (gameState.GetScreen() == GameScreen::MissionSelect) {
        DrawMissionSelect(gameState);
        EndDrawing();
        return;
    }

    if (gameState.GetScreen() == GameScreen::LobbyMultiplayer) {
        DrawMultiplayerLobby(gameState, network);
        EndDrawing();
        return;
    }

    // --- 3D SCENE RENDERING ---
    BeginMode3D(m_camera);

    DrawArena(gameState.GetLevel());
    DrawTreadMarks(gameState.GetParticleManager());
    DrawBlocks(gameState.GetLevel());
    DrawMines(gameState.GetMineManager());
    DrawTanks(gameState.GetTanks());
    DrawBullets(gameState.GetBulletManager());
    DrawParticles(gameState.GetParticleManager());

    EndMode3D();

    // --- 2D HUD & OVERLAYS ---
    DrawHUD(gameState, network);

    EndDrawing();
}

void Renderer3D::DrawArena(const Level& level) {
    float arenaW = level.GetWidth() * CELL_SIZE;
    float arenaH = level.GetHeight() * CELL_SIZE;

    // 1. Table Top Surface (Light warm felt matching Wii Play)
    DrawCube({ 0.0f, -0.2f, 0.0f }, arenaW, 0.4f, arenaH, { 242, 238, 228, 255 });

    // 2. Subtle Grid Pattern
    for (int y = 0; y < level.GetHeight(); ++y) {
        for (int x = 0; x < level.GetWidth(); ++x) {
            if ((x + y) % 2 == 1) {
                Vector2 center = level.GridToWorld(x, y);
                DrawCube({ center.x, 0.005f, center.y }, CELL_SIZE * 0.98f, 0.01f, CELL_SIZE * 0.98f, { 235, 230, 218, 255 });
            }
        }
    }

    // 3. Wooden Border Rails (Beveled table borders)
    float wallThick = 1.6f;
    float wallHeight = 1.1f;
    Color woodColor = { 165, 115, 75, 255 };
    Color woodBevel = { 190, 135, 90, 255 };

    // North wall
    DrawCube({ 0.0f, wallHeight * 0.5f, -arenaH * 0.5f - wallThick * 0.5f }, arenaW + wallThick * 2, wallHeight, wallThick, woodColor);
    DrawCubeWires({ 0.0f, wallHeight * 0.5f, -arenaH * 0.5f - wallThick * 0.5f }, arenaW + wallThick * 2, wallHeight, wallThick, woodBevel);

    // South wall
    DrawCube({ 0.0f, wallHeight * 0.5f, arenaH * 0.5f + wallThick * 0.5f }, arenaW + wallThick * 2, wallHeight, wallThick, woodColor);
    DrawCubeWires({ 0.0f, wallHeight * 0.5f, arenaH * 0.5f + wallThick * 0.5f }, arenaW + wallThick * 2, wallHeight, wallThick, woodBevel);

    // West wall
    DrawCube({ -arenaW * 0.5f - wallThick * 0.5f, wallHeight * 0.5f, 0.0f }, wallThick, wallHeight, arenaH, woodColor);
    DrawCubeWires({ -arenaW * 0.5f - wallThick * 0.5f, wallHeight * 0.5f, 0.0f }, wallThick, wallHeight, arenaH, woodBevel);

    // East wall
    DrawCube({ arenaW * 0.5f + wallThick * 0.5f, wallHeight * 0.5f, 0.0f }, wallThick, wallHeight, arenaH, woodColor);
    DrawCubeWires({ arenaW * 0.5f + wallThick * 0.5f, wallHeight * 0.5f, 0.0f }, wallThick, wallHeight, arenaH, woodBevel);
}

void Renderer3D::DrawBlocks(const Level& level) {
    for (int y = 0; y < level.GetHeight(); ++y) {
        for (int x = 0; x < level.GetWidth(); ++x) {
            TileType t = level.GetTile(x, y);
            if (t == TileType::Empty) continue;

            Vector2 pos = level.GridToWorld(x, y);

            if (t == TileType::CorkBlock || t == TileType::BlockVariant1) {
                // Breakable Cork Block
                DrawCube({ pos.x, 0.55f, pos.y }, CELL_SIZE * 0.95f, 1.1f, CELL_SIZE * 0.95f, { 215, 175, 120, 255 });
                DrawCubeWires({ pos.x, 0.55f, pos.y }, CELL_SIZE * 0.95f, 1.1f, CELL_SIZE * 0.95f, { 185, 145, 95, 255 });
                DrawCube({ pos.x, 1.11f, pos.y }, CELL_SIZE * 0.85f, 0.02f, CELL_SIZE * 0.85f, { 230, 190, 135, 255 });
            } else if (t == TileType::SolidBlock || t == TileType::BlockVariant2) {
                // Indestructible Stone Block
                DrawCube({ pos.x, 0.65f, pos.y }, CELL_SIZE * 0.95f, 1.3f, CELL_SIZE * 0.95f, { 115, 120, 130, 255 });
                DrawCubeWires({ pos.x, 0.65f, pos.y }, CELL_SIZE * 0.95f, 1.3f, CELL_SIZE * 0.95f, { 80, 85, 95, 255 });
                DrawCube({ pos.x, 1.31f, pos.y }, CELL_SIZE * 0.85f, 0.02f, CELL_SIZE * 0.85f, { 145, 150, 160, 255 });
            } else if (level.IsHole(x, y)) {
                // Hole / Trench
                DrawCube({ pos.x, -0.05f, pos.y }, CELL_SIZE * 0.98f, 0.2f, CELL_SIZE * 0.98f, { 50, 65, 80, 255 });
                DrawCubeWires({ pos.x, -0.05f, pos.y }, CELL_SIZE * 0.98f, 0.2f, CELL_SIZE * 0.98f, { 35, 45, 60, 255 });
            }
        }
    }
}

void Renderer3D::DrawTreadMarks(const ParticleManager& particles) {
    for (const auto& tm : particles.GetTreadMarks()) {
        Color c = tm.color;
        c.a = static_cast<unsigned char>(255 * tm.alpha);
        DrawCube({ tm.leftPos.x, 0.01f, tm.leftPos.y }, 0.28f, 0.01f, 0.28f, c);
        DrawCube({ tm.rightPos.x, 0.01f, tm.rightPos.y }, 0.28f, 0.01f, 0.28f, c);
    }
}

void Renderer3D::DrawTanks(const std::vector<Tank>& tanks) {
    for (const auto& tank : tanks) {
        if (!tank.IsAlive()) continue;
        DrawSingleTank(tank);
    }
}

void Renderer3D::DrawSingleTank(const Tank& tank) {
    Vector2 pos = tank.GetPosition();
    float chassisAngle = tank.GetChassisAngle();
    float turretAngle = tank.GetTurretAngle();
    float recoil = tank.GetRecoil();
    float alpha = tank.GetStealthAlpha();

    Color bodyCol = tank.GetConfig().bodyColor;
    Color treadCol = tank.GetConfig().treadColor;
    Color turretCol = tank.GetConfig().turretColor;

    bodyCol.a = static_cast<unsigned char>(255 * alpha);
    treadCol.a = static_cast<unsigned char>(255 * alpha);
    turretCol.a = static_cast<unsigned char>(255 * alpha);

    // 1. Drop Shadow
    DrawCube({ pos.x, 0.02f, pos.y }, 1.7f, 0.01f, 1.7f, { 0, 0, 0, static_cast<unsigned char>(60 * alpha) });

    rlPushMatrix();
    rlTranslatef(pos.x, 0.0f, pos.y);

    // 2. Chassis with Treads (Rotated by Chassis Angle)
    rlPushMatrix();
    rlRotatef(-chassisAngle * RAD2DEG, 0.0f, 1.0f, 0.0f);

    // Left tread
    DrawCube({ 0.0f, 0.24f, 0.55f }, 1.5f, 0.38f, 0.34f, treadCol);
    DrawCubeWires({ 0.0f, 0.24f, 0.55f }, 1.5f, 0.38f, 0.34f, { 20, 20, 20, treadCol.a });

    // Right tread
    DrawCube({ 0.0f, 0.24f, -0.55f }, 1.5f, 0.38f, 0.34f, treadCol);
    DrawCubeWires({ 0.0f, 0.24f, -0.55f }, 1.5f, 0.38f, 0.34f, { 20, 20, 20, treadCol.a });

    // Central chassis body
    DrawCube({ 0.0f, 0.32f, 0.0f }, 1.35f, 0.42f, 0.85f, bodyCol);
    DrawCubeWires({ 0.0f, 0.32f, 0.0f }, 1.35f, 0.42f, 0.85f, { 255, 255, 255, static_cast<unsigned char>(60 * alpha) });

    rlPopMatrix(); // End chassis

    // 3. Turret (Rotated by Turret Angle + Recoil)
    rlPushMatrix();
    rlRotatef(-turretAngle * RAD2DEG, 0.0f, 1.0f, 0.0f);

    // Recoil shift
    rlTranslatef(-recoil, 0.0f, 0.0f);

    // Turret dome
    DrawCube({ 0.0f, 0.58f, 0.0f }, 0.72f, 0.32f, 0.72f, turretCol);
    DrawCubeWires({ 0.0f, 0.58f, 0.0f }, 0.72f, 0.32f, 0.72f, { 255, 255, 255, static_cast<unsigned char>(80 * alpha) });

    // Cannon barrel
    DrawCube({ 0.65f, 0.58f, 0.0f }, 0.85f, 0.16f, 0.16f, { 50, 50, 55, turretCol.a });
    // Muzzle ring
    DrawCube({ 1.1f, 0.58f, 0.0f }, 0.15f, 0.22f, 0.22f, { 30, 30, 35, turretCol.a });

    rlPopMatrix(); // End turret

    rlPopMatrix(); // End tank transform
}

void Renderer3D::DrawBullets(const BulletManager& bullets) {
    for (const auto& b : bullets.GetBullets()) {
        if (!b.active) continue;

        // Motion trail ribbon
        for (size_t i = 1; i < b.trail.size(); ++i) {
            float alpha = float(i) / float(b.trail.size());
            Color trailCol = b.color;
            trailCol.a = static_cast<unsigned char>(180 * alpha);
            DrawCube({ b.trail[i].x, 0.4f, b.trail[i].y }, 0.18f * alpha, 0.18f * alpha, 0.18f * alpha, trailCol);
        }

        // Bullet projectile
        Vector3 pos3D = { b.position.x, 0.4f, b.position.y };
        if (b.isRocket) {
            DrawCube(pos3D, 0.55f, 0.25f, 0.25f, { 255, 80, 40, 255 });
            DrawCubeWires(pos3D, 0.55f, 0.25f, 0.25f, { 255, 220, 100, 255 });
        } else {
            DrawSphere(pos3D, BULLET_RADIUS * 1.3f, { 245, 190, 70, 255 });
            DrawSphereWires(pos3D, BULLET_RADIUS * 1.35f, 6, 6, { 180, 120, 30, 255 });
        }
    }
}

void Renderer3D::DrawMines(const MineManager& mines) {
    for (const auto& m : mines.GetMines()) {
        if (!m.active || m.detonated) continue;

        Vector3 pos = { m.position.x, 0.12f, m.position.y };
        // Base plate
        DrawCube(pos, 0.75f, 0.14f, 0.75f, { 45, 45, 50, 255 });
        DrawCubeWires(pos, 0.75f, 0.14f, 0.75f, { 25, 25, 30, 255 });

        // Blinking central LED indicator
        Color ledColor = (m.flashTimer > 0.0f) ? Color{ 255, 30, 30, 255 } : Color{ 80, 20, 20, 255 };
        DrawSphere({ pos.x, 0.24f, pos.z }, 0.16f, ledColor);
    }
}

void Renderer3D::DrawParticles(const ParticleManager& particles) {
    for (const auto& p : particles.GetParticles()) {
        if (p.type == ParticleType::Debris) {
            DrawCube(p.position, p.size, p.size, p.size, p.color);
        } else if (p.type == ParticleType::Fireball || p.type == ParticleType::Smoke) {
            DrawSphere(p.position, p.size, p.color);
        } else if (p.type == ParticleType::Spark) {
            DrawCube(p.position, p.size, p.size, p.size, p.color);
        } else if (p.type == ParticleType::Shockwave) {
            DrawCircle3D(p.position, p.size, { 0, 1, 0 }, 90.0f, p.color);
        }
    }
}

void Renderer3D::DrawHUD(GameState& gameState, NetworkManager* network) {
    int sw = GetScreenWidth();
    int sh = GetScreenHeight();

    // 1. Mission Title Banner at Top
    std::string missionText = "MISSION " + std::to_string(gameState.GetCurrentMission());
    int textW = MeasureText(missionText.c_str(), 32);
    DrawRectangle(sw / 2 - textW / 2 - 24, 16, textW + 48, 48, { 0, 0, 0, 180 });
    DrawRectangleLines(sw / 2 - textW / 2 - 24, 16, textW + 48, 48, { 220, 180, 100, 255 });
    DrawText(missionText.c_str(), sw / 2 - textW / 2, 24, 32, { 255, 230, 160, 255 });

    // 2. Remaining Enemy Tanks Counter (Bottom Right)
    int enemiesLeft = gameState.CountLivingEnemies();
    DrawRectangle(sw - 260, sh - 70, 240, 54, { 0, 0, 0, 180 });
    DrawRectangleLines(sw - 260, sh - 70, 240, 54, { 100, 100, 120, 255 });
    DrawText("ENEMIES REMAINING:", sw - 245, sh - 62, 14, { 180, 180, 200, 255 });
    
    // Draw colored dots for each remaining enemy
    int dotX = sw - 245;
    for (const auto& t : gameState.GetTanks()) {
        if (!t.isHuman && t.IsAlive()) {
            DrawCircle(dotX + 8, sh - 30, 8, t.GetConfig().bodyColor);
            DrawCircleLines(dotX + 8, sh - 30, 8, WHITE);
            dotX += 22;
        }
    }

    // 3. Player Lives & Score (Bottom Left)
    for (const auto& t : gameState.GetTanks()) {
        if (t.isHuman && t.GetId() == 0) {
            DrawRectangle(20, sh - 70, 220, 54, { 0, 0, 0, 180 });
            DrawRectangleLines(20, sh - 70, 220, 54, { 100, 100, 120, 255 });
            DrawText("LIVES:", 32, sh - 62, 14, { 180, 180, 200, 255 });
            for (int i = 0; i < t.GetLives(); ++i) {
                DrawRectangle(32 + i * 26, sh - 42, 18, 18, t.GetConfig().bodyColor);
                DrawRectangleLines(32 + i * 26, sh - 42, 18, 18, WHITE);
            }
            break;
        }
    }

    // 4. Multiplayer Status Badge (Top Right)
    if (network && network->GetRole() != NetworkRole::Offline) {
        std::string netRole = (network->GetRole() == NetworkRole::Server) ? "HOST (SERVER)" : "CLIENT";
        DrawRectangle(sw - 220, 16, 200, 44, { 0, 0, 0, 180 });
        DrawText(netRole.c_str(), sw - 205, 24, 16, { 100, 220, 140, 255 });
        std::string pCount = "Players: " + std::to_string(network->GetConnectedPlayerCount());
        DrawText(pCount.c_str(), sw - 205, 42, 14, { 200, 200, 200, 255 });
    }

    // 5. Stage Intro Overlay
    if (gameState.GetScreen() == GameScreen::StageIntro) {
        DrawRectangle(0, sh / 2 - 60, sw, 120, { 0, 0, 0, 210 });
        std::string introStr = "MISSION " + std::to_string(gameState.GetCurrentMission()) + " - START!";
        int introW = MeasureText(introStr.c_str(), 42);
        DrawText(introStr.c_str(), sw / 2 - introW / 2, sh / 2 - 21, 42, { 255, 220, 100, 255 });
    }

    // 6. Victory Overlay
    if (gameState.GetScreen() == GameScreen::Victory) {
        DrawRectangle(0, sh / 2 - 60, sw, 120, { 20, 120, 40, 220 });
        std::string vicStr = "MISSION CLEARED!";
        int vicW = MeasureText(vicStr.c_str(), 46);
        DrawText(vicStr.c_str(), sw / 2 - vicW / 2, sh / 2 - 23, 46, { 255, 255, 255, 255 });
    }

    // 7. Game Over Overlay
    if (gameState.GetScreen() == GameScreen::GameOver) {
        DrawRectangle(0, sh / 2 - 60, sw, 120, { 140, 20, 20, 220 });
        std::string goStr = "GAME OVER";
        int goW = MeasureText(goStr.c_str(), 46);
        DrawText(goStr.c_str(), sw / 2 - goW / 2, sh / 2 - 23, 46, { 255, 255, 255, 255 });
    }

    // 8. Crosshair at Mouse
    Vector2 mPos = GetMousePosition();
    DrawCircleLines(static_cast<int>(mPos.x), static_cast<int>(mPos.y), 10, { 255, 255, 255, 180 });
    DrawLine(static_cast<int>(mPos.x) - 15, static_cast<int>(mPos.y), static_cast<int>(mPos.x) + 15, static_cast<int>(mPos.y), { 255, 255, 255, 180 });
    DrawLine(static_cast<int>(mPos.x), static_cast<int>(mPos.y) - 15, static_cast<int>(mPos.x), static_cast<int>(mPos.y) + 15, { 255, 255, 255, 180 });
}

void Renderer3D::DrawTitleScreen(GameState& gameState) {
    int sw = GetScreenWidth();
    int sh = GetScreenHeight();

    // Background gradient box
    DrawRectangleGradientV(0, 0, sw, sh, { 20, 24, 32, 255 }, { 12, 14, 18, 255 });

    // Main Logo Banner
    const char* title = "WII PLAY: TANKS! NATIVE";
    int tw = MeasureText(title, 54);
    DrawText(title, sw / 2 - tw / 2, sh / 4 - 30, 54, { 255, 220, 110, 255 });

    const char* subtitle = "Authentic PowerPC Source Port & Online Multiplayer";
    int stw = MeasureText(subtitle, 20);
    DrawText(subtitle, sw / 2 - stw / 2, sh / 4 + 35, 20, { 180, 190, 210, 255 });

    // Menu Buttons
    int btnW = 320;
    int btnH = 50;
    int startY = sh / 2 - 40;
    int spacing = 65;

    struct MenuOption {
        const char* label;
        GameScreen targetScreen;
        GameMode mode;
    };

    std::vector<MenuOption> options = {
        { "1. SOLO CAMPAIGN (100 Missions)", GameScreen::StageIntro, GameMode::CampaignSingle },
        { "2. COOP CAMPAIGN (2-4 Players)", GameScreen::LobbyMultiplayer, GameMode::CampaignCoop },
        { "3. PVP ARENA DEATHMATCH", GameScreen::LobbyMultiplayer, GameMode::PvPDeathmatch },
        { "4. MISSION SELECTOR", GameScreen::MissionSelect, GameMode::CampaignSingle }
    };

    Vector2 mPos = GetMousePosition();

    for (size_t i = 0; i < options.size(); ++i) {
        Rectangle btnRect = { float(sw / 2 - btnW / 2), float(startY + i * spacing), float(btnW), float(btnH) };
        bool hovered = CheckCollisionPointRec(mPos, btnRect);

        DrawRectangleRec(btnRect, hovered ? Color{ 60, 80, 120, 255 } : Color{ 35, 45, 65, 255 });
        DrawRectangleLinesEx(btnRect, 2, hovered ? Color{ 255, 220, 100, 255 } : Color{ 100, 120, 150, 255 });

        int optW = MeasureText(options[i].label, 16);
        DrawText(options[i].label, sw / 2 - optW / 2, int(btnRect.y + 16), 16, hovered ? WHITE : Color{ 220, 220, 230, 255 });

        if (hovered && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
            gameState.SetMode(options[i].mode);
            if (options[i].targetScreen == GameScreen::StageIntro) {
                gameState.StartMission(1, false);
            } else {
                gameState.SetScreen(options[i].targetScreen);
            }
        }
    }

    // Controls footer
    const char* footer = "CONTROLS: [WASD / ZQSD] Move | [MOUSE] Aim | [LEFT CLICK] Shoot | [RIGHT CLICK / SPACE] Mine | [C] Camera";
    int fw = MeasureText(footer, 14);
    DrawText(footer, sw / 2 - fw / 2, sh - 40, 14, { 140, 150, 170, 255 });
}

void Renderer3D::DrawMissionSelect(GameState& gameState) {
    int sw = GetScreenWidth();
    int sh = GetScreenHeight();

    DrawRectangleGradientV(0, 0, sw, sh, { 20, 24, 32, 255 }, { 12, 14, 18, 255 });

    DrawText("SELECT MISSION", sw / 2 - MeasureText("SELECT MISSION", 36) / 2, 40, 36, { 255, 220, 110, 255 });

    // Grid of missions (1 to 30)
    int cols = 6;
    int rows = 5;
    int bSize = 64;
    int gap = 20;
    int startX = sw / 2 - ((cols * bSize + (cols - 1) * gap) / 2);
    int startY = 120;

    Vector2 mPos = GetMousePosition();

    for (int i = 1; i <= 30; ++i) {
        int idx = i - 1;
        int col = idx % cols;
        int row = idx / cols;

        Rectangle rect = { float(startX + col * (bSize + gap)), float(startY + row * (bSize + gap)), float(bSize), float(bSize) };
        bool hovered = CheckCollisionPointRec(mPos, rect);

        DrawRectangleRec(rect, hovered ? Color{ 70, 95, 140, 255 } : Color{ 40, 50, 70, 255 });
        DrawRectangleLinesEx(rect, 2, hovered ? Color{ 255, 220, 100, 255 } : Color{ 120, 140, 170, 255 });

        std::string numStr = std::to_string(i);
        int nw = MeasureText(numStr.c_str(), 24);
        DrawText(numStr.c_str(), int(rect.x + bSize / 2 - nw / 2), int(rect.y + bSize / 2 - 12), 24, WHITE);

        if (hovered && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
            gameState.StartMission(i, false);
        }
    }

    // Back Button
    Rectangle backBtn = { 40, float(sh - 70), 140, 44 };
    bool backHover = CheckCollisionPointRec(mPos, backBtn);
    DrawRectangleRec(backBtn, backHover ? Color{ 140, 50, 50, 255 } : Color{ 80, 30, 30, 255 });
    DrawRectangleLinesEx(backBtn, 2, WHITE);
    DrawText("< BACK", 65, sh - 58, 18, WHITE);

    if (backHover && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
        gameState.SetScreen(GameScreen::Title);
    }
}

void Renderer3D::DrawMultiplayerLobby(GameState& gameState, NetworkManager* network) {
    int sw = GetScreenWidth();
    int sh = GetScreenHeight();

    DrawRectangleGradientV(0, 0, sw, sh, { 20, 24, 32, 255 }, { 12, 14, 18, 255 });

    const char* lobbyTitle = "MULTIPLAYER LOBBY (ENet UDP)";
    DrawText(lobbyTitle, sw / 2 - MeasureText(lobbyTitle, 36) / 2, 40, 36, { 255, 220, 110, 255 });

    Vector2 mPos = GetMousePosition();

    // 1. Host Server Button
    Rectangle hostBtn = { float(sw / 2 - 220), 160.0f, 440.0f, 54.0f };
    bool hostHover = CheckCollisionPointRec(mPos, hostBtn);
    DrawRectangleRec(hostBtn, hostHover ? Color{ 50, 130, 80, 255 } : Color{ 30, 80, 50, 255 });
    DrawRectangleLinesEx(hostBtn, 2, hostHover ? Color{ 255, 220, 100, 255 } : Color{ 100, 200, 140, 255 });
    DrawText("HOST ONLINE SERVER (Port 7777)", sw / 2 - MeasureText("HOST ONLINE SERVER (Port 7777)", 18) / 2, 178, 18, WHITE);

    if (hostHover && IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && network) {
        network->StartServer(7777);
        gameState.StartMission(1, true);
    }

    // 2. Connect to Localhost / Direct IP Button
    Rectangle joinBtn = { float(sw / 2 - 220), 235.0f, 440.0f, 54.0f };
    bool joinHover = CheckCollisionPointRec(mPos, joinBtn);
    DrawRectangleRec(joinBtn, joinHover ? Color{ 60, 90, 150, 255 } : Color{ 35, 55, 95, 255 });
    DrawRectangleLinesEx(joinBtn, 2, joinHover ? Color{ 255, 220, 100, 255 } : Color{ 120, 160, 220, 255 });
    DrawText("JOIN LOCAL SERVER (127.0.0.1:7777)", sw / 2 - MeasureText("JOIN LOCAL SERVER (127.0.0.1:7777)", 18) / 2, 253, 18, WHITE);

    if (joinHover && IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && network) {
        network->ConnectToServer("127.0.0.1", 7777);
    }

    // Back Button
    Rectangle backBtn = { 40, float(sh - 70), 140, 44 };
    bool backHover = CheckCollisionPointRec(mPos, backBtn);
    DrawRectangleRec(backBtn, backHover ? Color{ 140, 50, 50, 255 } : Color{ 80, 30, 30, 255 });
    DrawRectangleLinesEx(backBtn, 2, WHITE);
    DrawText("< BACK", 65, sh - 58, 18, WHITE);

    if (backHover && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
        gameState.SetScreen(GameScreen::Title);
    }
}
