#!/bin/bash
# Builds Heroes IV for a phone or tablet from your own GOG copy (platforms/README.md).
#
#   platforms/build.sh ios [TEAM_ID]   ->  .cache/outputs/ios/dist/HeroesIV-ios.ipa, from
#                                       .cache/outputs/ios/build/Release-iphoneos/ReHoMM4.app
#                                       (+ the Xcode project h4_recomp.xcodeproj next to it)
#   platforms/build.sh android         ->  .cache/outputs/android/dist/HeroesIV-android_arm64.apk
#   platforms/build.sh linux-x86_64    ->  .cache/outputs/linux-x86_64/dist/HeroesIV-linux-x86_64.tar.gz
#                                       (Steam Deck; `linux` is the same)
#   platforms/build.sh linux-arm64     ->  .cache/outputs/linux-arm64/dist/HeroesIV-linux-arm64.tar.gz
#   platforms/build.sh windows-x86_64  ->  .cache/outputs/windows-x86_64/dist/HeroesIV-windows-x86_64.zip
#   platforms/build.sh switch          ->  .cache/outputs/switch/dist/ReHoMM4.nro (Switch homebrew)
#   platforms/build.sh switch2         ->  .cache/outputs/switch2/dist/ReHoMM4.nro (requires ounce-devkit)
#   platforms/build.sh macos-arm64     ->  .cache/outputs/macos-arm64/dist/ReHoMM4.app (+ HeroesIV-macos-arm64.dmg)
#   platforms/build.sh macos-x86_64    ->  .cache/outputs/macos-x86_64/dist/ReHoMM4.app (+ HeroesIV-macos-x86_64.dmg, Intel Macs)
#
# Needs what tools/build.sh made first: game/ (your GOG install), h4/generated/ (lifted heroes4.exe, kept up to date by tools/lift.sh)
# and third_party/x86recomp with patches/ applied (incl. x86recomp-mobile.patch) and its host build
# (recomp_lift, the fetched softfloat/core-math). Then platforms/deps.sh for the target.
# iOS without TEAM_ID: compiled but unsigned; with it (or H4_TEAM_ID), signed for your device.
set -euo pipefail
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
source "$ROOT/platforms/env.sh" "${1:-}"
X="$ROOT/third_party/x86recomp"
HOST_DEPS="$X/out/build/release/_deps"
die() { printf 'error: %s\n' "$*" >&2; exit 1; }

# h4/generated lifted again when what it is made from changed (tools/lift.sh).
bash "$ROOT/tools/lift.sh" || die "lifting heroes4.exe failed (tools/lift.sh)"
[[ -d "$HOST_DEPS/softfloat-src" ]] || die "no host build of x86recomp: run tools/build.sh first"
if [[ ! -f "$PREFIX/lib/libavcodec.a" || ! -f "$PREFIX/lib/librecomp_innoextract.a" ]] ||
   { [[ $TARGET == switch2 ]] &&
     ! cmp -s "$ROOT/platforms/switch/sdl-switch2-display.patch" "$PREFIX/share/sdl-switch2-display.patch"; }; then
  bash "$ROOT/platforms/deps.sh" "$TARGET"
fi

# The host build's sources, not a second download (core-math's server is slow).
FETCH=(-DFETCHCONTENT_SOURCE_DIR_SOFTFLOAT="$HOST_DEPS/softfloat-src"
       -DFETCHCONTENT_SOURCE_DIR_COREMATH="$HOST_DEPS/coremath-src"
       -DFETCHCONTENT_FULLY_DISCONNECTED=ON)
# The shaders are compiled at build time by the host's tools.
HOSTTOOLS=(-DRECOMP_GLSLANG="$(command -v glslangValidator)")
[[ $TARGET == ios || $TARGET == macos-* ]] && HOSTTOOLS+=(-DRECOMP_SPIRV_CROSS="$(command -v spirv-cross)")

B="$MOBILE_CACHE/build"
# The side panels beside the centred screens (platforms/wings.sh; the project's art, never committed).
WINGS="$(bash "$ROOT/platforms/wings.sh" "$MOBILE_CACHE/res")"
[[ -n "$WINGS" ]] || echo "warning: no art/combat_wings (HOMM4-extra-assets.zip: platforms/extra-assets.sh): black sides"
# The translations: released on their own from the ReHoMM4-translations repository. The app starts with
# the latest release's list and checks for a newer one at every start (platforms/languages.py).
LANGS="$ROOT/.cache/shared/languages"
LANG_URL="$(python3 -I "$ROOT/platforms/languages.py" "$LANGS" "$MOBILE_CACHE/res")"
LANG_ARGS=(-DH4_LANGUAGES_INC="$LANGS/languages.inc" -DH4_TRANSLATIONS_URL="$LANG_URL")
if [[ $TARGET == ios ]]; then
  TEAM="${2:-${H4_TEAM_ID:-}}"
  # The app icon (assets/icon: the project's own shield, drawn at build time).
  bash "$ROOT/assets/icon/icon.sh" ios "$MOBILE_CACHE" >/dev/null
  ASSETS=(-DH4_IOS_ASSETS="$MOBILE_CACHE/Assets.xcassets")
  # The licenses (platforms/notices.py), from the Xcode project's link line once it is made.
  NOTICES="$MOBILE_CACHE/res/THIRD_PARTY_NOTICES.txt"
  mkdir -p "$MOBILE_CACHE/res"; [[ -f "$NOTICES" ]] || : > "$NOTICES"
  cmake -S "$ROOT/h4" -B "$B" -G Xcode "${CMAKE_TARGET_ARGS[@]}" "${FETCH[@]}" "${HOSTTOOLS[@]}" \
    -DH4_TEAM_ID="$TEAM" "${ASSETS[@]}" -DH4_IOS_WINGS="$WINGS" -DH4_IOS_NOTICES="$NOTICES" -DH4_IOS_PACKS_DIR="$MOBILE_CACHE/res" "${LANG_ARGS[@]}" -DCMAKE_XCODE_ATTRIBUTE_ONLY_ACTIVE_ARCH=YES >"$B.configure.log" 2>&1 ||
    { tail -40 "$B.configure.log"; exit 1; }
  python3 -I "$ROOT/platforms/notices.py" ios "$B" "$NOTICES" >/dev/null
  sign=(CODE_SIGNING_ALLOWED=NO)
  [[ -n "$TEAM" ]] && sign=(-allowProvisioningUpdates DEVELOPMENT_TEAM="$TEAM")
  xcodebuild -project "$B/h4_recomp.xcodeproj" -scheme h4 -configuration Release \
    -destination 'generic/platform=iOS' -jobs "$JOBS" "${sign[@]}" \
    SYMROOT="$B" build >"$B.build.log" 2>&1 ||
    { grep -E 'error:|BUILD FAILED' "$B.build.log" | head -40; echo "log: $B.build.log"; exit 1; }
  APP="$B/Release-iphoneos/ReHoMM4.app"
  echo "ios: $APP ($(du -sh "$APP" | cut -f1))"
  # The .ipa (Payload/ReHoMM4.app, zipped): for sideloading (AltStore, Sideloadly re-sign it).
  IPA="$MOBILE_CACHE/dist/HeroesIV-ios.ipa"
  rm -rf "$MOBILE_CACHE/dist"; mkdir -p "$MOBILE_CACHE/dist/Payload"
  ditto "$APP" "$MOBILE_CACHE/dist/Payload/ReHoMM4.app"
  (cd "$MOBILE_CACHE/dist" && zip -qry -X "$IPA" Payload) && rm -rf "$MOBILE_CACHE/dist/Payload"
  cp "$NOTICES" "$MOBILE_CACHE/dist/THIRD_PARTY_NOTICES.txt"
  echo "ios: $IPA ($(du -h "$IPA" | cut -f1))"
  if [[ -n "$TEAM" ]]; then
    echo "install: connect the device, then  xcrun devicectl device install app --device <id> \"$APP\""
    echo "         (xcrun devicectl list devices)  or open $B/h4_recomp.xcodeproj and Run"
  else
    echo "unsigned: pass your team ID (Xcode > Settings > Accounts) to sign it for your device"
  fi
elif [[ $TARGET == android ]]; then
  cmake -S "$ROOT/h4" -B "$B" -G Ninja -DCMAKE_BUILD_TYPE=RelWithDebInfo "${CMAKE_TARGET_ARGS[@]}" \
    "${FETCH[@]}" "${HOSTTOOLS[@]}" "${LANG_ARGS[@]}" >"$B.configure.log" 2>&1 || { tail -40 "$B.configure.log"; exit 1; }
  # Its own log: a failed compile must stop here, not leave the last build's binary to be packed.
  build_ok=1; cmake --build "$B" --target h4 -j"$JOBS" >"$B.build.log" 2>&1 || build_ok=0
  { grep -E 'error|FAILED|Linking' "$B.build.log" || true; } | head -40
  [[ $build_ok == 1 ]] || die "the build failed (log: $B.build.log)"
  [[ -f "$B/libmain.so" ]] || die "libmain.so was not built (log above)"
  python3 -I "$ROOT/platforms/notices.py" android "$B" "$MOBILE_CACHE/res/THIRD_PARTY_NOTICES.txt" >/dev/null
  # The launcher icon (assets/icon, as the iOS one).
  ICON_RES="$(bash "$ROOT/assets/icon/icon.sh" android "$MOBILE_CACHE/icon-res")"
  H4_ASSETS_DIR="$MOBILE_CACHE/res" H4_RES_DIR="$ICON_RES" \
    bash "$ROOT/platforms/android/package.sh" "$B"
fi
if [[ $TARGET == switch* ]]; then
  cmake -S "$ROOT/h4" -B "$B" -G Ninja -DCMAKE_BUILD_TYPE=Release "${CMAKE_TARGET_ARGS[@]}" \
    "${FETCH[@]}" "${HOSTTOOLS[@]}" "${LANG_ARGS[@]}" -DH4_SWITCH_APP=ON -DRECOMP_STATIC_APP=ON \
    -DRECOMP_GPU=OFF >"$B.configure.log" 2>&1 || { tail -40 "$B.configure.log"; exit 1; }
  cmake --build "$B" --target h4 -j"$JOBS" >"$B.build.log" 2>&1 ||
    { tail -60 "$B.build.log"; exit 1; }
  [[ -f "$B/heroes4.elf" ]] || die "heroes4.elf was not built (log above)"
  python3 -I "$ROOT/platforms/notices.py" "$TARGET" "$B" "$MOBILE_CACHE/res/THIRD_PARTY_NOTICES.txt" >/dev/null
  bash "$ROOT/platforms/switch/package.sh" "$B" "$TARGET"
fi
if [[ $TARGET == linux* || $TARGET == macos-* || $TARGET == windows-* ]]; then
  desktop_args=(-DH4_DESKTOP_APP=ON -DRECOMP_STATIC_APP=ON)
  # Windows uses its own native installer/platform layer (WinHTTP and the standard file picker).
  [[ $TARGET == windows-* ]] && desktop_args=(-DH4_WINDOWS_APP=ON -DRECOMP_STATIC_APP=ON)
  cmake -S "$ROOT/h4" -B "$B" -G Ninja -DCMAKE_BUILD_TYPE=RelWithDebInfo "${CMAKE_TARGET_ARGS[@]}" \
    "${FETCH[@]}" "${HOSTTOOLS[@]}" "${LANG_ARGS[@]}" "${desktop_args[@]}" \
    >"$B.configure.log" 2>&1 || { tail -40 "$B.configure.log"; exit 1; }
  # Its own log: a failed compile must stop here, not leave the last build's binary to be packed.
  build_ok=1; cmake --build "$B" --target h4 -j"$JOBS" >"$B.build.log" 2>&1 || build_ok=0
  { grep -E 'error|FAILED|Linking' "$B.build.log" || true; } | head -40
  [[ $build_ok == 1 ]] || die "the build failed (log: $B.build.log)"
  [[ -f "$B/heroes4" || -f "$B/heroes4.exe" ]] || die "heroes4 was not built (log above)"
  notice_target="${TARGET%%-*}"; [[ $TARGET == windows-* ]] && notice_target=windows
  python3 -I "$ROOT/platforms/notices.py" "$notice_target" "$B" "$MOBILE_CACHE/res/THIRD_PARTY_NOTICES.txt" >/dev/null
  if [[ $TARGET == linux* ]]; then bash "$ROOT/platforms/linux/package.sh" "$B" "$TARGET"
  elif [[ $TARGET == windows-* ]]; then bash "$ROOT/platforms/windows/package.sh" "$B" "$TARGET"
  else bash "$ROOT/platforms/macos/package.sh" "$B" "$TARGET"; fi
fi
