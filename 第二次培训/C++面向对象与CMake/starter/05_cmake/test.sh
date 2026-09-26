#!/usr/bin/env bash
set -u

ROOT="$(cd "$(dirname "$0")" && pwd)"
BUILD="$(mktemp -d /tmp/cmake05-build.XXXXXX)"
BIN="$(mktemp /tmp/cmake05-test.XXXXXX)"
trap 'rm -rf "$BUILD"; rm -f "$BIN"' EXIT

echo "== CMake configure =="
if ! cmake -S "$ROOT" -B "$BUILD"; then
    echo "[FAIL] CMake configure"
    exit 1
fi
echo "[PASS] CMake configure"

echo
echo "== CMake targets =="
if ! cmake --build "$BUILD" --target robot_core; then
    echo "[FAIL] robot_core target"
    exit 1
fi
echo "[PASS] robot_core target"

if ! cmake --build "$BUILD" --target robot_demo; then
    echo "[FAIL] robot_demo target"
    exit 1
fi
echo "[PASS] robot_demo target"

EXPECTED='=== Mini Robot Demo ===
Robot initialization...
DM Motor 1 enabled.
Unitree Motor 2 enabled.
Set robot target position: 1.50 rad
DM Motor 1 -> target = 1.50 rad
Unitree Motor 2 -> target = 1.50 rad
Left motor position: 1.50 rad
Right motor position: 1.50 rad'

ACTUAL="$("$BUILD/robot_demo")"

echo
echo "== Demo output =="
if [[ "$ACTUAL" == "$EXPECTED" ]]; then
    echo "[PASS] robot_demo output"
else
    echo "[FAIL] robot_demo output"
    echo "--- expected ---"
    printf '%s\n' "$EXPECTED"
    echo "--- actual ---"
    printf '%s\n' "$ACTUAL"
    exit 1
fi

echo
echo "== Behavior tests =="
if ! g++ "$ROOT/tests/test_robot.cpp" \
    "$ROOT/src/dm_motor.cpp" \
    "$ROOT/src/unitree_motor.cpp" \
    "$ROOT/src/robot.cpp" \
    -I"$ROOT/include" -std=c++17 -Wall -Wextra -pedantic -o "$BIN"; then
    echo "[FAIL] behavior test compile"
    exit 1
fi

"$BIN"
