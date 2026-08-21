// The AI must not pull the trigger on a shot that comes back into its own hull,
// and must pull it when the barrel really is on the target.
#include "AI.hpp"
#include "Tank.hpp"
#include "Level.hpp"
#include <cstdio>
#include <cmath>

int main() {
    Level lv;
    lv.Reset();
    AIManager ai;

    // Ash bounces once, which is what makes a head-on wall shot suicidal.
    Tank t(1, TankType::EnemyAsh, { 0.0f, 0.0f });
    if (t.GetConfig().maxBounces < 1) { printf("FAIL: Ash should bounce\n"); return 1; }
    t.SetTurretAngle(0.0f);   // facing +x

    int gx, gy;
    lv.WorldToGrid({ CELL_SIZE * 3.0f, 0.0f }, gx, gy);
    lv.SetTile(gx, gy, TileType::SolidBlock1);

    // Target off to the side: the traced shot reaches nobody but the shooter.
    Vector2 aside = { 0.0f, CELL_SIZE * 6.0f };
    if (ai.ShotIsClear(t, aside, lv)) { printf("FAIL: fired a shot that returns into itself\n"); return 1; }

    // Target sitting on the return path, just past the shooter. Without the
    // self test the trace would happily report this as a hit -- and the shell
    // would kill the shooter on the way there.
    Vector2 behind = { -CELL_SIZE * 4.0f, 0.0f };
    if (ai.ShotIsClear(t, behind, lv)) { printf("FAIL: shot through its own hull to reach the target\n"); return 1; }

    // Clear the wall and put the target straight down the barrel.
    lv.SetTile(gx, gy, TileType::Empty);
    Vector2 ahead = { CELL_SIZE * 5.0f, 0.0f };
    if (!ai.ShotIsClear(t, ahead, lv)) { printf("FAIL: refused a clean point-and-shoot\n"); return 1; }

    // A hole between them changes nothing: the shell flies over it.
    lv.SetTile(gx, gy, TileType::Hole);
    if (!ai.ShotIsClear(t, ahead, lv)) { printf("FAIL: a hole blocked the shot\n"); return 1; }

    // Turret still swinging: solution exists but the barrel is 90 deg off.
    lv.SetTile(gx, gy, TileType::Empty);
    t.SetTurretAngle(1.5707963f);
    if (ai.ShotIsClear(t, ahead, lv)) { printf("FAIL: fired while the barrel was elsewhere\n"); return 1; }

    printf("ok: shot gate rejects self-hits and off-barrel shots\n");
    return 0;
}
