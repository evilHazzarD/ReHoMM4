#!/usr/bin/env bash
# Seeded discovery (converter/patches/game-recomp-discovery-seed.patch) in the vendored
# game-recomp, as configure_dynamic_core.sh applies its patch: not committed to the submodule.
set -euo pipefail
ROOT="$(cd "$(dirname "$0")/../.." && pwd)"
CORE="$ROOT/third_party/game-recomp"
PATCH="$ROOT/converter/patches/game-recomp-discovery-seed.patch"
if git -C "$CORE" apply --reverse --check "$PATCH" >/dev/null 2>&1; then
  exit 0
fi
git -C "$CORE" apply --check "$PATCH"
git -C "$CORE" apply "$PATCH"
