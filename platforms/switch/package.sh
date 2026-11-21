#!/bin/bash
# Packs the Switch build (platforms/build.sh switch) into a homebrew .nro:
#
#   HeroesIV.nro        heroes4.elf (stripped) + its romfs: wings/, ui/ (the language button's
#                       pictures), the translations built in (<code>.h4p), THIRD_PARTY_NOTICES.txt; the shield icon (assets/icon).
#   THIRD_PARTY_NOTICES.txt   the licenses (platforms/notices.py), beside it too
#
# On the Switch (Atmosphere): copy it to sdmc:/switch/ReHoMM4/HeroesIV.nro, the GOG
# installer files beside it, and start it from the homebrew menu in title takeover (hold R
# while starting a game: the album applet has too little memory for the game).
#
#   platforms/switch/package.sh <build dir>
set -euo pipefail
ROOT="$(cd "$(dirname "$0")/../.." && pwd)"
source "$ROOT/platforms/env.sh" switch
B="$1"
OUT="$MOBILE_CACHE/dist"
RF="$MOBILE_CACHE/romfs"
T="$DEVKITPRO/tools/bin"
rm -rf "$OUT" "$RF"; mkdir -p "$OUT" "$RF"
[[ -d "$MOBILE_CACHE/res/wings" ]] && cp -R "$MOBILE_CACHE/res/wings" "$RF/wings"
for p in "$MOBILE_CACHE"/res/*.h4p; do [[ -f "$p" ]] || continue; cp "$p" "$RF/"; done  # the translations in the app
cp "$MOBILE_CACHE/res/THIRD_PARTY_NOTICES.txt" "$RF/THIRD_PARTY_NOTICES.txt"
cp "$MOBILE_CACHE/res/THIRD_PARTY_NOTICES.txt" "$OUT/THIRD_PARTY_NOTICES.txt"
swift "$ROOT/platforms/linux/render_ui.swift" "$RF/ui" "${H4_VERSION:-}"
"${SW}strip" -o "$MOBILE_CACHE/heroes4.stripped.elf" "$B/heroes4.elf"
# The icon: 256x256 JPEG.
ICON="$MOBILE_CACHE/icon.jpg"
bash "$ROOT/assets/icon/icon.sh" switch "$ICON" >/dev/null
"$T/nacptool" --create "Heroes IV" "ReHoMM4" "$H4_VERSION" "$MOBILE_CACHE/heroes4.nacp" >/dev/null
"$T/elf2nro" "$MOBILE_CACHE/heroes4.stripped.elf" "$OUT/HeroesIV.nro" --icon="$ICON" \
  --nacp="$MOBILE_CACHE/heroes4.nacp" --romfsdir="$RF" >/dev/null
rm -f "$MOBILE_CACHE/heroes4.stripped.elf"
echo "switch: $OUT/HeroesIV.nro ($(du -sh "$OUT/HeroesIV.nro" | cut -f1))"
