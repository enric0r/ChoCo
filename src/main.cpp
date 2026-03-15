#include "Config.h"
#include "MIDI.h"
#include "Display.h"
#include "ChordEngine.h"
#include "Controls.h"

void setup() {
    Serial.begin(115200);
#if CHOCO_LOG_LEVEL >= CHOCO_LOG_LEVEL_INFO
    Serial.println("ChoCo MIDI Controller Starting...");
#endif
    
    //Initialize MIDI first
    setupMIDI();
#if CHOCO_LOG_LEVEL >= CHOCO_LOG_LEVEL_INFO
    Serial.println("MIDI initialized");
#endif
    
    setupControls();
#if CHOCO_LOG_LEVEL >= CHOCO_LOG_LEVEL_INFO
    Serial.println("Controls initialized");
#endif
    
    setupDisplay();
#if CHOCO_LOG_LEVEL >= CHOCO_LOG_LEVEL_INFO
    Serial.println("Display initialized");
#endif
    
    drawSplashScreen();
#if CHOCO_LOG_LEVEL >= CHOCO_LOG_LEVEL_INFO
    Serial.println("Setup complete - entering main loop");
#endif
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
        } else if (cButtonHeld && key == 'A') {
            toggleChordLatchMode();
            showStatus(String("Latch: ") + (isChordLatchMode() ? "ON" : "OFF"), 800);
            if (!isChordLatchMode() && controls.rawKey == NO_KEY) {
                stopCurrentChord();
                activeKey = NO_KEY;
                releaseStart = 0;
            }
        } else if (!cButtonHeld && key >= '0' && key <= '6') {
            const int* varIntervals = nullptr;
            int varSize = 0;
            const char* varName = nullptr;
            const JoystickDirection direction = controls.joyDirectionInstant;
            const int degree = key - '0';
            const bool hasVariation = getChordVariationForDirection(direction, degree, varIntervals, varSize, varName);

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

    handleJoystick(controls.joyX, controls.joyY, cButtonHeld, controls.joyBtnHeld, controls.rawKey);

    // Stop chord when key is released (debounced)
    if (activeKey != NO_KEY) {
        if (controls.rawKey == NO_KEY) {
            if (isChordLatchMode()) {
                activeKey = NO_KEY;
                releaseStart = 0;
            } else if (releaseStart == 0) {
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
