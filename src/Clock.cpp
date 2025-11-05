#include "Clock.h"
#include "Config.h"

// Internal state
static ClockMode clockMode = CLOCK_INTERNAL;
static float currentBPM = DEFAULT_BPM;  // Use constant from Config.h
static unsigned long lastBeatTime = 0;
static unsigned long lastTapTime = 0;
static int tapCount = 0;
static const int TAP_TIMEOUT = 2000;  // Reset tap count after 2 seconds
static const int MIN_TAPS = 2;        // Minimum taps to set tempo

// MIDI clock state (24 ppqn - pulses per quarter note)
static int midiClockPulseCount = 0;
static unsigned long lastMidiClockTime = 0;
static const int PULSES_PER_BEAT = 24;

// Beat/bar tracking
static int currentBeatInBar = 0;  // 0-3 for 4/4 time
static bool newBeatFlag = false;
static bool newBarFlag = false;
static unsigned long nextBeatTime = 0;

void setupClock() {
  lastBeatTime = millis();
  nextBeatTime = lastBeatTime + (unsigned long)((60000.0f / currentBPM));
  Serial.println("Clock initialized");
  Serial.print("Default BPM: ");
  Serial.println(currentBPM);
}

void updateClock() {
  unsigned long now = millis();
  
  // Clear flags from previous update
  newBeatFlag = false;
  newBarFlag = false;
  
  // Only update internal clock if in internal mode
  if (clockMode == CLOCK_INTERNAL) {
    if (now >= nextBeatTime) {
      newBeatFlag = true;
      lastBeatTime = now;
      
      // Calculate next beat time
      unsigned long beatInterval = (unsigned long)((60000.0f / currentBPM));
      nextBeatTime = lastBeatTime + beatInterval;
      
      // Update beat counter
      currentBeatInBar++;
      if (currentBeatInBar >= 4) {
        currentBeatInBar = 0;
        newBarFlag = true;
      }
    }
  }
  // MIDI clock mode updates happen in handleMIDIClockMessage
}

void tapTempo() {
  unsigned long now = millis();
  
  // Reset tap count if too much time has passed
  if (now - lastTapTime > TAP_TIMEOUT) {
    tapCount = 0;
  }
  
  // Calculate BPM from tap interval
  if (tapCount > 0) {
    unsigned long interval = now - lastTapTime;
    if (interval > 0) {
      float newBPM = 60000.0f / interval;
      
      // Clamp BPM to reasonable range (40-240)
      if (newBPM >= 40.0f && newBPM <= 240.0f) {
        // Average with current BPM for smoother tempo detection
        if (tapCount >= MIN_TAPS) {
          currentBPM = (currentBPM + newBPM) / 2.0f;
          Serial.print("Tap tempo BPM: ");
          Serial.println(currentBPM);
        }
      }
    }
  }
  
  tapCount++;
  lastTapTime = now;
  
  // Reset beat timing when tapping
  if (tapCount >= MIN_TAPS) {
    lastBeatTime = now;
    nextBeatTime = now + (unsigned long)((60000.0f / currentBPM));
    currentBeatInBar = 0;
  }
}

void setBPM(float bpm) {
  if (bpm >= 40.0f && bpm <= 240.0f) {
    currentBPM = bpm;
    // Reset timing
    unsigned long now = millis();
    lastBeatTime = now;
    nextBeatTime = now + (unsigned long)((60000.0f / currentBPM));
    Serial.print("BPM set to: ");
    Serial.println(currentBPM);
  }
}

float getBPM() {
  return currentBPM;
}

void setClockMode(ClockMode mode) {
  clockMode = mode;
  Serial.print("Clock mode: ");
  Serial.println(mode == CLOCK_INTERNAL ? "INTERNAL" : "MIDI_IN");
  
  if (mode == CLOCK_INTERNAL) {
    // Reset internal clock timing
    unsigned long now = millis();
    lastBeatTime = now;
    nextBeatTime = now + (unsigned long)((60000.0f / currentBPM));
    currentBeatInBar = 0;
  } else {
    // Reset MIDI clock state
    midiClockPulseCount = 0;
  }
}

ClockMode getClockMode() {
  return clockMode;
}

void handleMIDIClockMessage(uint8_t status) {
  unsigned long now = millis();
  
  if (clockMode != CLOCK_MIDI_IN) return;
  
  switch (status) {
    case 0xF8:  // MIDI Clock (24 ppqn)
      midiClockPulseCount++;
      
      // Every 24 pulses = 1 quarter note (1 beat)
      if (midiClockPulseCount >= PULSES_PER_BEAT) {
        midiClockPulseCount = 0;
        newBeatFlag = true;
        
        // Calculate BPM from MIDI clock timing
        if (lastMidiClockTime > 0) {
          unsigned long interval = now - lastMidiClockTime;
          if (interval > 0) {
            currentBPM = 60000.0f / interval;
          }
        }
        lastMidiClockTime = now;
        
        // Update beat counter
        currentBeatInBar++;
        if (currentBeatInBar >= 4) {
          currentBeatInBar = 0;
          newBarFlag = true;
        }
      }
      break;
      
    case 0xFA:  // MIDI Start
      midiClockPulseCount = 0;
      currentBeatInBar = 0;
      lastMidiClockTime = 0;
      Serial.println("MIDI Clock: Start");
      break;
      
    case 0xFB:  // MIDI Continue
      Serial.println("MIDI Clock: Continue");
      break;
      
    case 0xFC:  // MIDI Stop
      midiClockPulseCount = 0;
      Serial.println("MIDI Clock: Stop");
      break;
  }
}

bool isNewBeat() {
  return newBeatFlag;
}

bool isNewBar() {
  return newBarFlag;
}

int getCurrentBeat() {
  return currentBeatInBar;
}

unsigned long timeUntilNextBeat() {
  if (clockMode == CLOCK_INTERNAL) {
    unsigned long now = millis();
    if (now < nextBeatTime) {
      return nextBeatTime - now;
    }
    return 0;
  }
  return 0;  // Unknown for MIDI clock mode
}

unsigned long timeUntilNextBar() {
  unsigned long beatTime = timeUntilNextBeat();
  int beatsRemaining = 4 - currentBeatInBar;
  if (beatsRemaining == 4 || beatsRemaining == 0) {
    // Already at start of bar
    return beatTime;
  }
  
  unsigned long beatInterval = (unsigned long)((60000.0f / currentBPM));
  return beatTime + (beatInterval * (beatsRemaining - 1));
}

void resetBeatCounter() {
  currentBeatInBar = 0;
  midiClockPulseCount = 0;
  newBeatFlag = false;
  newBarFlag = false;
  unsigned long now = millis();
  lastBeatTime = now;
  nextBeatTime = now + (unsigned long)((60000.0f / currentBPM));
}
