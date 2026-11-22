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

# ---- ears --------------------------------------------------------------------------------------------------
def ear(m, s):
    d = f"M{m(96):g},112 L{m(80):g},104 L{m(62):g},112 L{m(78):g},121 Z"
    path(d, f"ear{s}-shape", fill=BLACK)
    path(d, f"ear{s}-leaf", stroke=GOLD, width=1.2, fill=RED, fo=0.5)
    path(f"M{m(93):g},112 L{m(68):g},112", f"ear{s}-vein", stroke=GOLD, width=0.9, so=0.6)
both(ear)

def sym(pts):                                   # a left half (top centre to bottom centre) and its mirror
    return pts + [(240 - x, y) for x, y in reversed(pts) if x != 120]

# ---- the mouth: the maw behind, then tongue, lower teeth and the jaw (these drop when it roars) ------------
path(P([(103, 174), (137, 174), (134, 196), (120, 202), (106, 196)], True), "maw-dark", fill=RED, fo=0.5)
path(P([(113, 182), (120, 167), (127, 182), (120, 186)], True), "tongue-t", stroke=GOLD, width=0.8, fill=AMBER, fo=0.9)
for i, (x, h, w) in enumerate([(109.5, 7, 2.4), (114.5, 3.5, 1.8), (125.5, 3.5, 1.8), (130.5, 7, 2.4)]):
    path(P([(x - w, 178), (x, 178 - h), (x + w, 178)], True), f"teethlo-{i}", fill=GOLD, fo=1.0)
jaw = sym([(120, 177), (104, 176), (107, 194), (113, 203), (120, 205)])
path(P(jaw, True), "jaw-shape", fill=BLACK)
path(P(jaw, True), "jaw-line", stroke=GOLD, width=1.3, fill=GOLD, fo=0.10)
path(P([(111, 189), (129, 189), (120, 204)], True), "jaw-chin", stroke=GOLD, width=0.7, so=0.4, fill=RED, fo=0.40)
path(P([(115, 205), (120, 219), (125, 205)]), "jaw-beard", stroke=GOLD, width=1.1, so=0.9)
path(P([(118, 206), (120, 213), (122, 206)]), "jaw-beard2", stroke=GOLD, width=0.8, so=0.5)

# ---- the head: straight edges and flat planes, as cut glass ------------------------------------------------
head = sym([(120, 80), (104, 84), (91, 100), (94, 124), (101, 146), (104, 164), (101, 178), (111, 174), (120, 177)])
path(P(head, True), "head-shape", fill=BLACK)
path(P(head, True), "head-line", stroke=GOLD, width=1.5, fill=GOLD, fo=0.08)
path(P([(120, 80), (110, 100), (120, 120), (130, 100)], True), "head-crown", stroke=GOLD, width=0.8, so=0.5, fill=AMBER, fo=0.42)
path(P([(113, 130), (127, 130), (127, 165), (113, 165)], True), "head-bridge", stroke=GOLD, width=0.7, so=0.35, fill=AMBER, fo=0.36)
def planes(m, s):
    path(P([(m(104), 84), (m(91), 100), (m(110), 100)], True), f"head-temple{s}", stroke=GOLD, width=0.8, so=0.45, fill=RED, fo=0.55)
    path(P([(m(91), 100), (m(94), 124), (m(113), 130), (m(120), 120), (m(110), 100)], True), f"head-socket{s}", stroke=GOLD, width=0.7, so=0.3, fill=RED, fo=0.22)
    path(P([(m(94), 124), (m(101), 146), (m(113), 150), (m(113), 130)], True), f"head-cheek{s}", stroke=GOLD, width=0.8, so=0.45, fill=RED, fo=0.42)
    path(P([(m(101), 146), (m(104), 164), (m(101), 178), (m(111), 174), (m(113), 165), (m(113), 150)], True), f"head-muzzle{s}", stroke=GOLD, width=0.8, so=0.45, fill=AMBER, fo=0.30)
    path(P([(m(115.5), 168.5), (m(117.5), 170.5)]), f"head-nostril{s}", stroke=BLACK, width=1.6)
    # the eye: slanted, the inner corner low; and a heavy brow over it
    path(P([(m(97), 119), (m(114), 127.5), (m(110), 132), (m(100), 128)], True), f"eye{s}-shape", stroke=GOLD, width=1.0, fill=AMBER, fo=1.0)
    path(P([(m(106.5), 122.5), (m(106), 130.5)]), f"eye{s}-pupil", stroke=BLACK, width=2.4)
    path(P([(m(92), 111), (m(116), 124), (m(116), 128.5), (m(94), 117.5)], True), f"brow{s}-shape", fill=BLACK)
    path(P([(m(92), 111), (m(116), 124), (m(116), 128.5), (m(94), 117.5)], True), f"brow{s}-fold", stroke=GOLD, width=1.1, fill=GOLD, fo=0.55)
both(planes)
path(P([(113, 165), (127, 165), (123.5, 172), (116.5, 172)], True), "head-nose", fill=BLACK)
path(P([(113, 165), (127, 165), (123.5, 172), (116.5, 172)], True), "head-nose2", stroke=GOLD, width=1.0, fill=GOLD, fo=0.75)
path("M120,172 L120,176.5", "head-philtrum", stroke=GOLD, width=1.0, so=0.8)
for i, (x, h, w) in enumerate([(105.5, 9, 2.6), (111.5, 4, 1.9), (116.5, 3, 1.6), (123.5, 3, 1.6), (128.5, 4, 1.9), (134.5, 9, 2.6)]):
    path(P([(x - w, 175), (x, 175 + h), (x + w, 175)], True), f"teethup-{i}", fill=GOLD, fo=1.0)

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
