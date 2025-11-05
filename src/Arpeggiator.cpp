#include "Arpeggiator.h"
#include "Clock.h"
#include "MIDI.h"
#include "Config.h"

// Arpeggiator state
static bool arpEnabled = false;
static bool arpActive = false;
static ArpPattern currentPattern = ARP_UP;
static int noteDivision = 4;  // Default to 16th notes

// Current arpeggio notes
static int arpNotes[8];
static int arpNoteCount = 0;
static int currentArpIndex = 0;
static int lastPlayedNote = -1;

// Timing
static unsigned long lastArpTime = 0;
static int arpeggioDirection = 1;  // 1 for up, -1 for down (used in UP_DOWN pattern)

void setupArpeggiator() {
  Serial.println("Arpeggiator initialized");
}

void updateArpeggiator() {
  if (!arpEnabled || !arpActive || arpNoteCount == 0) {
    return;
  }
  
  // Calculate time between notes based on BPM and division
  float bpm = getBPM();
  unsigned long noteInterval = (unsigned long)((60000.0f / bpm) / noteDivision);
  
  unsigned long now = millis();
  
  // Check if it's time to play the next note
  if (now - lastArpTime >= noteInterval) {
    // Turn off last note
    if (lastPlayedNote >= 0) {
      midiNoteOff(lastPlayedNote, 0);
    }
    
    // Determine next note based on pattern
    int noteToPlay = -1;
    
    switch (currentPattern) {
      case ARP_UP:
        noteToPlay = arpNotes[currentArpIndex];
        currentArpIndex = (currentArpIndex + 1) % arpNoteCount;
        break;
        
      case ARP_DOWN:
        noteToPlay = arpNotes[arpNoteCount - 1 - currentArpIndex];
        currentArpIndex = (currentArpIndex + 1) % arpNoteCount;
        break;
        
      case ARP_UP_DOWN:
        noteToPlay = arpNotes[currentArpIndex];
        currentArpIndex += arpeggioDirection;
        
        // Reverse direction at boundaries
        if (currentArpIndex >= arpNoteCount) {
          currentArpIndex = arpNoteCount - 2;
          arpeggioDirection = -1;
        } else if (currentArpIndex < 0) {
          currentArpIndex = 1;
          arpeggioDirection = 1;
        }
        break;
        
      case ARP_RANDOM:
        currentArpIndex = random(arpNoteCount);
        noteToPlay = arpNotes[currentArpIndex];
        break;
        
      case ARP_STRUM_UP:
        // Strum is handled at start, not in update loop
        break;
        
      case ARP_STRUM_DOWN:
        // Strum is handled at start, not in update loop
        break;
    }
    
    // Play the note
    if (noteToPlay >= 0) {
      midiNoteOn(noteToPlay, 100);
      lastPlayedNote = noteToPlay;
    }
    
    lastArpTime = now;
  }
}

void setArpeggiatorEnabled(bool enabled) {
  arpEnabled = enabled;
  Serial.print("Arpeggiator ");
  Serial.println(enabled ? "enabled" : "disabled");
  
  if (!enabled) {
    stopArpeggio();
  }
}

bool isArpeggiatorEnabled() {
  return arpEnabled;
}

void setArpPattern(ArpPattern pattern) {
  currentPattern = pattern;
  currentArpIndex = 0;
  arpeggioDirection = 1;
  
  const char* patternNames[] = {
    "UP", "DOWN", "UP_DOWN", "RANDOM", "STRUM_UP", "STRUM_DOWN"
  };
  Serial.print("Arp pattern: ");
  Serial.println(patternNames[pattern]);
}

ArpPattern getArpPattern() {
  return currentPattern;
}

void setArpNoteDivision(int division) {
  if (division >= 1 && division <= 16) {
    noteDivision = division;
    Serial.print("Arp division: ");
    Serial.println(division);
  }
}

int getArpNoteDivision() {
  return noteDivision;
}

void startArpeggio(const int* notes, int noteCount) {
  if (!arpEnabled || noteCount == 0 || noteCount > 8) {
    return;
  }
  
  // Stop any currently playing arpeggio
  stopArpeggio();
  
  // Copy notes
  arpNoteCount = noteCount;
  for (int i = 0; i < noteCount; i++) {
    arpNotes[i] = notes[i];
  }
  
  // Sort notes for consistent patterns
  for (int i = 0; i < arpNoteCount - 1; i++) {
    for (int j = i + 1; j < arpNoteCount; j++) {
      if (arpNotes[i] > arpNotes[j]) {
        int temp = arpNotes[i];
        arpNotes[i] = arpNotes[j];
        arpNotes[j] = temp;
      }
    }
  }
  
  // Initialize arpeggio state
  currentArpIndex = 0;
  arpeggioDirection = 1;
  lastArpTime = millis();
  arpActive = true;
  
  // Handle strum patterns (play all notes in quick succession)
  if (currentPattern == ARP_STRUM_UP || currentPattern == ARP_STRUM_DOWN) {
    const int STRUM_DELAY = 20;  // ms between notes
    
    if (currentPattern == ARP_STRUM_UP) {
      for (int i = 0; i < arpNoteCount; i++) {
        midiNoteOn(arpNotes[i], 100);
        delay(STRUM_DELAY);
      }
    } else {  // STRUM_DOWN
      for (int i = arpNoteCount - 1; i >= 0; i--) {
        midiNoteOn(arpNotes[i], 100);
        delay(STRUM_DELAY);
      }
    }
    
    // For strum, notes stay on (not arpeggiated in the loop)
    arpActive = false;
  }
  
  Serial.print("Arpeggio started with ");
  Serial.print(noteCount);
  Serial.println(" notes");
}

void stopArpeggio() {
  if (lastPlayedNote >= 0) {
    midiNoteOff(lastPlayedNote, 0);
    lastPlayedNote = -1;
  }
  
  arpActive = false;
  currentArpIndex = 0;
  arpeggioDirection = 1;
  arpNoteCount = 0;
}

bool isArpeggioActive() {
  return arpActive;
}
