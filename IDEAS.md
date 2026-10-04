# Practical next features

The unmodified joystick short click now provides a quick stop: it stops local
playback and cancels scheduled notes while preserving the selected modes.
It is not a MIDI-host panic command and cannot guarantee recovery from a lost
USB message or an externally held sustain pedal.

## 1. User presets

Save root, scale, octave, mode flags and per-degree inversions in a few slots.
Use an explicit save gesture/menu, a versioned record and checksum; do not write
flash on every input. Validate corrupt/older records and interrupted writes.
This needs a deliberate control/menu design before implementation.

## 2. Adjustable strum and velocity

Offer a small set of strum speeds and MIDI velocity levels, with an OLED preview.
The current note scheduler already supports variable spacing. Capture settings
when a chord starts to avoid changing an attack halfway through. Define how
these controls coexist with the existing C+joystick gestures.

## 3. Guided chord exploration

Show note names for a selected suggested degree before playing it. Label the
existing weighted suggestions as suggestions; they are not an analysis of the
audio or a guarantee of a musically correct progression. Keep the preview
independent of MIDI playback and fit it to the 128x64 display.

## 4. USB delivery diagnostics and recovery

The current transport does not check the return value of `usb_midi.write`.
Add a bounded outgoing event queue with explicit overflow handling, retry of
unsent packets, connection-state handling and a visible delivery-error state.
Define reconnect behaviour first (silent reset versus resume held notes).
Test full buffers, disconnects during strum, and note-off delivery on a real
host; the current simulated MIDI sink cannot establish those guarantees.

## 5. Joystick calibration

Add an explicitly invoked center-calibration gesture with a stability check
and bounded offsets. Avoid automatic startup calibration while the stick might
be held. Test noisy/resting/off-center input and show success or rejection.

These items are proposals; only the quick-stop gesture is implemented here.
