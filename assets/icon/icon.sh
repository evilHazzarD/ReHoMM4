#!/bin/bash
# icon.sh <platform> <out>: the app icon for platforms/build.sh, made from
# rehomm4.png. Its outer canvas is transparent; platform launchers apply
# their own required masks/backgrounds where applicable.
#   ios <dir>        <dir>/Assets.xcassets with AppIcon (1024 px, opaque)
#   android <dir>    <dir>: an adaptive launcher icon (extra res folder, platforms/android/package.sh)
#   macos <file>     AppIcon .icns (16..1024 px, the macOS grid)
#   linux <file>     256 px PNG
#   switch <file>    256 px JPEG (the .nro's icon)
set -euo pipefail
HERE="$(cd "$(dirname "$0")" && pwd)"
P="$1"; OUT="$2"
T="$(mktemp -d "${TMPDIR:-/tmp}/h4_icon.XXXXXX")"; trap 'rm -rf "$T"' EXIT
SOURCE="$HERE/rehomm4.png"
[[ -f "$SOURCE" ]] || { echo "missing project icon: $SOURCE" >&2; exit 1; }
resize_icon() { sips -z "$2" "$2" "$SOURCE" --out "$1" >/dev/null; }
case "$P" in
  ios)
    A="$OUT/Assets.xcassets"; rm -rf "$A"; mkdir -p "$A/AppIcon.appiconset"
    # A dedicated opaque square keeps the gold rim inside iOS's rounded
    # launcher mask instead of letting the system crop its outer points.
    sips -z 1024 1024 "$HERE/ios/rehomm4-ios.png" --out "$A/AppIcon.appiconset/icon-1024.png" >/dev/null
    echo '{ "info" : { "author" : "xcode", "version" : 1 } }' > "$A/Contents.json"
    cat > "$A/AppIcon.appiconset/Contents.json" <<'J'
{
  "images" : [ { "filename" : "icon-1024.png", "idiom" : "universal", "platform" : "ios", "size" : "1024x1024" } ],
  "info" : { "author" : "xcode", "version" : 1 }
}
J
    echo "$A" ;;
  android)
    # drawable-anydpi-v26/ic_launcher.xml overrides the placeholder drawable/ic_launcher.xml.
    rm -rf "$OUT"; mkdir -p "$OUT/drawable-nodpi" "$OUT/drawable-anydpi-v26" "$OUT/values"
    # 108 dp at xxxhdpi; the launcher's mask (a circle, a squircle) shows its middle 72 dp and
    # keeps whole only the middle 66 dp circle: the round emblem drawn that size, in the middle.
    resize_icon "$T/fg.png" 264
    magick -size 432x432 xc:none "$T/fg.png" -gravity center -composite "$OUT/drawable-nodpi/ic_launcher_fg.png"
    cat > "$OUT/values/ic_launcher_bg.xml" <<'X'
<?xml version="1.0" encoding="utf-8"?>
<resources><color name="ic_launcher_bg">#1A1209</color></resources>
X
    cat > "$OUT/drawable-anydpi-v26/ic_launcher.xml" <<'X'
<?xml version="1.0" encoding="utf-8"?>
<adaptive-icon xmlns:android="http://schemas.android.com/apk/res/android">
    <background android:drawable="@color/ic_launcher_bg" />
    <foreground android:drawable="@drawable/ic_launcher_fg" />
</adaptive-icon>
X
    # The Android TV launcher's banner (320x180 dp, at xhdpi): the emblem and the name.
    mkdir -p "$OUT/drawable-xhdpi"
    resize_icon "$T/banner_fg.png" 360
    swift "$HERE/banner.swift" "$T/banner_fg.png" "$OUT/drawable-xhdpi/banner.png"
    echo "$OUT" ;;
  macos)
    resize_icon "$T/mac.png" 1024
    mkdir -p "$T/i.iconset"
    for s in 16 32 128 256 512; do
      sips -z $s $s "$T/mac.png" --out "$T/i.iconset/icon_${s}x${s}.png" >/dev/null
      sips -z $((s * 2)) $((s * 2)) "$T/mac.png" --out "$T/i.iconset/icon_${s}x${s}@2x.png" >/dev/null
    done
    iconutil -c icns -o "$OUT" "$T/i.iconset"
    echo "$OUT" ;;
  linux)
    resize_icon "$OUT" 256
    echo "$OUT" ;;
  switch)
    resize_icon "$T/sw.png" 256
    sips -s format jpeg -s formatOptions 92 "$T/sw.png" --out "$OUT" >/dev/null
    echo "$OUT" ;;
  *) echo "icon.sh: ios|android|macos|linux|switch <out>" >&2; exit 2 ;;
esac
