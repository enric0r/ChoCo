#include "Config.h"
#include "MIDI.h"
#include "Display.h"
#include "ChordEngine.h"
#include "Controls.h"
#include "Clock.h"
#include "Arpeggiator.h"
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
    
    setupClock();
    Serial.println("Clock initialized");
    
    setupArpeggiator();
    Serial.println("Arpeggiator initialized");
    
    setupDisplay();
    Serial.println("Display initialized");
    
    drawSplashScreen();
    Serial.println("Setup complete - entering main loop");
}

void loop() {
    // Update clock system
    updateClock();
    
    // Update arpeggiator
    updateArpeggiator();
    
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
        // PANIC: C + 0 = All Notes Off
        if (cButtonHeld && key == '0') {
            Serial.println("PANIC triggered (C + 0)");
            midiPanic();
            stopCurrentChord();
            activeKey = NO_KEY;
            showStatus("PANIC!", 800);
        }
        // If we're in edit mode (C held) and a chord key (0-6) is pressed
        else if (cButtonHeld && key >= '0' && key <= '6') {
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

            // If joystick is held in a direction, apply the mapped variation immediately
            int xNow = analogRead(JOYSTICK_X);
            int yNow = analogRead(JOYSTICK_Y);
            auto dirFromAxisQuick = [](int v){ if (v < 350) return -1; if (v > 650) return 1; return 0; };
            int dx = dirFromAxisQuick(xNow);
            int dy = dirFromAxisQuick(yNow);

            const int* varIntervals = nullptr; int varSize = 0; const char* varName = nullptr;
            if (dx == 0 && dy == 1) {           // Up
                varIntervals = CHORD_MAJ7; varSize = 4; varName = "Maj7";
            } else if (dx == 0 && dy == -1) {   // Down
                varIntervals = CHORD_MIN7; varSize = 4; varName = "Min7";
            } else if (dx == -1 && dy == 0) {   // Left
                varIntervals = CHORD_SUS2; varSize = 3; varName = "Sus2";
            } else if (dx == 1 && dy == 0) {    // Right
                varIntervals = CHORD_SUS4; varSize = 3; varName = "Sus4";
            } else if (dx == 1 && dy == 1) {    // Up-Right
                varIntervals = CHORD_DOM9; varSize = 5; varName = "9";
            } else if (dx == -1 && dy == 1) {   // Up-Left
                varIntervals = CHORD_DOM11; varSize = 5; varName = "11";
            } else if (dx == 1 && dy == -1) {   // Down-Right
                varIntervals = CHORD_MIN9; varSize = 5; varName = "Min9";
            } else if (dx == -1 && dy == -1) {  // Down-Left
                varIntervals = CHORD_DIM; varSize = 3; varName = "Dim";
            }

            int degree = key - '0';
            if (varIntervals != nullptr) {
                Serial.print(" | Initial variation: "); Serial.println(varName);
                // Prime joystick state to avoid immediate duplicate re-application
                primeJoystickDirection(dx, dy);
                playChordForDegreeWithIntervals(degree, varIntervals, varSize, varName);
            } else {
                handleKeyPress(key);
            }
            activeKey = key;
            releaseStart = 0;
        }
        // Chord history: hold C and press 'B'
        else if (cButtonHeld && key == 'B') {
            Serial.println("Printing chord history...");
            printChordHistory();
            showStatus("History", 600);
        }
        // Tap tempo: hold C and press 'A'
        else if (cButtonHeld && key == 'A') {
            Serial.println("Tap tempo");
            handleTapTempo();
        }
        // Toggle arpeggiator: press 7
        else if (!cButtonHeld && key == '7') {
            setArpeggiatorEnabled(!isArpeggiatorEnabled());
            showStatus(String("Arp: ") + (isArpeggiatorEnabled() ? "ON" : "OFF"), 500);
            Serial.print("Arpeggiator: ");
            Serial.println(isArpeggiatorEnabled() ? "ON" : "OFF");
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