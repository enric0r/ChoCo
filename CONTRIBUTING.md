# Contributing to ChoCo

Thanks for taking the time to contribute. This project is a hobby-friendly
firmware stack, so clear communication and small, focused changes help a lot.

## Quick start

1. Install PlatformIO (VS Code extension or CLI).
2. Build the firmware:
   - `pio run`
3. Upload to a Pico:
   - `pio run -t upload`
4. Monitor serial output:
   - `pio device monitor`

## Tests

The CI runs two fast host-side logic tests and a full firmware build.

- Fast logic tests: see `.github/workflows/build.yml` for the exact compiler
  commands used in CI.
- Firmware build: `pio run`

If you cannot run the tests locally, please mention that in your PR.

## Code style and architecture

- Keep the main loop responsive. Avoid long blocking delays in the hot path.
- Prefer fixed-size buffers over heap allocation inside `loop()`.
- Gate logs behind `CHOCO_LOG_LEVEL` to keep release builds quiet.
- Keep pin mappings and compile-time tuning in `lib/Config/Config.h`.

## Pull requests

Please include:

- A short description of the change and why it is needed.
- Tests you ran (or a note if you did not run any).
- Hardware validation details when relevant (board, wiring, and results).

## Issues

Use the issue templates when opening bugs or feature requests.
