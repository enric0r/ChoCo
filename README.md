# ChoCo - USB MIDI Chord Controller (RP2040 + Keypad + Joystick + OLED)

ChoCo is a compact USB-MIDI controller for RP2040 boards.  
It plays scale-aware chords from a 3x4 keypad, applies chord variations from an analog joystick, and shows live status on a 128x64 SSD1306 OLED.

## Highlights

- Class-compliant USB MIDI over TinyUSB
- 3x4 keypad for chord degrees and functions
- 8-way joystick chord variations with mode-based mappings
- Per-degree inversion memory and optional smart voicing
- Smart voicing mode for smoother full-voice motion
- Chord latch mode and optional strummed chord attack
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
- `C` + `A`: toggle chord latch mode
- `C` + `B`: print chord history to serial

Release behavior:

- Active chord is stopped after a short release debounce (`~50 ms`)
- If latch mode is on, chord stays active after key release

## Joystick Controls

How it works:

- Hold a degree key (`0..6`) and move joystick to modify chord quality in real-time.
- Return joystick to center to restore the base diatonic triad for that degree.

Modifier behavior:

- `C` + joystick left/right: octave offset down/up (range `-2..+2`)
- `C` + joystick down: cycle joystick mode (`DEFAULT -> EXTENDED -> CHROMATIC`)
- `C` + joystick button short press: toggle bass mode
- `C` + `A` + joystick button short press: toggle strum mode
- `C` + joystick button long press: toggle smart voicing mode

### Joystick Chord Map (DEFAULT mode)

- Up: `Major <-> Minor` (depends on base quality)
- Down: `sus4`
- Left: `dim` (major base) / `minor` (minor or diminished base)
- Right: `Maj7` (major base) / `min7` (minor or diminished base)
- Up-left: `aug`
- Up-right: `dom7`
- Down-left: `Maj6` (major base) / `sus2` (minor or diminished base)
- Down-right: `Maj9` (major base) / `min9` (minor or diminished base)

### Joystick Chord Map (EXTENDED mode)

- Up: `Major <-> Minor`
- Down: `dom7#9`
- Left: `sus4+7`
- Right: `add11`
- Up-left: `m7b5` (half-diminished 7)
- Up-right: `dom9`
- Down-left: `add9`
- Down-right: `min11`

### Joystick Chord Map (CHROMATIC mode)

- Up: `min(maj7)`
- Down: `Maj13`
- Left: `m7b5`
- Right: `6/9`
- Up-left: `Maj7#11`
- Up-right: `dom13`
- Down-left: `dom7b9`
- Down-right: `dom7alt`

Chromatic bonus:

- In `CHROMATIC` mode, with no chord key held, joystick left/right shifts key root by `-1/+1` semitone.

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
- Smart voicing:
  - `SMART_VOICING_RANGE_MIN`, `SMART_VOICING_RANGE_MAX`
  - `SMART_VOICING_OCTAVE_MIN`, `SMART_VOICING_OCTAVE_MAX`
  - `SMART_WEIGHT_MOTION`, `SMART_WEIGHT_COMMON_TONE`
  - `SMART_WEIGHT_RANGE`, `SMART_WEIGHT_SPAN`, `SMART_TARGET_SPAN`
- Latch/strum:
  - `CHORD_LATCH_ENABLED_DEFAULT`
  - `STRUM_MODE_ENABLED_DEFAULT`
  - `STRUM_NOTE_DELAY_MS`
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

## Smart Voicing

When smart voicing mode is enabled, chord content stays fixed, but inversion/register
are selected to reduce total voice movement and keep common tones when possible.

Scoring priorities:

- lower total movement between previous and next voiced notes
- reward shared/common tones
- soft penalties for notes outside preferred range
- mild span shaping around `SMART_TARGET_SPAN`

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

## Contributing

See `CONTRIBUTING.md` for setup, tests, and PR expectations.

## License

Licensed under the PolyForm Noncommercial License 1.0.0. See `LICENSE`.
