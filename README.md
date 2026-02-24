# ChoCo - USB MIDI Chord Controller (RP2040 + Keypad + Joystick + OLED)

ChoCo is a compact USB-MIDI controller for RP2040 boards.  
It plays scale-aware chords from a 3x4 keypad, applies chord variations from an analog joystick, and shows live status on a 128x64 SSD1306 OLED.

## Highlights

- Class-compliant USB MIDI over TinyUSB
- 3x4 keypad for chord degrees and functions
- 8-way joystick chord variations (Maj7, Min7, Sus, 9, 11, Dim)
- Per-degree inversion memory and optional auto-voicing
- Optional bass pedal note and octave-shift controls
- OLED status display with mode badges and recent chord-degree history

## Hardware

- MCU: RP2040 board (example: Raspberry Pi Pico)
- Keypad: 3x4 matrix
  - Rows (outputs): GP2, GP1, GP0
  - Columns (inputs pull-up): GP3, GP4, GP5, GP6
- Joystick:
  - X axis: A0
  - Y axis: A1
  - Button: GP13 (active-low by default)
- OLED: SSD1306 128x64 over I2C
  - SDA: GP14
  - SCL: GP15
  - Bus selection: `I2C_PORT` (`0 = Wire`, `1 = Wire1`)

## Keypad Layout and Behavior

Physical layout (top to bottom):

- `A B C _`
- `1 3 5 _`
- `0 2 4 6`

Actions:

- `0..6`: play chord for scale degree
- `A`: increment root note (wraps chromatically)
- `B`: cycle scale type (Ionian -> ... -> Melodic Minor -> Ionian)
- `C` + `0..6`: cycle inversion for that degree
- `C` + `B`: print chord history to serial

Release behavior:

- Active chord is stopped after a short release debounce (`~50 ms`)

## Joystick Controls

Chord variations (while chord is active):

- Up: `Maj7`
- Down: `Min7`
- Left: `Sus2`
- Right: `Sus4`
- Up-right: `9`
- Up-left: `11`
- Down-right: `Min9`
- Down-left: `Dim`

Modifier behavior:

- `C` + joystick left/right: octave offset down/up (range `-2..+2`)
- `C` + joystick button short press: toggle bass mode
- `C` + joystick button long press: toggle auto-voicing mode

## Build and Upload (PlatformIO)

From CLI:

```powershell
pio run
pio run -t upload
pio device monitor
```

From VS Code:

1. Open this folder in VS Code
2. Build with PlatformIO
3. Upload firmware
4. Open serial monitor for diagnostics

## Configuration Reference (`lib/Config/Config.h`)

- Display:
  - `SCREEN_WIDTH`, `SCREEN_HEIGHT`, `SCREEN_ADDRESS`
- I2C:
  - `I2C_SDA_PIN`, `I2C_SCL_PIN`, `I2C_PORT`
- Joystick:
  - `JOYSTICK_X`, `JOYSTICK_Y`, `JOYSTICK_BTN`
  - `JOYSTICK_SWAP_AXES`, `JOYSTICK_INVERT_X`, `JOYSTICK_INVERT_Y`
  - `JOYSTICK_CENTER_X`, `JOYSTICK_CENTER_Y`
  - `JOYSTICK_TRIGGER_ENGAGE_PCT`, `JOYSTICK_TRIGGER_RELEASE_PCT`
  - `JOYSTICK_BUTTON_ACTIVE_LOW`
  - `JOYSTICK_GRACE_PERIOD`
- Timing:
  - `SPLASH_SCREEN_DURATION`
  - `STATUS_MESSAGE_DURATION`
  - `SCREENSAVER_TIMEOUT_MS`
  - `SCREENSAVER_ANIMATION_INTERVAL_MS`
- MIDI naming:
  - `USB_USE_CUSTOM_DESCRIPTORS`
  - `USB_MANUFACTURER`
  - `USB_PRODUCT`
- Logging:
  - `CHOCO_LOG_LEVEL`
  - `LOG_CHORDS`

## Project Structure

- `src/main.cpp` - runtime loop and integration flow
- `lib/Controls/*` - keypad scanning, snapshots, joystick actions
- `lib/ChordEngine/*` - harmony logic, voicing, chord state/history
- `lib/Display/*` - OLED rendering, splash, screensaver, status
- `lib/MIDI/*` - TinyUSB MIDI transport
- `lib/Config/*` - pins, timing, compile-time flags
- `test/logic/test_chord_logic.cpp` - fast logic boundary tests
- `test/logic/test_joystick_direction.cpp` - joystick octant/radius classifier tests

## Joystick Trigger Tuning

Joystick direction triggers now use an outer-ring octant classifier with hysteresis:

- Engage radius: `JOYSTICK_TRIGGER_ENGAGE_PCT` (default `72`)
- Release radius: `JOYSTICK_TRIGGER_RELEASE_PCT` (default `55`)
- Fixed center: `JOYSTICK_CENTER_X`, `JOYSTICK_CENTER_Y` (default `512`, `512`)

Recommended presets:

- More edge-focused:
  - `JOYSTICK_TRIGGER_ENGAGE_PCT=80`
  - `JOYSTICK_TRIGGER_RELEASE_PCT=62`
- More responsive:
  - `JOYSTICK_TRIGGER_ENGAGE_PCT=65`
  - `JOYSTICK_TRIGGER_RELEASE_PCT=50`

## Troubleshooting

- MIDI device not visible:
  - Confirm known-good USB data cable
  - Replug after flashing
  - Try `USB_USE_CUSTOM_DESCRIPTORS = 0` for maximum compatibility
- OLED not detected:
  - Verify power and `SCREEN_ADDRESS`
  - Verify SDA/SCL pins and selected `I2C_PORT`
  - Check serial output from I2C scanner during setup
- Keypad ghosting or missed input:
  - Confirm row/column wiring and pull-ups
  - Confirm rows idle HIGH and only one row is driven LOW during scan
- Joystick too sensitive:
  - Tune `JOYSTICK_TRIGGER_ENGAGE_PCT` and `JOYSTICK_TRIGGER_RELEASE_PCT`
  - Tune `JOYSTICK_GRACE_PERIOD` in `lib/Config/Config.h`

## Hardware Regression Checklist

Run before release:

1. Rapid tap `0..6` and confirm no stuck notes.
2. Hold `C` and press `0..6`, confirm inversion changes only (no chord playback).
3. Hold a chord and move joystick to all 8 directions, confirm correct variation names/behavior.
4. Hold `C` and move joystick left/right, confirm one octave step per direction change.
5. Hold `C` and short/long press joystick button, confirm bass and auto-voicing toggles.
6. Leave idle until screensaver starts, then wake by key or joystick movement.
7. Confirm OLED top-line state badges reflect edit/bass/auto-voicing modes.
