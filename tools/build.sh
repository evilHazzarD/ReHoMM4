#!/bin/bash
# Contains code from HoMM4Deadware (https://github.com/LeoDeadware/HoMM4Deadware): Copyright (c) 2026 LeoDeadware, MIT License.
# Developer build (players: download Heroes IV.dmg from Releases instead). From your own GOG installer
# of Heroes of Might and Magic IV Complete to the same DMG we release:
#
#   tools/build.sh [setup_heroes_of_might_and_magic_4_complete_*.exe]   ->  .cache/dmg/Heroes IV.dmg
#
# Steps (each skipped when done): Homebrew tools; the game files in game/ (local only, never committed);
# x86recomp at the pinned commit + patches/; the LGPL ffmpeg (platforms/macos-dmg/ffmpeg_lgpl.sh: Homebrew's is
# GPL); lift heroes4.exe and build h4 against it; our art (HOMM4-extra-assets.zip from Releases, in Downloads);
# then platforms/macos-dmg/make_dmg.sh. The DMG carries no game code and no game data: it builds the game on the
# player's Mac. Needs Apple's Command Line Tools and Homebrew; macOS 26 or later, Apple Silicon.
set -euo pipefail

H4PORT="$(cd "$(dirname "$0")/.." && pwd)"
X86RECOMP=https://github.com/JTM-rootstorm/x86recomp.git
X86RECOMP_COMMIT=1c90fe3a5dad37d0cca551fc9c8ab66a106ede1e
# heroes4.exe of GOG's "Complete 3.0" (setup_..._3.0_win11_gog_0.2_(76223)).
EXE_SHA256=b7c2f6caeeba640b1c5f1d812b172938bb4dd4ec62e34ec93ad827bd1ffaec87

say() { printf '\n== %s\n' "$*"; }
die() { printf 'error: %s\n' "$*" >&2; exit 1; }

[[ "$(uname -m)" == arm64 ]] || die "needs an Apple Silicon Mac"
xcode-select -p >/dev/null 2>&1 || { xcode-select --install; die "install the Command Line Tools (a window opened), then run this again"; }
[[ -x /opt/homebrew/bin/brew ]] && eval "$(/opt/homebrew/bin/brew shellenv)"
command -v brew >/dev/null || die "install Homebrew first: https://brew.sh"

say "Tools (Homebrew)"
brew install cmake ninja pkgconf sdl3 glslang spirv-cross spirv-tools innoextract llvm@22

say "Game files"
if [[ ! -f "$H4PORT/game/heroes4.exe" ]]; then
  setup="${1:-$(ls -t "$HOME"/Downloads/setup_heroes_of_might_and_magic_4_complete*.exe 2>/dev/null | head -1 || true)}"
  [[ -n "$setup" && -f "$setup" ]] || die "no GOG installer (setup_heroes_of_might_and_magic_4_complete_....exe) in Downloads"
  innoextract -s -d "$H4PORT/game" "$setup"
fi
[[ "$(shasum -a 256 "$H4PORT/game/heroes4.exe" | cut -d' ' -f1)" == "$EXE_SHA256" ]] ||
  die "game/heroes4.exe is not from GOG Complete 3.0, the version this port is made for"

say "x86recomp"
X="$H4PORT/third_party/x86recomp"
if [[ ! -d "$X/.git" ]]; then
  git clone -q "$X86RECOMP" "$X"
  git -C "$X" checkout -q "$X86RECOMP_COMMIT"
  git -C "$X" apply "$H4PORT/patches/x86recomp-h4.patch"
  for p in "$H4PORT"/patches/x86recomp-*.patch; do   # fixes on top, one patch each, in name order
    [[ "$p" == */x86recomp-h4.patch ]] || git -C "$X" apply "$p"
  done
fi
(cd "$X" && cmake --preset release >/dev/null && cmake --build --preset release --target recomp_lift)

say "ffmpeg (LGPL)"
bash "$H4PORT/platforms/macos-dmg/ffmpeg_lgpl.sh" | tail -1
export PKG_CONFIG_PATH="$H4PORT/.cache/ffmpeg-lgpl/lib/pkgconfig"

say "Recompiling the game"
bash "$H4PORT/tools/lift.sh"   # again when heroes4.exe, symbols/, the manifest or the lifter changed
cd "$H4PORT/h4"
cmake --preset release >/dev/null
cmake --build --preset release --target h4

bash "$H4PORT/platforms/extra-assets.sh"   # art/combat_wings: Downloads or the 0.1 release
[[ -d "$H4PORT/art/combat_wings" ]] ||
  die "no art/combat_wings: download HOMM4-extra-assets.zip (platforms/extra-assets.sh) into Downloads"

say "DMG"
bash "$H4PORT/platforms/macos-dmg/make_dmg.sh"
