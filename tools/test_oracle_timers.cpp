// Diffs the AI decision timers and the game's RNG against the real game.
//
// tools/fixtures/oracle_timers.txt is written by scratch/oracle, which runs Wii
// Play's AI tick (0x8026BB3C, PAL RHAP01 rev 1) natively on a Dolphin RAM dump.
// Each line holds, before the tick, the RNG state, the three counters, the two
// guards and the timer bounds, then the RNG state and counters after it.
//
// Within one tick the draws come in this order: the re-aim callee on [A+0x10C]
// (one draw for this tank), the reload of [A+0x110], the reload of [A+0x118].
// The fixture tank carries no mines, so the mine callee's own roll is not covered.
#include "AI.hpp"
#include <cstdio>
#include <fstream>
#include <sstream>
#include <string>

int main() {
    std::ifstream in("tools/fixtures/oracle_timers.txt");
    if (!in) { printf("missing tools/fixtures/oracle_timers.txt\n"); return 1; }

    int frames = 0, reaims = 0, fires = 0, mines = 0;
    std::string line;
    while (std::getline(in, line)) {
        if (line.empty() || line[0] == '#') continue;
        std::istringstream row(line);
        long f, lcg, lfsr, c10C, c110, c118, g70, g74, a24, a28, a2C, a54, a58;
        long plcg, plfsr, p10C, p110, p118;
        std::string arrow;
        row >> f >> lcg >> lfsr >> c10C >> c110 >> c118 >> g70 >> g74
            >> a24 >> a28 >> a2C >> a54 >> a58 >> arrow >> plcg >> plfsr >> p10C >> p110 >> p118;

        GameRng rng;
        rng.lcg = static_cast<uint32_t>(lcg);
        rng.lfsr = static_cast<uint32_t>(lfsr);

        long want10C = c10C - 1;
        if (want10C <= 0) { want10C = a24; rng.Step(); ++reaims; }
        long want110 = c110 - 1;
        if (want110 <= 0) { want110 = RollDecisionFrames(a28, a2C, rng); ++fires; }
        long want118 = c118 - 1;
        if (want118 <= 0) { want118 = RollDecisionFrames(a54, a58, rng); ++mines; }

        if (p10C != want10C || p110 != want110 || p118 != want118) {
            printf("frame %ld: counters %ld %ld %ld, game %ld %ld %ld\n",
                   f, want10C, want110, want118, p10C, p110, p118);
            return 1;
        }
        if (rng.lcg != static_cast<uint32_t>(plcg) || rng.lfsr != static_cast<uint32_t>(plfsr)) {
            printf("frame %ld: RNG %u %u, game %ld %ld\n", f, rng.lcg, rng.lfsr, plcg, plfsr);
            return 1;
        }
        ++frames;
    }
    if (frames == 0) { printf("fixture has no frames\n"); return 1; }
    printf("timers and RNG match the game on %d frames (%d re-aims, %d fire and %d mine reloads)\n",
           frames, reaims, fires, mines);
    return 0;
}
