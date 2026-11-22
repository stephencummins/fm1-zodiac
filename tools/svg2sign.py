#!/usr/bin/env python3
"""A sign's drawing (an SVG, 240 x 240 units) into C data for core/sign.c: lists of points and how to draw them.

  tools/svg2sign.py art/aries.svg core/art_aries.h aries

Reads <path> elements only, in the order they are in the file (later ones are drawn on top).
  stroke colour, fill colour   the nearest of the four inks below; black hides what is behind (fills) or cuts (lines)
  stroke-width                 the line's width in pixels
  stroke-opacity, fill-opacity how much ink (1 = full)
  id                           "group-name": paths of a group move together (the part before the first "-")
Curves are flattened to short straight pieces. Arcs (A) and transforms are not read: the tool says so and stops."""
import re, sys, math, xml.etree.ElementTree as ET

INKS = [(0xE8, 0xD9, 0xA8), (0xB3, 0x20, 0x2A), (0xF0, 0x8A, 0x1C), (0x2B, 0x3F, 0x9E)]   # gold, red, amber, blue
FILL_MAX = 96

def colour(s):
    s = (s or "").strip().lower()
    if s in ("", "none", "transparent"): return None
    names = {"black": "#000000", "white": "#ffffff"}
    s = names.get(s, s)
    m = re.fullmatch(r"#([0-9a-f]{3}|[0-9a-f]{6})", s)
    if not m: raise SystemExit(f"svg2sign: colour {s!r} not understood (use #RRGGBB)")
    h = m.group(1)
    if len(h) == 3: h = "".join(c * 2 for c in h)
    return tuple(int(h[i:i + 2], 16) for i in (0, 2, 4))

def ink_of(rgb):
    if max(rgb) < 24: return -1                                   # black
    return min(range(4), key=lambda k: sum((a - b) ** 2 for a, b in zip(rgb, INKS[k])))

def flatten(d):
    """-> a list of (points, closed) sub-paths"""
    tok = re.findall(r"[MmLlHhVvCcSsQqTtZzAa]|[-+]?(?:\d*\.\d+|\d+\.?)(?:[eE][-+]?\d+)?", d)
    subs, pts, i, cmd, cur, start, prev_c = [], [], 0, None, (0.0, 0.0), (0.0, 0.0), None
    def num():
        nonlocal i
        v = float(tok[i]); i += 1; return v
    def curve(p0, p1, p2, p3):
        ln = sum(math.dist(a, b) for a, b in ((p0, p1), (p1, p2), (p2, p3)))
        n = max(2, min(24, int(ln / 3.0 + 0.5)))
        for k in range(1, n + 1):
            t = k / n; u = 1 - t
            pts.append(tuple(u ** 3 * a + 3 * u * u * t * b + 3 * u * t * t * c + t ** 3 * e for a, b, c, e in zip(p0, p1, p2, p3)))
    while i < len(tok):
        if re.fullmatch(r"[A-Za-z]", tok[i]): cmd = tok[i]; i += 1
        rel = cmd.islower(); c = cmd.upper()
        ox, oy = cur if rel else (0.0, 0.0)
        if c == "Z":
            if pts: subs.append((pts, True))
            pts, cur, prev_c = [], start, None
            continue
        if c == "A": raise SystemExit("svg2sign: an arc (A) in a path: convert it to curves in the drawing app")
        if c == "M":
            if pts: subs.append((pts, False))
            cur = start = (ox + num(), oy + num()); pts = [cur]; cmd = "l" if rel else "L"; prev_c = None
        elif c == "L": cur = (ox + num(), oy + num()); pts.append(cur); prev_c = None
        elif c == "H": cur = (ox + num(), cur[1]); pts.append(cur); prev_c = None
        elif c == "V": cur = (cur[0], oy + num()); pts.append(cur); prev_c = None
        elif c in "CS":
            if c == "C": p1 = (ox + num(), oy + num())
            else: p1 = (2 * cur[0] - prev_c[0], 2 * cur[1] - prev_c[1]) if prev_c else cur
            p2 = (ox + num(), oy + num()); p3 = (ox + num(), oy + num())
            curve(cur, p1, p2, p3); cur, prev_c = p3, p2
        elif c in "QT":
            if c == "Q": q = (ox + num(), oy + num())
            else: q = (2 * cur[0] - prev_c[0], 2 * cur[1] - prev_c[1]) if prev_c else cur
            p3 = (ox + num(), oy + num())
            curve(cur, tuple(a + 2 / 3 * (b - a) for a, b in zip(cur, q)), tuple(a + 2 / 3 * (b - a) for a, b in zip(p3, q)), p3)
            cur, prev_c = p3, q
    if pts: subs.append((pts, False))
    return subs

def main(src, dst, name):
    root = ET.parse(src).getroot()
    groups, paths, allpts = [], [], []
    for el in root.iter():
        tag = el.tag.split("}")[-1]
        if el.get("transform"): raise SystemExit(f"svg2sign: a transform on <{tag}>: apply it in the drawing app first")
        if tag != "path": continue
        st = dict(kv.split(":", 1) for kv in (el.get("style") or "").split(";") if ":" in kv)
        g = lambda k, dflt=None: (st.get(k) or el.get(k) or dflt)
        fill, stroke = colour(g("fill", "none")), colour(g("stroke", "none"))
        op = float(g("opacity", "1"))
        gname = (el.get("id") or "x").split("-")[0]
        if gname not in groups: groups.append(gname)
        for pts, closed in flatten(el.get("d")):
            if closed and len(pts) > 1 and math.dist(pts[0], pts[-1]) < 0.01: pts = pts[:-1]
            first = len(allpts)
            allpts += pts
            if fill is not None:
                if len(pts) > FILL_MAX: raise SystemExit(f"svg2sign: {el.get('id')}: a filled shape of {len(pts)} points (most: {FILL_MAX})")
                k = ink_of(fill)
                lv = 0 if k < 0 else round(63 * op * float(g("fill-opacity", "1")))
                paths.append((1, max(k, 0), lv, 0, groups.index(gname), 1, first, len(pts)))
            if stroke is not None:
                k = ink_of(stroke)
                lv = 0 if k < 0 else round(63 * op * float(g("stroke-opacity", "1")))
                w = round(16 * float(g("stroke-width", "1")))
                paths.append((2 if k < 0 else 0, max(k, 0), lv, min(w, 255), groups.index(gname), int(closed), first, len(pts)))
    o = [f"/* made by tools/svg2sign.py from {src}: do not edit */",
         f"enum {{ " + ", ".join(f"{name.upper()}_G_{g.upper()}" for g in groups) + f", {name.upper()}_NGROUPS }};",
         f"static const int16_t {name}_pts[] = {{"]
    row = []
    for x, y in allpts:
        row.append(f"{round(x * 16)},{round(y * 16)}")
        if len(row) == 12: o.append("    " + ", ".join(row) + ","); row = []
    if row: o.append("    " + ", ".join(row) + ",")
    o += ["};", f"static const SignPath {name}_paths[] = {{   /* kind, ink, level, width, group, closed, first, n */"]
    o += ["    {" + ", ".join(str(v) for v in p) + "}," for p in paths]
    o += ["};", f"static const Sign {name}_art = {{{name}_paths, {len(paths)}, {name}_pts, {len(allpts)}}};", ""]
    open(dst, "w").write("\n".join(o))
    kb = (len(allpts) * 4 + len(paths) * 8) / 1024
    print(f"{dst}: {len(paths)} paths, {len(allpts)} points, {len(groups)} groups, about {kb:.1f} KB of flash")

if __name__ == "__main__":
    if len(sys.argv) != 4: raise SystemExit(__doc__)
    main(*sys.argv[1:])
