#!/bin/bash
# publish.sh - put a finished build on GitHub: commit the sources + binaries, then make the release the in-game
# updater reads (Co-op Options > Updates). Run build.sh first.
#
#   ./publish.sh SIGNING_KEY.pem "what changed" [--publish]
#
# Without --publish the release is created as a DRAFT (players don't see it until someone presses Publish on GitHub).
# The signing key must NOT be inside this folder; it is never committed or uploaded.
set -euo pipefail
cd "$(dirname "$0")"
REPO=sh2coop/Two-player-split-screen-co-op-for-Silent-Hill-2-PC-Enhanced-Edition-
KEY=${1:?usage: publish.sh SIGNING_KEY.pem "what changed" [--publish]}
MSG=${2:?usage: publish.sh SIGNING_KEY.pem "what changed" [--publish]}
PUBLISH=${3:-}

[ -f out/stage/sh2coop.asi ] || { echo "run build.sh first"; exit 1; }
case "$(realpath "$KEY")" in "$(realpath .)"/*) echo "the signing key is inside the repository folder - move it out"; exit 1;; esac
BUILD=$(strings out/stage/sh2coop.asi | grep -o 'sh2coop PC build [A-Za-z0-9]*' | head -1 | awk '{print $4}')
[ -n "$BUILD" ] || { echo "no build name in out/stage/sh2coop.asi"; exit 1; }
if gh release view "$BUILD" -R "$REPO" >/dev/null 2>&1; then echo "release $BUILD already exists on GitHub"; exit 1; fi

# 1. the repository: sources as they are here + the built binaries and VERSION.txt
cp out/stage/sh2coop.asi out/stage/sh2coop_check.exe "out/stage/Install co-op.exe" out/stage/VERSION.txt .
git add -A
if git diff --cached --quiet; then echo "nothing new to commit"; else git commit -q -m "$BUILD: $MSG"; fi
git push -q origin HEAD:main
echo "pushed $(git rev-parse --short HEAD) to $REPO main"

# 2. the release files (signed manifest) and the release itself
rm -rf out/release
python3 make_update_release.py out/stage out/release "$KEY" --with-zip
DRAFT=--draft; [ "$PUBLISH" = "--publish" ] && DRAFT=
gh release create "$BUILD" -R "$REPO" --target main $DRAFT --title "$BUILD - $MSG" \
  --notes "$MSG

Players who already have the mod: Options > Game Options > Co-op Options > Updates, press Enter, then Enter again.
First install: the green Code > Download ZIP button (or the SH2-co-op-mod zip below) and README.md.
The sh2coop-* files are what the in-game updater downloads." \
  out/release/sh2coop.asi out/release/sh2coop_check.exe out/release/sh2coop-README.txt out/release/sh2coop-VERSION.txt \
  out/release/sh2coop-manifest.txt out/release/sh2coop-manifest.sig \
  "out/release/SH2-co-op-mod-$BUILD.zip" out/release/Install-co-op.exe
echo "release $BUILD created${DRAFT:+ as a DRAFT - publish it on GitHub when ready}"
