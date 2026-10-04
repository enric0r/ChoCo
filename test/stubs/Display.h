#pragma once
#include "Config.h"
#include "JoystickTypes.h"
void setupDisplay();
void drawSplashScreen();
void updateDisplay();
void setEditModeIndicator(bool);
void setInteractionState(char, JoystickDirection, bool, bool);
void showStatus(const char*, unsigned long);
void showStatusValue(const char*, const char*, unsigned long);
void showStatusNumber(const char*, int, unsigned long);
void resetScreensaverTimer();
void updateScreensaver();
