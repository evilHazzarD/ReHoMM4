#!/bin/bash
# Package the portable MinGW Windows build made by platforms/build.sh.
set -euo pipefail
ROOT="$(cd "$(dirname "$0")/../.." && pwd)"
source "$ROOT/platforms/env.sh" "${2:-windows-x86_64}"
B="$1"
OUT="$MOBILE_CACHE/dist"
D="$OUT/ReHoMM4"
rm -rf "$OUT"; mkdir -p "$D"
cp "$B/heroes4.exe" "$D/ReHoMM4.exe"
# SDL3 is the one runtime DLL we build and ship; the remaining libraries are linked statically.
SDL_DLL="$(find "$PREFIX" -maxdepth 3 -iname 'SDL3.dll' -type f -print -quit)"
[[ -n "$SDL_DLL" ]] || { echo "no SDL3.dll in $PREFIX" >&2; exit 1; }
cp "$SDL_DLL" "$D/SDL3.dll"
# Homebrew/MinGW links this threading runtime dynamically; Windows does not
# provide it, so it must travel beside the executable.
WINPTHREAD="$(x86_64-w64-mingw32-g++ -print-sysroot)/x86_64-w64-mingw32/bin/libwinpthread-1.dll"
[[ -f "$WINPTHREAD" ]] || { echo "no libwinpthread-1.dll in the MinGW toolchain" >&2; exit 1; }
cp "$WINPTHREAD" "$D/libwinpthread-1.dll"
cp "$MOBILE_CACHE/res/THIRD_PARTY_NOTICES.txt" "$D/THIRD_PARTY_NOTICES.txt"
cat > "$D/README.txt" <<'TXT'
Heroes of Might and Magic IV for Windows (ReHoMM4)

This archive contains no game files. Buy Heroes of Might and Magic IV: Complete from GOG, then
start ReHoMM4.exe. The first start installs the game from your GOG account, GOG's offline
installer files, or an already installed game folder. Saves and settings are in
%LOCALAPPDATA%\\ReHoMM4; save sync uses Documents\\ReHOMM4 (or `folder=` in player\\sync.conf).
TXT
(cd "$OUT" && zip -qry "HeroesIV-windows-x86_64.zip" ReHoMM4)
echo "$TARGET: $OUT/HeroesIV-windows-x86_64.zip ($(du -sh "$OUT/HeroesIV-windows-x86_64.zip" | cut -f1))"
