#!/bin/bash
# The random map generator's regression check (check.cpp) on this computer, with the player's own
# game: tests/random_map/run.sh <game dir> [--save]  (--save writes the hashes now to classic.txt).
set -euo pipefail
HERE="$(cd "$(dirname "$0")" && pwd)"
ROOT="$(cd "$HERE/../.." && pwd)"
GAME="${1:?the game folder (with Data/ and heroes4.exe)}"
OUT="${TMPDIR:-/tmp}/rehomm4-random-map-check"
c++ -std=c++20 -O2 -ffp-contract=off -I"$ROOT/h4/src" \
  "$HERE/check.cpp" "$ROOT"/h4/src/random_map/*.cpp -lz -o "$OUT"
if [[ "${2:-}" == "--save" ]]; then
  "$OUT" "$GAME" > "$HERE/classic.txt"
else
  "$OUT" "$GAME" "$HERE/classic.txt"
fi
