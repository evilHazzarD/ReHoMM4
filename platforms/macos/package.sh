#!/bin/bash
# Packs the Mac build (platforms/build.sh macos-arm64|macos-x86_64) into ReHoMM4.app and a DMG:
#
#   ReHoMM4.app/Contents/MacOS/ReHoMM4           the game (with the in-app installer)
#   ReHoMM4.app/Contents/Resources/wings, <code>.h4p  the side panels and the translations built in
#   ReHoMM4.app/Contents/Resources/AppIcon.icns   the shield icon (assets/icon)
#   ReHoMM4.app/Contents/Resources/THIRD_PARTY_NOTICES.txt   the licenses (platforms/notices.py)
#   the DMG: the app, an Applications link and "Third-party notices.txt"
#
# Ad-hoc signed (as any app built on this Mac); on another Mac: right-click > Open the first time.
#
#   platforms/macos/package.sh <build dir> macos-arm64|macos-x86_64
set -euo pipefail
ROOT="$(cd "$(dirname "$0")/../.." && pwd)"
source "$ROOT/platforms/env.sh" "${2:-macos-arm64}"
B="$1"
OUT="$MOBILE_CACHE/dist"
APP="$OUT/ReHoMM4.app"
rm -rf "$OUT"; mkdir -p "$APP/Contents/MacOS" "$APP/Contents/Resources"
strip -S -o "$APP/Contents/MacOS/ReHoMM4" "$B/heroes4"
[[ -d "$MOBILE_CACHE/res/wings" ]] && cp -R "$MOBILE_CACHE/res/wings" "$APP/Contents/Resources/wings"
for p in "$MOBILE_CACHE"/res/*.h4p; do [[ -f "$p" ]] || continue; cp "$p" "$APP/Contents/Resources/"; done  # the translations in the app
cp "$MOBILE_CACHE/res/THIRD_PARTY_NOTICES.txt" "$APP/Contents/Resources/THIRD_PARTY_NOTICES.txt"
bash "$ROOT/assets/icon/icon.sh" macos "$APP/Contents/Resources/AppIcon.icns" >/dev/null
ICON=AppIcon
cat > "$APP/Contents/Info.plist" <<PLIST
<?xml version="1.0" encoding="UTF-8"?>
<!DOCTYPE plist PUBLIC "-//Apple//DTD PLIST 1.0//EN" "http://www.apple.com/DTDs/PropertyList-1.0.dtd">
<plist version="1.0"><dict>
  <key>CFBundleName</key><string>ReHoMM4</string>
  <key>CFBundleDisplayName</key><string>ReHoMM4</string>
  <key>CFBundleIdentifier</key><string>games.hazzard.heroes4</string>
  <key>CFBundleExecutable</key><string>ReHoMM4</string>
  <key>CFBundlePackageType</key><string>APPL</string>
  <key>CFBundleShortVersionString</key><string>$H4_VERSION</string>
  <key>CFBundleVersion</key><string>$H4_BUILD</string>
  <key>CFBundleIconFile</key><string>$ICON</string>
  <key>LSMinimumSystemVersion</key><string>13.0</string>
  <key>LSApplicationCategoryType</key><string>public.app-category.strategy-games</string>
  <key>NSHighResolutionCapable</key><true/>
  <key>NSSupportsAutomaticGraphicsSwitching</key><true/>
</dict></plist>
PLIST
codesign --force --sign - "$APP" >/dev/null 2>&1
( cd "$OUT" && ditto -c -k --keepParent ReHoMM4.app HeroesIV-macos-$MAC_ARCH.zip )
STAGE="$MOBILE_CACHE/dmg-stage"; rm -rf "$STAGE"; mkdir -p "$STAGE"
ditto "$APP" "$STAGE/ReHoMM4.app"
ln -s /Applications "$STAGE/Applications"
cp "$MOBILE_CACHE/res/THIRD_PARTY_NOTICES.txt" "$STAGE/Third-party notices.txt"
hdiutil create -quiet -fs HFS+ -volname "ReHoMM4" -srcfolder "$STAGE" -ov -format UDZO "$OUT/HeroesIV-macos-$MAC_ARCH.dmg"
rm -rf "$STAGE"
echo "macos: $APP"
echo "macos: $OUT/HeroesIV-macos-$MAC_ARCH.dmg ($(du -sh "$OUT/HeroesIV-macos-$MAC_ARCH.dmg" | cut -f1))"
