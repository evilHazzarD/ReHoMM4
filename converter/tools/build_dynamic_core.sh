#!/usr/bin/env bash
# Build the browser-side dynamic recompilation core. This only produces the converter module;
# publishing a player waits for the ReHoMM4 runtime adapter to consume its code.wasm output.
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/../.." && pwd)"
CORE="$ROOT/third_party/game-recomp"
OUT="${1:-$ROOT/.cache/outputs/converter-core}"

command -v cargo >/dev/null || { echo "cargo is required (install Rust 1.88 or newer)" >&2; exit 1; }
command -v wasm-bindgen >/dev/null || {
  echo "wasm-bindgen-cli 0.2.130 is required" >&2
  exit 1
}

bash "$ROOT/converter/tools/configure_dynamic_core.sh"
mkdir -p "$OUT"
(
  cd "$CORE"
  RC_MANIFESTS_DIR="$ROOT/converter/h4" \
    cargo build --release --target wasm32-unknown-unknown --manifest-path recomp/Cargo.toml -p rc-web
  wasm-bindgen --target web --no-typescript --out-dir "$OUT" \
    recomp/target/wasm32-unknown-unknown/release/rc_web.wasm
)
echo "dynamic converter core: $OUT"
