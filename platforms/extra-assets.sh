#!/bin/bash
# extra-assets.sh: makes sure art/combat_wings (the side panels; the project's own collages, never
# committed) is there. Takes it, in this order, from art/ itself, from HOMM4-extra-assets.zip (or the
# folder Safari unpacks it to) in ~/Downloads, or downloads it from H4_EXTRA_ASSETS_URL: the one copy,
# attached to release H4_EXTRA_ASSETS_RELEASE only (tools/release/make_release.sh adds it there).
# Prints a warning and exits 0 when it cannot: the apps then show black sides.
#
#   source platforms/extra-assets.sh --vars     only sets the two variables
H4_EXTRA_ASSETS_RELEASE=0.1
H4_EXTRA_ASSETS_URL="https://github.com/evilHazzarD/ReHoMM4/releases/download/$H4_EXTRA_ASSETS_RELEASE/HOMM4-extra-assets.zip"
[[ "${1:-}" == --vars ]] && return 0 2>/dev/null
set -euo pipefail
ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
A="$ROOT/art/combat_wings"
[[ -d "$A/factions" ]] && exit 0
got="$(ls -td "$HOME"/Downloads/HOMM4-extra-assets* 2>/dev/null | head -1 || true)"
if [[ -z "$got" ]]; then
  got="$ROOT/.cache/HOMM4-extra-assets.zip"
  if [[ ! -f "$got" ]]; then
    echo "extra-assets: downloading $H4_EXTRA_ASSETS_URL" >&2
    mkdir -p "$ROOT/.cache"
    curl -fsSL -o "$got.part" "$H4_EXTRA_ASSETS_URL" && mv "$got.part" "$got" ||
      { rm -f "$got.part"; echo "warning: could not download $H4_EXTRA_ASSETS_URL: black sides" >&2; exit 0; }
  fi
fi
# The zip (or folder) may hold art/combat_wings at its top or one folder down (a zip made in Finder):
# take the combat_wings folder that has factions/ in it, without Finder's __MACOSX and .DS_Store.
T="$(mktemp -d)"; trap 'rm -rf "$T"' EXIT
if [[ -d "$got" ]]; then src="$got"; else unzip -qo "$got" -x '__MACOSX/*' -d "$T" 2>/dev/null || true; src="$T"; fi
w="$(find "$src" -maxdepth 4 -type d -path '*/art/combat_wings' ! -path '*__MACOSX*' 2>/dev/null | head -1)"
if [[ -n "$w" && -d "$w/factions" ]]; then
  mkdir -p "$ROOT/art"; ditto "$w" "$A"; find "$A" -name '.DS_Store' -delete
fi
[[ -d "$A/factions" ]] || echo "warning: no art/combat_wings in $got: black sides" >&2
exit 0
