#!/usr/bin/env bash
set -u

ROOT="$(cd "$(dirname "$0")" && pwd)"
BIN="$(mktemp /tmp/motor02-test.XXXXXX)"
trap 'rm -f "$BIN"' EXIT

if ! g++ "$ROOT/tests/test.cpp" -std=c++17 -Wall -Wextra -pedantic -o "$BIN"; then
    echo "[FAIL] compile"
    exit 1
fi

echo "[PASS] compile"
"$BIN"
