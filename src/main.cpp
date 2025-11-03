#include "Config.h"
#include "MIDI.h"
#include "Display.h"
#include "ChordEngine.h"
#include "Controls.h"
#include <Wire.h>
#include <Adafruit_SSD1306.h>

void setup() {
    Serial.begin(115200);
    Serial.println("ChoCo MIDI Controller Starting...");
    
    //Initialize MIDI first
    setupMIDI();
    Serial.println("MIDI initialized");
    
    setupControls();
    Serial.println("Controls initialized");
    
    setupDisplay();
    Serial.println("Display initialized");
    
    drawSplashScreen();
    Serial.println("Setup complete - entering main loop");
}

void loop() {
    // Read keypad using custom scanner
    static char activeKey = NO_KEY;               // last key whose chord we started
    static unsigned long releaseStart = 0;        // when we first saw no-key

    char key = getKey();
    if (key != NO_KEY) {
        Serial.print("Key pressed: ");
        Serial.println(key);
        handleKeyPress(key);
        activeKey = key;           // track which key started the chord
        releaseStart = 0;          // reset release timing
    }
    
    // Read joystick
    int xValue = analogRead(JOYSTICK_X);
    int yValue = analogRead(JOYSTICK_Y);
    handleJoystick(xValue, yValue);
    
    // Stop chord when key is released (debounced)
    if (activeKey != NO_KEY) {
        if (getRawKey() == NO_KEY) {
            if (releaseStart == 0) {
                releaseStart = millis();
            } else if (millis() - releaseStart > 50) { // debounce release
                stopCurrentChord();
                activeKey = NO_KEY;
                releaseStart = 0;
                // Serial.println("Key released: stopping chord");
            }
        } else {
            // still held, reset release timer
            releaseStart = 0;
        }
    }

    updateDisplay();
}