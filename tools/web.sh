#!/bin/sh
# Builds the picture as one web page (build/aries.html) that opens straight from the disk. Needs emcc.
set -eu
cd "$(dirname "$0")/.."
python3 tools/svg2sign.py art/aries.svg core/art_aries.h aries
mkdir -p build
emcc -O2 -std=c99 core/gfx.c core/sign.c core/aries.c port/web/main.c -o build/aries.html \
  --shell-file port/web/shell.html -sSINGLE_FILE=1 -sENVIRONMENT=web -sEXPORTED_RUNTIME_METHODS=HEAPU8
echo build/aries.html
