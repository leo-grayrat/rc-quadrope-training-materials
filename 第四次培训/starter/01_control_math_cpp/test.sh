#!/usr/bin/env bash
set -euo pipefail

bin="$(mktemp)"
trap 'rm -f "$bin"' EXIT

g++ -std=c++17 -Wall -Wextra -pedantic \
  control_math.cpp tests.cpp \
  -o "$bin"

"$bin"
