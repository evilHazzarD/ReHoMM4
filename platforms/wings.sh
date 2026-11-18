#!/bin/bash
# wings.sh <out dir>: <out dir>/wings/*.jpg (quality H4_WINGS_QUALITY, 90), the side panels
# beside the game's centred 1280-wide screens (main menu, towns, combat: h4/src/resolution.cpp),
# and wings/index.txt listing them.
# The art is the project's own (art/combat_wings, from heroes4-mac-assets.zip on the Releases page,
# as the Mac build's platforms/macos-dmg/install_app.sh uses it); never committed. Prints nothing and makes no
# folder when there is no art: the app then shows black sides.
set -euo pipefail
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
OUT="$1/wings"
A="$ROOT/art/combat_wings"
if [[ ! -d "$A" ]]; then  # as tools/build.sh: the zip (or Safari's unpacked folder) in Downloads
  assets="$(ls -td "$HOME"/Downloads/heroes4-mac-assets* 2>/dev/null | head -1 || true)"
  if [[ -d "$assets" ]]; then ditto "$assets/art/combat_wings" "$A" 2>/dev/null || true
  elif [[ -f "$assets" ]]; then unzip -qo "$assets" 'art/combat_wings/*' -d "$ROOT" || true; fi
fi
rm -rf "$OUT"
[[ -d "$A" ]] || exit 0
mkdir -p "$OUT"
one() {  # <png> <name>
  sips -s format jpeg -s formatOptions "${H4_WINGS_QUALITY:-90}" "$1" --out "$OUT/$2.jpg" >/dev/null
}
for w in paradise hell; do one "$A/${w}_296x2048.png" "$w"; done
for f in "$A"/factions/*_light.png "$A"/factions/*_dark.png; do one "$f" "$(basename "$f" .png)"; done
(cd "$OUT" && ls *.jpg > index.txt)
echo "$OUT"
