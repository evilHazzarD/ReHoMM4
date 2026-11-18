#!/bin/bash
# Builds the Mac test harness (innoextract library built for the Mac on first use) and runs it.
set -euo pipefail
ROOT="$(cd "$(dirname "$0")/../../.." && pwd)"
export PATH="/opt/homebrew/bin:$PATH"
H="$ROOT/.cache/outputs/host"; P="$H/prefix"; SRC="$ROOT/.cache/shared/src"
if [[ ! -f "$P/lib/librecomp_innoextract.a" ]]; then
  [[ -d "$SRC/innoextract-1.9" ]] || { echo "run platforms/deps.sh ios first (it fetches the sources)"; exit 1; }
  cmake -S "$SRC/xz-5.8.1" -B "$H/xz" -G Ninja -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX="$P" \
    -DBUILD_SHARED_LIBS=OFF -DXZ_NLS=OFF -DXZ_DOC=OFF -DXZ_TOOL_XZ=OFF -DXZ_TOOL_XZDEC=OFF \
    -DXZ_TOOL_LZMADEC=OFF -DXZ_TOOL_LZMAINFO=OFF -DXZ_TOOL_SCRIPTS=OFF -DXZ_SANDBOX=no >/dev/null
  cmake --build "$H/xz" >/dev/null && cmake --install "$H/xz" >/dev/null
  cmake -S "$ROOT/libs/innoextract/lib" -B "$H/inno" -G Ninja -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_INSTALL_PREFIX="$P" -DINNOEXTRACT_SRC="$SRC/innoextract-1.9" \
    -DBOOST_SRC="$SRC/boost_1_89_0" -DLZMA_INCLUDE="$SRC/xz-5.8.1/src/liblzma/api" >/dev/null
  cmake --build "$H/inno" >/dev/null && cmake --install "$H/inno" >/dev/null
fi
cmake -S "$(dirname "$0")" -B "$H/test" -G Ninja -DCMAKE_BUILD_TYPE=Release -DPREFIX="$P" >/dev/null
cmake --build "$H/test" 2>&1 | grep -E 'error|warning: unused' | head -20 || true
"$H/test/install_test" "$@"
