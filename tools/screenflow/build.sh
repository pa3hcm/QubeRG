#!/bin/sh
# Renders all QubeRG screens from the real firmware and builds docs/screens.svg/.png
# Needs: g++, python3, the PlatformIO libraries (run "pio run" once), Inkscape for the PNG.
set -e
cd "$(dirname "$0")"
ROOT=../..
GFX="$ROOT/.pio/libdeps/esp32dev/Adafruit GFX Library"
TMP=$(mktemp -d)
trap 'rm -rf "$TMP"' EXIT

g++ -std=c++17 -Istubs -I"$GFX" -I"$ROOT/src" -o "$TMP/screens" screens.cpp
"$TMP/screens" "$TMP"
# second build with a callsign set, for the Beacon screen
sed 's/CALLSIGN = "N0CALL"/CALLSIGN = "PA3HCM"/' "$ROOT/src/main.cpp" > "$TMP/main.cpp"
g++ -std=c++17 -Istubs -I"$GFX" -I"$TMP" -o "$TMP/screens_b" screens.cpp
"$TMP/screens_b" "$TMP"

python3 make_diagram.py "$TMP" "$ROOT/docs/screens.svg"
