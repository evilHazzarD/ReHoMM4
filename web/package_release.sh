#!/bin/bash
# The browser version as a release file: web/package_release.sh <out.zip>
#   ReHoMM4/            the site (web/build.sh's .cache/outputs/web/dist) without the language packs
#                       (they reach players only from the ReHoMM4-translations releases)
#   ReHoMM4/serve.py    serves it on http://localhost:8080 (python3 serve.py)
#   ReHoMM4/README.txt
set -euo pipefail
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
OUT_ZIP="$1"
D="$ROOT/.cache/outputs/web/dist"
[[ -f "$D/index.html" && -f "$D/heroes4.wasm" ]] || { echo "no site in $D: run web/build.sh first" >&2; exit 1; }
T="$(mktemp -d)"; trap 'rm -rf "$T"' EXIT
mkdir -p "$T/ReHoMM4"
rsync -a --exclude translations "$D/" "$T/ReHoMM4/"
cp "$ROOT/web/serve.py" "$T/ReHoMM4/serve.py"
cat > "$T/ReHoMM4/README.txt" <<'TXT'
Heroes of Might and Magic IV in a web browser (ReHoMM4)

On this computer: install Python 3, then in this folder run
    python3 serve.py
and open http://localhost:8080 in Chrome, Edge, Firefox or Safari (64-bit, current).

For phones, tablets and other computers: put this folder on any HTTPS web server. It needs the
headers Cross-Origin-Opener-Policy: same-origin and Cross-Origin-Embedder-Policy: require-corp
(where a server cannot send them, coi-sw.js adds them). Or on your network:
    python3 serve.py --bind 0.0.0.0 --tls cert.pem key.pem
with a certificate the devices trust.

The first time, the game's installer opens: pick both files of GOG's offline installer
(setup_heroes_of_might_and_magic_4_complete_...exe and its -1.bin) or the folder of an installed
game. The game, saves and settings stay in the browser's storage; nothing is uploaded.

Languages: English. For a translation, make a folder "translations" here and put in it
languages.json and the language's .h4p from https://github.com/evilHazzarD/ReHoMM4-translations/releases
TXT
rm -f "$OUT_ZIP"
(cd "$T" && zip -qrX "$OUT_ZIP" ReHoMM4)
echo "web: $OUT_ZIP ($(du -h "$OUT_ZIP" | cut -f1))"
