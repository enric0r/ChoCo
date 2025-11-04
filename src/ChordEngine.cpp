#include "ChordEngine.h"
#include "Display.h" // for getNoteName()

const uint8_t MAJOR_INTERVALS[7] = {0, 2, 4, 5, 7, 9, 11}; //Ionian-Major
const uint8_t MINOR_INTERVALS[7] = {0, 2, 3, 5, 7, 8, 10}; //Aeolian-Minor

const uint16_t BIT(uint8_t n) { return (1u << n); }

const uint16_t TRIAD_MAJ = BIT(0) | BIT(4) | BIT(7); // 1-3-5
const uint16_t TRIAD_MIN = BIT(0) | BIT(3) | BIT(7); // 1-b3-5
const uint16_t TRIAD_DIM = BIT(0) | BIT(3) | BIT(6); // 1-b3-b5
const uint16_t TRIAD_AUG = BIT(0) | BIT(4) | BIT(8); // 1-3-#5
const uint16_t TRIAD_SUS2 = BIT(0) | BIT(2) | BIT(7); // 1-2-5
const uint16_t TRIAD_SUS4 = BIT(0) | BIT(5) | BIT(7); // 1-4-5

static Chord currentChord = {{0}, 0, 0, ""};
static int rootNote = BASE_NOTE;
static bool minorScale = false;
static int currentInversion = 0;
static bool bassMode = BASS_MODE_ENABLED_DEFAULT;
static int bassNote = -1; // Track active bass note (-1 = none)

const int CHORD_MAJ[] = {0, 4, 7};
const int CHORD_MIN[] = {0, 3, 7};
const int CHORD_DIM[] = {0, 3, 6};
const int CHORD_AUG[] = {0, 4, 8};
const int CHORD_SUS4[] = {0, 5, 7};
const int CHORD_MAJ7[] = {0, 4, 7, 11};
const int CHORD_MIN7[] = {0, 3, 7, 10};
const int CHORD_SUS2[] = {0, 2, 7};
const int CHORD_DOM7[] = {0, 4, 7, 10};
const int CHORD_DOM9[] = {0, 4, 7, 10, 14};
const int CHORD_DOM11[] = {0, 4, 7, 10, 17};
const int CHORD_MIN9[] = {0, 3, 7, 10, 14};

void playChordForDegree(int degree) {
  stopCurrentChord();
  
  const int* scale = minorScale ? MINOR_SCALE : MAJOR_SCALE;
  int root = rootNote + scale[degree];
  
  if (minorScale) {
    switch(degree) {
      case 0: playChord(root, CHORD_MIN, 3, "Min"); break;
      case 2: playChord(root, CHORD_MAJ, 3, "Maj"); break;
      default: playChord(root, CHORD_MIN, 3, "Min"); break;
    }
  } else {
    switch(degree) {
      case 0: case 3: case 4: playChord(root, CHORD_MAJ, 3, "Maj"); break;
      case 1: case 2: case 5: playChord(root, CHORD_MIN, 3, "Min"); break;
      case 6: playChord(root, CHORD_DIM, 3, "Dim"); break;
    }
  }
}

void playChord(int root, const int* intervals, int size, String name) {
  stopCurrentChord();
  
  // Play bass note if bass mode is enabled
  if (bassMode) {
    bassNote = root + BASS_OCTAVE_OFFSET;
    if (bassNote < 0) bassNote = 0; // Clamp to valid MIDI range
    midiNoteOn(bassNote, 80); // Slightly lower velocity for bass
  }
  
  for(int i = 0; i < size; i++) {
    int note = root + intervals[i];
    if (i < currentInversion) {
      note += 12;
    }
    currentChord.notes[i] = note;
    midiNoteOn(note, 100);
  }
  
  currentChord.size = size;
  currentChord.root = root; // track the active chord's actual root
  currentChord.name = name;
  if (currentInversion > 0) {
    currentChord.name += String(" ") + currentInversion + "inv";
  }

#if LOG_CHORDS
  // Determine scale degree by matching root offset to current scale intervals
  int interval = (root - rootNote) % 12;
  if (interval < 0) interval += 12;
  const int* scale = minorScale ? MINOR_SCALE : MAJOR_SCALE;
  int degree = -1;
  for (int i = 0; i < 7; i++) {
    if (scale[i] == interval) { degree = i; break; }
  }
  // Roman numeral helper (basic mapping)
  const char* romansMajor[7] = {"I","II","III","IV","V","VI","VII"};
  const char* romansMinor[7] = {"i","ii","iii","iv","v","vi","vii"};
  const char* rn = (degree >= 0) ? (minorScale ? romansMinor[degree] : romansMajor[degree]) : "?";

  // Log chord details: name, degree, inversion, root and notes
  Serial.print("Playing chord: ");
  Serial.print(currentChord.name);
  Serial.print(" | Degree: ");
  Serial.print(degree >= 0 ? degree : -1);
  Serial.print(" ("); Serial.print(rn); Serial.print(")");
  Serial.print(" | Inversion: ");
  Serial.print(currentInversion);
  Serial.print(" | Root: ");
  Serial.print(getNoteName(root));
  Serial.print(" (" ); Serial.print(root); Serial.print(")");
  Serial.print(" | Notes: ");
  for (int i = 0; i < currentChord.size; i++) {
    if (i) Serial.print(", ");
    Serial.print(getNoteName(currentChord.notes[i]));
    Serial.print(" (" ); Serial.print(currentChord.notes[i]); Serial.print(")");
  }
  if (bassNote >= 0) {
    Serial.print(" | Bass: ");
    Serial.print(getNoteName(bassNote));
    Serial.print(" ("); Serial.print(bassNote); Serial.print(")");
  }
  Serial.println();
#endif
}

void stopCurrentChord() {
#if LOG_CHORDS
  if (currentChord.size > 0) {
    Serial.print("Stopping chord: ");
    Serial.println(currentChord.name);
  }
#endif
  
  // Stop bass note if active
  if (bassNote >= 0) {
    midiNoteOff(bassNote, 0);
    bassNote = -1;
  }
  
  for(int i = 0; i < currentChord.size; i++) {
    midiNoteOff(currentChord.notes[i], 0);
  }
  currentChord.size = 0;
}

String getCurrentChordName() {
  return currentChord.name;
}

int getCurrentRootNote() {
  return rootNote;
}

void setCurrentRootNote(int newRoot) {
  // Clamp to 0..11 offset from BASE_NOTE
  int offset = (newRoot - BASE_NOTE) % 12;
  if (offset < 0) offset += 12;
  rootNote = BASE_NOTE + offset;
}

bool isMinorScale() {
  return minorScale;
}

void setMinorScale(bool isMinor) {
  minorScale = isMinor;
}

void setCurrentInversion(int inversion) {
  currentInversion = inversion;
}

int getCurrentInversion() {
  return currentInversion;
}

bool isChordActive() {
  return currentChord.size > 0;
}

int getActiveChordRoot() {
  if (currentChord.size > 0) return currentChord.root;
  return getCurrentRootNote();
}

bool isBassMode() {
  return bassMode;
}

void setBassMode(bool enabled) {
  bassMode = enabled;
}

void toggleBassMode() {
  bassMode = !bassMode;
  Serial.print("Bass mode: ");
  Serial.println(bassMode ? "ON" : "OFF");
}