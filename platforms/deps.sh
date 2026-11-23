#!/bin/bash
# Cross-builds the runtime's libraries for a mobile target into .cache/outputs/<target>/prefix:
# SDL3, glslang, SPIRV-Cross (MSL, iOS only), a minimal LGPL ffmpeg (the same decoders as
# platforms/macos-dmg/ffmpeg_lgpl.sh) and innoextract as a library (the in-app installer,
# libs/innoextract/README.md). Everything but SDL3 is static. Same versions as the Mac build's Homebrew.
#
#   platforms/deps.sh ios        iPhone/iPad, arm64, iOS 17+
#   platforms/deps.sh android    arm64-v8a, API 29+ (Vulkan 1.1), NDK from ANDROID_NDK or the newest in the SDK
#   platforms/deps.sh linux      x86-64 (Steam Deck), glibc 2.36+, cross-built against a Debian sysroot
#   platforms/deps.sh linux-arm64  the same for 64-bit ARM
#   platforms/deps.sh windows-x86_64  64-bit Windows, cross-built with Homebrew mingw-w64
#   platforms/deps.sh macos-arm64    the Mac app, Apple Silicon, macOS 13+
#   platforms/deps.sh macos-x86_64   the Mac app, Intel, macOS 13+
#   platforms/deps.sh switch     Nintendo Switch homebrew (devkitPro's SDL3 branch, no SDL_GPU)
#
# Each step is skipped when its output exists; pass --force as the 2nd argument to rebuild all.
set -euo pipefail
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
# shellcheck source=platforms/env.sh
source "$ROOT/platforms/env.sh" "${1:-}"
FORCE="${2:-}"

SDL_V=3.4.20
GLSLANG_V=16.6.0
SPIRV_CROSS_V=vulkan-sdk-1.4.363.0
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
ZLIB_V=1.3.1
ZLIB_SHA=9a93b2b7dfdac77ceba5a558a580e74667dd6fede4585b91eefb60f03b72df23
# The Switch: devkitPro's SDL 3.4 with its Switch backend (video, OpenGL ES, audio, Joy-Cons, touch).
SDL_SWITCH_COMMIT=c329016c0e338ab4397ad930f83330c9fe058348

SRC="$ROOT/.cache/shared/src"; BLD="$MOBILE_CACHE/build"
mkdir -p "$SRC" "$BLD" "$PREFIX"
say() { printf '\n== %s (%s)\n' "$*" "$TARGET"; }
done_or_force() { [[ -e "$1" && "$FORCE" != --force ]]; }

fetch_sha() {  # url sha256 -> $SRC/<file name>, checked
  local f="$SRC/${1##*/}"
  if [[ ! -f "$f" ]] || ! echo "$2  $f" | shasum -a 256 -c - >/dev/null 2>&1; then
    curl -fsSL -o "$f.part" "$1"
    echo "$2  $f.part" | shasum -a 256 -c - >/dev/null || { echo "checksum mismatch: $1"; exit 1; }
    mv "$f.part" "$f"
  fi
}

fetch_git() {  # name url tag
  [[ -d "$SRC/$1" ]] || git -c advice.detachedHead=false clone -q --depth 1 --branch "$3" "$2" "$SRC/$1"
}

cmake_dep() {  # name srcdir extra-args...
  local name=$1 src=$2; shift 2
  rm -rf "$BLD/$name"
  cmake -S "$src" -B "$BLD/$name" -G Ninja -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_INSTALL_PREFIX="$PREFIX" "${CMAKE_TARGET_ARGS[@]}" "$@" >"$BLD/$name.log" 2>&1 ||
    { tail -30 "$BLD/$name.log"; exit 1; }
  cmake --build "$BLD/$name" -j"$JOBS" >>"$BLD/$name.log" 2>&1 || { tail -30 "$BLD/$name.log"; exit 1; }
  cmake --install "$BLD/$name" >>"$BLD/$name.log" 2>&1
  rm -rf "$BLD/$name"   # disk is precious: keep only the install
}

say "SDL3 $SDL_V"
SDL_SWITCH2_PATCH="$ROOT/platforms/switch/sdl-switch2-display.patch"
SDL_SWITCH2_STAMP="$PREFIX/share/sdl-switch2-display.patch"
if ! done_or_force "$PREFIX/lib/cmake/SDL3/SDL3Config.cmake" ||
   { [[ $TARGET == switch2 ]] && ! cmp -s "$SDL_SWITCH2_PATCH" "$SDL_SWITCH2_STAMP"; }; then
  SDL_SRC="$SRC/SDL3-$SDL_V"
  if [[ $TARGET == switch* ]]; then
    SDL_SRC="$SRC/SDL-$SDL_SWITCH_COMMIT"
    [[ -d "$SDL_SRC" ]] || {
      curl -fsSL -o "$SRC/sdl-switch.tar.gz" "https://github.com/devkitPro/SDL/archive/$SDL_SWITCH_COMMIT.tar.gz"
      tar -xzf "$SRC/sdl-switch.tar.gz" -C "$SRC" && rm "$SRC/sdl-switch.tar.gz"; }
  fi
  [[ -d "$SDL_SRC" ]] || {
    curl -fsSL -o "$SRC/SDL3-$SDL_V.tar.gz" \
      "https://github.com/libsdl-org/SDL/releases/download/release-$SDL_V/SDL3-$SDL_V.tar.gz"
    tar -xzf "$SRC/SDL3-$SDL_V.tar.gz" -C "$SRC" && rm "$SRC/SDL3-$SDL_V.tar.gz"; }
  if [[ $TARGET == switch2 ]]; then
    SDL_SWITCH2_SRC="$MOBILE_CACHE/sdl-source"
    mkdir -p "$SDL_SWITCH2_SRC"
    cp -R "$SDL_SRC/." "$SDL_SWITCH2_SRC/"
    SDL_SRC="$SDL_SWITCH2_SRC"
    cp "$SDL_SWITCH2_PATCH" "$SDL_SRC/rehomm4-switch2-display.patch"
    patch -s -p1 -d "$SDL_SRC" < "$SDL_SRC/rehomm4-switch2-display.patch"
  fi
  # iOS: static (an app's frameworks need their own bundle); Android: libSDL3.so, which SDLActivity loads.
  if [[ $TARGET == ios || $TARGET == macos-* || $TARGET == switch* ]]; then sdl_shared=(-DSDL_SHARED=OFF -DSDL_STATIC=ON)
  else sdl_shared=(-DSDL_SHARED=ON -DSDL_STATIC=OFF); fi
  if [[ $TARGET == linux* ]]; then
    # X11 (Game Mode's gamescope runs games on Xwayland; the desktop has Xwayland too), Vulkan,
    # PipeWire/PulseAudio/ALSA and udev, all loaded at run time; no Wayland (its scanner is a
    # build-time tool for the target) or KMS/DRM.
    sdl_shared+=(-DSDL_WAYLAND=OFF -DSDL_KMSDRM=OFF -DSDL_X11=ON -DSDL_VULKAN=ON -DSDL_PULSEAUDIO=ON
      -DSDL_PIPEWIRE=ON -DSDL_ALSA=ON -DSDL_JACK=OFF -DSDL_SNDIO=OFF -DSDL_LIBUDEV=ON -DSDL_DBUS=ON
      -DSDL_IBUS=OFF -DSDL_HIDAPI_LIBUSB=OFF -DSDL_X11_XTEST=OFF -DSDL_DEPS_SHARED=ON)
  fi
  ( if [[ $TARGET == linux* ]]; then  # the sysroot's .pc files (SDL's PipeWire/PulseAudio checks)
      S="$H4_LINUX_SYSROOT"; export PKG_CONFIG_SYSROOT_DIR="$S"
      export PKG_CONFIG_LIBDIR="$S/usr/lib/$H4_LINUX_TRIPLE/pkgconfig:$S/usr/share/pkgconfig:$S/usr/lib/pkgconfig"
    fi
    cmake_dep sdl3 "$SDL_SRC" "${sdl_shared[@]}" -DSDL_TESTS=OFF -DSDL_EXAMPLES=OFF \
    -DSDL_TEST_LIBRARY=OFF )
  if [[ $TARGET == switch2 ]]; then
    cp "$SDL_SRC/rehomm4-switch2-display.patch" "$SDL_SWITCH2_STAMP"
  fi
fi
echo "sdl3: ok"

say "glslang $GLSLANG_V"
if [[ $TARGET == switch* ]]; then
  echo "glslang: not used (no SDL_GPU on the Switch)"
elif ! done_or_force "$PREFIX/lib/cmake/glslang/glslang-config.cmake"; then
  fetch_git glslang https://github.com/KhronosGroup/glslang.git "$GLSLANG_V"
  cmake_dep glslang "$SRC/glslang" -DBUILD_SHARED_LIBS=OFF -DENABLE_OPT=OFF -DENABLE_HLSL=OFF \
    -DGLSLANG_TESTS=OFF -DGLSLANG_ENABLE_INSTALL=ON -DENABLE_GLSLANG_BINARIES=OFF \
    -DCMAKE_POSITION_INDEPENDENT_CODE=ON
fi
[[ $TARGET == switch* ]] || echo "glslang: ok"

if [[ $TARGET == ios || $TARGET == macos-* ]]; then
  say "SPIRV-Cross $SPIRV_CROSS_V"
  if ! done_or_force "$PREFIX/share/spirv_cross_msl/cmake/spirv_cross_mslConfig.cmake"; then
    fetch_git SPIRV-Cross https://github.com/KhronosGroup/SPIRV-Cross.git "$SPIRV_CROSS_V"
    cmake_dep spirv-cross "$SRC/SPIRV-Cross" -DSPIRV_CROSS_SHARED=OFF -DSPIRV_CROSS_STATIC=ON \
      -DSPIRV_CROSS_CLI=OFF -DSPIRV_CROSS_ENABLE_TESTS=OFF -DSPIRV_CROSS_ENABLE_HLSL=OFF \
      -DSPIRV_CROSS_ENABLE_CPP=OFF -DSPIRV_CROSS_ENABLE_REFLECT=OFF -DSPIRV_CROSS_ENABLE_UTIL=OFF \
      -DCMAKE_POSITION_INDEPENDENT_CODE=ON
  fi
  echo "spirv-cross: ok"
fi

say "ffmpeg $FFMPEG_V (LGPL)"
if ! done_or_force "$PREFIX/lib/libavcodec.a"; then
  [[ -d "$SRC/ffmpeg-$FFMPEG_V" ]] || {
    curl -fsSL -o "$SRC/ffmpeg-$FFMPEG_V.tar.xz" "https://ffmpeg.org/releases/ffmpeg-$FFMPEG_V.tar.xz"
    echo "$FFMPEG_SHA  $SRC/ffmpeg-$FFMPEG_V.tar.xz" | shasum -a 256 -c - >/dev/null
    tar -xJf "$SRC/ffmpeg-$FFMPEG_V.tar.xz" -C "$SRC" && rm "$SRC/ffmpeg-$FFMPEG_V.tar.xz"; }
  rm -rf "$BLD/ffmpeg"; mkdir -p "$BLD/ffmpeg"
  (cd "$BLD/ffmpeg" && "$SRC/ffmpeg-$FFMPEG_V/configure" --prefix="$PREFIX" \
    --enable-static --disable-shared --enable-pic --disable-programs --disable-doc \
    --disable-everything --disable-autodetect --disable-network --disable-avdevice --disable-avfilter \
    --enable-avformat --enable-avcodec --enable-swscale --enable-swresample \
    --enable-decoder=mp3,mp3float,bink,binkaudio_rdft,binkaudio_dct \
    --enable-parser=mpegaudio --enable-demuxer=bink,mp3 --enable-protocol=file \
    "${FFMPEG_TARGET_ARGS[@]}" >"$BLD/ffmpeg.log" 2>&1) || { tail -30 "$BLD/ffmpeg.log"; exit 1; }
  grep -q "License: LGPL" "$BLD/ffmpeg.log" || { echo "ffmpeg: not LGPL"; exit 1; }
  make -C "$BLD/ffmpeg" -j"$JOBS" >>"$BLD/ffmpeg.log" 2>&1 || { tail -30 "$BLD/ffmpeg.log"; exit 1; }
  make -C "$BLD/ffmpeg" install >>"$BLD/ffmpeg.log" 2>&1
  cp "$SRC/ffmpeg-$FFMPEG_V/COPYING.LGPLv2.1" "$PREFIX/COPYING.ffmpeg.LGPLv2.1"
  # "-framework X" pairs: CMake de-duplicates the repeated "-framework" word of pkg-config
  # flags, leaving bare "CoreVideo" on the link line. One token each.
  sed -i '' -E 's/-framework ([A-Za-z]+)/-Wl,-framework,\1/g' "$PREFIX"/lib/pkgconfig/*.pc
  # Linux: libatomic inside the binary (not every system has libatomic.so.1).
  [[ $TARGET == linux* ]] && sed -i '' -E 's/-latomic/-l:libatomic.a/g' "$PREFIX"/lib/pkgconfig/*.pc
  rm -rf "$BLD/ffmpeg"
fi
echo "ffmpeg: ok"
if [[ $TARGET == windows-* ]] && ! done_or_force "$PREFIX/include/zlib.h"; then
  say "zlib $ZLIB_V"
  fetch_sha "https://zlib.net/fossils/zlib-$ZLIB_V.tar.gz" "$ZLIB_SHA"
  [[ -d "$SRC/zlib-$ZLIB_V" ]] || tar -xzf "$SRC/zlib-$ZLIB_V.tar.gz" -C "$SRC"
  cmake_dep zlib "$SRC/zlib-$ZLIB_V" -DBUILD_SHARED_LIBS=OFF -DZLIB_BUILD_EXAMPLES=OFF
  echo "zlib: ok"
fi
say "innoextract $INNO_V (library)"
if ! done_or_force "$PREFIX/lib/librecomp_innoextract.a"; then
  fetch_sha "https://constexpr.org/innoextract/files/innoextract-$INNO_V.tar.gz" "$INNO_SHA"
  fetch_sha "https://archives.boost.io/release/1.89.0/source/$BOOST_DIR.tar.bz2" "$BOOST_SHA"
  fetch_sha "https://github.com/tukaani-project/xz/releases/download/v$XZ_V/xz-$XZ_V.tar.gz" "$XZ_SHA"
  INNO="$SRC/innoextract-$INNO_V"
  if [[ ! -f "$INNO/.patched" ]]; then
    rm -rf "$INNO"; tar -xzf "$SRC/innoextract-$INNO_V.tar.gz" -C "$SRC"
    for p in "$ROOT"/libs/innoextract/patches/*.patch "$ROOT"/libs/innoextract/lib-patches/*.patch; do
      patch -s -p1 -d "$INNO" < "$p"
    done
    touch "$INNO/.patched"
  fi
  # Boost: only the headers and the two libraries' sources (the archive is 150 MB).
  [[ -d "$SRC/$BOOST_DIR/libs/iostreams/src" ]] || tar -xjf "$SRC/$BOOST_DIR.tar.bz2" -C "$SRC" \
    "$BOOST_DIR/boost" "$BOOST_DIR/libs/filesystem/src" "$BOOST_DIR/libs/iostreams/src" "$BOOST_DIR/LICENSE_1_0.txt"
  [[ -d "$SRC/xz-$XZ_V" ]] || tar -xzf "$SRC/xz-$XZ_V.tar.gz" -C "$SRC"
  xz_threads=posix
  [[ $TARGET == switch* ]] && xz_threads=no
  [[ $TARGET == windows-* ]] && xz_threads=win95
  cmake_dep xz "$SRC/xz-$XZ_V" -DBUILD_SHARED_LIBS=OFF -DXZ_NLS=OFF -DXZ_DOC=OFF -DXZ_TOOL_XZ=OFF \
    -DXZ_TOOL_XZDEC=OFF -DXZ_TOOL_LZMADEC=OFF -DXZ_TOOL_LZMAINFO=OFF -DXZ_TOOL_SCRIPTS=OFF \
    -DXZ_SANDBOX=no -DXZ_THREADS="$xz_threads" -DBUILD_TESTING=OFF \
    -DCMAKE_POSITION_INDEPENDENT_CODE=ON
  bz=()
  if [[ $TARGET == android || $TARGET == linux* || $TARGET == windows-* ]]; then  # no libbz2 in the NDK, sysroot or mingw prefix
    fetch_sha "https://sourceware.org/pub/bzip2/bzip2-$BZIP2_V.tar.gz" "$BZIP2_SHA"
    [[ -d "$SRC/bzip2-$BZIP2_V" ]] || tar -xzf "$SRC/bzip2-$BZIP2_V.tar.gz" -C "$SRC"
    bz=(-DBZIP2_SRC="$SRC/bzip2-$BZIP2_V")
  fi
  cmake_dep innoextract "$ROOT/libs/innoextract/lib" -DINNOEXTRACT_SRC="$INNO" \
    -DBOOST_SRC="$SRC/$BOOST_DIR" -DLZMA_INCLUDE="$SRC/xz-$XZ_V/src/liblzma/api" "${bz[@]}" \
    -DCMAKE_POSITION_INDEPENDENT_CODE=ON
fi
echo "innoextract: ok"
echo "deps ($TARGET): $PREFIX"
