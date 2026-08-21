// Pins the extracted mission table: every mission must load its map, resolve
// each slot to the type its row encodes, and place each enemy on its own
// spawn marker.  Build:
//   g++ -std=c++17 -Iinclude -Ibuild/_deps/raylib-src/src \
//       tools/test_missions.cpp src/Level.cpp -o /tmp/test_missions
#include "Level.hpp"
#include "../src/MissionTable.inc"
#include <cassert>
#include <cstdio>
#include <set>

int main() {
    Level level;
    int total = 0;

    // Mission 1: row { 0, 29, 29, {1,0,...} } -- one brown tank on map 29.
    MissionDef m1 = Level::GetMissionDef(1);
    assert(m1.mapIndex == 29 && m1.enemies.size() == 1);
    assert(m1.enemies[0].slot == 0 && m1.enemies[0].type == TankType::EnemyBrown);
    assert(!m1.bonusTank);

    // Word 0 is set on missions 5, 10, ... 95 and nowhere else.
    for (int m = 1; m <= 100; ++m)
        assert(Level::GetMissionDef(m).bonusTank == (m % 5 == 0 && m != 100));

    // Mission 9: row { 0, 3, 3, {2,5,5,2,2,2,0,0} }.  Record type 5 is Yellow,
    // which our enum orders after Red -- a swapped map would land on Red here.
    MissionDef m9 = Level::GetMissionDef(9);
    assert(m9.enemies[0].type == TankType::EnemyAsh);
    assert(m9.enemies[1].type == TankType::EnemyYellow);
    assert(m9.enemies[2].type == TankType::EnemyYellow);

    // Mission 100: row { 0, 29, 29, {7,9,9,8,1,9,9,1} }.
    MissionDef m100 = Level::GetMissionDef(100);
    assert(m100.mapIndex == 29 && m100.enemies.size() == 8);
    assert(m100.enemies[0].type == TankType::EnemyGreen);
    assert(m100.enemies[1].type == TankType::EnemyBlack);
    assert(m100.enemies[3].type == TankType::EnemyWhite);
    assert(m100.enemies[4].type == TankType::EnemyBrown);

    for (int m = 1; m <= 100; ++m) {
        const MissionRow& row = kMissionTable[m - 1];
        // Draw repeatedly: the ranged rows are random, so one pass proves little.
        for (int draw = 0; draw < 20; ++draw) {
            MissionDef def = Level::GetMissionDef(m);
            if (def.mapIndex < row.mapLo || def.mapIndex > row.mapHi) {
                printf("mission %d: map %d outside [%d,%d]\n", m, def.mapIndex,
                       row.mapLo, row.mapHi);
                return 1;
            }
            size_t k = 0;
            for (int i = 0; i < 8; ++i) {
                if (row.slot[i] == 0) continue;
                if (k >= def.enemies.size() || def.enemies[k].slot != i) {
                    printf("mission %d: slot %d missing\n", m, i);
                    return 1;
                }
                int v = row.slot[i];
                int lo = v < 10 ? v : v / 10, hi = v < 10 ? v : v % 10;
                bool inRange = false;
                for (int t = lo; t <= hi; ++t)
                    if (def.enemies[k].type == kRecordType[t]) inRange = true;
                if (!inRange) {
                    printf("mission %d slot %d: type outside [%d,%d]\n", m, i, lo, hi);
                    return 1;
                }
                ++k;
            }
            if (k != def.enemies.size()) {
                printf("mission %d: %zu enemies for %zu slots\n", m,
                       def.enemies.size(), k);
                return 1;
            }
        }

        if (!level.LoadMission(m)) {
            printf("mission %d failed to load\n", m);
            return 1;
        }
        const MissionDef& def = level.GetCurrentMission();
        const std::vector<EnemySpawn>& spawns = level.GetEnemySpawns();
        if (spawns.size() != def.enemies.size()) {
            printf("mission %d: %zu spawns for %zu enemies\n", m, spawns.size(),
                   def.enemies.size());
            return 1;
        }
        // Every map carries all eight markers, so no two enemies may share one.
        std::set<std::pair<int, int>> cells;
        for (size_t i = 0; i < spawns.size(); ++i) {
            if (spawns[i].type != def.enemies[i].type) {
                printf("mission %d: spawn %zu has the wrong type\n", m, i);
                return 1;
            }
            if (!cells.insert({ spawns[i].gridX, spawns[i].gridY }).second) {
                printf("mission %d: two enemies share a spawn cell\n", m);
                return 1;
            }
        }
        total += (int)def.enemies.size();
    }
    printf("ok: 100 missions, %d enemies placed\n", total);
    return 0;
}
