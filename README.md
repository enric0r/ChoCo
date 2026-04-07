# ChoCo

ChoCo is a compact USB MIDI chord controller for RP2040 boards. It combines a 3x4 keypad, an analog joystick, and a 128x64 OLED to play scale-aware chords, apply live variations, and show the current state directly on the device.

<p align="center">
  <img src="hardware/front.jpg" alt="ChoCo front view" width="48%">
  <img src="hardware/back.jpg" alt="ChoCo back view" width="48%">
</p>

## What It Does

- Sends class-compliant USB MIDI over TinyUSB
- Maps keypad degrees `0..6` to scale-aware chord playback
- Uses the joystick for real-time chord variations, octave shifts, and mode changes
- Stores inversions per degree
- Supports chord latch, bass note, strum, and smart voicing modes
- Displays status, mode badges, and recent chord history on an SSD1306 OLED

## Hardware

Target platform: RP2040 running Arduino via PlatformIO.

### Default pin mapping

- Keypad rows: `GP2`, `GP1`, `GP0`
- Keypad columns: `GP3`, `GP4`, `GP5`, `GP6`
- Joystick X/Y: `A0`, `A1`
- Joystick button: `GP13`
- OLED SDA/SCL: `GP14`, `GP15`
- I2C bus: `Wire1` (`I2C_PORT = 1`)

Hardware design files live in `hardware/`:

- `ChoCo.kicad_sch`
- `ChoCo.kicad_pcb`
- `ChoCo_REV-02.zip`

## Controls

### Keypad

Physical layout:

```text
A  B  C  _
1  3  5  _
0  2  4  6
```

- `0..6`: play the selected scale degree
- `A`: increment root note chromatically
- `B`: cycle scale type
- `C` + `0..6`: cycle inversion for that degree
- `C` + `A`: toggle chord latch
- `C` + `B`: print chord history to serial

### Joystick

With a degree key held, the joystick applies chord variations in real time. Returning to center restores the base diatonic triad.

- `C` + left/right: octave down/up
- `C` + down: cycle joystick mode
- `C` + joystick short press: toggle bass mode
- `C` + `A` + joystick short press: toggle strum mode
- `C` + joystick long press: toggle smart voicing mode
- In `CHROMATIC` mode with no chord held, left/right shifts the root by one semitone

## Build and Flash

### PlatformIO CLI

```powershell
pio run
pio run -t upload
pio device monitor
```

Notes:

- The default environment is `pico`.
- `platformio.ini` currently sets `upload_port = COM5`; change that locally if your board appears on another port.

### VS Code

1. Open the repository in VS Code.
2. Use the PlatformIO extension to build and upload.
3. Open the serial monitor for logs and debugging.

### Drag-and-drop UF2

The CI workflow can publish `firmware.uf2`. To install manually:

1. Hold `BOOTSEL` while connecting the RP2040 board.
2. Mount the `RPI-RP2` drive.
3. Copy `ChoCo-firmware.uf2` onto the drive.

## Configuration

Most hardware mappings and behavior tuning live in `lib/Config/Config.h`.

Useful settings include:

- OLED dimensions and I2C address
- Joystick axis swap and inversion
- Joystick engage and release thresholds
- Splash, status, and screensaver timings
- USB product/manufacturer strings
- Logging level through `CHOCO_LOG_LEVEL`
- Default states for latch, strum, and bass-related behavior

Change config values there instead of scattering constants across modules.

## Project Layout

- `src/main.cpp`: firmware setup and main event loop
- `lib/Controls/`: keypad scanning, joystick handling, control snapshots
- `lib/ChordEngine/`: harmony logic, inversions, voicing, chord history
- `lib/Display/`: OLED rendering, splash, status, screensaver
- `lib/MIDI/`: USB MIDI transport
- `lib/Config/`: compile-time configuration
- `test/logic/`: fast host-side logic tests
- `hardware/`: KiCad sources and board artifacts

## Testing

CI runs two fast logic tests plus a full firmware build.

### Firmware build

```powershell
pio run
```

### Fast host-side logic tests

```powershell
g++ -std=c++17 -Wall -Wextra -pedantic -Ilib/ChordEngine test/logic/test_chord_logic.cpp -o .test/choco_logic_tests.exe
.test/choco_logic_tests.exe
g++ -std=c++17 -Wall -Wextra -pedantic -Ilib/Controls test/logic/test_joystick_direction.cpp -o .test/choco_joystick_tests.exe
.test/choco_joystick_tests.exe
```

These tests cover degree bounds, scale wrapping, inversion normalization, and joystick direction classification. CI runs equivalent commands on Ubuntu.

## Troubleshooting

- MIDI device not detected: verify the USB cable carries data, then reconnect after flashing
- OLED not detected: check `SCREEN_ADDRESS`, SDA/SCL wiring, and selected `I2C_PORT`
- Keypad misses input: confirm row/column wiring and pull-up assumptions
- Joystick feels too sensitive: tune `JOYSTICK_TRIGGER_ENGAGE_PCT` and `JOYSTICK_TRIGGER_RELEASE_PCT`
- Need deeper serial diagnostics: see `DEBUGGING.md`

## Contributing

Contribution workflow, test expectations, and PR guidance are documented in `CONTRIBUTING.md`. Repository-specific contributor notes for agents also live in `AGENTS.md`.

## License

Licensed under the PolyForm Noncommercial License 1.0.0. See `LICENSE`.
