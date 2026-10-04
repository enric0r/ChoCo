# ChoCo diagnostics

Release builds use `CHOCO_LOG_LEVEL_NONE` and intentionally produce no serial messages.
For diagnostics, temporarily set `CHOCO_LOG_LEVEL` in `lib/Config/Config.h` to
`CHOCO_LOG_LEVEL_INFO` or `CHOCO_LOG_LEVEL_DEBUG`, build and flash, then run
`pio device monitor`. Restore `NONE` for normal performance testing.

## Startup

INFO logging reports MIDI initialization, controls initialization, I2C devices,
display initialization and splash completion. The splash runs during setup for
approximately two seconds; it does not add delays to normal playback. If OLED
buffer allocation fails, controls and MIDI continue without display rendering.

## Input and playback

- Matrix rows are GP2, GP1, GP0; columns are GP3, GP4, GP5, GP6 with pull-ups.
  Only the row being scanned drives LOW; inactive rows are inputs.
- Key and joystick-button press/release edges must remain stable for
  `INPUT_DEBOUNCE_MS` (default 10 ms). Tune this in `Config.h`.
- DEBUG logs include joystick variations and voicing decisions. INFO includes
  chord playback and mode changes. `C+B` prints chord history only with INFO
  or higher enabled; otherwise the display reports `Serial log OFF`.
- Host regressions run with `bash test/run_host_tests.sh` or, on Windows,
  `wsl -d Ubuntu -- bash test/run_host_tests.sh`. Use a native compiler, not an ARM
  cross-compiler. The host harness simulates GPIO, time and MIDI events.

## Physical acceptance checks

Use a MIDI event monitor or a DAW and test:

1. Tap and hold every degree. Each press should start once; release should stop
   its notes, including when C or another degree remains held.
2. Hold one degree, press another, then release the newer degree. It should stop
   unless latch is enabled, without replaying the older held degree.
3. Enable strum with C+A+joystick short press. Latch must stay unchanged. Release
   a chord during its attack and check that no delayed notes appear afterwards.
4. Enable smart voicing and strum together, then switch degrees quickly. Check for
   stuck or missing notes. Toggle bass while a chord sounds and check note-offs.
5. Leave the device idle for the screensaver, then press a degree once. It should
   both wake the display and play. Hold a chord beyond 30 seconds: status stays visible.
6. Check all joystick directions, center return, OLED labels and shortened text.
   Display degree 1 means physical key 0. The horizontal arrow mirror has been removed, including diagonals; the
   vertical mapping is retained. Validate all directions against the mounted device.
7. Check USB disconnect/reconnect and long sessions with your actual MIDI host.
   Host tests cannot validate USB delivery, audible latency or analog noise.
8. Hold a joystick variation, move partway toward center and play a new degree:
   the variation should remain until the release threshold is crossed. Release
   the degree and center the stick together: no extra note-ons should appear.
9. Short-click the joystick without C while a latched chord or strum is active.
   Playback should stop; latch/strum settings should remain unchanged. The held
   degree must be released and pressed again before it plays again.

A matrix without per-key diodes can produce ambiguous ghost keys with certain
multi-key combinations. The host harness does not model that electrical effect.
If chords or modifiers behave unexpectedly, first test individual switches and
then the specific combination on the real board.
