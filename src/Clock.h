#ifndef CLOCK_H
#define CLOCK_H

#include <Arduino.h>

// Clock synchronization modes
enum ClockMode {
  CLOCK_INTERNAL,  // Use internal tap tempo clock
  CLOCK_MIDI_IN    // Sync to incoming MIDI clock
};

// Initialize the clock system
void setupClock();

// Update clock state (call in main loop)
void updateClock();

// Tap tempo: call this when tap tempo button is pressed
void tapTempo();

// Set BPM manually
void setBPM(float bpm);

// Get current BPM
float getBPM();

// Set clock mode
void setClockMode(ClockMode mode);

// Get clock mode
ClockMode getClockMode();

// Handle incoming MIDI clock message
void handleMIDIClockMessage(uint8_t status);

// Check if we're at the start of a beat
bool isNewBeat();

// Check if we're at the start of a bar (4 beats)
bool isNewBar();

// Get current beat position within bar (0-3)
int getCurrentBeat();

// Get time until next beat in milliseconds
unsigned long timeUntilNextBeat();

// Get time until next bar in milliseconds
unsigned long timeUntilNextBar();

// Reset beat/bar counters
void resetBeatCounter();

#endif
