#!/usr/bin/env bash
# Makes the vendored dynamic converter read ReHoMM4's manifest directory.
# The patch is intentionally not committed into the git submodule: this script makes every CI
# checkout apply exactly the reviewed adaptation to the pinned upstream source.
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/../.." && pwd)"
CORE="$ROOT/third_party/game-recomp"
PATCH="$ROOT/converter/patches/game-recomp-manifest-dir.patch"

[[ -d "$CORE/.git" || -f "$CORE/.git" ]] || {
  echo "missing third_party/game-recomp; run git submodule update --init --recursive" >&2
  exit 1
}

if git -C "$CORE" apply --reverse --check "$PATCH" >/dev/null 2>&1; then
  exit 0
fi
git -C "$CORE" apply --check "$PATCH"
git -C "$CORE" apply "$PATCH"
