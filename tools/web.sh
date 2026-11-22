#!/bin/sh
# Builds the pictures as one web page (build/zodiac.html) that opens straight from the disk. Needs emcc.
set -eu
cd "$(dirname "$0")/.."
SRC="core/gfx.c core/sign.c core/scene.c core/signs.c"
for s in aries taurus gemini cancer; do python3 tools/svg2sign.py art/$s.svg core/art_$s.h $s >/dev/null; SRC="$SRC core/$s.c"; done
python3 tools/img2pic.py art/pic/aries.jpg core/pic_aries.h aries F0E4BC C0202A F58A1C 3048B8 >/dev/null
python3 tools/img2pic.py art/pic/taurus.png core/pic_taurus.h taurus ECDFB4 A4522C E4A93C 5C9E3C >/dev/null
python3 tools/img2pic.py art/pic/gemini.jpg core/pic_gemini.h gemini F6EDC4 C8409C F2C230 58B0E8 >/dev/null
python3 tools/img2pic.py art/pic/cancer.jpg core/pic_cancer.h cancer DCE0E8 EC5C4C 189C98 245C90 >/dev/null
for s in leo virgo libra scorpio; do SRC="$SRC core/$s.c"; done
python3 tools/img2pic.py art/pic/leo.jpg core/pic_leo.h leo F4E8C8 F2B830 D87020 4A2890 >/dev/null
python3 tools/img2pic.py art/pic/virgo.jpg core/pic_virgo.h virgo F5ECCE D8A838 7C9C6C 2C2C78 >/dev/null
python3 tools/img2pic.py art/pic/libra.jpg core/pic_libra.h libra F4ECCC E0607C 30C0B8 9C78D0 >/dev/null
python3 tools/img2pic.py art/pic/scorpio.jpg core/pic_scorpio.h scorpio ECE4D4 C82840 7038B0 148478 >/dev/null
mkdir -p build
emcc -O2 -std=c99 $SRC port/web/main.c -o build/zodiac.html \
  --shell-file port/web/shell.html -sSINGLE_FILE=1 -sENVIRONMENT=web -sEXPORTED_RUNTIME_METHODS=HEAPU8
echo build/zodiac.html
