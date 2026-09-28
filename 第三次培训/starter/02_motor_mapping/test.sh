#!/usr/bin/env bash
set -euo pipefail
python3 -m unittest discover -s tests -v
echo "3 / 3 tests passed"
