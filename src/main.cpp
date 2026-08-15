#include "Engine.hpp"
#include <iostream>

int main(int argc, char* argv[]) {
    std::cout << "========================================" << std::endl;
    std::cout << "   Wii Play: Tanks! Native Source Port   " << std::endl;
    std::cout << "   Online Multiplayer & Authentic Maps   " << std::endl;
    std::cout << "========================================" << std::endl;

    Engine engine;
    if (!engine.Init(1280, 720, "Wii Play: Tanks! Native (PPC Source Port)")) {
        std::cerr << "Failed to initialize game engine." << std::endl;
        return 1;
    }

    engine.Run();
    engine.Shutdown();

    return 0;
}
