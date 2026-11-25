#!/bin/bash
# Builds Heroes IV for the browser (web/README.md) into .cache/outputs/web/dist: a static site
# (index.html, heroes4.js/.wasm/.data) to serve with web/serve.py or any server that sends the
# cross-origin isolation headers (or with the service worker in it that adds them).
#
#   web/build.sh                 the site
#   web/build.sh --dynamic       game-code-free shell and browser converter in dist-dynamic/
#   ... --no-translations        no translations/ in the site: the page then reads the packs from
#                                   the translations repository's own GitHub Pages site (the
#                                   web-pages workflow: the site on <owner>.github.io)
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

SITE_TRANSLATIONS=1
args=()
for a in "$@"; do
  if [[ "$a" == --no-translations ]]; then SITE_TRANSLATIONS=0; else args+=("$a"); fi
done
set -- "${args[@]+"${args[@]}"}"
MODE_SUFFIX=""
DYNAMIC=OFF
if [[ "${1:-}" == --dynamic ]]; then
  MODE_SUFFIX="-dynamic"
  DYNAMIC=ON
  bash "$ROOT/tools/prepare-x86recomp.sh"
elif [[ -n "${1:-}" ]]; then
  die "unknown option: $1"
else
  ls "$ROOT"/h4/generated/gen_*.cpp >/dev/null 2>&1 || die "no lifted game in h4/generated: run tools/build.sh first"
fi
HOST_ARGS=(-DFETCHCONTENT_FULLY_DISCONNECTED=OFF)
if [[ -d "$HOST_DEPS/softfloat-src" && -d "$HOST_DEPS/coremath-src" ]]; then
  HOST_ARGS=(-DFETCHCONTENT_SOURCE_DIR_SOFTFLOAT="$HOST_DEPS/softfloat-src"
             -DFETCHCONTENT_SOURCE_DIR_COREMATH="$HOST_DEPS/coremath-src"
             -DFETCHCONTENT_FULLY_DISCONNECTED=ON)
elif [[ "$DYNAMIC" == OFF ]]; then
  die "no host build of x86recomp: run tools/build.sh first"
fi
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

# What the module carries in its file system (nothing of its own now: the settings draw themselves,
# h4/src/app_ui; the side panels are fetched, below).
RES="$WEB_CACHE/res"
mkdir -p "$RES"
# The wings: on the site beside the module (dist/wings), not in it: fetched after the game starts
# (web_main.cpp), black sides until then.
rm -rf "$RES/wings"
WINGS="$(bash "$ROOT/platforms/wings.sh" "$WEB_CACHE")"
[[ -n "$WINGS" ]] || echo "warning: no art/combat_wings (HOMM4-extra-assets.zip: platforms/extra-assets.sh): black sides"
rm -rf "$RES/ui"  # (the settings' pictures of older builds)
# The translations: the latest release's list (platforms/languages.py), as the apps start with it.
LANGS="$ROOT/.cache/shared/languages"
LANG_URL="$(python3 -I "$ROOT/platforms/languages.py" "$LANGS" "$WEB_CACHE/packs" || true)"
LANG_ARGS=()
[[ -f "$LANGS/languages.inc" ]] && LANG_ARGS+=(-DH4_LANGUAGES_INC="$LANGS/languages.inc")
[[ -n "$LANG_URL" ]] && LANG_ARGS+=(-DH4_TRANSLATIONS_URL="$LANG_URL")

B="$WEB_CACHE/build$MODE_SUFFIX"
# The module's own files (heroes4.data) only when there are any: Emscripten refuses an empty folder.
RES_ARG="$RES"
[[ -n "$(ls -A "$RES")" ]] || { RES_ARG=""; rm -f "$B/heroes4.data"; }
PKG_CONFIG_LIBDIR="$PREFIX/lib/pkgconfig" PKG_CONFIG_PATH="" \
emcmake cmake -S "$WEB" -B "$B" -G Ninja -DCMAKE_BUILD_TYPE=Release \
  -DH4_WEB_DYNAMIC="$DYNAMIC" \
  -DX86RECOMP_DIR="$XR" -DCMAKE_PREFIX_PATH="$PREFIX" -DCMAKE_FIND_ROOT_PATH="$PREFIX" \
  "${HOST_ARGS[@]}" \
  -DH4_RES_DIR="$RES_ARG" "${LANG_ARGS[@]}" >"$B.configure.log" 2>&1 || { tail -40 "$B.configure.log"; exit 1; }
# Its own log: a failed compile must stop here, not leave the last build's binary to be packed.
build_ok=1; cmake --build "$B" --target h4 -j"$JOBS" >"$B.build.log" 2>&1 || build_ok=0
{ grep -E 'error|FAILED|warning: undefined|Linking' "$B.build.log" || true; } | head -80
[[ $build_ok == 1 ]] || die "the build failed (log: $B.build.log)"
[[ -f "$B/heroes4.wasm" ]] || die "heroes4.wasm was not built (log above)"

# The site.
if [[ "$DYNAMIC" == ON ]]; then
  bash "$ROOT/converter/tools/build_web_emitter.sh" "$B/converter"
fi
D="$WEB_CACHE/dist$MODE_SUFFIX"
rm -rf "$D"; mkdir -p "$D"
cp "$B"/heroes4.js "$B"/heroes4.wasm "$D/"
[[ -f "$B/heroes4.data" ]] && cp "$B/heroes4.data" "$D/"
cp "$WEB"/shell/index.html "$WEB"/shell/coi-sw.js "$D/"
if [[ "$DYNAMIC" == ON ]]; then
  cp -R "$B/converter" "$D/converter"
fi
sed -i '' "s/@H4_VERSION@/$H4_VERSION/g" "$D/index.html"
# What the start page says the site is: the game's code in it, or made in the browser.
python3 -I - "$D/index.html" "$DYNAMIC" <<'PY'
import sys
p, dynamic = sys.argv[1], sys.argv[2] == 'ON'
s = open(p, encoding='utf-8').read()
if dynamic:
    what = 'the game recompiled to WebAssembly here, from your own copy.'
    first = ('The site itself is about 10&nbsp;MB: the game\'s code is made from your copy here, '
             'once (a minute or two), and kept in this browser.')
else:
    what = 'the game itself, recompiled to WebAssembly.'
    first = 'The first load downloads the game\'s code (about 100&nbsp;MB).'
open(p, 'w', encoding='utf-8').write(s.replace('@H4_WHAT@', what).replace('@H4_FIRST_LOAD@', first))
PY
bash "$ROOT/assets/icon/icon.sh" linux "$D/icon.png" >/dev/null 2>&1 || true
[[ -d "$WEB_CACHE/wings" ]] && cp -R "$WEB_CACHE/wings" "$D/wings"
cp "$ROOT/LICENSE" "$D/LICENSE.txt"
cp "$PREFIX/COPYING.ffmpeg.LGPLv2.1" "$D/"
# The translations on the site (web_platform.cpp's HttpGet takes GitHub release files from here:
# GitHub's downloads do not allow other sites' pages): the latest release's list and packs,
# downloaded once into the cache (checked against the list's sha256).
if [[ -n "$LANG_URL" && $SITE_TRANSLATIONS == 1 ]]; then
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
echo "serve: python3 web/serve.py --dir '$D'   (then open http://localhost:8080)"
