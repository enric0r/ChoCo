#include "Config.h"
#include "MIDI.h"
#include "Display.h"
#include "ChordEngine.h"
#include "Controls.h"

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
    static char activeKey = NO_KEY;        // last key whose chord we started
    static unsigned long releaseStart = 0; // when we first saw no-key
    static int lastXValue = JOYSTICK_CENTER_X;
    static int lastYValue = JOYSTICK_CENTER_Y;

    updateScreensaver();

    ControlSnapshot controls = {};
    pollControls(controls);
    setEditModeIndicator(controls.modifierCHeld);

    // Skip normal processing if screensaver is active
    if (isScreensaverActive()) {
        const bool axisMoved = (controls.joyDirectionInstant != JoystickDirection::Center);
        if (controls.rawKey != NO_KEY || axisMoved || controls.joyBtnHeld) {
            resetScreensaverTimer();
        }
        delay(10);
        return;
    }

    const bool cButtonHeld = controls.modifierCHeld;
    const char key = controls.debouncedKey;

    // Handle key presses
    if (key != NO_KEY) {
        resetScreensaverTimer();

        if (cButtonHeld && key >= '0' && key <= '6') {
            const int degree = key - '0';
            cycleInversionForDegree(degree);
            const int inv = getInversionForDegree(degree);
            showStatus(String("Deg ") + degree + " Inv: " + inv, 800);
        } else if (!cButtonHeld && key >= '0' && key <= '6') {
            const int* varIntervals = nullptr;
            int varSize = 0;
            const char* varName = nullptr;
            const JoystickDirection direction = controls.joyDirectionInstant;
            const bool hasVariation = getChordVariationForDirection(direction, varIntervals, varSize, varName);

            const int degree = key - '0';
            if (hasVariation) {
                primeJoystickDirection(direction);
                playChordForDegreeWithIntervals(degree, varIntervals, varSize, varName);
            } else {
                handleKeyPress(key);
            }
            activeKey = key;
            releaseStart = 0;
        } else if (cButtonHeld && key == 'B') {
            printChordHistory();
            showStatus("History", 600);
        } else if (!cButtonHeld && (key == 'A' || key == 'B')) {
            handleKeyPress(key);
        }
    }

    // Reset screensaver on joystick movement (with deadzone)
    if (abs(controls.joyX - lastXValue) > 50 || abs(controls.joyY - lastYValue) > 50) {
        resetScreensaverTimer();
        lastXValue = controls.joyX;
        lastYValue = controls.joyY;
    }

    handleJoystick(controls.joyX, controls.joyY, cButtonHeld, controls.joyBtnHeld);

    // Stop chord when key is released (debounced)
    if (activeKey != NO_KEY) {
        if (controls.rawKey == NO_KEY) {
            if (releaseStart == 0) {
                releaseStart = millis();
            } else if (millis() - releaseStart > 50) {
                stopCurrentChord();
                activeKey = NO_KEY;
                releaseStart = 0;
            }
        } else {
            releaseStart = 0;
        }
    }

    static unsigned long lastUi = 0;
    if (millis() - lastUi >= 100) {
        updateDisplay();
        lastUi = millis();
    }
}
