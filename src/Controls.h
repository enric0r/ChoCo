#ifndef CONTROLS_H
#define CONTROLS_H

#include "Config.h"

#define NO_KEY '\0'

char getKey();
// Immediate read of the matrix (no debounce, returns currently pressed key or NO_KEY)
char getRawKey();
void setupControls();
void handleKeyPress(char key);
void handleJoystick(int x, int y);

#endif