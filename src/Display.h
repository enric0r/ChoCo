#ifndef DISPLAY_H
#define DISPLAY_H

#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

void setupDisplay();
void drawSplashScreen();
void updateDisplay();
void showStatus(String message);
String getNoteName(int noteNumber);
void drawBitmap(int16_t x, int16_t y, const uint8_t *bitmap, int16_t w, int16_t h);

// Make display object accessible to other modules if needed
extern Adafruit_SSD1306 display;

#endif