#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""Zodiac's site (zodiac.stephen8n.com) from a release package:

  tools/site.py app/build/zodiac-X.Y.Z.fwsc X.Y.Z [OUT_DIR, default build/site]

fm1-base's tools/make_site.py makes the installer, the browser emulator, the words files and the firmware folder;
then this puts Zodiac's own front page over the base's (site/index.html, site/brand.css, site/mark.png, site/img),
adds the controls page (site/controls.html with tools/controls.py's drawing of the panel)
and dresses the made installer and the browser emulator in the same look (the emulator gets what each control does written over its panel). Nothing in base/ is changed: only the copy in OUT_DIR.
The browser emulator must be built first (./build.sh --web)."""
import re
import shutil
import subprocess
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / "tools"))
import controls  # noqa: E402  (the panel drawn: tools/controls.py)
SIGNS = [  # name, element, its own knob, a line on its piece
    ("Aries", "fire", "Heat", "Soft low horns and a muted trumpet, a slow swing. When the ram bleats the horns blare."),
    ("Taurus", "earth", "Growth", "Cellos on long notes over a bass that plods, one hoof a beat."),
    ("Gemini", "air", "Wind", "A light dance in six for two harpsichord hands, one answering the other."),
    ("Cancer", "water", "Tide", "A violin over a falling Spanish ground; the claws click and it is a paso doble."),
    ("Leo", "fire", "Blaze", "A march: low brass, kettledrums, a horn tune. On the roar, a fanfare."),
    ("Virgo", "earth", "Harvest", "A jig on a hurdy-gurdy, two drones and a buzzing bridge."),
    ("Libra", "air", "Breeze", "Four high violins that float, no beat anywhere, and the wind."),
    ("Scorpio", "water", "Depth", "Deep, warm notes: a low ground and a slow tune in the tenor."),
    ("Sagittarius", "fire", "Flight", "Heroic horns leaping up the chord, strings driving in threes."),
    ("Capricorn", "earth", "Climb", "Alpine horns called across a valley, and the mountain under them."),
    ("Aquarius", "air", "Flow", "Small bells falling in ripples, a glass voice above, soft strings below."),
    ("Pisces", "water", "Dream", "A rushing torrent, three soft notes far inside the water, and now and then a low, slow call."),
]
FONTS = ('<link rel="stylesheet" href="https://fonts.googleapis.com/css2?family=Archivo:wght@400;500;600'
         '&family=IBM+Plex+Mono:wght@400;500&display=swap">')
INSTALL_CSS = """<style>
  /* Zodiac's look over the installer's own (tools/site.py) */
  :root, :root[data-theme="light"] { --bg: #ffffff; --fg: #111110; --dim: #6e6a64; --accent: #d83f00; --on-accent: #fff; --bad: #a3271b; }
  @media (prefers-color-scheme: dark) { :root:not([data-theme="light"]) { --bg: #0e0d0c; --fg: #f3f1ee; --dim: #98928a; --accent: #ff5f1f; --on-accent: #1a0a02; --bad: #ff8a73; } }
  :root[data-theme="dark"] { --bg: #0e0d0c; --fg: #f3f1ee; --dim: #98928a; --accent: #ff5f1f; --on-accent: #1a0a02; --bad: #ff8a73; }
  body { font-family: "Archivo", "Helvetica Neue", Helvetica, Arial, sans-serif; }
  .zbar { max-width: 520px; margin: 0 auto; padding: 16px 16px 0; display: flex; align-items: center; gap: 10px; }
  .zbar img { width: 32px; height: 32px; } .zbar a { font-weight: 600; font-size: 19px; color: var(--fg); text-decoration: none; }
  .zbar span { font: 12.5px/1.5 "IBM Plex Mono", ui-monospace, Menlo, monospace; letter-spacing: 0.045em; color: var(--dim); }
  .zrow { max-width: 520px; margin: 14px auto 0; padding: 0 16px; display: grid; grid-template-columns: repeat(6, 1fr); gap: 4px; }
  .zrow img { width: 100%; height: auto; image-rendering: pixelated; display: block; }
  main { padding-top: 24px !important; }
  h1 { font-weight: 600; letter-spacing: -0.02em; }
  button { border-radius: 0 !important; font-family: "IBM Plex Mono", ui-monospace, Menlo, monospace !important; font-size: 13.5px !important; letter-spacing: 0.045em; padding: 12px 18px !important; }
  .lang { padding: 4px 0 !important; }
</style>"""

EMU_CSS = """<style>
  /* Zodiac's look over the emulator page's own (tools/site.py) */
  :root, :root[data-theme="light"] { --ground: #ffffff; --ink: #111110; --muted: #6e6a64; --rule: #d9d6d1; --accent: #d83f00; --card: #f6f5f3; }
  @media (prefers-color-scheme: dark) { :root:not([data-theme="light"]) { --ground: #0e0d0c; --ink: #f3f1ee; --muted: #98928a; --rule: #2e2b27; --accent: #ff5f1f; --card: #181614; } }
  :root[data-theme="dark"] { --ground: #0e0d0c; --ink: #f3f1ee; --muted: #98928a; --rule: #2e2b27; --accent: #ff5f1f; --card: #181614; }
  :root { --zwall: 0.16; } :root[data-theme="dark"] { --zwall: 0.22; }
  @media (prefers-color-scheme: dark) { :root:not([data-theme="light"]) { --zwall: 0.22; } }
  body { font-family: "Archivo", "Helvetica Neue", Helvetica, Arial, sans-serif; padding: 0; -webkit-font-smoothing: antialiased; }
  a { color: inherit; text-decoration-color: var(--accent); text-underline-offset: 3px; }
  :focus-visible { outline: 2px solid var(--accent); outline-offset: 2px; }
  .zwall { position: fixed; inset: 0; z-index: -1; pointer-events: none; background: url(../img/wall.png) center top / 360px; image-rendering: pixelated; opacity: var(--zwall);
    --m: linear-gradient(to right, #000 calc(50% - 560px), transparent calc(50% - 500px), transparent calc(50% + 500px), #000 calc(50% + 560px)); -webkit-mask-image: var(--m); mask-image: var(--m); }
  .zsheet { max-width: 960px; margin: 24px auto; border: 1px solid var(--rule); background: var(--ground); }
  @media (max-width: 1000px) { .zsheet { margin: 0; border-width: 0 0 1px; } }
  .zsheet main { max-width: none; padding: 16px; }
  header.zbar { display: flex; flex-wrap: wrap; align-items: center; gap: 8px 16px; padding: 12px 16px; margin: 0; max-width: none; border-bottom: 1px solid var(--rule); }
  .bar button, .speak button { border-radius: 0; border-color: var(--ink); background: none; font: 500 13px/1 "IBM Plex Mono", ui-monospace, Menlo, monospace; letter-spacing: 0.045em; padding: 9px 14px; }
  .bar button:hover, .speak button:hover { border-color: var(--accent); }
  .start button { border-radius: 0; background: var(--accent); color: #fff; font: 500 15px/1 "IBM Plex Mono", ui-monospace, Menlo, monospace; letter-spacing: 0.045em; padding: 14px 22px; }
  .speak { border-radius: 0; background: none; padding: 14px 16px; }
  .speak summary h2 { font: 12.5px/1.55 "IBM Plex Mono", ui-monospace, Menlo, monospace; letter-spacing: 0.045em; color: var(--muted); font-weight: 400; text-transform: lowercase; }
  .speak input { accent-color: var(--accent); }
  .zfoot { padding: 18px 16px 22px; font-size: 13.5px; color: var(--muted); border-top: 1px solid var(--rule); } .zfoot p { margin: 0 0 6px; max-width: 80ch; }
  .zbar img { width: 32px; height: 32px; flex: none; }
  .zbar h1 { font: 600 19px/1.2 "Archivo", sans-serif; letter-spacing: -0.01em; margin: 0; }
  .zbar h1 a { color: var(--ink); text-decoration: none; }
  .zbar .model, .zbar nav, .zskins, .zhow .label { font: 12.5px/1.5 "IBM Plex Mono", ui-monospace, Menlo, monospace; letter-spacing: 0.045em; color: var(--muted); }
  .zbar .model { flex: 1 1 200px; }
  .zbar nav { display: flex; flex-wrap: wrap; gap: 4px 14px; margin: 0; }
  .zbar nav a, .zbar nav button { color: var(--muted); text-decoration: none; background: none; border: 0; padding: 0; font: inherit; letter-spacing: inherit; cursor: pointer; }
  .zbar nav a:hover, .zbar nav button:hover { color: var(--ink); }
  .zhow { padding: 18px 16px; border-top: 1px solid var(--rule); }
  .zhow h2 { font: 600 24px/1.15 "Archivo", sans-serif; letter-spacing: -0.02em; margin: 0 0 6px; }
  .zhow p { margin: 0 0 12px; max-width: 64ch; }
  .device svg.zlab { z-index: 1; } .zlab text { font-family: "Archivo", "Helvetica Neue", Arial, sans-serif; text-anchor: middle; }
  .zlab .k { font-size: 12.5px; fill: #e8e8ea; } .zlab .s { font-size: 9.5px; fill: #c9cdd6; }
  .zlab .f { font-size: 13px; font-weight: 600; fill: #17181b; }
  :root[data-zlab="off"] .zlab { display: none; }
</style>"""
EMU_JS = """<script>
(function () {
  var k = "zodiac:theme", r = document.documentElement, t = null;
  try { t = localStorage.getItem(k); } catch (e) {}
  if (t) r.dataset.theme = t;
  document.getElementById("ztheme").addEventListener("click", function () {
    var dark = r.dataset.theme ? r.dataset.theme === "dark" : matchMedia("(prefers-color-scheme: dark)").matches;
    r.dataset.theme = dark ? "light" : "dark";
    try { localStorage.setItem(k, r.dataset.theme); } catch (e) {}
  });
  var lk = "zodiac:labels", lb = document.getElementById("zlabels"), l = null;
  function labels(on) { r.dataset.zlab = on ? "on" : "off"; lb.setAttribute("aria-pressed", on ? "true" : "false"); }
  try { l = localStorage.getItem(lk); } catch (e) {}
  labels(l !== "off");
  lb.addEventListener("click", function () {
    var on = r.dataset.zlab === "off";
    labels(on);
    try { localStorage.setItem(lk, on ? "on" : "off"); } catch (e) {}
  });
})();
</script>"""


def emu_labels():
    """what each control does, written over the emulator's own drawn panel (base/web's, viewBox 0 0 1000 640): the places
    are that drawing's. Pointer events pass through it."""
    t = lambda x, y, w, c, extra="": f'<text x="{x:.1f}" y="{y:.1f}" class="{c}"{extra}>{w}</text>'
    o = ['<svg class="zlab" viewBox="0 0 1000 640" aria-hidden="true" focusable="false">']
    for x, y, w in ((105, 143, "Volume"), (215, 143, "Choose a sign"), (105, 248, "Next sign"), (215, 248, "Tips"),
                    (113, 334, "Stop, play"), (211, 334, "Keep K1 to K4")):
        o.append(t(x, y, w, "k"))
    for i, w in enumerate(("Low pass", "High pass", "Echo", "Reverb")):
        o.append(t(610 + i * 104, 143, w, "k"))
    for i, n in enumerate(controls.SIGNS):
        o.append(t(582 + (i % 6) * 63 + 27, (174 if i < 6 else 236) + 45.5, n, "s"))
    fx = controls.effects()
    x0, y0, w, h = 36, 340, 928, 270
    pitch = (w - 24) / 16
    after = [0, 1, 2, 4, 5, 7, 8, 9, 11, 12, 14]
    longs, shorts = [0, 2, 4, 6, 7, 9, 11, 12, 14, 16, 18, 19, 21, 23, 24, 26], [1, 3, 5, 8, 10, 13, 15, 17, 20, 22, 25]
    for i, k in enumerate(longs):
        cx, cy = x0 + 12 + (i + 0.5) * pitch + 4.5, y0 + h * 0.69
        o.append(t(cx, cy, fx[k] or "The sign answers", "f", f' transform="rotate(-90 {cx:.1f} {cy:.1f})"'))
    for i, k in enumerate(shorts):
        cx, cy = x0 + 12 + (after[i] + 1) * pitch + 4.5, y0 + h * 0.225
        o.append(t(cx, cy, fx[k], "f", f' transform="rotate(-90 {cx:.1f} {cy:.1f})"'))
    return "".join(o) + "</svg>"


def main(pkg, version, out):
    out = Path(out).resolve()
    subprocess.run([sys.executable, str(ROOT / "base/tools/make_site.py"), str(ROOT / "app"), str(pkg), version, str(out)],
                   check=True)
    name = f"zodiac-{re.sub(r'[^A-Za-z0-9.-]', '-', version)}.fwsc"
    signs = "\n".join(
        f'      <li><img src="img/{n.lower()}.png" width="480" height="480" loading="lazy" alt="{n}: the sign\'s picture, as on the FM-1\'s screen">'
        f'<h2>{n.upper()} <span>{el}</span></h2><p>{line}</p></li>' for n, el, knob, line in SIGNS)
    html = (ROOT / "site/index.html").read_text(encoding="utf-8")
    html = html.replace("{{SIGNS}}", signs).replace("{{VERSION}}", version).replace("{{PKG}}", name)
    (out / "index.html").write_text(html, encoding="utf-8")
    for f in ("brand.css", "mark.png"):
        shutil.copy(ROOT / "site" / f, out / f)
    shutil.copytree(ROOT / "site/img", out / "img", dirs_exist_ok=True)
    (out / "controls").mkdir(exist_ok=True)
    page = (ROOT / "site/controls.html").read_text(encoding="utf-8")
    (out / "controls/index.html").write_text(page.replace("{{PANEL}}", controls.panel()).replace("{{SKINS}}", controls.skin_buttons()).replace("{{VERSION}}", version),
                                             encoding="utf-8")
    inst = out / "install/index.html"
    h = inst.read_text(encoding="utf-8")
    if h.count("</head>") != 1 or h.count("<body>") != 1:
        raise SystemExit("the installer's page has changed shape: tools/site.py no longer fits it")
    # the base writes the build's name ("zodiac") into its sentences: there it is the firmware's, with a capital
    h = re.sub(r'(?<![\w/.\-=>])zodiac(?![\w.\-/:"])', "Zodiac", h)
    row = "".join(f'<img src="../img/{n.lower()}.png" alt="">' for n, *_ in SIGNS)
    h = h.replace("</head>", f'<link rel="icon" type="image/png" href="../mark.png">\n{FONTS}\n{INSTALL_CSS}\n</head>')
    h = h.replace("<body>", f'<body>\n<div class="zbar"><img src="../mark.png" alt=""><a href="../">zodiac</a>'
                            f'<span>install · {version}</span></div>\n<div class="zrow" aria-hidden="true">{row}</div>')
    inst.write_text(h, encoding="utf-8")
    # the browser emulator: Zodiac's bar and nav on top, and under it the drawing of what every control does
    emu = out / "emu/index.html"
    h = emu.read_text(encoding="utf-8")
    head = "<header>\n  <h1>zodiac</h1>\n</header>"
    if h.count("</head>") != 1 or h.count(head) != 1 or h.count("</main>") != 1:
        raise SystemExit("the emulator's page has changed shape: tools/site.py no longer fits it")
    h = h.replace("<title>zodiac in the browser</title>", "<title>Zodiac in the browser</title>")
    h = h.replace("</head>", f'<link rel="icon" type="image/png" href="../mark.png">\n{FONTS}\n{EMU_CSS}\n</head>')
    h = h.replace(head, f'<div class="zwall" aria-hidden="true"></div>\n<div class="zsheet">\n<header class="zbar"><img src="../mark.png" alt=""><h1><a href="../">zodiac</a></h1>'
                        f'<span class="model">in the browser · {version}</span>'
                        '<nav aria-label="Zodiac"><button type="button" id="ztheme">theme</button><button type="button" id="zlabels" aria-pressed="true">labels</button><a href="../">home</a>'
                        '<a href="../controls/">controls</a><a href="../install/">install</a>'
                        '<a href="https://github.com/stephencummins/fm1-zodiac">source</a><a href="https://aisling.stephen8n.com/">aisling</a>'
                        '<a href="https://stephen8n.com">stephen8n</a></nav></header>')
    ctl = '<div id="controls"'
    if h.count(ctl) != 1:
        raise SystemExit("the emulator's page has changed shape: tools/site.py no longer fits it")
    h = h.replace(ctl, emu_labels() + "\n  " + ctl)
    h = h.replace("</main>", "</main>\n<section class=\"zhow\"><p>To begin you need three: OCT− stops the music and starts it again, "
                             "MASTER is the volume, and ALGORITHM turns through the tips on the screen. Each lower button is an effect "
                             "for as long as you hold it. <a href=\"../controls/\">The controls, in words</a>.</p></section>\n<footer class=\"zfoot\"><p>Zodiac is free software under the GNU General Public License, "
                             "version 3. Source: <a href=\"https://github.com/stephencummins/fm1-zodiac\">github.com/stephencummins/fm1-zodiac</a>.</p>"
                             "<p>Not made by or connected with M-VAVE.</p></footer>\n</div>\n" + EMU_JS, 1)
    emu.write_text(h, encoding="utf-8")
    print(f"zodiac's pages: {out}/index.html, install/ and emu/ dressed")


if __name__ == "__main__":
    if len(sys.argv) < 3:
        raise SystemExit(__doc__)
    main(Path(sys.argv[1]).resolve(), sys.argv[2], sys.argv[3] if len(sys.argv) > 3 else ROOT / "build/site")
