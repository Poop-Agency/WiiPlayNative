#pragma once

#include "Common.hpp"
#include "GameState.hpp"
#include "Renderer3D.hpp"
#include "Network.hpp"

class Engine {
public:
    Engine();
    ~Engine();

    bool Init(int width = 1280, int height = 720, const char* title = "Wii Play: Tanks! Native");
    void Run();
    void Shutdown();

private:
    void ProcessInput();
    void Update(float dt);
    void Render();

    int m_width;
    int m_height;
    bool m_running;

    GameState m_gameState;
    Renderer3D m_renderer;
    NetworkManager m_network;
};
