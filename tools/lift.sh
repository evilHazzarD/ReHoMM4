#!/bin/bash
# Lifts game/heroes4.exe into h4/generated/ (C++), again whenever anything it depends on changed:
# heroes4.exe, symbols/, h4/lifter.manifest or the lifter itself (x86recomp at the pinned commit with
# patches/, rebuilt here first). Their hash is kept in h4/generated/.lift-stamp; a different one
# (or none) means the old output is removed and the exe lifted again. Run by tools/build.sh and
# platforms/build.sh; needs x86recomp set up by tools/build.sh.
#
#   tools/lift.sh            lifts when needed, prints what it did
#   tools/lift.sh --check    exit 1 if h4/generated/ is missing or stale (lifts nothing)
set -euo pipefail
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
X="$ROOT/third_party/x86recomp"
LIFT="$X/out/build/release/tools/lifter/recomp_lift"
OUT="$ROOT/h4/generated"
die() { printf 'error: %s\n' "$*" >&2; exit 1; }

[[ -f "$ROOT/game/heroes4.exe" ]] || die "no game/heroes4.exe: run tools/build.sh first"
[[ -d "$X/.git" ]] || die "no third_party/x86recomp: run tools/build.sh first"
if [[ "${1:-}" != --check ]]; then
  [[ -x /opt/homebrew/bin/brew ]] && eval "$(/opt/homebrew/bin/brew shellenv)"
  log="$X/out/lift-build.log"; mkdir -p "$X/out"
  (cd "$X" && cmake --preset release && cmake --build --preset release --target recomp_lift) >"$log" 2>&1 ||
    { tail -30 "$log"; die "building recomp_lift failed (log: $log)"; }
fi
[[ -x "$LIFT" ]] || die "no recomp_lift: run tools/build.sh first"

stamp() {
  {
    shasum -a 256 "$ROOT/game/heroes4.exe" "$ROOT/h4/lifter.manifest" "$LIFT" | cut -d' ' -f1
    (cd "$ROOT" && find symbols -type f ! -name '*.log' | LC_ALL=C sort | xargs shasum -a 256)
  } | shasum -a 256 | cut -d' ' -f1
}
want="$(stamp)"
have="$(cat "$OUT/.lift-stamp" 2>/dev/null || true)"
if [[ -d "$OUT" && "$have" == "$want" ]]; then
  [[ "${1:-}" == --check ]] || echo "lift: h4/generated is up to date"
  exit 0
fi
if [[ "${1:-}" == --check ]]; then
  state="missing or unstamped"; [[ -n "$have" ]] && state="stale (its inputs changed)"
  echo "lift: h4/generated is $state: run tools/lift.sh" >&2
  exit 1
fi
echo "lift: ${have:+inputs changed, }lifting heroes4.exe into h4/generated"
rm -rf "$OUT" "$OUT.tmp"
(cd "$ROOT/h4" && "$LIFT" --exe ../game/heroes4.exe --symbols ../symbols --out generated.tmp \
  --manifest lifter.manifest >/dev/null) || { rm -rf "$OUT.tmp"; die "recomp_lift failed"; }
mv "$OUT.tmp" "$OUT"
echo "$want" > "$OUT/.lift-stamp"
echo "lift: done ($(ls "$OUT" | wc -l | tr -d ' ') files)"
