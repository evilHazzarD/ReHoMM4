#!/bin/bash
# Contains code from HoMM4Deadware (https://github.com/LeoDeadware/HoMM4Deadware): Copyright (c) 2026 LeoDeadware, MIT License.
# Release gate (kit RULES R-L1/R-L2): a .app/.dmg/zip for anyone but the owner must contain NO game
# data and nothing derived from it (GOG files: .exe/.dll/.h4r/.aop/.ico/.pdf/.bik..., a game/ tree,
# the translations' files made from the game: their build folders (loc/build, loc-build), a
# language pack .h4p: the packs reach players only from the ReHoMM4-translations releases).
#   tools/release/release_check.sh path/to/Heroes\ IV.app|.dmg|.zip
# ALLOW_HD_PACKS=1 (only platforms/macos-dmg/make_dmg.sh, R-L5): Heroes IV.app/Contents/Resources/mods/hd_* allowed.
# Heroes IV has no HD packs (2D art scaled by FSR at run time), so nothing is allowed by it today.
set -uo pipefail
t="$1"; tmp=""
case "$t" in
  *.dmg) tmp="$(mktemp -d)"; hdiutil attach -nobrowse -readonly -mountpoint "$tmp" "$t" >/dev/null || exit 2; root="$tmp" ;;
  *.zip) tmp="$(mktemp -d)"; unzip -q "$t" -d "$tmp"; root="$tmp" ;;
  *) root="$t" ;;
esac
bad="$(find "$root" \( -iname '*.exe' -o -iname '*.dll' -o -iname '*.h4r' -o -iname '*.aop' -o -iname '*.asi' \
  -o -iname '*.ico' -o -iname '*.bik' -o -iname '*.pdf' -o -iname '*.chm' -o -iname '*.h4c' -o -iname '*.h4s' \
  -o -path '*/game/*' -o -path '*/game-*/*' -o -path '*/loc/build/*' -o -path '*/loc-build/*' -o -iname '*.h4p' \
  -o -path '*/mods/hd_*' -o -iname '*.gpr' \) -print 2>/dev/null)"
[[ "${ALLOW_HD_PACKS:-0}" == 1 ]] && bad="$(grep -vE "^$root/Heroes IV\.app/Contents/Resources/mods/hd_[a-z0-9_]+(/|\$)" <<< "$bad")"
[[ "$t" == *.dmg ]] && hdiutil detach -quiet "$tmp"
[[ -n "$tmp" && "$t" == *.zip ]] && rm -rf "$tmp"
if [[ -n "$bad" ]]; then echo "release_check: FAIL — game data / derived content:"; head -20 <<< "$bad"; exit 1; fi
echo "release_check: PASS (no game data found in $t)"
