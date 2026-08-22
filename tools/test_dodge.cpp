// Who dodges, and how far they look, comes from record field 18 (shellTrackPx).
#include "AI.hpp"
#include "Tank.hpp"
#include "Bullet.hpp"
#include <cstdio>


static bool Dodges(TankType t, float shellDistWorld, float dirX) {
    AIManager ai;
    BulletManager bullets;
    Tank tank(1, t, { 0.0f, 0.0f });
    bullets.SpawnBullet(2, { shellDistWorld, 0.0f }, { dirX, 0.0f },
                        BULLET_SPEED_NORMAL, 1, false, WHITE);
    return Vector2Length(ai.FindDodgeVector(tank, bullets)) > 0.1f;
}

int main() {
    // Field 18: Brown 0, Ash 40 px, Black 100 px. A pixel is CELL_SIZE/32.
    if (GetTankConfig(TankType::EnemyBrown).shellTrackPx != 0.0f
        || GetTankConfig(TankType::EnemyAsh).shellTrackPx != 40.0f
        || GetTankConfig(TankType::EnemyBlack).shellTrackPx != 100.0f) {
        printf("FAIL: shellTrackPx does not match record field 18\n"); return 1;
    }

    // Brown never scans, whatever is coming at it.
    if (Dodges(TankType::EnemyBrown, 20.0f * PX, -1.0f)) { printf("FAIL: Brown dodged\n"); return 1; }

    // Black looks out to 100 px, Ash only to 40. A shell 60 px away separates them.
    if (!Dodges(TankType::EnemyBlack, 60.0f * PX, -1.0f)) { printf("FAIL: Black missed a shell at 60 px\n"); return 1; }
    if (Dodges(TankType::EnemyAsh, 60.0f * PX, -1.0f)) { printf("FAIL: Ash dodged past its 40 px range\n"); return 1; }
    if (!Dodges(TankType::EnemyAsh, 20.0f * PX, -1.0f)) { printf("FAIL: Ash missed a shell at 20 px\n"); return 1; }

    // A shell already moving away is not tracked (the dot product gate).
    if (Dodges(TankType::EnemyBlack, 60.0f * PX, 1.0f)) { printf("FAIL: dodged a shell going the other way\n"); return 1; }

    printf("ok: dodge range follows field 18, receding shells ignored\n");
    return 0;
}
