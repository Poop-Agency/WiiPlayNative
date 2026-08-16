# Current AI Behaviour Specification

This document provides an exhaustive and precise specification of the enemy AI as currently implemented in the codebase.

## 1. Persistent Per-Tank AI State

The AI state is maintained between frames via the `AIState` struct (`include/AI.hpp:11-19`) and properties on the `Tank` class (`include/Tank.hpp`).
The state is stored in a vector inside `AIManager` (`include/AI.hpp:39`) and is zero-initialised when the vector is resized (`src/AI.cpp:128`).

### AIState Struct (`include/AI.hpp:11`)
* `moveTimer` (float, `include/AI.hpp:12`): Controls duration before picking a new wander direction. Mutated at `src/AI.cpp:217`, `225`, `245`, `263`, `274`.
* `moveTarget` (Vector2, `include/AI.hpp:13`): Stores the chosen wander direction. Mutated at `src/AI.cpp:273`.
* `shootTimer` (float, `include/AI.hpp:14`): Cooldown until next shot. Mutated at `src/AI.cpp:173`, `206`, `209`, `212`.
* `mineTimer` (float, `include/AI.hpp:15`): Cooldown until next mine can be dropped. Mutated at `src/AI.cpp:239`, `249`.
* `burstTimer` (float, `include/AI.hpp:16`): Declared but **Not found** in usage (never mutated or read).
* `burstCount` (int, `include/AI.hpp:17`): Tracks shots fired in a burst sequence. Mutated at `src/AI.cpp:204`, `208`.
* `dodgeTimer` (float, `include/AI.hpp:18`): Declared but **Not found** in usage (never mutated or read).

### Tank Class Output State
The AI mutates the following `Tank` state variables to execute actions:
* `aimTarget` (Vector2, `include/Tank.hpp:52`): The world coordinate the turret rotates toward. Mutated at `src/AI.cpp:181`, `186`, `189`, `192`.
* `moveInput` (Vector2, `include/Tank.hpp:51`): Normalized direction vector for chassis movement. Mutated at `src/AI.cpp:156`, `283`, `285`.
* `shootRequested` (bool, `include/Tank.hpp:53`): Trigger to fire a bullet. Mutated at `src/AI.cpp:174`, `196`.
* `mineRequested` (bool, `include/Tank.hpp:54`): Trigger to drop a mine. Mutated at `src/AI.cpp:175`, `248`.

## 2. Per-Frame Decision Flow

The following pseudocode details the exact execution order of `AIManager::UpdateEnemy` (`src/AI.cpp:137-287`), which runs every frame for every living enemy.

```text
Find the closest living player tank (src/AI.cpp:141-154).
If no living player is found:
    Set moveInput = {0, 0} and return early (src/AI.cpp:155-158).

Calculate predicted target position (src/AI.cpp:164-170):
    If tank isRocket OR EnemyBlack OR EnemyTeal:
        target = player position + (player moveInput * 1.5)
    Else:
        target = player position

Decrement shootTimer (src/AI.cpp:173).
Reset shootRequested = false, mineRequested = false (src/AI.cpp:174-175).
Initialize canShoot = false (src/AI.cpp:178).

Evaluate aiming (src/AI.cpp:180-193):
    If FindDirectShot finds clear Line-of-Sight:
        aimTarget = direct shot aim position
        canShoot = true
    Else if maxBounces > 0 AND FindBankShot finds a valid ricochet:
        aimTarget = bank shot aim position
        canShoot = true
    Else:
        aimTarget = predicted target position

Evaluate firing (src/AI.cpp:195-214):
    If canShoot AND shootTimer <= 0.0:
        shootRequested = true
        If EnemyGreen:
            Increment burstCount
            If burstCount < 2:
                shootTimer = 0.15
            Else:
                burstCount = 0
                shootTimer = config shootCooldown
        Else:
            shootTimer = config shootCooldown * random jitter between 0.9 and 1.09

Decrement moveTimer (src/AI.cpp:217).
Initialize moveDir = {0, 0} (src/AI.cpp:218).

Evaluate dodging (src/AI.cpp:221-227):
    If EnemyBlack OR EnemyTeal OR EnemyWhite:
        dodgeVector = FindDodgeVector()
        If length(dodgeVector) > 0.1:
            moveDir = dodgeVector
            moveTimer = 0.4

Evaluate movement if not dodging (moveDir length < 0.1) (src/AI.cpp:229-279):
    Switch on Tank Type:
        Case EnemyBrown, EnemyGreen (src/AI.cpp:231-235):
            moveDir = {0, 0}
        
        Case EnemyYellow (src/AI.cpp:237-251):
            Decrement mineTimer
            If distance to player < 7.0:
                moveDir = direction directly away from player
            Else if moveTimer <= 0.0:
                moveDir = random angle vector
                moveTimer = random value between 2.0 and 3.98
            If mineTimer <= 0.0 AND distance to player < 5.0:
                mineRequested = true
                mineTimer = 3.5
                
        Case EnemyTeal, EnemyBlack, EnemyPurple, EnemyWhite (src/AI.cpp:253-265):
            If distance to player > 8.0:
                moveDir = direction directly towards player
            Else if moveTimer <= 0.0:
                moveDir = random angle vector
                moveTimer = random value between 1.5 and 3.15
                
        Case EnemyAsh, EnemyRed, Default (src/AI.cpp:267-277):
            If moveTimer <= 0.0:
                moveTarget = random angle vector
                moveTimer = random value between 2.5 and 4.975
            moveDir = moveTarget

Apply movement (src/AI.cpp:281-286):
    If length(moveDir) > 0.01:
        moveInput = moveDir normalized
    Else:
        moveInput = {0, 0}
```

## 3. Movement
* **Direction Generation**: Direction is determined via a state machine (`src/AI.cpp:230-279`). Wandering generates a completely random angle (`(rand() % 360) * DEG2RAD`). Approach and Flee generate vectors directly towards or away from the closest player. Dodging generates a perpendicular vector to an incoming bullet (`src/AI.cpp:104-105`).
* **Obstacle Handling**: The AI logic ignores level geometry entirely. There is no obstacle avoidance. It relies on the engine's physical collision resolution (`Tank::Update` calling `level.CheckTankCollision` in `src/Tank.cpp:121`) to push the tank out of walls.
* **Pathfinding**: **Not found**. Purely local reactive steering is used.
* **Re-evaluation Timing**:
  * Distance-based thresholds (Approach/Flee) are evaluated every single frame.
  * Bullet dodging is evaluated every single frame. If a dodge occurs, it forces `moveTimer` to `0.4f` (`src/AI.cpp:225`).
  * Wandering re-evaluates strictly based on `moveTimer`. When it reaches `<= 0.0f`, a new random angle and a new random timer duration are chosen.

## 4. Aiming
* **Turret Target Computation**: The computed `aimTarget` world position is written to the tank. The tank's physical update loop smoothly rotates `m_turretAngle` towards this target (`src/Tank.cpp:85-87`).
* **Target Leading**: Yes. If the tank has `isRocket`, or is `EnemyBlack`/`EnemyTeal`, it calculates a predicted target by adding the player's current `moveInput * 1.5f` to the player's position (`src/AI.cpp:165-170`).
* **Ricochet/Bounce Considered**: Yes. If no direct shot exists and the config allows `maxBounces > 0`, the AI calls `FindBankShot` (`src/AI.cpp:183`). This tests 48 radial angles and calculates ray bounces to see if a reflected line passes near the target (`src/AI.cpp:36-81`).
* **Line of Sight Testing**: Yes. `FindDirectShot` (`src/AI.cpp:28`) checks LOS using a grid-based raycast (`level.Raycast`). Segment-AABB is not used for this.

## 5. Firing
* **Shoot Condition**: The tank requests a shot if `canShoot` is true (either a direct or bank shot is valid) AND `shootTimer <= 0.0f` (`src/AI.cpp:195`).
* **Cooldown Handling**: Upon firing, `shootTimer` is reset to the base config cooldown multiplied by a random jitter factor between `0.9f` and `1.09f` (`src/AI.cpp:212`).
* **Ammo/Active-Shell Limits**: The AI itself ignores shell limits. It continually asserts `shootRequested`, and `Tank::Shoot` returns false and does nothing if the limit is reached (`src/Tank.cpp:143-144`).
* **Special Cases**: `EnemyGreen` uses a unique burst fire pattern (`src/AI.cpp:202-211`). It fires two shots with a fixed `0.15f` cooldown between them, and only resets to the full config cooldown after the second shot.

## 6. Mine Laying
* **When**: A mine is requested if `mineTimer <= 0.0f` and the closest player is `< 5.0f` distance away (`src/AI.cpp:247-250`).
* **Where**: The mine drops at the tank's exact current position (`Tank::PlantMine`, `src/Tank.cpp:179`).
* **Which Types**: Only `EnemyYellow` evaluates mine laying in the AI update loop (`src/AI.cpp:238`).

## 7. AI Constants Table

| Value | Controls | File:Line | Provenance Comment |
| :--- | :--- | :--- | :--- |
| `0.1f` | Min player distance to evaluate direct shot | `src/AI.cpp:22` | none |
| `48` | Number of radial angles to test for bank shots | `src/AI.cpp:38` | none |
| `2.0f * PI` | 360 degrees in radians (for angle distribution) | `src/AI.cpp:41` | none |
| `40.0f` | Maximum distance for bank shot raycast | `src/AI.cpp:49` | none |
| `0.1f` | Min length of a bounced ray segment to consider valid | `src/AI.cpp:55` | none |
| `1.6f` | Multiplier for `TANK_RADIUS` for bank shot clearance | `src/AI.cpp:62` | none |
| `5.0f` | Multiplier to project aim target out for bank shot | `src/AI.cpp:64` | none |
| `2.0f` | Reflection vector dot product multiplier | `src/AI.cpp:74` | none |
| `0.05f` | Normal offset added to ray position after a bounce | `src/AI.cpp:76` | none |
| `7.0f` | Max distance of an incoming bullet to consider dodging | `src/AI.cpp:92` | none |
| `0.1f` | Min distance of an incoming bullet to consider dodging | `src/AI.cpp:92` | none |
| `2.2f` | Multiplier for `TANK_RADIUS` for lateral dodge clearance | `src/AI.cpp:102` | none |
| `1e9f` | Initial search distance for finding closest player | `src/AI.cpp:143` | none |
| `1.5f` | Multiplier for target lead (player velocity prediction) | `src/AI.cpp:167-168`| none |
| `0.15f`| EnemyGreen burst shot fire delay | `src/AI.cpp:206` | none |
| `0.9f`, `100`, `500.0f`| Math to compute cooldown jitter (0.9 to 1.09) | `src/AI.cpp:212` | `// Cooldowns come from TnkGameParam.bin (col 37, frames at 60 Hz), with a small\n// jitter so a pack of identical tanks does not fire in lockstep.` |
| `0.1f` | Min dodge vector length to accept dodge | `src/AI.cpp:223` | none |
| `0.4f` | Move timer override after initiating a dodge | `src/AI.cpp:225` | none |
| `0.1f` | Min vector length to assume tank is dodging/moving | `src/AI.cpp:229` | none |
| `7.0f` | EnemyYellow distance threshold to begin fleeing | `src/AI.cpp:240` | none |
| `360` | Degrees to modulo for random wander angles | `src/AI.cpp:243, 261, 272`| none |
| `2.0f`, `100`, `50.0f` | EnemyYellow wander timer random range (2.0-3.98) | `src/AI.cpp:245` | none |
| `5.0f` | EnemyYellow max distance to player to plant mine | `src/AI.cpp:247` | none |
| `3.5f` | EnemyYellow mine cooldown timer | `src/AI.cpp:249` | none |
| `8.0f` | Elite tanks max distance threshold to approach | `src/AI.cpp:258` | none |
| `1.5f`, `100`, `60.0f` | Elite tanks wander timer random range (1.5-3.15) | `src/AI.cpp:263` | none |
| `2.5f`, `100`, `40.0f` | Default/Wanderer timer random range (2.5-4.975) | `src/AI.cpp:274` | none |
| `0.01f`| Min move vector length required to apply input | `src/AI.cpp:282` | none |
| `TANK_RADIUS` | Base clearance radius (`15.0f * PX`) | `include/Common.hpp:32` | `//   tank   15 px  Tank::collide  0x8025a3a4 (lfs 15.0, sdata2 0x8045a6e4)` |

## 8. Not from the original

The following constants, thresholds, formulas, and rules are entirely bespoke replacements. They have **no** comments citing a retail address, a data file, or a reverse-engineering source:

* The direct shot minimum validation distance (`0.1f`, `src/AI.cpp:22`).
* The bank shot ray count (`48`), raycast max distance (`40.0f`), and segment validation minimum length (`0.1f`) (`src/AI.cpp:38, 49, 55`).
* The bank shot lateral clearance multiplier (`1.6f * TANK_RADIUS`, `src/AI.cpp:62`).
* The bank shot aim target projection distance (`5.0f`, `src/AI.cpp:64`).
* The bank shot normal offset to prevent wall snagging (`0.05f`, `src/AI.cpp:76`).
* The dodge search distances: maximum (`7.0f`) and minimum (`0.1f`) (`src/AI.cpp:92`).
* The dodge lateral clearance multiplier (`2.2f * TANK_RADIUS`, `src/AI.cpp:102`).
* The target prediction lead multiplier (`1.5f * player moveInput`, `src/AI.cpp:167-168`).
* The EnemyGreen burst fire logic and its inter-shot delay timer (`0.15f`, `src/AI.cpp:202-211`).
* The shoot timer random jitter formula (`0.9f + (rand() % 100) / 500.0f`, `src/AI.cpp:212`).
* The dodge movement override timer lockout (`0.4f`, `src/AI.cpp:225`).
* The EnemyYellow flee distance threshold (`7.0f`, `src/AI.cpp:240`).
* The EnemyYellow mine planting distance threshold (`5.0f`) and hardcoded cooldown timer (`3.5f`) (`src/AI.cpp:247-249`).
* The EnemyYellow wander timer range logic (`2.0f + (rand() % 100) / 50.0f`, `src/AI.cpp:245`).
* The Elite tanks approach distance threshold (`8.0f`, `src/AI.cpp:258`).
* The Elite tanks wander timer range logic (`1.5f + (rand() % 100) / 60.0f`, `src/AI.cpp:263`).
* The default/wanderer timer range logic (`2.5f + (rand() % 100) / 40.0f`, `src/AI.cpp:274`).
* The random angular spread generation using `rand() % 360` (`src/AI.cpp:243, 261, 272`).
* The logic hardcoding `EnemyBlack` and `EnemyTeal` to lead shots instead of relying strictly on the `isRocket` attribute (`src/AI.cpp:165`).
* The hardcoded switch cases tying specific behaviours to enumerations rather than properties (e.g., locking mine laying strictly to `EnemyYellow` despite other tanks having `maxMines > 0`).

## 9. Per-Tank-Type Differentiation

The AI implements differentiation largely via a hardcoded switch block (`src/AI.cpp:230-279`) and a few specific if-statements.

### Implemented Differences
* **Stationary**: `EnemyBrown` and `EnemyGreen` simply receive `moveDir = {0,0}`.
* **Flee & Mine**: `EnemyYellow` specifically checks player distance to run away and is the *only* tank coded to plant mines.
* **Flank & Dodge**: `EnemyTeal`, `EnemyBlack`, and `EnemyWhite` will dodge bullets and try to close distance with the player. 
* **Flank without Dodging**: `EnemyPurple` shares the Elite movement properties (closing distance) but is explicitly omitted from the bullet dodging condition (`src/AI.cpp:221`).
* **Random Wandering**: `EnemyAsh` and `EnemyRed` (and any unspecified defaults) solely wander in random directions with longer timers.
* **Aim Leading**: Target leading is restricted to tanks with `isRocket == true`, `EnemyBlack`, or `EnemyTeal`.
* **Burst Fire**: Restricted entirely to `EnemyGreen`.

### Conspicuously Absent Differences
* **Mine Laying**: According to `include/Common.hpp:169-180`, `EnemyPurple`, `EnemyWhite`, and `EnemyBlack` all have `maxMines > 0`. However, the AI only evaluates mine planting in the `EnemyYellow` case (`src/AI.cpp:238`). Consequently, Purple, White, and Black never plant mines.
* **Dodging**: `EnemyPurple` is categorised as an elite flanker for movement but does not check for bullet dodging (`src/AI.cpp:221`).
* **Shared Logic Groups**: Ash and Red use identical behaviour trees. Teal, Black, and White use identical behaviour trees. Brown and Green use identical movement trees. Differentiation within these groups is driven only by `TankConfig` speeds and cooldowns, not AI logic.
* **Ammo Awareness**: AI firing requests do not account for active bullets in the air. The AI continuously asks to fire, completely oblivious to `maxBullets`, relying on `Tank::Shoot` to throttle the action.
* **Unused State Variables**: `AIState::burstTimer` and `AIState::dodgeTimer` are declared but never utilised by any tank type.
