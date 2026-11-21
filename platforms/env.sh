#!/bin/bash
# Sourced by the mobile scripts: `source platforms/env.sh <ios|android|linux>` (linux: x86-64, the
# Steam Deck; cross-built against platforms/linux/sysroot.py's Debian sysroot). Sets TARGET, MOBILE_CACHE,
# PREFIX (the cross-built libraries), JOBS, CMAKE_TARGET_ARGS (toolchain flags for any CMake project)
# and FFMPEG_TARGET_ARGS (ffmpeg's configure).
TARGET="${1:-}"
case "$TARGET" in ios|android|linux|macos-arm64|macos-x86_64|switch) ;;
  *) echo "usage: $0 ios|android|linux|macos-arm64|macos-x86_64|switch" >&2; exit 2 ;; esac
ROOT="${ROOT:-$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)}"
# Homebrew first: an older cmake (MacPorts) cannot read the x86recomp presets.
[[ -x /opt/homebrew/bin/brew ]] && eval "$(/opt/homebrew/bin/brew shellenv)"
export PATH="/opt/homebrew/bin:$PATH"
MOBILE_CACHE="$ROOT/.cache/outputs/$TARGET"
# The release version (VERSION: MAJOR.MINOR[.PATCH]) and the build number from it, which only grows
# (MAJOR*10000 + MINOR*100 + PATCH: 0.1 -> 100): Android's versionCode, the Apple bundle version.
H4_VERSION="$(tr -d ' \n' < "$ROOT/VERSION")"
[[ "$H4_VERSION" =~ ^([0-9]+)\.([0-9]+)(\.([0-9]+))?$ ]] || { echo "VERSION: not MAJOR.MINOR[.PATCH]: $H4_VERSION" >&2; exit 2; }
H4_BUILD=$(( BASH_REMATCH[1] * 10000 + BASH_REMATCH[2] * 100 + ${BASH_REMATCH[4]:-0} ))
export H4_VERSION H4_BUILD
PREFIX="$MOBILE_CACHE/prefix"
JOBS="${H4_BUILD_JOBS:-$(sysctl -n hw.perflevel0.physicalcpu 2>/dev/null || echo 4)}"

IOS_MIN=17.0
ANDROID_API=29

if [[ $TARGET == ios ]]; then
  IOS_SDK="$(xcrun --sdk iphoneos --show-sdk-path)"
  CMAKE_TARGET_ARGS=(-DCMAKE_SYSTEM_NAME=iOS -DCMAKE_OSX_ARCHITECTURES=arm64
    -DCMAKE_OSX_DEPLOYMENT_TARGET=$IOS_MIN -DCMAKE_OSX_SYSROOT=iphoneos
    -DCMAKE_PREFIX_PATH="$PREFIX" -DCMAKE_FIND_ROOT_PATH="$PREFIX"
    -DCMAKE_FIND_ROOT_PATH_MODE_PROGRAM=NEVER -DCMAKE_FIND_ROOT_PATH_MODE_PACKAGE=BOTH
    -DCMAKE_FIND_ROOT_PATH_MODE_LIBRARY=BOTH -DCMAKE_FIND_ROOT_PATH_MODE_INCLUDE=BOTH)
  FFMPEG_TARGET_ARGS=(--enable-cross-compile --target-os=darwin --arch=aarch64
    --cc="$(xcrun --sdk iphoneos -f clang)" --sysroot="$IOS_SDK"
    --extra-cflags="-arch arm64 -mios-version-min=$IOS_MIN"
    --extra-ldflags="-arch arm64 -mios-version-min=$IOS_MIN")
elif [[ $TARGET == macos-* ]]; then
  # The Mac app: macos-arm64 (Apple Silicon) or macos-x86_64 (Intel), against this prefix only.
  MAC_ARCH="${TARGET#macos-}"
  MAC_MIN=13.0
  CMAKE_TARGET_ARGS=(-DCMAKE_OSX_ARCHITECTURES=$MAC_ARCH -DCMAKE_OSX_DEPLOYMENT_TARGET=$MAC_MIN
    -DCMAKE_PREFIX_PATH="$PREFIX" -DCMAKE_FIND_FRAMEWORK=LAST -DCMAKE_IGNORE_PREFIX_PATH=/opt/homebrew
    -DCMAKE_SYSTEM_IGNORE_PREFIX_PATH=/opt/homebrew)
  if [[ $MAC_ARCH == x86_64 ]]; then
    # No x86 assembly (it needs nasm): the decoders' C code is quick enough for Bink and MP3.
    FFMPEG_TARGET_ARGS=(--enable-cross-compile --target-os=darwin --arch=x86_64 --disable-x86asm --cc=clang)
  else
    FFMPEG_TARGET_ARGS=(--arch=aarch64 --cc=clang)
  fi
  FFMPEG_TARGET_ARGS+=(--extra-cflags="-arch $MAC_ARCH -mmacosx-version-min=$MAC_MIN"
    --extra-ldflags="-arch $MAC_ARCH -mmacosx-version-min=$MAC_MIN")
elif [[ $TARGET == switch ]]; then
  # Nintendo Switch homebrew (an .nro for Atmosphere's homebrew menu): devkitPro (/opt/devkitpro:
  # the installer from github.com/devkitPro/pacman, then dkp-pacman -S switch-dev switch-mesa
  # switch-libdrm_nouveau switch-zlib switch-bzip2 switch-liblzma switch-curl switch-mbedtls).
  export DEVKITPRO="${DEVKITPRO:-/opt/devkitpro}" DEVKITA64="${DEVKITPRO:-/opt/devkitpro}/devkitA64"
  export H4_SWITCH_PREFIX="$PREFIX"
  [[ -x "$DEVKITA64/bin/aarch64-none-elf-gcc" ]] || { echo "no devkitA64 in $DEVKITPRO (platforms/env.sh)" >&2; exit 2; }
  CMAKE_TARGET_ARGS=(-DCMAKE_TOOLCHAIN_FILE="$ROOT/platforms/switch/toolchain.cmake"
    -DPKG_CONFIG_EXECUTABLE="$ROOT/platforms/switch/pkg-config")
  SW="$DEVKITA64/bin/aarch64-none-elf-"
  SWFLAGS="-march=armv8-a+crc+crypto -mtune=cortex-a57 -mtp=soft -fPIE -D__SWITCH__ -I$DEVKITPRO/libnx/include -I$DEVKITPRO/portlibs/switch/include"
  FFMPEG_TARGET_ARGS=(--enable-cross-compile --target-os=none --arch=aarch64 --cross-prefix="$SW"
    --disable-pic --disable-runtime-cpudetect --disable-pthreads --disable-asm
    --extra-cflags="$SWFLAGS" --extra-ldflags="-specs=$DEVKITPRO/libnx/switch.specs -L$DEVKITPRO/libnx/lib")
elif [[ $TARGET == linux ]]; then
  export H4_LINUX_SYSROOT="$MOBILE_CACHE/sysroot" H4_LINUX_PREFIX="$PREFIX"
  export H4_LINUX_LLVM="${H4_LINUX_LLVM:-$(brew --prefix llvm 2>/dev/null)}"
  ANDROID_HOME="${ANDROID_HOME:-$HOME/Library/Android/sdk}"
  export H4_LINUX_LLD="${H4_LINUX_LLD:-$(ls -d "$ANDROID_HOME"/ndk/*/toolchains/llvm/prebuilt/darwin-x86_64/bin/ld.lld 2>/dev/null | sort -V | tail -1)}"
  [[ -x "$H4_LINUX_LLVM/bin/clang" ]] || { echo "no Homebrew llvm (brew install llvm)" >&2; exit 2; }
  [[ -x "$H4_LINUX_LLD" ]] || { echo "no ld.lld (the Android NDK's, or set H4_LINUX_LLD)" >&2; exit 2; }
  [[ -f "$H4_LINUX_SYSROOT/.complete" ]] ||
    python3 -I "$ROOT/platforms/linux/sysroot.py" "$H4_LINUX_SYSROOT" >/dev/null || exit 1
  CMAKE_TARGET_ARGS=(-DCMAKE_TOOLCHAIN_FILE="$ROOT/platforms/linux/toolchain.cmake"
    -DCMAKE_PREFIX_PATH="$PREFIX")
  LCC="$H4_LINUX_LLVM/bin/clang --target=x86_64-linux-gnu --sysroot=$H4_LINUX_SYSROOT"
  FFMPEG_TARGET_ARGS=(--enable-cross-compile --target-os=linux --arch=x86_64 --disable-x86asm
    --cc="$LCC" --cxx="$H4_LINUX_LLVM/bin/clang++ --target=x86_64-linux-gnu --sysroot=$H4_LINUX_SYSROOT"
    --ld="$LCC --ld-path=$H4_LINUX_LLD" --nm="$H4_LINUX_LLVM/bin/llvm-nm" --ar="$H4_LINUX_LLVM/bin/llvm-ar"
    --ranlib="$H4_LINUX_LLVM/bin/llvm-ranlib" --strip="$H4_LINUX_LLVM/bin/llvm-strip"
    --extra-cflags="-march=x86-64-v3" --host-cc=/usr/bin/clang)
else
  if [[ -z "${ANDROID_NDK:-}" ]]; then
    ANDROID_HOME="${ANDROID_HOME:-$HOME/Library/Android/sdk}"
    ANDROID_NDK="$(ls -d "$ANDROID_HOME"/ndk/* 2>/dev/null | sort -V | tail -1)"
  fi
  [[ -f "$ANDROID_NDK/build/cmake/android.toolchain.cmake" ]] ||
    { echo "no Android NDK (set ANDROID_NDK)" >&2; exit 2; }
  TC="$ANDROID_NDK/toolchains/llvm/prebuilt/darwin-x86_64"
  CMAKE_TARGET_ARGS=(-DCMAKE_TOOLCHAIN_FILE="$ANDROID_NDK/build/cmake/android.toolchain.cmake"
    -DANDROID_ABI=arm64-v8a -DANDROID_PLATFORM=android-$ANDROID_API -DANDROID_STL=c++_shared
    -DCMAKE_PREFIX_PATH="$PREFIX" -DCMAKE_FIND_ROOT_PATH="$PREFIX")
  FFMPEG_TARGET_ARGS=(--enable-cross-compile --target-os=android --arch=aarch64
    --cc="$TC/bin/aarch64-linux-android$ANDROID_API-clang"
    --cxx="$TC/bin/aarch64-linux-android$ANDROID_API-clang++"
    --nm="$TC/bin/llvm-nm" --ar="$TC/bin/llvm-ar" --ranlib="$TC/bin/llvm-ranlib"
    --strip="$TC/bin/llvm-strip" --sysroot="$TC/sysroot")
fi
export PKG_CONFIG_LIBDIR="$PREFIX/lib/pkgconfig"   # never the host's .pc files
unset PKG_CONFIG_PATH
