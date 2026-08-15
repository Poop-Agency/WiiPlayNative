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
    , m_camPitch(60.0f)
    , m_camYaw(0.0f)
    , m_screenWidth(1280)
    , m_screenHeight(720)
{
}

Renderer3D::~Renderer3D() {}

void Renderer3D::Init(int screenWidth, int screenHeight) {
    m_screenWidth = screenWidth;
    m_screenHeight = screenHeight;

    // Authentic Wii Play perspective: Top-Down with a gentle forward tilt
    m_camera.position = { 0.0f, 38.0f, 14.0f };
    m_camera.target = { 0.0f, 0.0f, 0.0f };
    m_camera.up = { 0.0f, 1.0f, 0.0f };
    m_camera.fovy = 38.0f;
    m_camera.projection = CAMERA_PERSPECTIVE;
}

void Renderer3D::ToggleCameraMode() {
    m_cameraMode = (m_cameraMode + 1) % 3;
    if (m_cameraMode == 0) {
        // Authentic Wii Style (Gently tilted perspective)
        m_camera.position = { 0.0f, 38.0f, 14.0f };
        m_camera.target = { 0.0f, 0.0f, 0.0f };
        m_camera.fovy = 38.0f;
        m_camera.projection = CAMERA_PERSPECTIVE;
    } else if (m_cameraMode == 1) {
        // Pure 2D Top-Down View
        m_camera.position = { 0.0f, 42.0f, 0.001f };
        m_camera.target = { 0.0f, 0.0f, 0.0f };
        m_camera.fovy = 36.0f;
        m_camera.projection = CAMERA_PERSPECTIVE;
    } else if (m_cameraMode == 2) {
        // 3D Isometric View
        m_camera.position = { 0.0f, 30.0f, 26.0f };
        m_camera.target = { 0.0f, 0.0f, -1.0f };
        m_camera.fovy = 46.0f;
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
    ClearBackground({ 185, 205, 175, 255 }); // Warm Wii Play background border

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

    // 1. Table Top Surface (Oak wood grain tone matching Image 0)
    DrawCube({ 0.0f, -0.2f, 0.0f }, arenaW, 0.4f, arenaH, { 236, 212, 162, 255 });

    // 2. Wood Plank Lines
    for (int y = 0; y < level.GetHeight(); ++y) {
        float z = (y - level.GetHeight() * 0.5f + 0.5f) * CELL_SIZE;
        DrawCube({ 0.0f, 0.005f, z }, arenaW * 0.99f, 0.01f, 0.05f, { 218, 192, 142, 255 });
    }

    // 3. Wooden Border Rails (Toy building blocks framing the arena)
    float wallThick = 2.0f;
    float wallHeight = 1.3f;
    Color blockWood = { 210, 175, 125, 255 };
    Color blockEdge = { 180, 145, 95, 255 };

    // North wall
    DrawCube({ 0.0f, wallHeight * 0.5f, -arenaH * 0.5f - wallThick * 0.5f }, arenaW + wallThick * 2, wallHeight, wallThick, blockWood);
    DrawCubeWires({ 0.0f, wallHeight * 0.5f, -arenaH * 0.5f - wallThick * 0.5f }, arenaW + wallThick * 2, wallHeight, wallThick, blockEdge);

    // South wall
    DrawCube({ 0.0f, wallHeight * 0.5f, arenaH * 0.5f + wallThick * 0.5f }, arenaW + wallThick * 2, wallHeight, wallThick, blockWood);
    DrawCubeWires({ 0.0f, wallHeight * 0.5f, arenaH * 0.5f + wallThick * 0.5f }, arenaW + wallThick * 2, wallHeight, wallThick, blockEdge);

    // West wall
    DrawCube({ -arenaW * 0.5f - wallThick * 0.5f, wallHeight * 0.5f, 0.0f }, wallThick, wallHeight, arenaH, blockWood);
    DrawCubeWires({ -arenaW * 0.5f - wallThick * 0.5f, wallHeight * 0.5f, 0.0f }, wallThick, wallHeight, arenaH, blockEdge);

    // East wall
    DrawCube({ arenaW * 0.5f + wallThick * 0.5f, wallHeight * 0.5f, 0.0f }, wallThick, wallHeight, arenaH, blockWood);
    DrawCubeWires({ arenaW * 0.5f + wallThick * 0.5f, wallHeight * 0.5f, 0.0f }, wallThick, wallHeight, arenaH, blockEdge);
}

void Renderer3D::DrawBlocks(const Level& level) {
    for (int y = 0; y < level.GetHeight(); ++y) {
        for (int x = 0; x < level.GetWidth(); ++x) {
            TileType t = level.GetTile(x, y);
            if (t == TileType::Empty) continue;

            Vector2 pos = level.GridToWorld(x, y);

            if (level.IsDestructible(x, y)) {
                // Breakable Cork Block (Dotted warm cork matching Image 0)
                DrawCube({ pos.x, 0.65f, pos.y }, CELL_SIZE * 0.96f, 1.3f, CELL_SIZE * 0.96f, { 210, 160, 115, 255 });
                DrawCubeWires({ pos.x, 0.65f, pos.y }, CELL_SIZE * 0.96f, 1.3f, CELL_SIZE * 0.96f, { 175, 130, 85, 255 });
                // Cork texture dots
                DrawCube({ pos.x, 1.31f, pos.y }, CELL_SIZE * 0.82f, 0.02f, CELL_SIZE * 0.82f, { 225, 175, 130, 255 });
            } else if (level.IsSolid(x, y)) {
                // Solid Wooden Toy Block (Natural pine/oak block matching Image 0)
                DrawCube({ pos.x, 0.7f, pos.y }, CELL_SIZE * 0.96f, 1.4f, CELL_SIZE * 0.96f, { 235, 195, 130, 255 });
                DrawCubeWires({ pos.x, 0.7f, pos.y }, CELL_SIZE * 0.96f, 1.4f, CELL_SIZE * 0.96f, { 190, 150, 90, 255 });
                DrawCube({ pos.x, 1.41f, pos.y }, CELL_SIZE * 0.82f, 0.02f, CELL_SIZE * 0.82f, { 250, 215, 155, 255 });
            }
        }
    }
}

void Renderer3D::DrawTreadMarks(const ParticleManager& particles) {
    for (const auto& tm : particles.GetTreadMarks()) {
        Color c = tm.color;
        c.a = static_cast<unsigned char>(180 * tm.alpha);
        DrawCube({ tm.leftPos.x, 0.01f, tm.leftPos.y }, 0.32f, 0.01f, 0.32f, c);
        DrawCube({ tm.rightPos.x, 0.01f, tm.rightPos.y }, 0.32f, 0.01f, 0.32f, c);
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
    DrawCube({ pos.x, 0.02f, pos.y }, 1.9f, 0.01f, 1.9f, { 0, 0, 0, static_cast<unsigned char>(70 * alpha) });

    rlPushMatrix();
    rlTranslatef(pos.x, 0.0f, pos.y);

    // 2. Chassis with Treads
    rlPushMatrix();
    rlRotatef(-chassisAngle * RAD2DEG, 0.0f, 1.0f, 0.0f);

    // Left tread
    DrawCube({ 0.0f, 0.28f, 0.60f }, 1.6f, 0.44f, 0.38f, treadCol);
    DrawCubeWires({ 0.0f, 0.28f, 0.60f }, 1.6f, 0.44f, 0.38f, { 20, 20, 20, treadCol.a });

    // Right tread
    DrawCube({ 0.0f, 0.28f, -0.60f }, 1.6f, 0.44f, 0.38f, treadCol);
    DrawCubeWires({ 0.0f, 0.28f, -0.60f }, 1.6f, 0.44f, 0.38f, { 20, 20, 20, treadCol.a });

    // Central chassis body
    DrawCube({ 0.0f, 0.36f, 0.0f }, 1.45f, 0.48f, 0.95f, bodyCol);
    DrawCubeWires({ 0.0f, 0.36f, 0.0f }, 1.45f, 0.48f, 0.95f, { 255, 255, 255, static_cast<unsigned char>(70 * alpha) });

    rlPopMatrix(); // End chassis

    // 3. Turret
    rlPushMatrix();
    rlRotatef(-turretAngle * RAD2DEG, 0.0f, 1.0f, 0.0f);

    // Recoil shift
    rlTranslatef(-recoil, 0.0f, 0.0f);

    // Turret dome
    DrawCube({ 0.0f, 0.66f, 0.0f }, 0.82f, 0.38f, 0.82f, turretCol);
    DrawCubeWires({ 0.0f, 0.66f, 0.0f }, 0.82f, 0.38f, 0.82f, { 255, 255, 255, static_cast<unsigned char>(90 * alpha) });

    // Cannon barrel
    DrawCube({ 0.75f, 0.66f, 0.0f }, 0.95f, 0.20f, 0.20f, { 50, 50, 55, turretCol.a });
    // Muzzle ring
    DrawCube({ 1.25f, 0.66f, 0.0f }, 0.16f, 0.26f, 0.26f, { 30, 30, 35, turretCol.a });

    rlPopMatrix(); // End turret

    rlPopMatrix(); // End tank transform

    // 4. Player 1 Aiming Laser & Reticle (Matching Image 0)
    if (tank.isHuman && tank.GetId() == 0) {
        Vector2 tip = tank.GetBarrelTip();
        Vector2 aim = tank.aimTarget;
        Vector2 dir = { aim.x - tip.x, aim.y - tip.y };
        float dist = Vector2Length(dir);

        if (dist > 0.5f) {
            Vector2 norm = { dir.x / dist, dir.y / dist };
            int dots = std::min(12, int(dist / 1.2f));
            for (int d = 1; d <= dots; ++d) {
                float t = float(d) / float(dots + 1);
                Vector2 p = { tip.x + dir.x * t, tip.y + dir.y * t };
                DrawSphere({ p.x, 0.35f, p.y }, 0.12f, { 70, 160, 255, 200 });
            }
        }
    }
}

void Renderer3D::DrawBullets(const BulletManager& bullets) {
    for (const auto& b : bullets.GetBullets()) {
        if (!b.active) continue;

        // Motion trail
        for (size_t i = 1; i < b.trail.size(); ++i) {
            float alpha = float(i) / float(b.trail.size());
            Color trailCol = b.color;
            trailCol.a = static_cast<unsigned char>(200 * alpha);
            DrawSphere({ b.trail[i].x, 0.4f, b.trail[i].y }, 0.14f * alpha, trailCol);
        }

        Vector3 pos3D = { b.position.x, 0.45f, b.position.y };
        if (b.isRocket) {
            DrawCube(pos3D, 0.6f, 0.28f, 0.28f, { 255, 80, 40, 255 });
            DrawCubeWires(pos3D, 0.6f, 0.28f, 0.28f, { 255, 220, 100, 255 });
        } else {
            DrawSphere(pos3D, BULLET_RADIUS * 1.4f, { 255, 200, 60, 255 });
            DrawSphereWires(pos3D, BULLET_RADIUS * 1.42f, 6, 6, { 190, 130, 30, 255 });
        }
    }
}

void Renderer3D::DrawMines(const MineManager& mines) {
    for (const auto& m : mines.GetMines()) {
        if (!m.active || m.detonated) continue;

        Vector3 pos = { m.position.x, 0.12f, m.position.y };
        DrawCube(pos, 0.85f, 0.15f, 0.85f, { 45, 45, 50, 255 });
        DrawCubeWires(pos, 0.85f, 0.15f, 0.85f, { 25, 25, 30, 255 });

        Color ledColor = (m.flashTimer > 0.0f) ? Color{ 255, 30, 30, 255 } : Color{ 80, 20, 20, 255 };
        DrawSphere({ pos.x, 0.26f, pos.z }, 0.18f, ledColor);
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

    // 1. Player 1 Indicator Circle Bubble (Matching Image 0)
    for (const auto& t : gameState.GetTanks()) {
        if (t.isHuman && t.GetId() == 0 && t.IsAlive()) {
            Vector2 screenPos = GetWorldToScreen({ t.GetPosition().x, 1.6f, t.GetPosition().y }, m_camera);
            if (screenPos.x > 0 && screenPos.y > 0) {
                DrawCircleLines(static_cast<int>(screenPos.x), static_cast<int>(screenPos.y) - 25, 22, { 30, 140, 255, 255 });
                DrawCircle(static_cast<int>(screenPos.x), static_cast<int>(screenPos.y) - 25, 20, { 255, 255, 255, 240 });
                DrawText("P1", static_cast<int>(screenPos.x) - 10, static_cast<int>(screenPos.y) - 34, 18, { 30, 140, 255, 255 });
            }
            break;
        }
    }

    // 2. Mission Banner at Top
    std::string missionText = "MISSION " + std::to_string(gameState.GetCurrentMission());
    int textW = MeasureText(missionText.c_str(), 28);
    DrawRectangleRounded({ float(sw / 2 - textW / 2 - 20), 14, float(textW + 40), 42 }, 0.4f, 8, { 0, 0, 0, 170 });
    DrawText(missionText.c_str(), sw / 2 - textW / 2, 21, 28, { 255, 235, 170, 255 });

    // 3. Remaining Enemy Tanks Counter
    int enemiesLeft = gameState.CountLivingEnemies();
    DrawRectangleRounded({ float(sw - 230), float(sh - 65), 210, 48 }, 0.3f, 8, { 0, 0, 0, 170 });
    DrawText("ENEMIES:", sw - 215, sh - 58, 14, { 200, 200, 210, 255 });
    
    int dotX = sw - 140;
    for (const auto& t : gameState.GetTanks()) {
        if (!t.isHuman && t.IsAlive()) {
            DrawCircle(dotX, sh - 41, 7, t.GetConfig().bodyColor);
            DrawCircleLines(dotX, sh - 41, 7, WHITE);
            dotX += 18;
        }
    }

    // 4. Lives Counter
    for (const auto& t : gameState.GetTanks()) {
        if (t.isHuman && t.GetId() == 0) {
            DrawRectangleRounded({ 20, float(sh - 65), 180, 48 }, 0.3f, 8, { 0, 0, 0, 170 });
            DrawText("LIVES:", 34, sh - 58, 14, { 200, 200, 210, 255 });
            for (int i = 0; i < t.GetLives(); ++i) {
                DrawRectangle(85 + i * 22, sh - 51, 16, 16, t.GetConfig().bodyColor);
                DrawRectangleLines(85 + i * 22, sh - 51, 16, 16, WHITE);
            }
            break;
        }
    }

    // 5. Stage Overlays
    if (gameState.GetScreen() == GameScreen::StageIntro) {
        DrawRectangle(0, sh / 2 - 50, sw, 100, { 0, 0, 0, 200 });
        std::string introStr = "MISSION " + std::to_string(gameState.GetCurrentMission()) + " - START!";
        int introW = MeasureText(introStr.c_str(), 38);
        DrawText(introStr.c_str(), sw / 2 - introW / 2, sh / 2 - 19, 38, { 255, 225, 120, 255 });
    } else if (gameState.GetScreen() == GameScreen::Victory) {
        DrawRectangle(0, sh / 2 - 50, sw, 100, { 25, 130, 45, 210 });
        std::string vicStr = "MISSION CLEARED!";
        int vicW = MeasureText(vicStr.c_str(), 42);
        DrawText(vicStr.c_str(), sw / 2 - vicW / 2, sh / 2 - 21, 42, WHITE);
    } else if (gameState.GetScreen() == GameScreen::GameOver) {
        DrawRectangle(0, sh / 2 - 50, sw, 100, { 150, 25, 25, 210 });
        std::string goStr = "GAME OVER";
        int goW = MeasureText(goStr.c_str(), 42);
        DrawText(goStr.c_str(), sw / 2 - goW / 2, sh / 2 - 21, 42, WHITE);
    }

    // 6. Crosshair (Wii Remote Blue Reticle matching Image 0)
    Vector2 mPos = GetMousePosition();
    DrawCircleLines(static_cast<int>(mPos.x), static_cast<int>(mPos.y), 11, { 60, 160, 255, 255 });
    DrawCircle(static_cast<int>(mPos.x), static_cast<int>(mPos.y), 3, { 60, 160, 255, 255 });
    DrawLine(static_cast<int>(mPos.x) - 16, static_cast<int>(mPos.y), static_cast<int>(mPos.x) - 7, static_cast<int>(mPos.y), { 60, 160, 255, 255 });
    DrawLine(static_cast<int>(mPos.x) + 7, static_cast<int>(mPos.y), static_cast<int>(mPos.x) + 16, static_cast<int>(mPos.y), { 60, 160, 255, 255 });
    DrawLine(static_cast<int>(mPos.x), static_cast<int>(mPos.y) - 16, static_cast<int>(mPos.x), static_cast<int>(mPos.y) - 7, { 60, 160, 255, 255 });
    DrawLine(static_cast<int>(mPos.x), static_cast<int>(mPos.y) + 7, static_cast<int>(mPos.x), static_cast<int>(mPos.y) + 16, { 60, 160, 255, 255 });
}

void Renderer3D::DrawTitleScreen(GameState& gameState) {
    int sw = GetScreenWidth();
    int sh = GetScreenHeight();

    DrawRectangleGradientV(0, 0, sw, sh, { 24, 28, 36, 255 }, { 14, 16, 22, 255 });

    const char* title = "WII PLAY: TANKS! NATIVE";
    int tw = MeasureText(title, 50);
    DrawText(title, sw / 2 - tw / 2, sh / 4 - 30, 50, { 255, 220, 110, 255 });

    const char* subtitle = "Authentic PowerPC Source Port & Online Multiplayer";
    int stw = MeasureText(subtitle, 20);
    DrawText(subtitle, sw / 2 - stw / 2, sh / 4 + 32, 20, { 180, 190, 210, 255 });

    int btnW = 340;
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

    const char* footer = "CONTROLS: [WASD / ZQSD] Move | [MOUSE] Aim | [LEFT CLICK] Shoot | [RIGHT CLICK / SPACE] Mine | [C] Camera";
    int fw = MeasureText(footer, 14);
    DrawText(footer, sw / 2 - fw / 2, sh - 40, 14, { 140, 150, 170, 255 });
}

void Renderer3D::DrawMissionSelect(GameState& gameState) {
    int sw = GetScreenWidth();
    int sh = GetScreenHeight();

    DrawRectangleGradientV(0, 0, sw, sh, { 24, 28, 36, 255 }, { 14, 16, 22, 255 });
    DrawText("SELECT MISSION", sw / 2 - MeasureText("SELECT MISSION", 36) / 2, 40, 36, { 255, 220, 110, 255 });

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

    DrawRectangleGradientV(0, 0, sw, sh, { 24, 28, 36, 255 }, { 14, 16, 22, 255 });
    const char* lobbyTitle = "MULTIPLAYER LOBBY (ENet UDP)";
    DrawText(lobbyTitle, sw / 2 - MeasureText(lobbyTitle, 36) / 2, 40, 36, { 255, 220, 110, 255 });

    Vector2 mPos = GetMousePosition();

    Rectangle hostBtn = { float(sw / 2 - 220), 160.0f, 440.0f, 54.0f };
    bool hostHover = CheckCollisionPointRec(mPos, hostBtn);
    DrawRectangleRec(hostBtn, hostHover ? Color{ 50, 130, 80, 255 } : Color{ 30, 80, 50, 255 });
    DrawRectangleLinesEx(hostBtn, 2, hostHover ? Color{ 255, 220, 100, 255 } : Color{ 100, 200, 140, 255 });
    DrawText("HOST ONLINE SERVER (Port 7777)", sw / 2 - MeasureText("HOST ONLINE SERVER (Port 7777)", 18) / 2, 178, 18, WHITE);

    if (hostHover && IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && network) {
        network->StartServer(7777);
        gameState.StartMission(1, true);
    }

    Rectangle joinBtn = { float(sw / 2 - 220), 235.0f, 440.0f, 54.0f };
    bool joinHover = CheckCollisionPointRec(mPos, joinBtn);
    DrawRectangleRec(joinBtn, joinHover ? Color{ 60, 90, 150, 255 } : Color{ 35, 55, 95, 255 });
    DrawRectangleLinesEx(joinBtn, 2, joinHover ? Color{ 255, 220, 100, 255 } : Color{ 120, 160, 220, 255 });
    DrawText("JOIN LOCAL SERVER (127.0.0.1:7777)", sw / 2 - MeasureText("JOIN LOCAL SERVER (127.0.0.1:7777)", 18) / 2, 253, 18, WHITE);

    if (joinHover && IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && network) {
        network->ConnectToServer("127.0.0.1", 7777);
    }

    Rectangle backBtn = { 40, float(sh - 70), 140, 44 };
    bool backHover = CheckCollisionPointRec(mPos, backBtn);
    DrawRectangleRec(backBtn, backHover ? Color{ 140, 50, 50, 255 } : Color{ 80, 30, 30, 255 });
    DrawRectangleLinesEx(backBtn, 2, WHITE);
    DrawText("< BACK", 65, sh - 58, 18, WHITE);

    if (backHover && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
        gameState.SetScreen(GameScreen::Title);
    }
}
