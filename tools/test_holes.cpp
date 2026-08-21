// Tile 200 is a hole: it stops a tank and lets a shell fly over.
// Build: g++ -std=c++17 -Iinclude -Ibuild/_deps/raylib-src/src \
//        tools/test_holes.cpp src/Level.cpp -o /tmp/test_holes
#include "Level.hpp"
#include <cassert>
#include <cstdio>
#include <cstdlib>

int main() {
    Level lv;
    lv.Reset();

    const int gx = 11, gy = 8;
    lv.SetTile(gx, gy, TileType::Hole);
    assert(lv.IsHole(gx, gy));
    assert(lv.IsSolid(gx, gy) && "a hole still stops a tank");
    assert(!lv.IsHole(gx + 1, gy));

    Vector2 c = lv.GridToWorld(gx, gy);
    Vector2 hp, hn; int tx, ty;
    Vector2 from = { c.x - CELL_SIZE * 3.0f, c.y };
    Vector2 dir  = { 1.0f, 0.0f };

    // A shell (ignoreHoles = true) crosses the pit untouched.
    assert(!lv.Raycast(from, dir, CELL_SIZE * 5.0f, hp, hn, tx, ty, true)
           && "shell must fly over the hole");

    // The same ray as a ground query (ignoreHoles = false) stops on it.
    assert(lv.Raycast(from, dir, CELL_SIZE * 5.0f, hp, hn, tx, ty, false)
           && tx == gx && ty == gy && "ground query must stop on the hole");

    // A real block still stops the shell, so the flag did not disable the wall.
    lv.SetTile(gx, gy, TileType::SolidBlock1);
    assert(!lv.IsHole(gx, gy));
    assert(lv.Raycast(from, dir, CELL_SIZE * 5.0f, hp, hn, tx, ty, true)
           && tx == gx && ty == gy && "a block must still stop the shell");

    // A tank cannot stand in the pit.
    lv.SetTile(gx, gy, TileType::Hole);
    Vector2 push;
    assert(lv.CheckTankCollision(c, TANK_RADIUS, push) && "tank must not enter a hole");

    // Mission 4 draws map 27, and that map is nothing but holes.
    const MissionDef& d = Level::GetMissionDef(4);
    if (d.mapIndex != 27) { printf("FAIL: mission 4 maps to %d, want 27\n", d.mapIndex); return 1; }

    printf("ok: hole tile 200 blocks tanks, passes shells; mission 4 -> map 27\n");
    return 0;
}
