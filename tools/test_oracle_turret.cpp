// Diffs our turret slew against the real game, frame by frame.
//
// tools/fixtures/oracle_turret.txt is written by scratch/oracle, which runs Wii
// Play's own AI tick (0x8026BB3C, PAL RHAP01 rev 1) natively on a Dolphin RAM
// dump. Each line is one frame: the slew tangent A+0x20, the turret direction
// A+0x8C and aim direction A+0x80 before the tick, and A+0x8C after it. The
// game works in the XZ plane; mapping (x, z) to our (x, y) is a reflection at
// worst, which flips input and output alike, so the angles stay comparable.
#include "Tank.hpp"
#include "Level.hpp"
#include "Particle.hpp"
#include <cmath>
#include <cstdio>
#include <fstream>
#include <sstream>
#include <string>

static float Wrap(float a) {
    while (a > PI) a -= 2.0f * PI;
    while (a < -PI) a += 2.0f * PI;
    return a;
}

int main() {
    std::ifstream in("tools/fixtures/oracle_turret.txt");
    if (!in) { printf("missing tools/fixtures/oracle_turret.txt\n"); return 1; }
    Level level;
    if (!level.LoadMission(1)) { printf("no map\n"); return 1; }
    ParticleManager particles;

    int frames = 0, snapped = 0;
    float worst = 0.0f;
    std::string line;
    while (std::getline(in, line)) {
        if (line.empty() || line[0] == '#') continue;
        std::istringstream row(line);
        int frame;
        float s, tx, tz, ax, az, nx, nz;
        row >> frame >> s >> tx >> tz >> ax >> az >> nx >> nz;

        // Any enemy whose record carries this slew; Brown and Ash both use 0.01.
        TankType type = TankType::Player1;
        for (int k = (int)TankType::EnemyBrown; k <= (int)TankType::EnemyBlack; ++k) {
            if (GetTankConfig((TankType)k).turretSlewTan == s) { type = (TankType)k; break; }
        }
        if (GetTankConfig(type).turretSlewTan != s) {
            printf("frame %d: no tank type has slew %.9g\n", frame, s);
            return 1;
        }

        Tank t(1, type, { 0.0f, 0.0f });
        t.SetTurretAngle(std::atan2(tz, tx));
        t.aimTarget = { 100.0f * ax, 100.0f * az };
        t.Update(1.0f / 60.0f, level, particles);

        const float want = std::atan2(nz, nx);
        const float err = std::fabs(Wrap(t.GetTurretAngle() - want));
        if (std::fabs(Wrap(want - std::atan2(az, ax))) < 1e-6f) ++snapped;
        if (err > worst) worst = err;
        if (err > 1e-4f) {
            printf("frame %d: turret %.6f rad, game %.6f rad (off by %.6f)\n",
                   frame, t.GetTurretAngle(), want, err);
            return 1;
        }
        ++frames;
    }
    if (frames == 0) { printf("fixture has no frames\n"); return 1; }
    printf("turret matches the game on %d frames (%d on target), worst %.2e rad\n",
           frames, snapped, worst);
    return 0;
}
