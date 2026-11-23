#!/bin/bash
# Builds Heroes IV for the browser (web/README.md) into .cache/outputs/web/dist: a static site
# (index.html, heroes4.js/.wasm/.data) to serve with web/serve.py or any server that sends the
# cross-origin isolation headers (or with the service worker in it that adds them).
#
#   web/build.sh                 the site
#   web/build.sh --save-patch    keeps the changes made in .cache/outputs/web/x86recomp as
#                                   web/patches/x86recomp-web.patch (and builds nothing)
#
# Needs what tools/build.sh made first, as the app builds do (platforms/build.sh): h4/generated/
# (the lifted heroes4.exe) and third_party/x86recomp with patches/ applied and its host build (the
# fetched SoftFloat and CORE-MATH sources). The Emscripten SDK and the libraries come from
# web/deps.sh (run here when missing).
set -euo pipefail
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
# shellcheck source=web/env.sh
source "$ROOT/web/env.sh"
X="$ROOT/third_party/x86recomp"
HOST_DEPS="$X/out/build/release/_deps"
BASE="$WEB_CACHE/x86recomp-base" XR="$WEB_CACHE/x86recomp"
PATCH="$WEB/patches/x86recomp-web.patch"
die() { printf 'error: %s\n' "$*" >&2; exit 1; }

if [[ "${1:-}" == --save-patch ]]; then
  [[ -d "$BASE" && -d "$XR" ]] || die "no staged x86recomp in $WEB_CACHE (run a build first)"
  python3 -I "$WEB/tools/save_patch.py" "$BASE" "$XR" "$PATCH"
  exit 0
fi

ls "$ROOT"/h4/generated/gen_*.cpp >/dev/null 2>&1 || die "no lifted game in h4/generated: run tools/build.sh first"
[[ -d "$HOST_DEPS/softfloat-src" ]] || die "no host build of x86recomp: run tools/build.sh first"
web_emsdk
[[ -f "$PREFIX/lib/librecomp_innoextract.a" ]] || bash "$WEB/deps.sh"

# The runtime: x86recomp as the apps build it (patches/ applied, by tools/build.sh), in a copy with
# the browser's changes on top. The copy is made again every time: the patch is what is kept.
mkdir -p "$WEB_CACHE"
rsync -a --delete --exclude out --exclude .git "$X/" "$BASE/"
if [[ -d "$XR" ]]; then
  python3 -I "$WEB/tools/save_patch.py" "$BASE" "$XR" "$WEB_CACHE/.staged.patch" >/dev/null
  cmp -s "$WEB_CACHE/.staged.patch" "$PATCH" ||
    die "$XR has changes that are not in $PATCH: keep them (web/build.sh --save-patch) or remove that folder"
fi
rsync -a --delete "$BASE/" "$XR/"
patch -s -p1 -d "$XR" < "$PATCH" || die "$PATCH does not apply to third_party/x86recomp"

# What the module carries in its file system: the language button's pictures (ui/, as the Linux
# app's) and the side panels (wings/, the project's art, when it is there).
RES="$WEB_CACHE/res"
mkdir -p "$RES"
# The wings: on the site beside the module (dist/wings), not in it: fetched after the game starts
# (web_main.cpp), black sides until then.
rm -rf "$RES/wings"
WINGS="$(bash "$ROOT/platforms/wings.sh" "$WEB_CACHE")"
[[ -n "$WINGS" ]] || echo "warning: no art/combat_wings (HOMM4-extra-assets.zip: platforms/extra-assets.sh): black sides"
swift "$ROOT/platforms/linux/render_ui.swift" "$RES/ui" "$H4_VERSION" >/dev/null
# The translations: the latest release's list (platforms/languages.py), as the apps start with it.
LANGS="$ROOT/.cache/shared/languages"
LANG_URL="$(python3 -I "$ROOT/platforms/languages.py" "$LANGS" "$WEB_CACHE/packs" || true)"
LANG_ARGS=()
[[ -f "$LANGS/languages.inc" ]] && LANG_ARGS+=(-DH4_LANGUAGES_INC="$LANGS/languages.inc")
[[ -n "$LANG_URL" ]] && LANG_ARGS+=(-DH4_TRANSLATIONS_URL="$LANG_URL")

B="$WEB_CACHE/build"
PKG_CONFIG_LIBDIR="$PREFIX/lib/pkgconfig" PKG_CONFIG_PATH="" \
emcmake cmake -S "$WEB" -B "$B" -G Ninja -DCMAKE_BUILD_TYPE=Release \
  -DX86RECOMP_DIR="$XR" -DCMAKE_PREFIX_PATH="$PREFIX" -DCMAKE_FIND_ROOT_PATH="$PREFIX" \
  -DFETCHCONTENT_SOURCE_DIR_SOFTFLOAT="$HOST_DEPS/softfloat-src" \
  -DFETCHCONTENT_SOURCE_DIR_COREMATH="$HOST_DEPS/coremath-src" -DFETCHCONTENT_FULLY_DISCONNECTED=ON \
  -DH4_RES_DIR="$RES" "${LANG_ARGS[@]}" >"$B.configure.log" 2>&1 || { tail -40 "$B.configure.log"; exit 1; }
{ cmake --build "$B" --target h4 -j"$JOBS" 2>&1 | grep -E 'error|FAILED|warning: undefined|Linking' || true; } | head -80
[[ -f "$B/heroes4.wasm" ]] || die "heroes4.wasm was not built (log above)"

# The site.
D="$WEB_CACHE/dist"
rm -rf "$D"; mkdir -p "$D"
cp "$B"/heroes4.js "$B"/heroes4.wasm "$D/"
[[ -f "$B/heroes4.data" ]] && cp "$B/heroes4.data" "$D/"
cp "$WEB"/shell/index.html "$WEB"/shell/coi-sw.js "$D/"
sed -i '' "s/@H4_VERSION@/$H4_VERSION/g" "$D/index.html"
bash "$ROOT/assets/icon/icon.sh" linux "$D/icon.png" >/dev/null 2>&1 || true
[[ -d "$WEB_CACHE/wings" ]] && cp -R "$WEB_CACHE/wings" "$D/wings"
cp "$ROOT/LICENSE" "$D/LICENSE.txt"
cp "$PREFIX/COPYING.ffmpeg.LGPLv2.1" "$D/"
# The translations on the site (web_platform.cpp's HttpGet takes GitHub release files from here:
# GitHub's downloads do not allow other sites' pages): the latest release's list and packs,
# downloaded once into the cache (checked against the list's sha256).
if [[ -n "$LANG_URL" ]]; then
  T="$WEB_CACHE/translations"
  mkdir -p "$T" "$D/translations"
  if curl -fsSL "$LANG_URL" -o "$T/languages.json.part"; then mv "$T/languages.json.part" "$T/languages.json"; fi
  if [[ -f "$T/languages.json" ]]; then
    cp "$T/languages.json" "$D/translations/"
    python3 -I - "$T" "$D/translations" <<'PY' || echo "warning: translations: not all packs are on the site"
import hashlib, json, os, shutil, sys, urllib.request
t, d = sys.argv[1], sys.argv[2]
ok = True
for l in json.load(open(os.path.join(t, 'languages.json')))['languages']:
    name = l['url'].rsplit('/', 1)[1]
    path = os.path.join(t, name)
    def good():
        if not os.path.isfile(path): return False
        h = hashlib.sha256()
        with open(path, 'rb') as f:
            for b in iter(lambda: f.read(1 << 20), b''): h.update(b)
        return h.hexdigest() == l['sha256']
    if not good():
        urllib.request.urlretrieve(l['url'], path + '.part')
        os.replace(path + '.part', path)
    if not good():
        print(f"translations: {name}: wrong sha256", file=sys.stderr); ok = False; continue
    shutil.copy(path, os.path.join(d, name))
    print(f"translations: {l['code']} ({name}, {l['size'] // 1048576} MB)")
sys.exit(0 if ok else 1)
PY
  else
    echo "warning: translations: no release list ($LANG_URL): the browser build has only English"
  fi
fi
echo "web: $D ($(du -sh "$D" | cut -f1); heroes4.wasm $(du -h "$D/heroes4.wasm" | cut -f1))"
echo "serve: python3 web/serve.py   (then open http://localhost:8080)"
