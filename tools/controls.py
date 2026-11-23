#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""The FM-1's panel drawn, every control labelled with what it does in Zodiac: the picture on the site's controls page
(site/controls.html, put together by tools/site.py). No photograph: M-VAVE's are not ours to use.

  tools/controls.py [OUT.svg]      writes the drawing alone, to look at

Where the controls sit follows the FM-1: two pairs of knobs and the two OCT buttons at the left, the screen in the
middle, four knobs over two rows of six buttons at the right, the 27 lower buttons along the bottom edge (16 long ones
with 11 short ones above, in groups of 3, 2, 3, 2, 1). The effects' names and which button holds which are read from
the firmware's own source (core/music.c, app/app.c), so the drawing cannot say something the FM-1 does not do."""
import re
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
SIGNS = ("Aries", "Taurus", "Gemini", "Cancer", "Leo", "Virgo", "Libra", "Scorpio", "Sagittarius", "Capricorn",
         "Aquarius", "Pisces")
ELEMENTS = (("Fire", "#FF7A3D"), ("Earth", "#86D46A"), ("Air", "#F4E7B4"), ("Water", "#58A9FF"))     # core/wheel.c's inks
BUTTONS = ("FX", "SEL", "ENV", "LFO", "EDIT", "GLO", "HOME", "SAVE", "ARP", "SEQ", "PLAY", "REC")
W, H = 1500, 900

# The FM-1 is sold in more than one colour, so the drawing's colours are variables, a set to a skin: data-skin on the svg
# chooses (the page's buttons set it). The sets are AISLING's drawing's, by eye from the two FM-1s.
SKINS = {
    "black": ("Black", dict(body="#14161c", tray="#1e222b", edge="#323846", glass="#050608", ink="#f3f1ee", dim="#c9cdd6",
                            knob="#08090b", knobedge="#454c5c", foot="#1e222b", mark="#f3f1ee", btn="#2a303c",
                            btnedge="#454c5c", btnink="#f3f1ee", one="#ff5f1f", oneink="#1a0a02")),
    "orange": ("Orange", dict(body="#cb6b45", tray="#85473c", edge="#a3532f", glass="#120b09", ink="#24110a", dim="#3a1a0e",
                              knob="#1c1614", knobedge="#3a2a24", foot="#ab5636", mark="#fbf0e4", btn="#8f4d40",
                              btnedge="#5a2c23", btnink="#fbf0e4", one="#fbf0e4", oneink="#24110a")),
}
CSS = "".join(f'svg.zp{"" if k == "black" else f"[data-skin={k}]"} {{ ' + " ".join(f"--{n}: {v};" for n, v in sk.items()) + " }\n"
              for k, (_, sk) in SKINS.items()) + """
svg.zp { display: block; width: 100%; height: auto; font-family: "Archivo", "Helvetica Neue", Helvetica, Arial, sans-serif; }
svg.zp .body { fill: var(--body); } svg.zp .tray { fill: var(--tray); } svg.zp .frame { fill: var(--glass); stroke: var(--edge); }
svg.zp .knob { fill: var(--knob); stroke: var(--knobedge); } svg.zp .foot { fill: var(--foot); } svg.zp .mark { stroke: var(--mark); }
svg.zp .b { fill: var(--btn); stroke: var(--btnedge); } svg.zp .b.one { fill: var(--one); stroke: var(--one); }
svg.zp text { fill: var(--ink); }
svg.zp .lbl { font-weight: 600; font-size: 17px; letter-spacing: 0.06em; }
svg.zp .what { font-size: 17px; fill: var(--dim); }
svg.zp .btn { font-weight: 600; font-size: 15px; letter-spacing: 0.05em; fill: var(--btnink); }
svg.zp .sign { font-size: 16.5px; fill: var(--btnink); }
svg.zp .fx { font-weight: 500; font-size: 16px; fill: var(--btnink); } svg.zp .fx.on { fill: var(--oneink); font-weight: 600; }
svg.zp .note { font-size: 17px; fill: var(--dim); } svg.zp .strong { fill: var(--ink); font-weight: 600; }
svg.zp .ontray { fill: var(--btnink); }
"""


def skin_buttons():
    return "".join(f'<button type="button" data-skin="{k}">{name}</button>' for k, (name, _) in SKINS.items())


def effects():
    """the 27 lower buttons' jobs, left to right as they are wired (key 0 is the bottom left one)"""
    music = (ROOT / "core/music.c").read_text(encoding="utf-8")
    names = re.findall(r'"([^"]+)"', re.search(r"z_fx_name\[Z_NFX\]\s*=\s*\{(.*?)\};", music, re.S).group(1))
    app = (ROOT / "app/app.c").read_text(encoding="utf-8")
    keyfx = [int(v) for v in re.search(r"KEYFX\[27\]\s*=\s*\{(.*?)\}", app).group(1).split(",")]
    if len(keyfx) != 27 or keyfx[0] != -1 or sorted(keyfx[1:]) != list(range(len(names))):
        raise SystemExit("tools/controls.py: the lower buttons are wired differently now; the drawing needs a look")
    return [None] + [names[i] for i in keyfx[1:]]


def text(x, y, t, cls, anchor="middle", extra=""):
    return f'<text x="{x:.1f}" y="{y:.1f}" text-anchor="{anchor}" class="{cls}"{extra}>{t}</text>'


def lines(x, y, t, cls, lead=20):
    return "".join(text(x, y + i * lead, part, cls) for i, part in enumerate(t.split("|")))


def knob(x, y, label, does):
    r = 26
    return (f'<circle cx="{x}" cy="{y}" r="{r + 6}" class="foot"/><circle cx="{x}" cy="{y}" r="{r}" class="knob" stroke-width="2.5"/>'
            f'<line x1="{x}" y1="{y - r + 5}" x2="{x}" y2="{y - r + 16}" class="mark" stroke-width="3" stroke-linecap="round"/>'
            + text(x, y - r - 14, label, "lbl") + lines(x, y + r + 30, does, "what"))


def button(x, y, w, h, label, cls="b"):
    return (f'<rect x="{x}" y="{y}" width="{w}" height="{h}" rx="10" class="{cls}" stroke-width="1.5"/>'
            + text(x + w / 2, y + h / 2 + 5, label, "btn"))


def panel(screen="../img/leo.png"):
    fx = effects()
    s = [f'<svg class="zp" viewBox="0 0 {W} {H}" role="img" aria-label="The FM-1 running Zodiac, every knob and button '
         f'labelled with what it does"><style>{CSS}</style><rect width="{W}" height="{H}" rx="40" class="body"/>']
    # the left: two pairs of knobs, the OCT buttons under them
    for (name, does), (x, y) in zip((("MASTER", "Volume"), ("SELECT", "The wheel:|choose a sign"),
                                     ("PRESETS", "The next sign,|or the one before"), ("ALGORITHM", "The tips,|a page a click")),
                                    ((110, 104), (268, 104), (110, 272), (268, 272))):
        s.append(knob(x, y, name, does))
    s.append('<rect x="42" y="386" width="296" height="64" rx="14" class="tray"/>')
    s.append(button(56, 398, 128, 40, "OCT−") + button(196, 398, 128, 40, "OCT+"))
    s.append(lines(120, 478, "Stop, and|play again", "what") + lines(260, 478, "Keep the four|knobs for next time", "what"))
    # the screen
    s.append('<rect x="384" y="40" width="304" height="304" rx="22" class="frame" stroke-width="2"/>')
    s.append(f'<image href="{screen}" x="410" y="66" width="252" height="252" style="image-rendering:pixelated"/>')
    s.append(lines(536, 372, "The sign's picture, moving|with its music", "what"))
    # the four knobs
    for i, does in enumerate(("Low pass:|down to a rumble", "High pass:|thin, a whisper", "Echo", "Reverb:|a great hall")):
        s.append(knob(830 + 170 * i, 104, f"K{i + 1}", does))
    # the twelve buttons: a sign each, in its element's colour
    s.append('<rect x="726" y="212" width="732" height="252" rx="22" class="tray"/>')
    for i, (b, sign) in enumerate(zip(BUTTONS, SIGNS)):
        x, y = 746 + 118 * (i % 6), 232 + 114 * (i // 6)
        s.append(button(x, y, 102, 44, b))
        s.append(f'<rect x="{x + 36}" y="{y + 56}" width="30" height="5" rx="2.5" fill="{ELEMENTS[i % 4][1]}"/>')
        s.append(text(x + 51, y + 82, sign, "sign"))
    s.append(text(1092, 492, 'A sign each. Its own button is lit while it plays.', "what"))
    for i, (name, col) in enumerate(ELEMENTS):
        x = 806 + 150 * i
        s.append(f'<rect x="{x}" y="512" width="18" height="18" rx="4" fill="{col}" stroke="var(--knob)" stroke-width="1.5"/>' + text(x + 28, 527, name, "what", "start"))
    # the lower buttons
    s.append(text(56, 556, '<tspan class="strong">The lower buttons:</tspan> each is an effect for as long as you hold it. '
                           'Hold several at once.', "note", "start"))
    s.append('<rect x="36" y="574" width="1428" height="296" rx="22" class="tray"/>')
    n_long = 0
    for k in range(27):
        short = k % 12 in (1, 3, 5, 8, 10)
        if short:
            x, y, w, h = 56 + 88 * n_long - 33, 590, 54, 118
        else:
            x, y, w, h = 56 + 88 * n_long, 722, 66, 132
            n_long += 1
        s.append(f'<rect x="{x}" y="{y}" width="{w}" height="{h}" rx="{w / 2}" class="b{" one" if k == 0 else ""}" stroke-width="1.5"/>')
        cx, cy = x + w / 2 + 5, y + h / 2
        if k:
            s.append(text(cx, cy, fx[k], "fx", extra=f' transform="rotate(-90 {cx} {cy})"'))
        else:
            s.append(text(cx - 9, cy, "The sign", "fx on", extra=f' transform="rotate(-90 {cx - 9} {cy})"')
                     + text(cx + 9, cy, "answers", "fx on", extra=f' transform="rotate(-90 {cx + 9} {cy})"'))
    return "".join(s) + "</svg>"


if __name__ == "__main__":
    out = Path(sys.argv[1] if len(sys.argv) > 1 else ROOT / "build/controls.svg")
    out.parent.mkdir(parents=True, exist_ok=True)
    out.write_text(panel().replace("<svg ", '<svg xmlns="http://www.w3.org/2000/svg" '), encoding="utf-8")
    print(out)
