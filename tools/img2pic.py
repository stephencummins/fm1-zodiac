#!/usr/bin/env python3
"""A finished picture (PNG or JPEG, square, on black) into the screen's own form: 240 x 240 bytes, each one of
four inks and how much of it. Usage: img2pic.py picture out.h name RRGGBB RRGGBB RRGGBB RRGGBB
Each pixel takes the ink nearest its colour; near-black becomes bare ground, so the ground's tint shows through."""
import os, sys
from PIL import Image, ImageFilter

src, out, name = sys.argv[1:4]
inks = [tuple(int(h[i:i + 2], 16) for i in (0, 2, 4)) for h in sys.argv[4:8]]
im = Image.open(src).convert("RGB")
s = min(im.size)
im = im.crop(((im.width - s) // 2, (im.height - s) // 2, (im.width + s) // 2, (im.height + s) // 2))
im = im.filter(ImageFilter.MedianFilter(5)).resize((240, 240), Image.BOX)

def norm(c):
    m = max(c) or 1
    return [v / m for v in c]

ni = [norm(c) for c in inks]
data = []
for r, g, b in im.getdata():
    mx = max(r, g, b)
    if mx < 44:
        data.append(0); continue
    n = norm((r, g, b))
    k = min(range(4), key=lambda j: sum((n[i] - ni[j][i]) ** 2 for i in range(3)))
    lum, ilum = 0.3 * r + 0.59 * g + 0.11 * b, 0.3 * inks[k][0] + 0.59 * inks[k][1] + 0.11 * inks[k][2]
    lv = max(6, min(63, round(63 * lum / ilum)))
    data.append(k << 6 | lv)
sys.path.insert(0, os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", "base", "tools"))
import lz4blk
lz = lz4blk.compress(bytes(data))
assert lz4blk.decompress(lz) == bytes(data)
with open(out, "w") as f:
    f.write(f"/* made by tools/img2pic.py from {src}: do not edit */\n#include <stdint.h>\n")
    f.write("/* Z_PACKED (the FM-1): the picture is kept packed (LZ4, a block) and unpacked into z_pic when its sign starts */\n")
    f.write(f"#ifdef Z_PACKED\nstatic const uint8_t {name}_lz[{len(lz)}] = {{\n")
    for i in range(0, len(lz), 240):
        f.write(",".join(str(v) for v in lz[i:i + 240]) + ",\n")
    f.write(f"}};\n#define {name}_pic z_pic\n#define {name}_pic_load() z_unpack({name}_lz, (int)sizeof {name}_lz)\n#else\n")
    f.write(f"static const uint8_t {name}_pic[240 * 240] = {{\n")
    for y in range(240):
        f.write(",".join(str(v) for v in data[y * 240:(y + 1) * 240]) + ",\n")
    f.write(f"}};\n#define {name}_pic_load() ((void)0)\n#endif\n")
print(out, len(data))
