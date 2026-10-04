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
    static bool latchGesturePending = false;
    static int lastXValue = JOYSTICK_CENTER_X;
    static int lastYValue = JOYSTICK_CENTER_Y;

    ControlSnapshot controls = {};
    pollControls(controls);
    setEditModeIndicator(controls.modifierCHeld);
    setInteractionState(controls.rawKey, controls.joyDirection, controls.modifierCHeld, controls.joyBtnHeld);

    // Wake and process the same input; held controls and sounding chords keep
    // the status visible even after the inactivity timeout.
    if (controls.rawKey != NO_KEY || controls.joyBtnHeld ||
        controls.joyDirection != JoystickDirection::Center || isChordActive()) {
        resetScreensaverTimer();
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
            char message[STATUS_TEXT_CAPACITY];
            snprintf(message, sizeof(message), "Deg %d Inv: %d", degree + 1, inv);
            showStatus(message, 800);
        } else if (cButtonHeld && key == 'A') {
            // Resolve on A release so C+A+button only changes strum.
            latchGesturePending = !controls.joyBtnHeld;
        } else if (!cButtonHeld && key >= '0' && key <= '6') {
            const int* varIntervals = nullptr;
            int varSize = 0;
            const char* varName = nullptr;
            const JoystickDirection direction = controls.joyDirection;
            const int degree = key - '0';
            const bool hasVariation = getChordVariationForDirection(direction, degree, varIntervals, varSize, varName);

            primeJoystickDirection(direction);
            if (hasVariation) {
                playChordForDegreeWithIntervals(degree, varIntervals, varSize, varName);
            } else {
                handleKeyPress(key);
            }
            activeKey = key;
        } else if (cButtonHeld && key == 'B') {
            printChordHistory();
#if CHOCO_LOG_LEVEL >= CHOCO_LOG_LEVEL_INFO
            showStatus("History sent", 600);
#else
            showStatus("Serial log OFF", 600);
#endif
        } else if (!cButtonHeld && (key == 'A' || key == 'B')) {
            handleKeyPress(key);
        }
    }

    // Reset screensaver on joystick movement (with deadzone)
    if (abs(controls.joyX - lastXValue) > JOYSTICK_ACTIVITY_DELTA || abs(controls.joyY - lastYValue) > JOYSTICK_ACTIVITY_DELTA) {
        resetScreensaverTimer();
        lastXValue = controls.joyX;
        lastYValue = controls.joyY;
    }

    if (controls.joyBtnHeld) latchGesturePending = false;
    if (latchGesturePending && !(controls.heldKeys & (1u << 7))) {
        latchGesturePending = false;
        toggleChordLatchMode();
        showStatusValue("Latch", isChordLatchMode() ? "ON" : "OFF", 800);
        if (!isChordLatchMode() && (activeKey == NO_KEY ||
            !(controls.heldKeys & (1u << (activeKey - '0'))))) {
            stopCurrentChord();
            activeKey = NO_KEY;
        }
    }
    // Release the actual sounding degree, even when A/B/C or another degree
    // remains held. The input layer already debounced this edge.
    if (activeKey != NO_KEY && !(controls.heldKeys & (1u << (activeKey - '0')))) {
        if (!isChordLatchMode()) stopCurrentChord();
        activeKey = NO_KEY;
    }
    const char gestureKey = (controls.heldKeys & (1u << 7)) ? 'A' : controls.rawKey;
    handleJoystick(controls.joyDirection, cButtonHeld, controls.joyBtnHeld, gestureKey);

    updateChordPlayback();
    // Full OLED transfers occupy the bus for milliseconds; defer them until
    // a strum has finished so note spacing and input scans stay responsive.
    if (isChordPlaybackPending()) return;
    updateScreensaver();

    static unsigned long lastUi = 0;
    if (millis() - lastUi >= UI_REFRESH_MS) {
        updateDisplay();
        lastUi = millis();
    }
}
