#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""Zodiac's site (zodiac.stephen8n.com) from a release package:

  tools/site.py app/build/zodiac-X.Y.Z.fwsc X.Y.Z [OUT_DIR, default build/site]

fm1-base's tools/make_site.py makes the installer, the browser emulator, the words files and the firmware folder;
then this puts Zodiac's own front page over the base's (site/index.html, site/brand.css, site/mark.svg, site/img)
and dresses the made installer in the same look. Nothing in base/ is changed: only the copy in OUT_DIR.
The browser emulator must be built first (./build.sh --web)."""
import re
import shutil
import subprocess
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
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
    ("Pisces", "water", "Dream", "A rushing torrent, and far inside the water three soft notes."),
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
    for f in ("brand.css", "mark.svg"):
        shutil.copy(ROOT / "site" / f, out / f)
    shutil.copytree(ROOT / "site/img", out / "img", dirs_exist_ok=True)
    inst = out / "install/index.html"
    h = inst.read_text(encoding="utf-8")
    if h.count("</head>") != 1 or h.count("<body>") != 1:
        raise SystemExit("the installer's page has changed shape: tools/site.py no longer fits it")
    # the base writes the build's name ("zodiac") into its sentences: there it is the firmware's, with a capital
    h = re.sub(r'(?<![\w/.\-=>])zodiac(?![\w.\-/:"])', "Zodiac", h)
    row = "".join(f'<img src="../img/{n.lower()}.png" alt="">' for n, *_ in SIGNS)
    h = h.replace("</head>", f'<link rel="icon" type="image/svg+xml" href="../mark.svg">\n{FONTS}\n{INSTALL_CSS}\n</head>')
    h = h.replace("<body>", f'<body>\n<div class="zbar"><img src="../mark.svg" alt=""><a href="../">zodiac</a>'
                            f'<span>install · {version}</span></div>\n<div class="zrow" aria-hidden="true">{row}</div>')
    inst.write_text(h, encoding="utf-8")
    print(f"zodiac's pages: {out}/index.html, install/ dressed")


if __name__ == "__main__":
    if len(sys.argv) < 3:
        raise SystemExit(__doc__)
    main(Path(sys.argv[1]).resolve(), sys.argv[2], sys.argv[3] if len(sys.argv) > 3 else ROOT / "build/site")
