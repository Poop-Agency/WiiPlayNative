// Guards the six AI cadence fields wired from TnkGameParam.bin. If a column ever
// slides, or an initialiser loses a field, this fails instead of the tanks quietly
// changing behaviour. Values and their VMAs: docs/tnkgameparam.md.
#include "../include/Common.hpp"
#include <cassert>
#include <cstdio>

int main() {
    struct { TankType t; float spread; int reaim, fmin, fmax, mmin, mmax;
             float range, farPct, nearPct; } want[] = {
        { TankType::EnemyBrown,  170.0f, 60, 30, 45,  0,  0, 100.0f,  0.0f,  0.0f },
        { TankType::EnemyAsh,     40.0f, 45, 30, 45,  0,  0, 100.0f,  0.0f,  0.0f },
        { TankType::EnemyTeal,     0.0f,  8,  5, 10,  0,  0, 100.0f,  0.0f,  0.0f },
        { TankType::EnemyYellow,  40.0f, 30, 30, 45, 40, 60, 100.0f, 50.0f, 50.0f },
        { TankType::EnemyRed,     40.0f, 20,  5, 10,  0,  0, 100.0f,  0.0f,  0.0f },
        { TankType::EnemyGreen,   80.0f, 30,  5, 10,  0,  0, 100.0f,  0.0f,  0.0f },
        { TankType::EnemyPurple,  40.0f, 20,  5, 10, 40, 60, 100.0f,  3.0f,  5.0f },
        { TankType::EnemyWhite,   40.0f, 30,  5, 10, 40, 60, 100.0f,  3.0f,  5.0f },
        { TankType::EnemyBlack,    5.0f, 20,  5, 10, 40, 60, 100.0f,  3.0f,  5.0f },
    };
    for (auto& w : want) {
        TankConfig c = GetTankConfig(w.t);
        assert(c.aimSpread == w.spread);
        assert(c.reaimFrames == w.reaim);
        assert(c.fireDecisionMin == w.fmin && c.fireDecisionMax == w.fmax);
        assert(c.mineDecisionMin == w.mmin && c.mineDecisionMax == w.mmax);
        // Every tank must be able to decide: a zero span would divide by zero.
        assert(c.fireDecisionMax > c.fireDecisionMin);
        // The mine timer is armed for exactly the tanks that carry mines.
        assert((c.mineDecisionMax > 0) == (c.maxMines > 0));
        assert(c.mineRangePx == w.range);
        assert(c.mineChanceFar == w.farPct && c.mineChanceNear == w.nearPct);
        // A drop chance is armed for exactly the tanks that carry mines.
        assert((c.mineChanceFar > 0.0f) == (c.maxMines > 0));
    }
    printf("AI cadence fields OK (%zu tanks)\n", sizeof(want) / sizeof(want[0]));
    return 0;
}
