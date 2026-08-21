// The barrel may not swing faster than one turretSlewTan cone per 60 Hz frame.
// Build:
//   g++ -std=c++17 -Iinclude -Ibuild/_deps/raylib-src/src tools/test_turret.cpp \
//       src/Tank.cpp src/Level.cpp src/Particle.cpp -o /tmp/test_turret \
//       -Lbuild/_deps/raylib-build/raylib -lraylib -lm -lpthread -ldl
#include "Tank.hpp"
#include "Level.hpp"
#include "Particle.hpp"
#include <cassert>
#include <cmath>
#include <cstdio>

static float Wrap(float a) {
    while (a > PI) a -= 2.0f * PI;
    while (a < -PI) a += 2.0f * PI;
    return a;
}

int main() {
    Level level;
    if (!level.LoadMission(1)) { printf("no map\n"); return 1; }
    ParticleManager particles;
    const float dt = 1.0f / 60.0f;

    struct Case { TankType type; float tan_; const char* name; };
    const Case cases[] = {
        { TankType::EnemyBrown, 0.01f, "Brown" },
        { TankType::EnemyBlack, 0.03f, "Black" },
        { TankType::Player1,    0.05f, "Player 1" },
    };

    for (const Case& c : cases) {
        Tank t(1, c.type, { 0.0f, 0.0f });
        assert(t.GetConfig().turretSlewTan == c.tan_);
        const float step = std::atan(c.tan_); // radians per frame

        // Ask for a half turn: the barrel must take ceil(pi / step) frames.
        t.SetTurretAngle(0.0f);
        t.aimTarget = { -100.0f, 0.0f };
        float prev = 0.0f;
        int frames = 0;
        for (; frames < 4000; ++frames) {
            t.Update(dt, level, particles);
            float moved = std::fabs(Wrap(t.GetTurretAngle() - prev));
            if (moved > step * 1.001f) {
                printf("%s: swung %.5f rad in one frame, cap is %.5f\n",
                       c.name, moved, step);
                return 1;
            }
            prev = t.GetTurretAngle();
            if (std::fabs(Wrap(prev - PI)) < 1e-4f) break;
        }
        int expected = (int)std::ceil(PI / step);
        if (frames + 1 < expected || frames + 1 > expected + 1) {
            printf("%s: half turn took %d frames, expected %d\n", c.name,
                   frames + 1, expected);
            return 1;
        }
        printf("%s: half turn in %d frames (cap %.4f deg/frame)\n", c.name,
               frames + 1, step * 180.0f / PI);
    }
    printf("ok\n");
    return 0;
}
