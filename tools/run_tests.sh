#!/bin/sh
# Build and run every check under tools/. Needs the raylib that build/ fetched.
set -e
cd "$(dirname "$0")/.."
INC="-Iinclude -Ibuild/_deps/raylib-src/src"
LIB="-Lbuild/_deps/raylib-build/raylib -lraylib -lm -lpthread -ldl"
OUT=${TMPDIR:-/tmp}/wiitanks-tests
mkdir -p "$OUT"

run() {
    name=$1; shift
    g++ -std=c++17 $INC "tools/$name.cpp" "$@" -o "$OUT/$name"
    "$OUT/$name" >"$OUT/$name.log" 2>&1 || { echo "FAIL $name"; cat "$OUT/$name.log"; exit 1; }
    tail -1 "$OUT/$name.log"
}

run test_missions src/Level.cpp
run test_holes    src/Level.cpp
run test_shotgate src/AI.cpp src/Tank.cpp src/Level.cpp src/Particle.cpp src/Bullet.cpp src/Mine.cpp $LIB
run test_turret   src/Tank.cpp src/Level.cpp src/Particle.cpp src/Bullet.cpp src/Mine.cpp $LIB
run test_oracle_turret src/Tank.cpp src/Level.cpp src/Particle.cpp src/Bullet.cpp src/Mine.cpp $LIB
run test_oracle_timers
run test_dodge     src/AI.cpp src/Tank.cpp src/Level.cpp src/Particle.cpp src/Bullet.cpp src/Mine.cpp $LIB
run test_aicadence src/AI.cpp src/Tank.cpp src/Level.cpp src/Particle.cpp src/Bullet.cpp src/Mine.cpp $LIB
