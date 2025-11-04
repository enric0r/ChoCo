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
    static bool lastEditMode = false;             // track transitions for logging

    // Check if C button is being held (for inversion edit mode)
    bool cButtonHeld = isModifierCHeld();
    if (cButtonHeld != lastEditMode) {
        char pressed[32];
        debugScanPressedKeys(pressed, sizeof(pressed));
        Serial.print("EditMode ");
        Serial.print(cButtonHeld ? "ON" : "OFF");
        Serial.print(" | Cheld="); Serial.print(cButtonHeld ? "1" : "0");
        Serial.print(" | pressed=["); Serial.print(pressed); Serial.println("]");
        lastEditMode = cButtonHeld;
    }
    
    // Get debounced key
    char key = getKey();
    
    // Handle key presses
    if (key != NO_KEY) {
        // If we're in edit mode (C held) and a chord key (0-6) is pressed
        if (cButtonHeld && key >= '0' && key <= '6') {
            int degree = key - '0';
            Serial.print("Inversion-edit key press | degree="); Serial.print(degree);
            Serial.print(" | before inv="); Serial.println(getInversionForDegree(degree));
            cycleInversionForDegree(degree);
            int inv = getInversionForDegree(degree);
            showStatus(String("Deg ") + degree + " Inv: " + inv, 800);
            Serial.print("Inversion set | degree="); Serial.print(degree);
            Serial.print(" | after inv="); Serial.println(inv);
            // Don't play chord or set activeKey in edit mode
        }
        // Normal mode - play chords (only if C is NOT held)
        else if (!cButtonHeld && key >= '0' && key <= '7') {
            Serial.print("Play key press | key="); Serial.print(key);
            handleKeyPress(key);
            activeKey = key;
            releaseStart = 0;
        }
        // Chord history: hold C and press 'B'
        else if (cButtonHeld && key == 'B') {
            Serial.println("Printing chord history...");
            printChordHistory();
            showStatus("History", 600);
        }
        // Function keys (A, B) work normally when C not held
        else if (!cButtonHeld && (key == 'A' || key == 'B')) {
            Serial.print("Function key press | key="); Serial.println(key);
            handleKeyPress(key);
        }
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
            }
        } else {
            // still held, reset release timer
            releaseStart = 0;
        }
    }

    static unsigned long lastUi = 0;
    if (millis() - lastUi >= 100) {
        updateDisplay();
        lastUi = millis();
    }
}