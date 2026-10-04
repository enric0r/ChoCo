#include <cassert>
#include <cstring>
#include <vector>
#include "ChordEngine.h"
#include "Controls.h"
#include "Display.h"

void setup();
void loop();
TestSerial Serial;
static uint32_t clockMs = 3000;
static uint16_t keysDown = 0;
static bool buttonDown = false;
static int joyX = 512, joyY = 512;
static int modes[30] = {};
static bool sleeping = false;
static std::vector<int> midi;
unsigned long millis() { return clockMs; }
void pinMode(int pin, int mode) { modes[pin] = mode; }
void digitalWrite(int, int) {}
void delayMicroseconds(unsigned int) {}
void analogReadResolution(int) {}
int analogRead(int pin) {
  const int x = JOYSTICK_INVERT_X ? 1023 - joyX : joyX;
  const int y = JOYSTICK_INVERT_Y ? 1023 - joyY : joyY;
  return pin == JOYSTICK_X ? (JOYSTICK_SWAP_AXES ? y : x) : (JOYSTICK_SWAP_AXES ? x : y);
}
int digitalRead(int pin) {
  if (pin == JOYSTICK_BTN) return buttonDown ? LOW : HIGH;
  static const int rows[] = {2, 1, 0};
  static const int map[3][4] = {{0, 2, 4, 6}, {1, 3, 5, -1}, {7, 8, 9, -1}};
  if (pin >= 3 && pin <= 6) {
    for (int r = 0; r < 3; ++r) {
      int bit = map[r][pin - 3];
      if (modes[rows[r]] == OUTPUT && bit >= 0 && (keysDown & (1u << bit))) return LOW;
    }
  }
  return HIGH;
}
void setupMIDI() {}
void midiNoteOn(byte pitch, byte) { midi.push_back(pitch); }
void midiNoteOff(byte pitch, byte) { midi.push_back(-int(pitch)); }
void setupDisplay() {}
void drawSplashScreen() {}
void updateDisplay() {}
void setEditModeIndicator(bool) {}
void setInteractionState(char, JoystickDirection, bool, bool) {}
void showStatus(const char*, unsigned long) {}
void showStatusValue(const char*, const char*, unsigned long) {}
void showStatusNumber(const char*, int, unsigned long) {}
void resetScreensaverTimer() { sleeping = false; }
void updateScreensaver() {}

static void tick(uint32_t ms = 1) { clockMs += ms; loop(); }
static void press(uint16_t mask) { keysDown = mask; tick(); tick(INPUT_DEBOUNCE_MS); }

int main() {
  setup();
  sleeping = true;
  press(1u << 0);
  assert(!sleeping && isChordActive()); // first wake gesture also plays
  assert(midi == std::vector<int>({60, 64, 67}));
  keysDown = 0; tick(1); keysDown = 1; tick(1); tick(20);
  assert(midi.size() == 3); // contact bounce must not retrigger
  press((1u << 0) | (1u << 9));
  press(1u << 9);
  assert(!isChordActive()); // C still held must not sustain the released degree
  press(0);

  press(1u << 0);
  press((1u << 0) | (1u << 1));
  assert(getActiveChordDegree() == 1); // new degree wins, even with old held
  press(1u << 0);
  assert(!isChordActive()); // old degree is not spuriously retriggered
  press(0);

  // C+A+button changes strum without also toggling latch.
  press((1u << 9) | (1u << 7));
  buttonDown = true; tick(); tick(INPUT_DEBOUNCE_MS);
  buttonDown = false; tick(40); tick(INPUT_DEBOUNCE_MS);
  press(0);
  assert(isStrumMode() && !isChordLatchMode());
  press((1u << 9) | (1u << 7));
  press(0);
  assert(isChordLatchMode());
  setChordLatchMode(false);

  midi.clear();
  press(1u << 0);
  assert(midi == std::vector<int>({60}));
  assert(isChordPlaybackPending());
  press(0); // release before second note is due
  tick(100);
  assert(midi == std::vector<int>({60, -60}));
  assert(!isChordActive() && !isChordPlaybackPending());

  setStrumMode(false);
  setCurrentInversion(4);
  playChord(60, CHORD_MAJ9, 5, "Maj9");
  assert(std::strcmp(getCurrentChordName(), "C Maj9") == 0); // lowest is C, not D
  setCurrentInversion(1);
  playChord(60, CHORD_MAJ, 3, "Maj");
  assert(std::strcmp(getCurrentChordName(), "C Maj/E") == 0);
  setBassMode(true);
  assert(midi.back() == 48);
  assert(std::strcmp(getCurrentChordName(), "C Maj") == 0);
  setBassMode(false);
  assert(midi.back() == -48);
  assert(std::strcmp(getCurrentChordName(), "C Maj/E") == 0);
  stopCurrentChord();
  assert(std::strcmp(getCurrentChordName(), "") == 0);
  assert(std::strcmp(getNoteName(-1), "B") == 0);
  assert(getChordHistoryCount() <= CHORD_HISTORY_SIZE);

  // A pitch shared by an old chord and the next bass must stay sounding.
  setCurrentInversion(0);
  setBassMode(true);
  playChord(60, CHORD_MAJ, 3, "Maj");
  midi.clear();
  playChord(72, CHORD_MAJ, 3, "Maj");
  for (int event : midi) assert(event != -60 && event != 60);
  stopCurrentChord();
  int off60 = 0;
  for (int event : midi) if (event == -60) ++off60;
  assert(off60 == 1);
  setBassMode(false);

  // Invalid voicings must not relabel the previously sounding chord.
  playChordForDegree(0);
  setCurrentRootNote(71);
  setOctaveOffset(2);
  const int outOfRange[] = {0, 4, 30};
  playChordForDegreeWithIntervals(6, outOfRange, 3, "Invalid");
  assert(getActiveChordDegree() == 0);
  assert(std::strcmp(getCurrentChordName(), "C Maj") == 0);
  stopCurrentChord();
  setCurrentRootNote(BASE_NOTE);
  setOctaveOffset(0);

  // A newly played chord in the hysteresis band must keep the held variation.
  joyX = 1023; tick(200);
  joyX = 800; tick();
  press(1);
  assert(std::strcmp(getCurrentChordName(), "C Maj7") == 0);
  press(0);
  joyX = 512; tick();

  // Disabling latch must stop its chord even if an inversion-edit key is held.
  setChordLatchMode(true);
  press(1); press(0);
  press(1u << 9);
  press((1u << 9) | (1u << 1));
  press((1u << 9) | (1u << 1) | (1u << 7));
  press((1u << 9) | (1u << 1));
  assert(!isChordLatchMode() && !isChordActive());
  press(0);

  // Releasing the degree and centering the joystick in the same scan must
  // not briefly start the base triad before sending note-offs.
  joyX = 1023; tick(200);
  press(1);
  keysDown = 0; tick();
  midi.clear();
  joyX = 512; tick(INPUT_DEBOUNCE_MS);
  assert(!isChordActive());
  for (int event : midi) assert(event < 0);

  // Unmodified short click stops a latched chord without changing modes.
  setChordLatchMode(true);
  press(1); press(0);
  assert(isChordActive());
  buttonDown = true; tick(); tick(INPUT_DEBOUNCE_MS);
  buttonDown = false; tick(40); tick(INPUT_DEBOUNCE_MS);
  assert(!isChordActive() && isChordLatchMode() && !isSingleNoteMode());

  // Stop also cancels a strum while the degree remains physically held.
  setChordLatchMode(false);
  setStrumMode(true);
  press(1);
  buttonDown = true; tick(); tick(INPUT_DEBOUNCE_MS);
  buttonDown = false; tick(); tick(INPUT_DEBOUNCE_MS);
  assert(!isChordActive() && !isChordPlaybackPending());
  midi.clear(); tick(200);
  assert(midi.empty());
  press(0);
}
