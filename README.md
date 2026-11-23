# Zodiac

A firmware for the M-VAVE FM-1: twelve signs of the zodiac, each a moving picture and a piece of ambient music
that plays itself. No numbers are shown on the screen: the picture is the readout.

**State: runs on a computer and in a browser. It has not yet been built for, or run on, an FM-1.**

## The twelve pieces

Every piece is ambient and changes by itself: the tempo drifts, and most pieces move between three and four
beats in the bar. The lowest key sets off the sign's own event, in the music and in the picture together; left
alone, each piece does this by itself now and then.

| Sign | The piece | On the lowest key |
|---|---|---|
| Aries | soft, dark, slightly jazzy horns | a blare, as the ram bleats |
| Taurus | cellos in long notes over a low bass, plodding | a huff |
| Gemini | a light dance for two harpsichord hands, the twins | both run up the scale together |
| Cancer | a sweet, sad violin | a paso doble with castanets |
| Leo | a majestic march | a trumpet fanfare, as the lion roars |
| Virgo | a jig on a hurdy-gurdy | a turn of the crank |
| Libra | floating violins and wind | a gust |
| Scorpio | deep, warm, slow notes | one very deep note and a bell |
| Sagittarius | a heroic theme | the arrow is loosed: the brass rips upward |
| Capricorn | alpine horns in a minor key over a rumble | an earthquake |
| Aquarius | airy and glassy: small bells falling in ripples | the water pours: the bells run upward |
| Pisces | a rushing torrent, a waterfall | a fish darts: a splash and bubbles |

## What is here

- `core/`   everything that is Zodiac, with no hardware in it: the drawing (`gfx`, `sign`, `scene`), one file for
  each sign's picture (`aries.c` …) and one for each sign's piece (`m_aries.c` …), and the shared instruments,
  room and echo (`music.c`, `music.h`)
- `port/web/`   the browser page; `port/host/`   the computer build that writes sound files
- `art/`   the pictures' sources: drawings (`*.svg`), the chosen pictures (`art/pic/`) and the candidates they
  were chosen from (`art/gen/`)
- `tools/`   the build scripts below, and the converters from pictures to C headers
- `app/`   the FM-1 app. Still fm1-base's template under a new name; it does not yet run the core
- `base/`   [fm1-base](https://github.com/charlesvestal/fm1-base), a git submodule

## Try it

    git clone --recurse-submodules <this repository>
    sh tools/web.sh           # builds build/zodiac.html: open it from the disk. Needs emcc and python3
    sh tools/sound.sh 120 23  # each piece to build/sound/<sign>.wav: 120 seconds, a key every 23. Needs cc
    sh tools/stills.sh        # stills of each sign to build/stills/

In the page, the `Sound` button starts the music.

## On the FM-1 (so far only in the simulator)

`app/` is the FM-1 firmware: fm1-base's platform around the core (`app/core` is a link to `core/`).
PRESET steps through the signs, SELECT turns the wheel of their symbols (the one it rests on opens), the twelve buttons FX to REC are the twelve signs in order, a MIDI program change 0 to 11 chooses a sign, and OCT+ keeps
the knobs for next time (every switch-on begins with the welcome, at Aries). ALGO turns through the tips. The knobs and keys are a DJ's hands on the piece (`z_dj` in `core/music.h`); with the knobs at
zero and no key held, a piece sounds exactly as written.

| Control | What it does |
|---|---|
| K1 | low pass, with resonance: up closes the piece down to a rumble |
| K2 | high pass, with resonance: up thins it to a whisper |
| K3 | echo: the whole piece into a dub echo that feeds on itself |
| K4 | reverb: a great hall |
| lowest key, any MIDI note | strikes: the sign answers |
| white keys, low to high | Echo, Dub, Repeat, Roll, Gate, Pump, Half time, Tape stop, Wow, Drive, Crush, Ring, Phaser, Wash, Freeze |
| black keys, low to high | Close, Rise, Sweep, Wah, Drop, Radio, Riser, Siren, Vinyl, Spin, Cut |

A key's effect lasts while the key is held, and several can be held together. They use no memory of their own:
Repeat and Roll borrow the echo's line, and Half time, Tape stop and Wow play the piece itself slower.

    ./build.sh --host
    app/build/host/zodiac_host app/tests/basic.txt app/build/host

For the FM-1 the core is built with three switches (`app/fm1.toml`): `Z_PACKED` (the pictures packed, 691 KB
down to 286 KB, unpacked one at a time), `Z_ECHO16` (the echo line as 16-bit numbers) and `Z_FM1` (the big
buffers in the platform's pool). The computer and browser builds do not use them and sound as before.

## Not done yet

- Built for the FM-1 once (22 November 2026, in a container on another Mac; this one has no container runtime):
  the image is 457,352 bytes of the 581 KB slot, and memory 462,476 of 475,136 bytes, 12,660 free where 8,192
  must stay spare. It fits, with little memory left over. Nothing has been on an FM-1.
- How much of the FM-1's processor the pictures and the music need has not been measured.
- The 16-bit echo measures about 95 dB below the float one, but has not been listened to.
- `tests/` is the template's.

## Licence and credits

Zodiac is free software under the GNU General Public License, version 3 (see `LICENSE`).

The files that came from fm1-base's template (`app/app.c`, `app/app.h`, `app/words.tsv`,
`.github/workflows/release.yml`) are by Charles Vestal and stay under the Mozilla Public License 2.0, as their
headers say. `base/` is fm1-base under its own licences.

Zodiac is built on fm1-base by Charles Vestal and felucca-bsp, which come from Felucca by Leo Kuroshita,
Hügelton Instruments.

The pictures in `art/pic/` and `art/gen/` were made with an image-generating model and then chosen and
converted by hand. The music is written as code; nothing in it is sampled or quoted.
