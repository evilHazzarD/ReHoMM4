#!/usr/bin/env bash
# Writes converter/h4/discover-<sha256>.txt for a supported heroes4.exe (run again after a
# change of game-recomp, its discovery patch or h4/lifter.manifest: the converter then ignores
# the old one and takes ten times as long).
#   converter/tools/discovery_seed.sh <heroes4.exe>
set -euo pipefail
ROOT="$(cd "$(dirname "$0")/../.." && pwd)"
EXE="$(cd "$(dirname "$1")" && pwd)/$(basename "$1")"
"$ROOT/converter/tools/apply_discovery_patch.sh"
STAGE="$(mktemp -d)"
trap 'rm -rf "$STAGE"' EXIT
node "$ROOT/converter/tools/stage_manifest.mjs" "$STAGE"
H4_DISCOVERY_VERSION="$("$ROOT/converter/tools/discovery_version.sh")" \
  CARGO_TARGET_DIR="${CARGO_TARGET_DIR:-$ROOT/.cache/outputs/converter-emitter-native}" \
  cargo run --locked --release --manifest-path "$ROOT/converter/emitter/Cargo.toml" \
  --features discovery --example discovery_seed -- "$EXE" "$STAGE/h4" "$ROOT/converter/h4"
