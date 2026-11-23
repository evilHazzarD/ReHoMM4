#!/bin/bash
# Sourced by web/deps.sh and web/build.sh: the browser build's paths and flags.
#
#   EMSDK      the Emscripten SDK (.cache/shared/emsdk, installed by deps.sh if missing)
#   WEB_CACHE  .cache/outputs/web: build/, prefix/ (the cross-built libraries), dist/ (the site)
#   WEB_FLAGS  what every object of the build is compiled with: one wasm module with shared memory
#              (pthreads) needs atomics in every object, and the runtime's SEH resumes are C++
#              exceptions (native wasm exceptions, never the JS emulation)
ROOT="${ROOT:-$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)}"
# Sourced from another shell than bash (zsh has no BASH_SOURCE), ROOT would be wrong: stop there.
[[ -f "$ROOT/VERSION" && -d "$ROOT/web" ]] || { echo "web/env.sh: not in the repository ($ROOT); run the scripts with bash" >&2; return 2 2>/dev/null || exit 2; }
WEB="$ROOT/web"
WEB_CACHE="$ROOT/.cache/outputs/web"
PREFIX="$WEB_CACHE/prefix"
EMSDK="${EMSDK:-$ROOT/.cache/shared/emsdk}"
JOBS="${H4_BUILD_JOBS:-$(sysctl -n hw.perflevel0.physicalcpu 2>/dev/null || nproc 2>/dev/null || echo 4)}"
H4_VERSION="$(tr -d ' \n' < "$ROOT/VERSION")"
[[ -x /opt/homebrew/bin/brew ]] && eval "$(/opt/homebrew/bin/brew shellenv)"

WEB_FLAGS="-pthread -fwasm-exceptions -msimd128"
export CFLAGS="$WEB_FLAGS" CXXFLAGS="$WEB_FLAGS" LDFLAGS="-pthread -fwasm-exceptions"

web_emsdk() {  # the SDK on PATH (installed the first time)
  if [[ ! -f "$EMSDK/emsdk_env.sh" ]]; then
    git clone -q --depth 1 https://github.com/emscripten-core/emsdk.git "$EMSDK"
    "$EMSDK/emsdk" install latest && "$EMSDK/emsdk" activate latest >/dev/null
    rm -rf "$EMSDK/downloads"
  fi
  # shellcheck disable=SC1091
  EMSDK_QUIET=1 source "$EMSDK/emsdk_env.sh" >/dev/null 2>&1
  command -v emcc >/dev/null || { echo "no emcc in $EMSDK" >&2; exit 2; }
}
