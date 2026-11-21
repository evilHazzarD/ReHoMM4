#!/bin/bash
# Packs the Linux build (platforms/build.sh linux) into a folder and a .tar.gz:
#
#   Heroes IV/heroes4            the game (with the in-app installer)
#   Heroes IV/lib/libSDL3.so.0   SDL3 (found through the binary's $ORIGIN/lib)
#   Heroes IV/wings/, <code>.h4p the side panels and the translations built in (platforms/languages.py)
#   Heroes IV/ui/                the language button's pictures (platforms/linux/render_ui.swift)
#   Heroes IV/heroes4.png        the shield icon (assets/icon)
#   Heroes IV/THIRD_PARTY_NOTICES.txt   the licenses (platforms/notices.py)
#   Heroes IV/install-desktop-entry.sh   adds it to the desktop's menu (Steam Deck: Desktop Mode)
#
#   platforms/linux/package.sh <build dir>
set -euo pipefail
ROOT="$(cd "$(dirname "$0")/../.." && pwd)"
source "$ROOT/platforms/env.sh" linux
B="$1"
OUT="$MOBILE_CACHE/dist"
D="$OUT/Heroes IV"
rm -rf "$OUT"; mkdir -p "$D/lib"
"$H4_LINUX_LLVM/bin/llvm-strip" --strip-debug -o "$D/heroes4" "$B/heroes4"
chmod 755 "$D/heroes4"
cp -L "$PREFIX/lib/libSDL3.so.0" "$D/lib/libSDL3.so.0"
[[ -d "$MOBILE_CACHE/res/wings" ]] && cp -R "$MOBILE_CACHE/res/wings" "$D/wings"
for p in "$MOBILE_CACHE"/res/*.h4p; do [[ -f "$p" ]] || continue; cp "$p" "$D/"; done  # the translations in the app
cp "$MOBILE_CACHE/res/THIRD_PARTY_NOTICES.txt" "$D/THIRD_PARTY_NOTICES.txt"
# The language button and its note, drawn into the frame (h4/src/menu_button_linux.cpp).
swift "$ROOT/platforms/linux/render_ui.swift" "$D/ui" "${H4_VERSION:-}"
bash "$ROOT/assets/icon/icon.sh" linux "$D/heroes4.png" >/dev/null
cat > "$D/install-desktop-entry.sh" <<'SH'
#!/bin/sh
# Adds Heroes IV to the desktop's application menu (from where it is now).
here="$(cd "$(dirname "$0")" && pwd)"
dir="${XDG_DATA_HOME:-$HOME/.local/share}/applications"
mkdir -p "$dir"
cat > "$dir/heroes4-rehomm4.desktop" <<EOF2
[Desktop Entry]
Type=Application
Name=Heroes of Might and Magic IV
Exec="$here/heroes4"
Path=$here
Icon=$here/heroes4.png
Categories=Game;StrategyGame;
Terminal=false
EOF2
echo "added: $dir/heroes4-rehomm4.desktop"
SH
chmod 755 "$D/install-desktop-entry.sh"
cat > "$D/README.txt" <<'TXT'
Heroes of Might and Magic IV for Linux and the Steam Deck (ReHoMM4)

Run ./heroes4. The first time it installs the game from your GOG copy: download the Windows
installer from your GOG library (setup_heroes_of_might_and_magic_4_complete_...exe and its -1.bin)
into ~/Downloads; the game finds them there (or pick both). It is unpacked into
~/.local/share/ReHoMM4/game; saves and settings go to ~/.local/share/ReHoMM4/player.

Steam Deck: in Desktop Mode, unpack this folder (e.g. to ~/Games), then in Steam: Add a Game >
Add a Non-Steam Game > Browse > heroes4. Back in Game Mode it is in your library. The trackpad
and touch screen move the pointer (Steam's default for non-Steam games), R2/right trackpad click
is the left button.

Language: Steam > the game > Properties > Launch options: --lang=<code> (uk Ukrainian, ...) or --lang=en.
TXT
tar -czf "$OUT/HeroesIV-linux-x86_64.tar.gz" -C "$OUT" "Heroes IV"
echo "linux: $OUT/HeroesIV-linux-x86_64.tar.gz ($(du -sh "$OUT/HeroesIV-linux-x86_64.tar.gz" | cut -f1))"
