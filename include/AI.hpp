#pragma once

#include "Common.hpp"
#include <vector>

class Tank;
class Level;
class BulletManager;
class MineManager;

// The game's one random generator: an LCG and a 16-bit LFSR stepped together
// (0x8026bcb8..0x8026bcf8). tools/test_oracle_timers.cpp checks it draw for draw
// against the game's own tick.
struct GameRng {
    // ponytail: arbitrary seed; the game's seeding (0x801b6b30) is not read yet.
    uint32_t lcg = 0x5C0B5A1Du;
    uint32_t lfsr = 0x94D5u;

    uint32_t Step() {
        lcg = lcg * 0x41C64E6Du + 12345u;
        if (lfsr & 1u) lfsr ^= 0x00011020u;
        lfsr >>= 1;
        return lcg ^ lfsr;
    }
};

// Reload of the decision timers [A+0x110] and [A+0x118], in frames: min + draw %
// (max - min), where draw is (x >> 4) & 0xFFFF (rlwinm 3,0,28,16,31 at 0x8026bcfc).
// A zero span still draws, and since divw's quotient is multiplied back by that
// zero span, the raw draw survives: a tank without mines waits up to 65535 frames.
inline int RollDecisionFrames(int min, int max, GameRng& rng) {
    const int draw = static_cast<int>((rng.Step() >> 4) & 0xFFFFu);
    const int span = max - min;
    return min + (span != 0 ? draw % span : draw);
}

struct AIState {
    float moveTimer;
    Vector2 moveTarget;
    float shootTimer;   // Green's burst spacing only; the decision is fireFrames
    int fireFrames;     // [A+0x110]
    int mineFrames;     // [A+0x118]
    float burstTimer;
    int burstCount;
    float dodgeTimer;
    float aimTimer;   // counts down the re-aim beat, field 39 -> A+0x24
    float aimError;   // radians, held between re-aims, drawn from field 28
    Vector2 heldAim;  // the aim decided on the last beat, kept until the next
    bool hasAim;
};

class AIManager {
public:
    AIManager();
    ~AIManager();

    void Reset();
    void Update(float dt, std::vector<Tank>& tanks, Level& level, 
                const BulletManager& bullets, MineManager& mines);

    // Trace the shot the barrel would actually fire right now, bounces and all.
    // Public so tools/test_shotgate.cpp can drive it directly.
    bool ShotIsClear(const Tank& enemy, Vector2 targetPos, const Level& level);

    // Public for tools/test_dodge.cpp.
    Vector2 FindDodgeVector(const Tank& enemy, const BulletManager& bullets);

private:
    void UpdateEnemy(Tank& enemy, AIState& state, float dt, int frames,
                     const std::vector<Tank>& tanks, Level& level, 
                     const BulletManager& bullets, MineManager& mines);

    bool FindDirectShot(const Tank& enemy, Vector2 targetPos, const Level& level, Vector2& outAimPos);
    bool FindBankShot(const Tank& enemy, Vector2 targetPos, const Level& level, int maxBounces, Vector2& outAimPos);
    std::vector<AIState> m_states;
    GameRng m_rng;
    float m_frameCarry = 0.0f;  // the original counts 60 Hz frames; dt is not fixed
};
