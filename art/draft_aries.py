#!/usr/bin/env python3
"""Writes art/aries.svg: a first draft of the ram, drawn by numbers. Once the SVG is redrawn by hand in a
drawing app this script is history: tools/svg2sign.py reads the SVG, whoever made it.

The colours are the four inks (tools/svg2sign.py INKS); black fills hide what is behind them."""
import math, pathlib

GOLD, RED, AMBER, BLUE, BLACK = "#E8D9A8", "#B3202A", "#F08A1C", "#2B3F9E", "#000000"
out = []

def path(d, pid, stroke=None, width=1.2, fill=None, so=1.0, fo=1.0):
    a = f'<path id="{pid}" d="{d}"'
    a += f' fill="{fill}" fill-opacity="{fo:g}"' if fill else ' fill="none"'
    if stroke:
        a += f' stroke="{stroke}" stroke-width="{width:g}" stroke-opacity="{so:g}" stroke-linecap="round" stroke-linejoin="round"'
    out.append(a + "/>")

def P(pts, close=False):
    return "M" + " L".join(f"{x:.2f},{y:.2f}" for x, y in pts) + (" Z" if close else "")

def mirror_d(segs):
    """segs: a start point and cubic segments down the left half, from the top centre to the bottom centre.
    Returns the closed outline, the right half mirrored about x = 120."""
    start, cubs = segs
    d = f"M{start[0]:g},{start[1]:g}"
    for c in cubs:
        d += " C" + " ".join(f"{x:g},{y:g}" for x, y in c)
    pts = [start] + [p for c in cubs for p in c]          # start, c1, c2, end, c1, c2, end ...
    rev = [(240 - x, y) for x, y in reversed(pts)]
    for i in range(1, len(rev), 3):
        d += " C" + " ".join(f"{x:g},{y:g}" for x, y in rev[i:i + 3])
    return d + " Z"

def both(fn):
    fn(lambda x: x, "L")
    fn(lambda x: 240 - x, "R")

# ---- the neck: a fleece of scales --------------------------------------------------------------------------
neck = [(103, 168), (94, 200), (84, 240), (156, 240), (146, 200), (137, 168)]
path(P(neck, True), "neck-shape", fill=BLACK)
def in_neck(x, y):
    t = (y - 168) / 72.0
    half = 17 + 19 * t
    return abs(x - 120) < half - 3
row = 0
y = 204.0
while y < 246:
    r = 6.5
    off = r if row % 2 else 0.0
    x = 120 - 8 * 2 * r + off
    while x < 240:
        if in_neck(x, y - 3):
            arc = [(x + r * math.cos(math.pi * k / 6), y - 6 + r * 0.9 * math.sin(math.pi * k / 6)) for k in range(7)]
            path(P(arc), f"neck-s{row}_{int(x)}", stroke=GOLD, width=1.0, so=0.5)
        x += 2 * r
    y += 8.0
    row += 1

# ---- ears --------------------------------------------------------------------------------------------------
def ear(m, s):
    d = f"M{m(96):g},128 C{m(84):g},127 {m(72):g},137 {m(63):g},153 C{m(77):g},153 {m(91):g},147 {m(98):g},138 Z"
    path(d, f"ear{s}-shape", fill=BLACK)
    path(d, f"ear{s}-leaf", stroke=GOLD, width=1.2, fill=RED, fo=0.42)
    path(f"M{m(93):g},133 C{m(84):g},136 {m(76):g},142 {m(69):g},150", f"ear{s}-vein", stroke=GOLD, width=0.9, so=0.6)
both(ear)

# ---- the face ----------------------------------------------------------------------------------------------
face = ((120, 84), [
    [(109, 84), (100, 86), (96, 94)],
    [(92, 102), (94, 116), (97, 128)],
    [(100, 142), (103, 160), (107, 176)],
    [(109, 184), (109, 192), (112, 196)],
    [(115, 199), (118, 200), (120, 200)],
])
fd = mirror_d(face)
path(fd, "face-shape", fill=BLACK)
path(fd, "face-mask", stroke=GOLD, width=1.4, fill=GOLD, fo=0.13)

def features(m, s):
    eye = f"M{m(98):g},127 C{m(102):g},119.5 {m(110):g},119 {m(114.5):g},125.5 C{m(110):g},130 {m(102):g},131 {m(98):g},127 Z"
    path(eye, f"eye{s}-almond", stroke=GOLD, width=1.1, fill=AMBER, fo=0.95)
    path(f"M{m(103):g},125 L{m(110):g},125", f"eye{s}-pupil", stroke=BLACK, width=2.2)
    path(f"M{m(96.5):g},119 C{m(102):g},113.5 {m(110):g},113.5 {m(116):g},119.5", f"eye{s}-brow", stroke=GOLD, width=1.0, so=0.75)
    path(f"M{m(115):g},130 C{m(115.5):g},150 {m(116.5):g},168 {m(116.5):g},181", f"face-bridge{s}", stroke=GOLD, width=0.9, so=0.55)
    path(f"M{m(112.5):g},184.5 C{m(114):g},189 {m(117.5):g},190.5 {m(120):g},189", f"face-nostril{s}", stroke=GOLD, width=1.2)
    path(f"M{m(100.5):g},140 C{m(103):g},152 {m(105.5):g},164 {m(108):g},174", f"face-cheek{s}", stroke=GOLD, width=0.8, so=0.35)
both(features)
path("M120,189 L120,195 M114.5,196.5 C117,198.5 123,198.5 125.5,196.5", "face-mouth", stroke=GOLD, width=1.0, so=0.8)
for i, (cy, r) in enumerate([(96, 4.2), (106.5, 3.2), (115, 2.4)]):
    path(P([(120, cy - r * 1.35), (120 + r, cy), (120, cy + r * 1.35), (120 - r, cy)], True), f"jewel-{i}",
         stroke=GOLD, width=0.9, fill=RED, fo=0.95)

# ---- the horns: a ridged band along a spiral ---------------------------------------------------------------
def horn(m, s):
    cx, cy, total = 58.0, 112.0, 630.0
    def at(deg, side):                       # side -1 inner edge, +1 outer edge, 0 the middle
        u = deg / total
        r = 47 - 41 * u ** 0.9
        w = 16 * (1 - u) ** 0.8 + 2
        phi = math.radians(-21 - deg)
        rr = r + side * w / 2
        return (m(cx + rr * math.cos(phi)), cy + rr * math.sin(phi))
    step, deg, i = 15.0, 0.0, 0
    ridges = [0.0]
    while deg < total - 1:
        nxt = min(deg + step, total)
        mid = (deg + nxt) / 2
        quad = [at(deg, 1), at(mid, 1), at(nxt, 1), at(nxt, -1), at(mid, -1), at(deg, -1)]
        ink, fo = [(RED, 0.60), (AMBER, 0.50), (RED, 0.40), (AMBER, 0.62)][i % 4]
        path(P(quad, True), f"horn{s}-cell{i}", fill=ink, fo=fo)
        ridges.append(nxt)
        deg, i = nxt, i + 1
    n = 84
    for side, name in ((1, "outer"), (-1, "inner")):
        path(P([at(total * k / n, side) for k in range(n + 1)]), f"horn{s}-{name}", stroke=GOLD, width=1.3)
    for k, deg in enumerate(ridges):
        path(P([at(deg, -1), at(deg, 1)]), f"horn{s}-ridge{k}", stroke=GOLD, width=0.9, so=0.8)
both(horn)

svg = ('<svg xmlns="http://www.w3.org/2000/svg" width="720" height="720" viewBox="0 0 240 240">\n'
       '<rect width="240" height="240" fill="#000"/>\n' + "\n".join(out) + "\n</svg>\n")
p = pathlib.Path(__file__).with_name("aries.svg")
p.write_text(svg)
print(p, len(out), "paths")
