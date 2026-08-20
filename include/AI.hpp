#pragma once

#include "Common.hpp"
#include <vector>

class Tank;
class Level;
class BulletManager;
class MineManager;

struct AIState {
    float moveTimer;
    Vector2 moveTarget;
    float shootTimer;
    float mineTimer;
    float burstTimer;
    int burstCount;
    float dodgeTimer;
    float aimTimer;   // counts down the re-aim beat, field 39 -> A+0x24
    float aimError;   // radians, held between re-aims, drawn from field 28
};

class AIManager {
public:
    AIManager();
    ~AIManager();

    void Reset();
    void Update(float dt, std::vector<Tank>& tanks, Level& level, 
                const BulletManager& bullets, MineManager& mines);

private:
    void UpdateEnemy(Tank& enemy, AIState& state, float dt, 
                     const std::vector<Tank>& tanks, Level& level, 
                     const BulletManager& bullets, MineManager& mines);

    bool FindDirectShot(const Tank& enemy, Vector2 targetPos, const Level& level, Vector2& outAimPos);
    bool FindBankShot(const Tank& enemy, Vector2 targetPos, const Level& level, int maxBounces, Vector2& outAimPos);
    Vector2 FindDodgeVector(const Tank& enemy, const BulletManager& bullets);

    std::vector<AIState> m_states;
};
