#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")/.."
out="$(mktemp -d)"
trap 'rm -rf "$out"' EXIT
cxx="${CXX:-g++}"
flags=(-std=c++17 -Wall -Wextra -pedantic)
for spec in chord_logic:ChordEngine joystick_direction:Controls debounced_input:Controls chord_playback:ChordEngine display_direction:Display; do
  name="${spec%%:*}"
  lib="${spec##*:}"
  "$cxx" "${flags[@]}" "-Ilib/$lib" "test/logic/test_$name.cpp" -o "$out/$name"
  "$out/$name"
  echo "PASS $name"
done
"$cxx" "${flags[@]}" -Itest/stubs -Ilib/Config -Ilib/Controls -Ilib/ChordEngine -Ilib/MIDI \
  test/logic/test_firmware_controls.cpp src/main.cpp lib/Controls/Controls.cpp \
  lib/ChordEngine/ChordEngine.cpp -o "$out/firmware_controls"
"$out/firmware_controls"
echo "PASS firmware_controls"
