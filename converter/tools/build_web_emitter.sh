#!/usr/bin/env bash
set -euo pipefail
ROOT="$(cd "$(dirname "$0")/../.." && pwd)"
OUT="${1:-$ROOT/.cache/outputs/web-emitter}"
TARGET="${CARGO_TARGET_DIR:-$ROOT/.cache/outputs/converter-emitter}"
mkdir -p "$OUT" "$TARGET"
OUT="$(cd "$OUT" && pwd)"
TARGET="$(cd "$TARGET" && pwd)"

if [[ "$(wasm-bindgen --version)" != "wasm-bindgen 0.2.130" ]]; then
  echo "wasm-bindgen-cli 0.2.130 is required" >&2
  exit 1
fi
CARGO_TARGET_DIR="$TARGET" cargo build --locked --release --target wasm32-unknown-unknown \
  --manifest-path "$ROOT/converter/emitter/Cargo.toml" --features browser
wasm-bindgen --target web --no-typescript --out-dir "$OUT" \
  "$TARGET/wasm32-unknown-unknown/release/rehomm4_emit_wasm.wasm"
node "$ROOT/converter/tools/stage_manifest.mjs" "$OUT"
cp "$ROOT/converter/src/emit_worker.mjs" "$OUT/emit_worker.mjs"
