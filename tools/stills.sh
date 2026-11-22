#!/bin/sh
# Builds the core for this computer and writes stills of the picture to build/stills (PNG, three times size)
# and one sheet of them all.
set -eu
cd "$(dirname "$0")/.."
python3 tools/svg2sign.py art/aries.svg core/art_aries.h aries
mkdir -p build/stills
cc -std=c99 -O2 -Wall -Wextra -Wno-misleading-indentation -ffp-contract=off core/gfx.c core/sign.c core/aries.c port/host/main.c -o build/zhost
build/zhost build/stills
python3 - <<'PY'
from PIL import Image
import glob, os
fs = sorted(glob.glob("build/stills/*.ppm"))
ims = [Image.open(f).resize((720, 720), Image.NEAREST) for f in fs]
for f, im in zip(fs, ims): im.save(f[:-4] + ".png"); os.remove(f)
sheet = Image.new("RGB", (720 * 3 + 40, 720 * 2 + 30), (24, 24, 24))
for i, im in enumerate(ims): sheet.paste(im, (10 + (i % 3) * 730, 10 + (i // 3) * 730))
sheet.save("build/stills/sheet.png"); print("build/stills/sheet.png")
PY
