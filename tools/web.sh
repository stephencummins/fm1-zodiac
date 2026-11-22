#!/bin/sh
# Builds the pictures as one web page (build/zodiac.html) that opens straight from the disk. Needs emcc.
set -eu
cd "$(dirname "$0")/.."
SRC="core/gfx.c core/sign.c core/scene.c core/signs.c"
for s in aries taurus gemini cancer; do python3 tools/svg2sign.py art/$s.svg core/art_$s.h $s >/dev/null; SRC="$SRC core/$s.c"; done
mkdir -p build
emcc -O2 -std=c99 $SRC port/web/main.c -o build/zodiac.html \
  --shell-file port/web/shell.html -sSINGLE_FILE=1 -sENVIRONMENT=web -sEXPORTED_RUNTIME_METHODS=HEAPU8
echo build/zodiac.html
