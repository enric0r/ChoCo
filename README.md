# ChoCo — USB MIDI Chord Controller (RP2040 + Keypad + Joystick + OLED)

ChoCo is a compact USB‑MIDI controller built on RP2040 that lets you trigger diatonic chords from a keypad, shape them with a thumb joystick (7ths, 9ths, sus, dim, etc.), and see status on a small SSD1306 OLED. It enumerates as a class‑compliant USB MIDI device and works with DAWs and synths on Windows/macOS/Linux.

## Highlights

- Keypad matrix triggers chord degrees instantly; release stops the chord
- Joystick applies expressive chord variations (8‑way) only when a chord is active
- OLED shows root, scale (Maj/Min), inversion, and quick status messages
- Class‑compliant USB MIDI via TinyUSB (works without drivers)
- Built with PlatformIO (Arduino core for RP2040)

## Hardware

- MCU: Any RP2040 board (e.g., Raspberry Pi Pico)
- Keypad: 3×4 matrix
  - Rows (outputs/scan): GP2, GP1, GP0
  - Cols (inputs with pull‑ups/read): GP3, GP4, GP5, GP6
- Joystick: 2‑axis analog + push button
  - X: A0
  - Y: A1
  - Button: GP7 (active‑low by default)
- OLED: SSD1306 128×64 I2C on I2C1
  - SDA: GP14
  - SCL: GP15

Notes
- Matrix is wired column‑to‑row: rows are driven (outputs), columns are sensed (INPUT_PULLUP).
- Ensure the OLED module is 3.3V‑compatible or level‑shifted. Use pull‑ups to 3.3V.

## Keypad layout and actions

Physical mapping (top → bottom):

- A  B  C  _
- 1  3  5  _
- 0  2  4  6

Behavior
- Digits 0..6: playChordForDegree(degree)
- 7: toggle arpeggiator on/off
- A: increment root note (wraps across octaves)
- B: toggle scale (Major/Minor)
- C: modifier key (used in combinations)
- C + 0: Panic (All Notes Off)
- C + 1-6: cycle inversion for that degree
- C + A: tap tempo (tap multiple times to set BPM)
- C + B: show chord history
- Release: a brief debounce (~50 ms), then stopCurrentChord()

Wiring arrays (see `src/Controls.cpp`)
- `rowPins[ROWS] = {2, 1, 0}`
- `colPins[COLS] = {3, 4, 5, 6}`

## Joystick variations

Variations apply only when a chord is currently active:

- Up: Maj7
- Down: Min7
- Left: Sus2
- Right: Sus4
- Up‑Right: 9
- Up‑Left: 11
- Down‑Right: Min9
- Down‑Left: Dim

Responsiveness
- Deadzone tightened: X/Y < 350 ⇒ −1, > 650 ⇒ +1, otherwise 0 (see `dirFromAxis`)
- First movement out of center applies immediately; subsequent changes use a short grace period
- Grace period: `JOYSTICK_GRACE_PERIOD` = 120 ms (see `src/Config.h`)
- Button polarity: `JOYSTICK_BUTTON_ACTIVE_LOW` (1 = active‑low)

## Clock and Timing

ChoCo includes an internal clock system with the following features:

- **Tap Tempo**: Hold C and press A multiple times to set BPM (40-240 range)
- **Internal Clock**: Runs at the set BPM (default 120 BPM)
- **Beat Indicator**: Visual metronome on OLED display shows current beat (4/4 time)
- **MIDI Clock Sync**: Can sync to incoming MIDI clock messages (24 ppqn)
- **Quantization**: Support for quantizing chord changes to beat or bar boundaries

## Arpeggiator and Strums

The arpeggiator plays chord notes in sequence, synchronized to the internal clock:

- **Toggle**: Press key 7 to enable/disable arpeggiator
- **Patterns**: UP, DOWN, UP_DOWN, RANDOM, STRUM_UP, STRUM_DOWN
- **Note Division**: 16th notes by default (configurable)
- **Sync**: All arpeggios and strums sync to the internal clock BPM
- **Strum Patterns**: Quick successive note triggering for guitar-like effects

Configuration in `src/Config.h`:
- `DEFAULT_BPM` = 120.0
- `ARP_DEFAULT_DIVISION` = 4 (16th notes)
- `STRUM_DELAY_MS` = 20 (delay between strum notes)

## Panic Function

- **C + 0**: All Notes Off (MIDI panic)
  - Sends CC 123 (All Notes Off) on all 16 MIDI channels
  - Sends Note Off for all 128 MIDI notes as fallback
  - Stops current chord and arpeggio

## USB MIDI and naming

- By default, the project uses the core’s default descriptors for maximum compatibility.
- You can force custom VID/PID and strings in `src/Config.h`:
  - `USB_USE_CUSTOM_DESCRIPTORS` (0/1)
  - `USB_VENDOR_ID`, `USB_PRODUCT_ID`
  - `USB_MANUFACTURER`, `USB_PRODUCT`, `USB_SERIAL`, `USB_MIDI_INTERFACE`

Tip (Windows): If USB name changes don’t appear, unplug/replug and try a different product string to refresh the cache.

## Build and upload (PlatformIO)

VS Code + PlatformIO (recommended)
1) Install the PlatformIO extension
2) Open the `ChoCo` folder
3) Build (checkmark), then Upload (arrow)
4) Monitor (plug icon) to see logs

CLI (optional)
- `pio run`
- `pio run -t upload`
- `pio device monitor`

## Configuration reference (src/Config.h)

- Display & I2C
  - `I2C_SDA_PIN` = 14, `I2C_SCL_PIN` = 15, `I2C_PORT` = 1 (Wire1)
- Joystick
  - `JOYSTICK_X` = A0, `JOYSTICK_Y` = A1, `JOYSTICK_BTN` = 7
  - `JOYSTICK_BUTTON_ACTIVE_LOW` = 1
  - `JOYSTICK_GRACE_PERIOD` = 120
- Logging
  - `LOG_CHORDS` = 1 (verbose chord start/stop logs)
- Clock and Timing
  - `DEFAULT_BPM` = 120.0
  - `ARP_DEFAULT_DIVISION` = 4 (16th notes)
  - `STRUM_DELAY_MS` = 20
- USB descriptors (see previous section)

## Project structure

- `src/`
  - `main.cpp` — main loop, input polling, release handling
  - `Controls.cpp/.h` — keypad scan, joystick mapping, variations
  - `ChordEngine.cpp/.h` — chord generation, inversion, state
  - `Clock.cpp/.h` — timing, tap tempo, MIDI clock sync
  - `Arpeggiator.cpp/.h` — arpeggio and strum patterns
  - `Display.cpp/.h` — OLED status display
  - `MIDI.cpp/.h` — USB MIDI plumbing (TinyUSB)
  - `Config.h/.cpp` — pins, flags, timing, descriptors
- `lib/` — optional libraries
- `include/` — headers
- `platformio.ini` — PlatformIO project config

## Troubleshooting

- USB MIDI not recognized
  - Try setting `USB_USE_CUSTOM_DESCRIPTORS` = 0 (core defaults)
  - Change `USB_PRODUCT` temporarily to refresh the Windows cache
  - Use a known‑good USB cable/port and power‑cycle the board
- Keypad double‑triggers or cross‑talk
  - Verify row/col wiring: rows → GP2/1/0 (outputs), cols → GP3/4/5/6 (inputs with pull‑ups)
  - Ensure rows idle HIGH; only one row is pulled LOW while scanning (handled in code)
- Joystick too sensitive or too slow
  - Adjust `dirFromAxis` thresholds in `Controls.cpp`
  - Tweak `JOYSTICK_GRACE_PERIOD` in `Config.h`
- No OLED output
  - Confirm OLED power (3.3V), I2C pins (GP14/GP15), and that `I2C_PORT` is set to 1

## Roadmap / ideas

- On‑screen hints for current variation and inversion
---

Made with ❤️ for quick harmony exploration. Plug it in, press a key, and jam.
