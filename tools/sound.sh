#!/bin/sh
# Renders every piece to build/sound/<sign>.wav and prints its levels. Usage: sh tools/sound.sh [seconds] [a key every N seconds]
set -e
cd "$(dirname "$0")/.."
mkdir -p build/sound
cc -std=c99 -O2 -Wall -Wextra -Wno-misleading-indentation -ffp-contract=off core/music.c core/m_*.c port/host/wav.c -o build/zwav
i=0
for s in aries taurus gemini cancer leo virgo libra scorpio sagittarius capricorn aquarius pisces; do
    build/zwav $i "${1:-120}" build/sound/$s.wav 0.45 0 0.15 0.7 "${2:-23}" | tail -1
    i=$((i+1))
done
python3 tools/levels.py
