#!/bin/sh
# The app in fm1-base's host simulator, run through its checks.
set -eu
cd "$(dirname "$0")/.."
./build.sh --host >/dev/null
for t in app/tests/basic.txt; do app/build/host/zodiac_host "$t" app/build/host; done
