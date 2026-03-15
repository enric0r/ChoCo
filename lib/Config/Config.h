#ifndef CONFIG_H
#define CONFIG_H

#include <Arduino.h>

// Display configuration
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET -1
#define SCREEN_ADDRESS 0x3C 

// Keypad configuration
#define ROWS 3
#define COLS 4

// Joystick pins
#define JOYSTICK_X A0
#define JOYSTICK_Y A1
// Joystick button mapped to GP13
#define JOYSTICK_BTN 13

// Joystick orientation mapping for current PCB revision.
// 1 swaps X/Y so logical left-right follows physical left-right.
#ifndef JOYSTICK_SWAP_AXES
#define JOYSTICK_SWAP_AXES 1
#endif

// Apply inversion after optional axis swap.
#ifndef JOYSTICK_INVERT_X
#define JOYSTICK_INVERT_X 1
#endif

#ifndef JOYSTICK_INVERT_Y
#define JOYSTICK_INVERT_Y 1
#endif

// Fixed joystick center for radial classification.
#ifndef JOYSTICK_CENTER_X
#define JOYSTICK_CENTER_X 512
#endif

#ifndef JOYSTICK_CENTER_Y
#define JOYSTICK_CENTER_Y 512
#endif

// Outer-ring trigger thresholds.
#ifndef JOYSTICK_TRIGGER_ENGAGE_PCT
#define JOYSTICK_TRIGGER_ENGAGE_PCT 72
#endif

#ifndef JOYSTICK_TRIGGER_RELEASE_PCT
#define JOYSTICK_TRIGGER_RELEASE_PCT 55
#endif

#if (JOYSTICK_TRIGGER_ENGAGE_PCT <= 0) || (JOYSTICK_TRIGGER_ENGAGE_PCT >= 100)
#error "JOYSTICK_TRIGGER_ENGAGE_PCT must be between 1 and 99"
#endif

#if (JOYSTICK_TRIGGER_RELEASE_PCT <= 0) || (JOYSTICK_TRIGGER_RELEASE_PCT >= 100)
#error "JOYSTICK_TRIGGER_RELEASE_PCT must be between 1 and 99"
#endif

#if (JOYSTICK_TRIGGER_RELEASE_PCT >= JOYSTICK_TRIGGER_ENGAGE_PCT)
#error "JOYSTICK_TRIGGER_RELEASE_PCT must be less than JOYSTICK_TRIGGER_ENGAGE_PCT"
#endif

// I2C pins (change these if your wiring uses different GPIOs)
// Set to match CircuitPython wiring: SDA=GP14, SCL=GP15
// (These are the GPIO numbers used in the Arduino core.)
#define I2C_SDA_PIN 14
#define I2C_SCL_PIN 15

// Select I2C port: 0 -> Wire (I2C0), 1 -> Wire1 (I2C1)
// Use 1 because your CircuitPython wiring uses GP14/GP15 (I2C1)
#define I2C_PORT 1
#if (I2C_PORT != 0) && (I2C_PORT != 1)
#error "I2C_PORT must be 0 (Wire) or 1 (Wire1)"
#endif

// Musical constants
#define BASE_NOTE 60 // Middle C
// Bass mode: when enabled, plays the root note one octave below the chord
#define BASS_MODE_ENABLED_DEFAULT false
#define BASS_OCTAVE_OFFSET -12 // One octave below chord root

// Single note mode: when enabled, plays just the degree note instead of a chord
#define SINGLE_NOTE_MODE_ENABLED_DEFAULT false

// Chord latch mode: keep last played chord sounding after key release.
#ifndef CHORD_LATCH_ENABLED_DEFAULT
#define CHORD_LATCH_ENABLED_DEFAULT false
#endif

// Strum mode: stagger chord note-ons for guitar-like attack.
#ifndef STRUM_MODE_ENABLED_DEFAULT
#define STRUM_MODE_ENABLED_DEFAULT false
#endif
#ifndef STRUM_NOTE_DELAY_MS
#define STRUM_NOTE_DELAY_MS 14
#endif
#if (STRUM_NOTE_DELAY_MS < 0) || (STRUM_NOTE_DELAY_MS > 100)
#error "STRUM_NOTE_DELAY_MS must be between 0 and 100"
#endif

// Auto-voicing mode: automatically selects inversions to keep chords within octave
#define AUTO_VOICING_ENABLED_DEFAULT false
#define AUTO_VOICING_MIN_NOTE 60  // C4 - lowest note allowed
#define AUTO_VOICING_MAX_NOTE 72  // C5 - highest note allowed (one octave range)

// Smart voicing tuning (used when smart voicing mode is active).
#ifndef SMART_VOICING_RANGE_MIN
#define SMART_VOICING_RANGE_MIN AUTO_VOICING_MIN_NOTE
#endif

#ifndef SMART_VOICING_RANGE_MAX
#define SMART_VOICING_RANGE_MAX AUTO_VOICING_MAX_NOTE
#endif

#ifndef SMART_VOICING_OCTAVE_MIN
#define SMART_VOICING_OCTAVE_MIN -2
#endif

#ifndef SMART_VOICING_OCTAVE_MAX
#define SMART_VOICING_OCTAVE_MAX 2
#endif

#ifndef SMART_WEIGHT_MOTION
#define SMART_WEIGHT_MOTION 8
#endif

#ifndef SMART_WEIGHT_COMMON_TONE
#define SMART_WEIGHT_COMMON_TONE 22
#endif

#ifndef SMART_WEIGHT_RANGE
#define SMART_WEIGHT_RANGE 10
#endif

#ifndef SMART_WEIGHT_SPAN
#define SMART_WEIGHT_SPAN 2
#endif

#ifndef SMART_TARGET_SPAN
#define SMART_TARGET_SPAN 12
#endif

#if (SMART_VOICING_RANGE_MIN >= SMART_VOICING_RANGE_MAX)
#error "SMART_VOICING_RANGE_MIN must be less than SMART_VOICING_RANGE_MAX"
#endif

#if (SMART_VOICING_OCTAVE_MIN > SMART_VOICING_OCTAVE_MAX)
#error "SMART_VOICING_OCTAVE_MIN must be less than or equal to SMART_VOICING_OCTAVE_MAX"
#endif

// Time gating between successive joystick-driven chord variation changes.
// Lower value makes the joystick feel more responsive.
#define JOYSTICK_GRACE_PERIOD 120 // ms (was 300)

// Timing constants
#define SPLASH_SCREEN_DURATION 2000
#define STATUS_MESSAGE_DURATION 1000

// Screensaver configuration
#ifndef SCREENSAVER_TIMEOUT_MS
#define SCREENSAVER_TIMEOUT_MS 30000  // 30 seconds of inactivity before screensaver
#endif

#ifndef SCREENSAVER_ANIMATION_INTERVAL_MS
#define SCREENSAVER_ANIMATION_INTERVAL_MS 100  // Animation frame update interval (faster = smoother bouncing)
#endif

extern const int MAJOR_SCALE[];
extern const int MINOR_SCALE[];

// Logging controls
#define CHOCO_LOG_LEVEL_NONE 0
#define CHOCO_LOG_LEVEL_INFO 1
#define CHOCO_LOG_LEVEL_DEBUG 2
#define CHOCO_LOG_LEVEL_VERBOSE 3
#ifndef CHOCO_LOG_LEVEL
#define CHOCO_LOG_LEVEL CHOCO_LOG_LEVEL_NONE
#endif

// Chord logs are enabled only when log level is INFO or higher.
#ifndef LOG_CHORDS
#define LOG_CHORDS (CHOCO_LOG_LEVEL >= CHOCO_LOG_LEVEL_INFO)
#endif

// USB Device Descriptors (can be overridden via build_flags -D MACRO=VALUE)
#ifndef USB_USE_CUSTOM_DESCRIPTORS
#define USB_USE_CUSTOM_DESCRIPTORS 0  // 1 = force custom product/manufacturer strings, 0 = core defaults
#endif

// Joystick button polarity (default active-low with INPUT_PULLUP)
#ifndef JOYSTICK_BUTTON_ACTIVE_LOW
#define JOYSTICK_BUTTON_ACTIVE_LOW 1
#endif

#ifndef USB_MANUFACTURER
#define USB_MANUFACTURER "ChoCo"
#endif

#ifndef USB_PRODUCT
#define USB_PRODUCT "ChoCo MIDI Controller"
#endif

#endif
