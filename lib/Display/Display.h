#ifndef DISPLAY_H
#define DISPLAY_H

#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include "Config.h"
#include "JoystickTypes.h"

void setupDisplay();
void drawSplashScreen();
void updateDisplay();
void setEditModeIndicator(bool enabled);
void setInteractionState(char rawKey, JoystickDirection direction, bool modifierCHeld, bool joyBtnHeld);
void showStatus(const char* message, unsigned long durationMs = STATUS_MESSAGE_DURATION);
void showStatusValue(const char* label, const char* value, unsigned long durationMs);
void showStatusNumber(const char* label, int value, unsigned long durationMs);
void drawBitmap(int16_t x, int16_t y, const uint8_t *bitmap, int16_t w, int16_t h);
const char* getScaleAbbreviation(const char* fullName);

// Screensaver functions
void resetScreensaverTimer();
bool isScreensaverActive();
void updateScreensaver();

// Make display object accessible to other modules if needed
extern Adafruit_SSD1306 display;

#endif
