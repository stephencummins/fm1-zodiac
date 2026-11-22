# Zodiac

A firmware for the M-VAVE FM-1: twelve signs, each a picture and a piece of music that plays itself.
Not started beyond the skeleton: `app/` is still fm1-base's template under a new name.

- `app/`   the device app (from fm1-base's template, MPL-2.0 files by Charles Vestal until rewritten)
- `base/`  fm1-base (git submodule, with felucca-bsp inside)
- `./build.sh --host` builds it for the computer; `sh tests/run_tests.sh` runs the checks
  (the template's Bluetooth checks fail: Bluetooth is off here, and the checks will be rewritten).

The plan is in `~/Projects/aisling-drafts/v2-look/PLAN.md`.
