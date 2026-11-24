#!/usr/bin/env bash
# Stage the runtime sources shared by local and build-server targets.
set -euo pipefail
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
X="$ROOT/third_party/x86recomp"
URL=https://github.com/JTM-rootstorm/x86recomp.git
REVISION=1c90fe3a5dad37d0cca551fc9c8ab66a106ede1e

if [[ -f "$X/src/runtime/dispatch.cpp" ]]; then
  exit 0
fi
git clone -q "$URL" "$X"
git -C "$X" checkout -q "$REVISION"
git -C "$X" apply "$ROOT/patches/x86recomp-h4.patch"
for patch in "$ROOT"/patches/x86recomp-*.patch; do
  [[ "$patch" == */x86recomp-h4.patch ]] || git -C "$X" apply "$patch"
done
