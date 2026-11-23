#!/bin/bash
# Cross-builds the runtime's libraries for the browser into .cache/outputs/web/prefix, as
# platforms/deps.sh does for the apps (same versions, same sources in .cache/shared/src):
# SDL3 (Emscripten backend, pthreads), a minimal LGPL ffmpeg (MP3 and Bink, plain C), and
# innoextract as a library (unpacking GOG's offline installer in the page). zlib comes from
# Emscripten's ports. Every step is skipped when its output exists; --force rebuilds all.
#
#   web/deps.sh [--force]
set -euo pipefail
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
# shellcheck source=web/env.sh
source "$ROOT/web/env.sh"
FORCE="${1:-}"
web_emsdk

SDL_V=3.4.20
FFMPEG_V=9.0.2
FFMPEG_SHA=8c3850283eb25fa026482078a04051e0be17347b09ef81a0849bec15a96e002e
INNO_V=1.9
INNO_SHA=6344a69fc1ed847d4ed3e272e0da5998948c6b828cb7af39c6321aba6cf88126
BOOST_DIR=boost_1_89_0
BOOST_SHA=85a33fa22621b4f314f8e85e1a5e2a9363d22e4f4992925d4bb3bc631b5a0c7a
XZ_V=5.8.1
XZ_SHA=507825b599356c10dca1cd720c9d0d0c9d5400b9de300af00e4d1ea150795543
BZIP2_V=1.0.8
BZIP2_SHA=ab5a03176ee106d3f0fa90e381da478ddae405918153cca248e682cd0c4a2269

SRC="$ROOT/.cache/shared/src"; BLD="$WEB_CACHE/deps-build"
mkdir -p "$SRC" "$BLD" "$PREFIX"
say() { printf '\n== %s (web)\n' "$*"; }
done_or_force() { [[ -e "$1" && "$FORCE" != --force ]]; }

fetch_sha() {  # url sha256 -> $SRC/<file name>, checked
  local f="$SRC/${1##*/}"
  if [[ ! -f "$f" ]] || ! echo "$2  $f" | shasum -a 256 -c - >/dev/null 2>&1; then
    curl -fsSL -o "$f.part" "$1"
    echo "$2  $f.part" | shasum -a 256 -c - >/dev/null || { echo "checksum mismatch: $1"; exit 1; }
    mv "$f.part" "$f"
  fi
}

cmake_dep() {  # name srcdir extra-args...
  local name=$1 src=$2; shift 2
  rm -rf "$BLD/$name"
  emcmake cmake -S "$src" -B "$BLD/$name" -G Ninja -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_INSTALL_PREFIX="$PREFIX" -DCMAKE_PREFIX_PATH="$PREFIX" \
    -DCMAKE_FIND_ROOT_PATH="$PREFIX" "$@" >"$BLD/$name.log" 2>&1 ||
    { tail -30 "$BLD/$name.log"; exit 1; }
  cmake --build "$BLD/$name" -j"$JOBS" >>"$BLD/$name.log" 2>&1 || { tail -30 "$BLD/$name.log"; exit 1; }
  cmake --install "$BLD/$name" >>"$BLD/$name.log" 2>&1
  rm -rf "$BLD/$name"   # disk is precious: keep only the install
}

say "SDL3 $SDL_V"
if ! done_or_force "$PREFIX/lib/cmake/SDL3/SDL3Config.cmake"; then
  SDL_SRC="$SRC/SDL3-$SDL_V"
  [[ -d "$SDL_SRC" ]] || {
    curl -fsSL -o "$SRC/SDL3-$SDL_V.tar.gz" \
      "https://github.com/libsdl-org/SDL/releases/download/release-$SDL_V/SDL3-$SDL_V.tar.gz"
    tar -xzf "$SRC/SDL3-$SDL_V.tar.gz" -C "$SRC" && rm "$SRC/SDL3-$SDL_V.tar.gz"; }
  cmake_dep sdl3 "$SDL_SRC" -DSDL_SHARED=OFF -DSDL_STATIC=ON -DSDL_PTHREADS=ON -DSDL_TESTS=OFF \
    -DSDL_EXAMPLES=OFF -DSDL_TEST_LIBRARY=OFF
fi
echo "sdl3: ok"

say "ffmpeg $FFMPEG_V (LGPL)"
if ! done_or_force "$PREFIX/lib/libavcodec.a"; then
  [[ -d "$SRC/ffmpeg-$FFMPEG_V" ]] || {
    curl -fsSL -o "$SRC/ffmpeg-$FFMPEG_V.tar.xz" "https://ffmpeg.org/releases/ffmpeg-$FFMPEG_V.tar.xz"
    echo "$FFMPEG_SHA  $SRC/ffmpeg-$FFMPEG_V.tar.xz" | shasum -a 256 -c - >/dev/null
    tar -xJf "$SRC/ffmpeg-$FFMPEG_V.tar.xz" -C "$SRC" && rm "$SRC/ffmpeg-$FFMPEG_V.tar.xz"; }
  rm -rf "$BLD/ffmpeg"; mkdir -p "$BLD/ffmpeg"
  # The same decoders as the apps' (platforms/deps.sh); plain C (no wasm assembly in ffmpeg) and
  # no threads of its own (the decoders run on the game's threads).
  (cd "$BLD/ffmpeg" && "$SRC/ffmpeg-$FFMPEG_V/configure" --prefix="$PREFIX" \
    --enable-static --disable-shared --disable-programs --disable-doc \
    --disable-everything --disable-autodetect --disable-network --disable-avdevice --disable-avfilter \
    --enable-avformat --enable-avcodec --enable-swscale --enable-swresample \
    --enable-decoder=mp3,mp3float,bink,binkaudio_rdft,binkaudio_dct \
    --enable-parser=mpegaudio --enable-demuxer=bink,mp3 --enable-protocol=file \
    --enable-cross-compile --target-os=none --arch=x86_32 --cpu=generic --disable-asm \
    --disable-inline-asm --disable-x86asm --disable-pthreads --disable-runtime-cpudetect \
    --cc=emcc --cxx=em++ --ar=emar --ranlib=emranlib --nm="$(command -v llvm-nm || echo emnm)" \
    --extra-cflags="$WEB_FLAGS -O3" --extra-ldflags="$LDFLAGS" >"$BLD/ffmpeg.log" 2>&1) ||
    { tail -30 "$BLD/ffmpeg.log"; exit 1; }
  grep -q "License: LGPL" "$BLD/ffmpeg.log" || { echo "ffmpeg: not LGPL"; exit 1; }
  make -C "$BLD/ffmpeg" -j"$JOBS" >>"$BLD/ffmpeg.log" 2>&1 || { tail -30 "$BLD/ffmpeg.log"; exit 1; }
  make -C "$BLD/ffmpeg" install >>"$BLD/ffmpeg.log" 2>&1
  cp "$SRC/ffmpeg-$FFMPEG_V/COPYING.LGPLv2.1" "$PREFIX/COPYING.ffmpeg.LGPLv2.1"
  rm -rf "$BLD/ffmpeg"
fi
echo "ffmpeg: ok"

say "innoextract $INNO_V (library)"
if ! done_or_force "$PREFIX/lib/librecomp_innoextract.a"; then
  fetch_sha "https://constexpr.org/innoextract/files/innoextract-$INNO_V.tar.gz" "$INNO_SHA"
  fetch_sha "https://archives.boost.io/release/1.89.0/source/$BOOST_DIR.tar.bz2" "$BOOST_SHA"
  fetch_sha "https://github.com/tukaani-project/xz/releases/download/v$XZ_V/xz-$XZ_V.tar.gz" "$XZ_SHA"
  fetch_sha "https://sourceware.org/pub/bzip2/bzip2-$BZIP2_V.tar.gz" "$BZIP2_SHA"
  INNO="$SRC/innoextract-$INNO_V"
  if [[ ! -f "$INNO/.patched" ]]; then
    rm -rf "$INNO"; tar -xzf "$SRC/innoextract-$INNO_V.tar.gz" -C "$SRC"
    for p in "$ROOT"/libs/innoextract/patches/*.patch "$ROOT"/libs/innoextract/lib-patches/*.patch; do
      patch -s -p1 -d "$INNO" < "$p"
    done
    touch "$INNO/.patched"
  fi
  [[ -d "$SRC/$BOOST_DIR/libs/iostreams/src" ]] || tar -xjf "$SRC/$BOOST_DIR.tar.bz2" -C "$SRC" \
    "$BOOST_DIR/boost" "$BOOST_DIR/libs/filesystem/src" "$BOOST_DIR/libs/iostreams/src" "$BOOST_DIR/LICENSE_1_0.txt"
  [[ -d "$SRC/xz-$XZ_V" ]] || tar -xzf "$SRC/xz-$XZ_V.tar.gz" -C "$SRC"
  [[ -d "$SRC/bzip2-$BZIP2_V" ]] || tar -xzf "$SRC/bzip2-$BZIP2_V.tar.gz" -C "$SRC"
  cmake_dep xz "$SRC/xz-$XZ_V" -DBUILD_SHARED_LIBS=OFF -DXZ_NLS=OFF -DXZ_DOC=OFF -DXZ_TOOL_XZ=OFF \
    -DXZ_TOOL_XZDEC=OFF -DXZ_TOOL_LZMADEC=OFF -DXZ_TOOL_LZMAINFO=OFF -DXZ_TOOL_SCRIPTS=OFF \
    -DXZ_SANDBOX=no -DXZ_THREADS=posix -DBUILD_TESTING=OFF
  # zlib: Emscripten's port (-sUSE_ZLIB=1), its headers in the SDK's sysroot once it is built.
  embuilder build zlib >/dev/null
  cmake_dep innoextract "$ROOT/libs/innoextract/lib" -DINNOEXTRACT_SRC="$INNO" \
    -DBOOST_SRC="$SRC/$BOOST_DIR" -DLZMA_INCLUDE="$SRC/xz-$XZ_V/src/liblzma/api" \
    -DBZIP2_SRC="$SRC/bzip2-$BZIP2_V" -DCMAKE_CXX_FLAGS="$WEB_FLAGS -sUSE_ZLIB=1"
fi
echo "innoextract: ok"
echo "deps (web): $PREFIX"
