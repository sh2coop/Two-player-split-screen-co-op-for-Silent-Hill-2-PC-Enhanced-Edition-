#!/bin/bash
# build.sh - builds the mod (sh2coop.asi), the one-click installer, the checker, and the player zip.
# Needs the MinGW-w64 32-bit toolchain (Debian/Ubuntu: apt install gcc-mingw-w64-i686) and zip.
# Nothing here needs any file from the game or from the Enhanced Edition.
set -e
cd "$(dirname "$0")"
mkdir -p out/stage
i686-w64-mingw32-gcc -O2 -Wall -shared -static-libgcc -o out/sh2coop.asi sh2coop.c sh2coop_d9.c sh2coop_title.c sh2coop_ce.c sh2coop_chr.c -ldinput8 -ldxguid
i686-w64-mingw32-gcc -O2 -Wall -Wextra -s -mconsole -static-libgcc -o "out/Install co-op.exe" sh2coop_setup.c -ladvapi32 -lole32 -lshell32
i686-w64-mingw32-gcc -O2 -Wall -Wextra -s -mconsole -static-libgcc -DCHECK_ONLY -o out/sh2coop_check.exe sh2coop_setup.c -ladvapi32
BUILD=$(strings out/sh2coop.asi | grep -o 'sh2coop PC build [A-Za-z0-9]*' | head -1 | awk '{print $4}')
ST=out/stage; rm -rf "$ST"; mkdir -p "$ST"
cp out/sh2coop.asi "$ST/"; cp sh2coop.ini "$ST/"; cp "sh2coop README.txt" "$ST/README.txt"
cp out/sh2coop_check.exe "out/Install co-op.exe" "$ST/"
sed -i 's/\r$//; s/$/\r/' "$ST/README.txt" "$ST/sh2coop.ini"
{
  echo "Silent Hill 2 co-op mod - build $BUILD"
  echo "packaged $(date -u +%Y-%m-%d) (UTC)"
  echo "made for: Enhanced Edition exe 1.0 (sha1 3201a5e1029f3ae3b77770255dcbe10360b3cd09), SH2 Enhancements module 3.0.2218.0"
  echo; echo "files (sha1):"
  (cd "$ST" && sha1sum sh2coop.asi sh2coop.ini "Install co-op.exe" sh2coop_check.exe README.txt)
} > "$ST/VERSION.txt"; sed -i 's/$/\r/' "$ST/VERSION.txt"
ZIP="out/SH2 co-op mod $BUILD.zip"; rm -f "$ZIP"
(cd "$ST" && zip -q -X "../../$ZIP" "Install co-op.exe" sh2coop.asi sh2coop.ini sh2coop_check.exe README.txt VERSION.txt)
echo "built: $ZIP"; unzip -l "$ZIP"
