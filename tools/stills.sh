#!/bin/sh
# Builds the core for this computer and writes stills of each sign to build/stills/SIGN (PNG, three times size),
# a sheet for each sign, and build/stills/all.png: each sign at rest and struck.
set -eu
cd "$(dirname "$0")/.."
SIGNS="aries taurus gemini cancer"
SRC="core/gfx.c core/sign.c core/scene.c core/signs.c"
for s in $SIGNS; do python3 tools/svg2sign.py art/$s.svg core/art_$s.h $s; SRC="$SRC core/$s.c"; done
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
