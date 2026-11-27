#!/usr/bin/env bash
# The discovery code's version (the game-recomp commit and the discovery patch): a seed
# (converter/h4/discover-<sha256>.txt) written by another version is not used.
set -euo pipefail
ROOT="$(cd "$(dirname "$0")/../.." && pwd)"
COMMIT="$(git -C "$ROOT/third_party/game-recomp" rev-parse HEAD)"
PATCH="$(shasum -a 256 "$ROOT/converter/patches/game-recomp-discovery-seed.patch" | cut -c1-16)"
echo "${COMMIT:0:16}-$PATCH"
