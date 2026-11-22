#!/bin/sh
# Builds the core for this computer and writes stills of each sign to build/stills/SIGN (PNG, three times size),
# a sheet for each sign, and build/stills/all.png: each sign at rest and struck.
set -eu
cd "$(dirname "$0")/.."
SIGNS="aries taurus gemini cancer"
SRC="core/gfx.c core/sign.c core/scene.c core/signs.c"
for s in $SIGNS; do python3 tools/svg2sign.py art/$s.svg core/art_$s.h $s; SRC="$SRC core/$s.c"; done
python3 tools/img2pic.py art/pic/aries.jpg core/pic_aries.h aries F0E4BC C0202A F58A1C 3048B8 >/dev/null
python3 tools/img2pic.py art/pic/taurus.png core/pic_taurus.h taurus ECDFB4 A4522C E4A93C 5C9E3C >/dev/null
python3 tools/img2pic.py art/pic/gemini.jpg core/pic_gemini.h gemini F6EDC4 C8409C F2C230 58B0E8 >/dev/null
python3 tools/img2pic.py art/pic/cancer.jpg core/pic_cancer.h cancer DCE0E8 EC5C4C 189C98 245C90 >/dev/null
mkdir -p build/stills
cc -std=c99 -O2 -Wall -Wextra -Wno-misleading-indentation -ffp-contract=off $SRC port/host/main.c -o build/zhost
i=0
for s in $SIGNS; do rm -rf build/stills/$s; mkdir -p build/stills/$s; build/zhost $i build/stills/$s; i=$((i + 1)); done
python3 - $SIGNS <<'PY'
from PIL import Image
import glob, os, sys
both = []
for s in sys.argv[1:]:
    fs = sorted(glob.glob(f"build/stills/{s}/*.ppm"))
    ims = [Image.open(f).resize((720, 720), Image.NEAREST) for f in fs]
    for f, im in zip(fs, ims): im.save(f[:-4] + ".png"); os.remove(f)
    sheet = Image.new("RGB", (720 * 4 + 50, 720 * 2 + 30), (24, 24, 24))
    for i, im in enumerate(ims): sheet.paste(im, (10 + (i % 4) * 730, 10 + (i // 4) * 730))
    sheet.save(f"build/stills/{s}/sheet.png")
    both.append(ims[:2])
al = Image.new("RGB", (720 * len(both) + 10 * (len(both) + 1), 720 * 2 + 30), (24, 24, 24))
for i, pair in enumerate(both):
    for j, im in enumerate(pair): al.paste(im, (10 + i * 730, 10 + j * 730))
al.save("build/stills/all.png"); print("build/stills/all.png")
PY
