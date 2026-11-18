#!/bin/bash
# icon.sh <platform> <out>: the app icon for platforms/build.sh, drawn by icon.swift (the project's own
# art: a shield with "IV"; nothing from the game).
#   ios <dir>        <dir>/Assets.xcassets with AppIcon (1024 px, opaque)
#   android <dir>    <dir>: an adaptive launcher icon (extra res folder, platforms/android/package.sh)
#   macos <file>     AppIcon .icns (16..1024 px, the macOS grid)
#   linux <file>     256 px PNG
#   switch <file>    256 px JPEG (the .nro's icon)
set -euo pipefail
HERE="$(cd "$(dirname "$0")" && pwd)"
P="$1"; OUT="$2"
T="$(mktemp -d "${TMPDIR:-/tmp}/h4_icon.XXXXXX")"; trap 'rm -rf "$T"' EXIT
draw() { swift "$HERE/icon.swift" "$@"; }
case "$P" in
  ios)
    A="$OUT/Assets.xcassets"; rm -rf "$A"; mkdir -p "$A/AppIcon.appiconset"
    draw "$A/AppIcon.appiconset/icon-1024.png" full 1024
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
    draw "$OUT/drawable-nodpi/ic_launcher_fg.png" fg 432  # 108 dp at xxxhdpi
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
    echo "$OUT" ;;
  macos)
    draw "$T/mac.png" mac 1024
    mkdir -p "$T/i.iconset"
    for s in 16 32 128 256 512; do
      sips -z $s $s "$T/mac.png" --out "$T/i.iconset/icon_${s}x${s}.png" >/dev/null
      sips -z $((s * 2)) $((s * 2)) "$T/mac.png" --out "$T/i.iconset/icon_${s}x${s}@2x.png" >/dev/null
    done
    iconutil -c icns -o "$OUT" "$T/i.iconset"
    echo "$OUT" ;;
  linux)
    draw "$OUT" plain 256
    echo "$OUT" ;;
  switch)
    draw "$T/sw.png" full 256
    sips -s format jpeg -s formatOptions 92 "$T/sw.png" --out "$OUT" >/dev/null
    echo "$OUT" ;;
  *) echo "icon.sh: ios|android|macos|linux|switch <out>" >&2; exit 2 ;;
esac
