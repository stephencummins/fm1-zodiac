"""Shared by the art/draft_*.py scripts, which write a sign's first drawing as an SVG by numbers.
The four colours stand for a sign's four inks (ink 0 the line, then 1, 2, 3): each sign's code says what colours
they really are. Black fills hide what is behind them."""
import math, pathlib

GOLD, RED, AMBER, BLUE, BLACK = "#E8D9A8", "#B3202A", "#F08A1C", "#2B3F9E", "#000000"
INK0, INK1, INK2, INK3 = GOLD, RED, AMBER, BLUE
out = []

def path(d, pid, stroke=None, width=1.2, fill=None, so=1.0, fo=1.0):
    a = f'<path id="{pid}" d="{d}"'
    a += f' fill="{fill}" fill-opacity="{fo:g}"' if fill else ' fill="none"'
    if stroke:
        a += f' stroke="{stroke}" stroke-width="{width:g}" stroke-opacity="{so:g}" stroke-linecap="round" stroke-linejoin="round"'
    out.append(a + "/>")

def P(pts, close=False):
    return "M" + " L".join(f"{x:.2f},{y:.2f}" for x, y in pts) + (" Z" if close else "")

def both(fn):
    fn(lambda x: x, "L")
    fn(lambda x: 240 - x, "R")

def sym(pts):                                   # a left half (top centre to bottom centre) and its mirror
    return pts + [(240 - x, y) for x, y in reversed(pts) if x != 120]

def shape(pts, pid, line=GOLD, width=1.2, so=1.0, fill=None, fo=1.0, hide=True):
    """a closed plane: hides what is behind, then its colour and its outline"""
    if hide: path(P(pts, True), pid + "_h", fill=BLACK)
    path(P(pts, True), pid, stroke=line, width=width, so=so, fill=fill, fo=fo)

def band(pid, at, total, step, cells, n=84, line=GOLD):
    """a ridged band (a horn): at(pos, side) gives a point, side -1 / 0 / +1 across it, pos 0 .. total along it"""
    pos, i, ridges = 0.0, 0, [0.0]
    while pos < total - 1e-6:
        nxt = min(pos + step, total); mid = (pos + nxt) / 2
        ink, fo = cells[i % len(cells)]
        path(P([at(pos, 1), at(mid, 1), at(nxt, 1), at(nxt, -1), at(mid, -1), at(pos, -1)], True), f"{pid}-cell{i}", fill=ink, fo=fo)
        ridges.append(nxt); pos, i = nxt, i + 1
    for side, name in ((1, "outer"), (-1, "inner")):
        path(P([at(total * k / n, side) for k in range(n + 1)]), f"{pid}-{name}", stroke=line, width=1.3)
    for k, pos in enumerate(ridges):
        path(P([at(pos, -1), at(pos, 1)]), f"{pid}-ridge{k}", stroke=line, width=0.9, so=0.8)

def write(name):
    svg = ('<svg xmlns="http://www.w3.org/2000/svg" width="720" height="720" viewBox="0 0 240 240">\n'
           '<rect width="240" height="240" fill="#000"/>\n' + "\n".join(out) + "\n</svg>\n")
    p = pathlib.Path(__file__).with_name(name + ".svg")
    p.write_text(svg)
    print(p, len(out), "paths")
