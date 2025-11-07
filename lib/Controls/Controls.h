#ifndef CONTROLS_H
#define CONTROLS_H

#include "Config.h"

#define NO_KEY '\0'

char getKey();
// Immediate read of the matrix (no debounce, returns currently pressed key or NO_KEY)
char getRawKey();
// Debug: scan entire matrix and return a space-separated list of pressed keys in `out`.
// Safe to call occasionally for logging; ignores NO_KEY entries.
void debugScanPressedKeys(char* out, int maxLen);
// Returns true if the modifier key 'C' is currently held down.
// Uses a full matrix scan so it works even when other keys are pressed simultaneously.
bool isModifierCHeld();
void setupControls();
void handleKeyPress(char key);
void handleJoystick(int x, int y);

// Seed the joystick direction state to avoid immediate re-application
// of the same variation right after starting a chord with a held joystick.
void primeJoystickDirection(int dx, int dy);

#endif