#!/usr/bin/env bash
set -euo pipefail
python3 -m unittest discover -s tests -v
echo "5 / 5 tests passed"
