#ifndef CONTROLS_H
#define CONTROLS_H

#include <stdint.h>

#define NO_KEY '\0'

#include "../Config/JoystickTypes.h"

struct ControlSnapshot {
  char debouncedKey;
  char rawKey;
  uint16_t heldKeys; // debounced bits: degrees 0..6, A=7, B=8, C=9
  bool modifierCHeld;
  int joyX;
  int joyY;
  bool joyBtnHeld;
  JoystickDirection joyDirection;
};

char getKey();
// Immediate read of the matrix (no debounce, returns currently pressed key or NO_KEY)
char getRawKey();
// Debug: scan entire matrix and return a space-separated list of pressed keys in `out`.
// Safe to call occasionally for logging; ignores NO_KEY entries.
void debugScanPressedKeys(char* out, int maxLen);
// Returns true if the modifier key 'C' is currently held down.
// Uses a full matrix scan so it works even when other keys are pressed simultaneously.
bool isModifierCHeld();
// Poll all controls once for the current loop tick.
void pollControls(ControlSnapshot& out);
// Classify the current joystick direction using edge-trigger instant mode.
JoystickDirection classifyJoystickDirectionInstant(int x, int y);
// Lookup chord variation intervals associated with a joystick direction.
bool getChordVariationForDirection(JoystickDirection direction, int degree, const int*& intervals, int& size, const char*& name);
void setupControls();
void handleKeyPress(char key);
void handleJoystick(JoystickDirection direction, bool modifierCHeld, bool joyBtnHeld, char rawKey);
const char* getJoystickChordModeName();

// Seed the joystick direction state to avoid immediate re-application
// of the same variation right after starting a chord with a held joystick.
void primeJoystickDirection(JoystickDirection direction);

#endif
