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
// Joystick button: mapped to GP7 in your CircuitPython config
#define JOYSTICK_BTN 13

// I2C pins (change these if your wiring uses different GPIOs)
// Set to match CircuitPython wiring: SDA=GP14, SCL=GP15
// (These are the GPIO numbers used in the Arduino core.)
#define I2C_SDA_PIN 14
#define I2C_SCL_PIN 15

// Select I2C port: 0 -> Wire (I2C0), 1 -> Wire1 (I2C1)
// Use 1 because your CircuitPython wiring uses GP14/GP15 (I2C1)
#define I2C_PORT 1

// Musical constants
#define BASE_NOTE 60 // Middle C
// Bass mode: when enabled, plays the root note one octave below the chord
#define BASS_MODE_ENABLED_DEFAULT false
#define BASS_OCTAVE_OFFSET -12 // One octave below chord root

// Auto-voicing mode: automatically selects inversions to keep chords within octave
#define AUTO_VOICING_ENABLED_DEFAULT false
#define AUTO_VOICING_MIN_NOTE 60  // C4 - lowest note allowed
#define AUTO_VOICING_MAX_NOTE 72  // C5 - highest note allowed (one octave range)

// Time gating between successive joystick-driven chord variation changes.
// Lower value makes the joystick feel more responsive.
#define JOYSTICK_GRACE_PERIOD 120 // ms (was 300)

// Timing constants
#define SPLASH_SCREEN_DURATION 2000
#define STATUS_MESSAGE_DURATION 1000

extern const int MAJOR_SCALE[];
extern const int MINOR_SCALE[];

// Debug / logging controls
#ifndef LOG_CHORDS
#define LOG_CHORDS 1  // Set to 0 to silence chord start/stop logs
#endif

// USB Device Descriptors (can be overridden via build_flags -D MACRO=VALUE)
#ifndef USB_USE_CUSTOM_DESCRIPTORS
#define USB_USE_CUSTOM_DESCRIPTORS 0  // Set to 1 to force custom VID/PID and names; 0 = use core defaults (most compatible)
#endif

// Joystick button polarity (default active-low with INPUT_PULLUP)
#ifndef JOYSTICK_BUTTON_ACTIVE_LOW
#define JOYSTICK_BUTTON_ACTIVE_LOW 1
#endif

#ifndef USB_VENDOR_ID
#define USB_VENDOR_ID 0xCafe
#endif

#ifndef USB_PRODUCT_ID
#define USB_PRODUCT_ID 0x4011
#endif

#ifndef USB_MANUFACTURER
#define USB_MANUFACTURER "ChoCo"
#endif

#ifndef USB_PRODUCT
#define USB_PRODUCT "ChoCo MIDI Controller"
#endif

#ifndef USB_SERIAL
#define USB_SERIAL "0001"
#endif

#ifndef USB_MIDI_INTERFACE
#define USB_MIDI_INTERFACE "ChoCo MIDI"
#endif

#endif