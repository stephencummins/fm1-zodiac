#!/bin/sh
# Build Zodiac with fm1-base (the submodule in base/, with felucca-bsp in base/bsp):
#   ./build.sh                    app/build/zodiac.fwsc, a development build
#   ./build.sh --release 0.1.0    app/build/zodiac-0.1.0.fwsc
#   ./build.sh --host             the app in fm1-base's host simulator (plain cc, no toolchain)
#   ./build.sh --web              the browser emulator: app/build/emu/
# The firmware itself needs JieLi's toolchain and the AC79 SDK: base/README.md.
set -eu
cd "$(dirname "$0")"
[ -f base/tools/build.py ] || git submodule update --init --recursive
exec python3 base/tools/build.py app "$@"
