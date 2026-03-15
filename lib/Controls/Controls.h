#ifndef CONTROLS_H
#define CONTROLS_H

#include <stdint.h>

#define NO_KEY '\0'

enum class JoystickDirection : uint8_t {
  Center = 0,
  Up,
  UpRight,
  Right,
  DownRight,
  Down,
  DownLeft,
  Left,
  UpLeft
};

struct ControlSnapshot {
  char debouncedKey;
  char rawKey;
  bool modifierCHeld;
  int joyX;
  int joyY;
  bool joyBtnHeld;
  JoystickDirection joyDirectionInstant;
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
void handleJoystick(int x, int y, bool modifierCHeld, bool joyBtnHeld, char rawKey);

// Seed the joystick direction state to avoid immediate re-application
// of the same variation right after starting a chord with a held joystick.
void primeJoystickDirection(JoystickDirection direction);

#endif
