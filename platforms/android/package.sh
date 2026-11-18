#!/bin/bash
# The APK from a finished native build (platforms/build.sh android calls this):
#   platforms/android/package.sh <native build dir>  ->  .cache/outputs/android/dist/HeroesIV-android_arm64.apk
# Takes SDL's Java glue and Gradle wrapper from the SDL source platforms/deps.sh fetched (nothing of SDL
# is committed here), and the JDK from Homebrew (openjdk@17) unless JAVA_HOME is set.
set -euo pipefail
ROOT="$(cd "$(dirname "$0")/../.." && pwd)"
source "$ROOT/platforms/env.sh" android
B="${1:?native build dir}"
HERE="$ROOT/platforms/android"
SDL_SRC="$(ls -d "$ROOT"/.cache/shared/src/SDL3-* | sort -V | tail -1)"
JNI="$MOBILE_CACHE/jniLibs"
WRAP="$MOBILE_CACHE/gradle-wrapper"

rm -rf "$JNI"; mkdir -p "$JNI/arm64-v8a"
cp "$B/libmain.so" "$JNI/arm64-v8a/"
cp "$PREFIX/lib/libSDL3.so" "$JNI/arm64-v8a/"
cp "$TC/sysroot/usr/lib/aarch64-linux-android/libc++_shared.so" "$JNI/arm64-v8a/"

if [[ ! -x "$WRAP/gradlew" ]]; then
  mkdir -p "$WRAP"
  cp "$SDL_SRC/android-project/gradlew" "$WRAP/"
  cp -R "$SDL_SRC/android-project/gradle" "$WRAP/"
fi
# A JAVA_HOME left by a removed Android Studio is common: only a working one is kept.
[[ -x "${JAVA_HOME:-}/bin/java" ]] || export JAVA_HOME=/opt/homebrew/opt/openjdk@17
export ANDROID_HOME="${ANDROID_HOME:-$HOME/Library/Android/sdk}"
# SDL's Java, with one change: SDLThread (which runs the game) gets a 256 MiB stack, the size the
# runtime gives guest threads (process.cpp kHostStack). Android runs the game on it unswitched.
JAVA_SRC="$MOBILE_CACHE/sdl-java"
rm -rf "$JAVA_SRC"; cp -R "$SDL_SRC/android-project/app/src/main/java" "$JAVA_SRC"
A="$JAVA_SRC/org/libsdl/app/SDLActivity.java"
grep -q 'new Thread(new SDLMain(), "SDLThread")' "$A" || { echo "package: SDLActivity.java changed upstream"; exit 1; }
sed -i '' 's/new Thread(new SDLMain(), "SDLThread")/new Thread(null, new SDLMain(), "SDLThread", 256L << 20)/' "$A"
"$WRAP/gradlew" -q -p "$HERE" assembleRelease \
  -PsdlJava="$JAVA_SRC" -PjniDir="$JNI" -PassetsDir="${H4_ASSETS_DIR:-}" -PresDir="${H4_RES_DIR:-}"
# The release file's name (README): the ABI in it.
APK="$ROOT/.cache/outputs/android/dist/HeroesIV-android_arm64.apk"
mkdir -p "$(dirname "$APK")"
cp "$HERE/app/build/outputs/apk/release/app-release.apk" "$APK"
echo "android: $APK ($(du -h "$APK" | cut -f1))"
echo "install: adb install -r \"$APK\""
echo "game:    adb push game /sdcard/Android/data/games.hazzard.heroes4/files/"
