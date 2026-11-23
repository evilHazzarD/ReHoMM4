#!/bin/bash
# A release in one command, on the maintainer's Mac (the game is lifted from your own heroes4.exe,
# which never leaves it; GitHub's CI only checks the repository: .github/workflows/check.yml).
#
#   tools/release/make_release.sh <version>            build, check, collect: .cache/release-<version>/
#   tools/release/make_release.sh <version> --draft    and then push the tag (v<version>) and make a DRAFT release
#                                                      on GitHub with the files (gh); you publish it
#
# <version> must be what VERSION says (every app takes its version from there) and have its section
# "## <version>" in CHANGELOG.md: that section is the release's notes. The working tree must be
# committed (the release is built from what the tag points at). Steps:
#   1. every target with platforms/build.sh (iOS unsigned: AltStore/Sideloadly sign it);
#   2. the files under their release names (reHoMM4-v<version>-<platform>.<ext>), each with <file>.sha256 (kept here, not uploaded:
#      GitHub shows each file's SHA-256 itself); for the one release that holds
#      HOMM4-extra-assets.zip (art/combat_wings, the side panels for builds from source; never
#      committed; platforms/extra-assets.sh downloads it from there), that zip too;
#   3. release_check.sh on every file, publish_check.py --release on the tree and the files;
#   4. --draft: tag v<version> on HEAD (if missing), push it, gh release create --draft.
# H4_RELEASE_DIRTY=1 tries steps 1-3 on uncommitted changes.
set -euo pipefail
ROOT="$(cd "$(dirname "$0")/../.." && pwd)"
cd "$ROOT"
die() { printf 'error: %s\n' "$*" >&2; exit 1; }
say() { printf '\n== %s\n' "$*"; }

V="${1:-}"; MODE="${2:-}"
[[ -n "$V" ]] || die "usage: tools/release/make_release.sh <version> [--draft]"
[[ -z "$MODE" || "$MODE" == --draft ]] || die "unknown option: $MODE"
[[ "$(tr -d ' \n' < VERSION)" == "$V" ]] || die "VERSION says $(cat VERSION), not $V: change it, commit, run again"
NOTES="$(awk -v v="$V" '$0 == "## " v {on=1; next} /^## / && on {exit} on' CHANGELOG.md)"
[[ -n "${NOTES//[[:space:]]/}" ]] || die "CHANGELOG.md has no section \"## $V\""
# H4_RELEASE_DIRTY=1: try it on uncommitted changes (never with --draft).
if [[ -n "$(git status --porcelain)" ]]; then
  [[ "${H4_RELEASE_DIRTY:-}" == 1 && "$MODE" != --draft ]] ||
    die "uncommitted changes: commit first (the release is built from HEAD)"
  echo "warning: uncommitted changes (H4_RELEASE_DIRTY=1): a trial, not a release"
fi
# The tag is v<version> (v0.3), as the files' names (reHoMM4-v0.3-…); VERSION has the number alone (0.3). The apps'
# update checks drop a leading v (0.1 and 0.2 were tagged without it).
TAG="v$V"
if git rev-parse -q --verify "refs/tags/$TAG" >/dev/null; then
  [[ "$(git rev-parse "$TAG^{commit}")" == "$(git rev-parse HEAD)" ]] || die "tag $TAG exists and is not HEAD"
fi

[[ -d art/combat_wings/factions ]] || die "no art/combat_wings: the apps would have black sides"
OUT="$ROOT/.cache/release-$V"
rm -rf "$OUT"; mkdir -p "$OUT"
printf '%s\n' "$NOTES" | sed -e '/./,$!d' > "$OUT/NOTES.md"   # without the blank lines on top

say "Building"
# target  file it makes (in .cache/outputs/<target>/dist)  its name in the release (with the version:
# the apps' update checks look for reHoMM4-…-<the part after the version>, 0.2 for HeroesIV-…)
TARGETS=(
  "macos-arm64  HeroesIV-macos-arm64.dmg       reHoMM4-v$V-macos-arm64.dmg"
  "macos-x86_64 HeroesIV-macos-x86_64.dmg      reHoMM4-v$V-macos-x86_64.dmg"
  "ios          HeroesIV-ios.ipa               reHoMM4-v$V-ios.ipa"
  "android      HeroesIV-android_arm64.apk     reHoMM4-v$V-android_arm64.apk"
  "windows-x86_64 HeroesIV-windows-x86_64.zip  reHoMM4-v$V-windows-x86_64.zip"
  "linux-x86_64 HeroesIV-linux-x86_64.tar.gz   reHoMM4-v$V-linux-x86_64.tar.gz"
  "linux-arm64  HeroesIV-linux-arm64.tar.gz    reHoMM4-v$V-linux-arm64.tar.gz"
  "switch       HeroesIV.nro                   reHoMM4-v$V-switch.nro"
)
for t in "${TARGETS[@]}"; do
  read -r target built file <<< "$t"
  log="$OUT/build-$target.log"
  printf '%-13s ' "$target"
  start=$SECONDS
  if [[ $target == ios ]]; then H4_TEAM_ID= bash platforms/build.sh ios >"$log" 2>&1
  else bash platforms/build.sh "$target" >"$log" 2>&1; fi || { echo "FAILED"; tail -20 "$log"; die "build failed: $log"; }
  src="$ROOT/.cache/outputs/$target/dist/$built"
  [[ -f "$src" ]] || die "$target: no $src (log: $log)"
  cp "$src" "$OUT/$file"
  (cd "$OUT" && shasum -a 256 "$file" > "$file.sha256")
  echo "$(du -h "$OUT/$file" | cut -f1 | tr -d ' ')  $((SECONDS - start)) s"
done

# The wings for builds from source: only in release $H4_EXTRA_ASSETS_RELEASE, which they are
# downloaded from (platforms/extra-assets.sh).
source platforms/extra-assets.sh --vars
EXTRA=()
if [[ "$V" == "$H4_EXTRA_ASSETS_RELEASE" ]]; then
(cd "$ROOT" && find art/combat_wings -type f ! -name '.*' | LC_ALL=C sort | zip -qX "$OUT/HOMM4-extra-assets.zip" -@)
(cd "$OUT" && shasum -a 256 HOMM4-extra-assets.zip > HOMM4-extra-assets.zip.sha256)
printf '%-13s %s\n' "wings" "$(du -h "$OUT/HOMM4-extra-assets.zip" | cut -f1 | tr -d ' ')"
EXTRA=("$OUT/HOMM4-extra-assets.zip")
fi

say "Checking"
# The APK must be signed with the release key (platforms/android/app/build.gradle), or it will not
# install over the old version.
apksigner="$(ls -d "${ANDROID_HOME:-$HOME/Library/Android/sdk}"/build-tools/*/apksigner 2>/dev/null | sort -V | tail -1)"
[[ -x "$apksigner" ]] || die "no apksigner (Android SDK build-tools)"
# apksigner runs the java on PATH (macOS's /usr/bin/java is only a stub): Homebrew's JDK, as
# platforms/android/package.sh, unless JAVA_HOME has a working one.
[[ -x "${JAVA_HOME:-}/bin/java" ]] || export JAVA_HOME=/opt/homebrew/opt/openjdk@17
export PATH="$JAVA_HOME/bin:$PATH"
cert="$("$apksigner" verify --print-certs "$OUT/reHoMM4-v$V-android_arm64.apk" |
  sed -n 's/^Signer #1 certificate SHA-256 digest: //p')"
[[ "$cert" == "$(cat platforms/android/release-cert.sha256)" ]] ||
  die "the APK is not signed with the release key (got ${cert:-nothing}): see platforms/android/app/build.gradle"
echo "apk: signed with the release key"
T="$(mktemp -d)"; trap 'rm -rf "$T"' EXIT
mkdir -p "$T/linux" "$T/linux-arm64"
tar -xzf "$OUT/reHoMM4-v$V-linux-x86_64.tar.gz" -C "$T/linux"
tar -xzf "$OUT/reHoMM4-v$V-linux-arm64.tar.gz" -C "$T/linux-arm64"
for f in "$OUT"/*.dmg "$OUT"/*.ipa "$OUT"/*.apk "$OUT"/*-windows-x86_64.zip; do
  case "$f" in
    *.dmg) bash tools/release/release_check.sh "$f" 2>/dev/null ;;
    *) cp "$f" "$T/a.zip"; bash tools/release/release_check.sh "$T/a.zip" | sed "s#$T/a.zip#${f##*/}#"; rm "$T/a.zip" ;;
  esac
done
bash tools/release/release_check.sh "$T/linux" | sed "s#$T/linux#linux tar.gz#"
bash tools/release/release_check.sh "$T/linux-arm64" | sed "s#$T/linux-arm64#linux-arm64 tar.gz#"
python3 tools/release/publish_check.py --release "$OUT"/*.dmg "$OUT"/*.ipa "$OUT"/*.apk "$OUT"/*-windows-x86_64.zip "$T/linux" "$T/linux-arm64" "$OUT"/*.nro ${EXTRA[@]+"${EXTRA[@]}"} |
  grep -E '^(FAIL|publish_check)' | sed "s#$T/##"
python3 tools/release/publish_check.py --release "$OUT"/*.dmg "$OUT"/*.ipa "$OUT"/*.apk "$OUT"/*-windows-x86_64.zip "$T/linux" "$T/linux-arm64" "$OUT"/*.nro ${EXTRA[@]+"${EXTRA[@]}"} >/dev/null ||
  die "publish_check failed (above)"

say "Release $V: $OUT"
ls -1 "$OUT" | grep -v '\.log$'

if [[ "$MODE" != --draft ]]; then
  echo
  echo "Next: tools/release/make_release.sh $V --draft   (pushes tag $V, makes a draft release with these files)"
  exit 0
fi

say "GitHub draft"
command -v gh >/dev/null || die "no gh (brew install gh)"
gh auth status >/dev/null 2>&1 || die "gh is not signed in: gh auth login -h github.com"
git rev-parse -q --verify "refs/tags/$TAG" >/dev/null || git tag "$TAG"
git push origin "$TAG"
files=()
# The .sha256 files stay here: GitHub shows every file's SHA-256 on the release itself.
for f in "$OUT"/reHoMM4-* "$OUT"/HOMM4-extra-assets.zip; do
  if [[ -e "$f" && "$f" != *.log && "$f" != *.sha256 ]]; then files+=("$f"); fi
done
gh release create "$TAG" --draft --verify-tag --title "ReHoMM4 $TAG" --notes-file "$OUT/NOTES.md" "${files[@]}"
echo "Draft made: check it on GitHub (Releases), then press Publish."
