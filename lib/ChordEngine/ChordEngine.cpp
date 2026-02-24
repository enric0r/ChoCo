#include "ChordEngine.h"
#include "ChordLogic.h"
#include "Display.h" // for getNoteName()

// Interval sets for supported scales (7-note)
static const uint8_t IONIAN_INTERVALS[7]   = {0, 2, 4, 5, 7, 9, 11}; // Major
static const uint8_t DORIAN_INTERVALS[7]   = {0, 2, 3, 5, 7, 9, 10};
static const uint8_t PHRYGIAN_INTERVALS[7] = {0, 1, 3, 5, 7, 8, 10};
static const uint8_t LYDIAN_INTERVALS[7]   = {0, 2, 4, 6, 7, 9, 11};
static const uint8_t MIXOLYDIAN_INTERVALS[7]={0, 2, 4, 5, 7, 9, 10};
static const uint8_t AEOLIAN_INTERVALS[7]  = {0, 2, 3, 5, 7, 8, 10}; // Natural minor
static const uint8_t LOCRIAN_INTERVALS[7]  = {0, 1, 3, 5, 6, 8, 10};
static const uint8_t HARM_MIN_INTERVALS[7] = {0, 2, 3, 5, 7, 8, 11};
static const uint8_t MELO_MIN_INTERVALS[7] = {0, 2, 3, 5, 7, 9, 11}; // Jazz melodic minor

const uint16_t BIT(uint8_t n) { return (1u << n); }

const uint16_t TRIAD_MAJ = BIT(0) | BIT(4) | BIT(7); // 1-3-5
const uint16_t TRIAD_MIN = BIT(0) | BIT(3) | BIT(7); // 1-b3-5
const uint16_t TRIAD_DIM = BIT(0) | BIT(3) | BIT(6); // 1-b3-b5
const uint16_t TRIAD_AUG = BIT(0) | BIT(4) | BIT(8); // 1-3-#5
const uint16_t TRIAD_SUS2 = BIT(0) | BIT(2) | BIT(7); // 1-2-5
const uint16_t TRIAD_SUS4 = BIT(0) | BIT(5) | BIT(7); // 1-4-5

static Chord currentChord = {{0}, 0, 0, ""};
static int rootNote = BASE_NOTE;
static ScaleType currentScale = SCALE_IONIAN;
static int currentInversion = 0;
static bool bassMode = BASS_MODE_ENABLED_DEFAULT;
static int bassNote = -1; // Track active bass note (-1 = none)

// Per-chord inversion memory: stores inversion for each degree (0-6)
static int chordInversions[7] = {0, 0, 0, 0, 0, 0, 0};

// Auto-voicing mode: automatically selects inversions to keep within octave
static bool autoVoicingMode = AUTO_VOICING_ENABLED_DEFAULT;
static int lastPlayedNote = BASE_NOTE; // Track last note for voice leading
// Octave offset: adjusts all chords up/down by octaves (-2 to +2)
static int octaveOffset = 0;
// Chord history: circular buffer storing last 6 chords
#define CHORD_HISTORY_SIZE 6
static String chordHistory[CHORD_HISTORY_SIZE];
static int chordHistoryDegrees[CHORD_HISTORY_SIZE]; // Store degree numbers for display
static int historyWriteIndex = 0;
static int historyCount = 0;

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

bool isValidDegree(int degree) {
  return isValidDegreeIndex(degree);
}

void playChordForDegree(int degree) {
  if (!isValidDegree(degree)) {
    return;
  }

  stopCurrentChord();
  
  // Resolve current scale intervals
  auto getIntervals = [](ScaleType t) -> const uint8_t* {
    switch (t) {
      case SCALE_IONIAN:         return IONIAN_INTERVALS;
      case SCALE_DORIAN:         return DORIAN_INTERVALS;
      case SCALE_PHRYGIAN:       return PHRYGIAN_INTERVALS;
      case SCALE_LYDIAN:         return LYDIAN_INTERVALS;
      case SCALE_MIXOLYDIAN:     return MIXOLYDIAN_INTERVALS;
      case SCALE_AEOLIAN:        return AEOLIAN_INTERVALS;
      case SCALE_LOCRIAN:        return LOCRIAN_INTERVALS;
      case SCALE_HARMONIC_MINOR: return HARM_MIN_INTERVALS;
      case SCALE_MELODIC_MINOR:  return MELO_MIN_INTERVALS;
      default:                   return IONIAN_INTERVALS;
    }
  };

  const uint8_t* scale = getIntervals(currentScale);
  int root = rootNote + scale[degree];
  
  // Apply octave offset
  root += (octaveOffset * 12);

  // Determine triad quality by inspecting 1-3-5 of the current scale at this degree
  auto triadFromScale = [&](int deg, const int* &intervals, int &size, const char* &name) {
    int d = deg % 7; if (d < 0) d += 7;
    int third = scale[(d + 2) % 7];
    int fifth = scale[(d + 4) % 7];
    int thirdInt = (third - scale[d] + 12) % 12;
    int fifthInt = (fifth - scale[d] + 12) % 12;
    if (thirdInt == 4 && fifthInt == 7) { intervals = CHORD_MAJ; size = 3; name = "Maj"; }
    else if (thirdInt == 3 && fifthInt == 7) { intervals = CHORD_MIN; size = 3; name = "Min"; }
    else if (thirdInt == 3 && fifthInt == 6) { intervals = CHORD_DIM; size = 3; name = "Dim"; }
    else if (thirdInt == 4 && fifthInt == 8) { intervals = CHORD_AUG; size = 3; name = "Aug"; }
    else { intervals = CHORD_MIN; size = 3; name = "Min"; } // fallback
  };

  const int* triad = nullptr; int triadSize = 0; const char* triadName = "";
  triadFromScale(degree, triad, triadSize, triadName);
  
  // Determine inversion to use
  int inversionToUse = chordInversions[degree];
  
  // If auto-voicing is enabled, calculate best inversion and octave to stay in range
  if (autoVoicingMode) {
    // Try different octaves and inversions to find the best fit
    int bestInversion = 0;
    int bestOctave = 0;
    int bestScore = -10000;
    
    // Try octaves from -1 to +1 relative to current root
    for (int octaveShift = -1; octaveShift <= 1; octaveShift++) {
      int testRoot = root + (octaveShift * 12);
      
      // Try each inversion
      for (int testInv = 0; testInv < 3; testInv++) {
        int lowestNote = 127;
        int highestNote = 0;
        
        // Calculate actual notes for this combination
        for (int i = 0; i < triadSize; i++) {
          int testNote = testRoot + triad[i];
          if (i < testInv) testNote += 12;
          if (testNote < lowestNote) lowestNote = testNote;
          if (testNote > highestNote) highestNote = testNote;
        }
        
        // Score this combination
        int score = 0;
        
        // Heavily prefer combinations that keep ALL notes within the target range
        bool inRange = (lowestNote >= AUTO_VOICING_MIN_NOTE && highestNote <= AUTO_VOICING_MAX_NOTE);
        if (inRange) {
          score += 10000;
        } else {
          // Heavy penalty for notes outside range
          if (lowestNote < AUTO_VOICING_MIN_NOTE) score -= (AUTO_VOICING_MIN_NOTE - lowestNote) * 100;
          if (highestNote > AUTO_VOICING_MAX_NOTE) score -= (highestNote - AUTO_VOICING_MAX_NOTE) * 100;
        }
        
        // Prefer smoother voice leading (closer to last played note)
        int distance = abs(lowestNote - lastPlayedNote);
        score -= distance;
        
        if (score > bestScore) {
          bestScore = score;
          bestInversion = testInv;
          bestOctave = octaveShift;
        }
      }
    }
    
    // Apply the best octave shift to the root
    root = root + (bestOctave * 12);
    inversionToUse = bestInversion;
  currentInversion = inversionToUse;
    
#if CHOCO_LOG_LEVEL >= CHOCO_LOG_LEVEL_DEBUG
    Serial.print("Auto-voicing: degree=");
    Serial.print(degree);
    Serial.print(" root=");
    Serial.print(root);
    Serial.print(" octave=");
    Serial.print(bestOctave);
    Serial.print(" inversion=");
    Serial.println(bestInversion);
#endif
  } else {
    // Use the stored inversion for this degree
    currentInversion = inversionToUse;
  }

  // Play triad determined by current scale
  playChord(root, triad, triadSize, String(triadName));
  
  // Update last played note for voice leading
  if (currentChord.size > 0) {
    lastPlayedNote = currentChord.notes[0];
  }
}

// Variant: play a specific degree but with custom chord intervals (e.g., Maj7/Sus)
void playChordForDegreeWithIntervals(int degree, const int* forcedIntervals, int forcedSize, const char* forcedName) {
  if (!isValidDegree(degree) || forcedIntervals == nullptr || forcedSize <= 0) {
    return;
  }

  stopCurrentChord();

  // Resolve current scale intervals
  auto getIntervals = [](ScaleType t) -> const uint8_t* {
    switch (t) {
      case SCALE_IONIAN:         return IONIAN_INTERVALS;
      case SCALE_DORIAN:         return DORIAN_INTERVALS;
      case SCALE_PHRYGIAN:       return PHRYGIAN_INTERVALS;
      case SCALE_LYDIAN:         return LYDIAN_INTERVALS;
      case SCALE_MIXOLYDIAN:     return MIXOLYDIAN_INTERVALS;
      case SCALE_AEOLIAN:        return AEOLIAN_INTERVALS;
      case SCALE_LOCRIAN:        return LOCRIAN_INTERVALS;
      case SCALE_HARMONIC_MINOR: return HARM_MIN_INTERVALS;
      case SCALE_MELODIC_MINOR:  return MELO_MIN_INTERVALS;
      default:                   return IONIAN_INTERVALS;
    }
  };

  const uint8_t* scale = getIntervals(currentScale);
  int root = rootNote + scale[degree];
  
  // Apply octave offset
  root += (octaveOffset * 12);

  // Determine inversion to use
  int inversionToUse = chordInversions[degree];

  // Auto-voicing support using the provided intervals
  if (autoVoicingMode) {
    int bestInversion = 0;
    int bestOctave = 0;
    int bestScore = -10000;
    for (int octaveShift = -1; octaveShift <= 1; octaveShift++) {
      int testRoot = root + (octaveShift * 12);
      for (int testInv = 0; testInv < 3; testInv++) {
        int lowestNote = 127, highestNote = 0;
        for (int i = 0; i < forcedSize; i++) {
          int n = testRoot + forcedIntervals[i];
          if (i < testInv) n += 12;
          if (n < lowestNote) lowestNote = n;
          if (n > highestNote) highestNote = n;
        }
        int score = 0;
        bool inRange = (lowestNote >= AUTO_VOICING_MIN_NOTE && highestNote <= AUTO_VOICING_MAX_NOTE);
        if (inRange) score += 10000; else {
          if (lowestNote < AUTO_VOICING_MIN_NOTE) score -= (AUTO_VOICING_MIN_NOTE - lowestNote) * 100;
          if (highestNote > AUTO_VOICING_MAX_NOTE) score -= (highestNote - AUTO_VOICING_MAX_NOTE) * 100;
        }
        int distance = abs(lowestNote - lastPlayedNote);
        score -= distance;
        if (score > bestScore) { bestScore = score; bestInversion = testInv; bestOctave = octaveShift; }
      }
    }
    root = root + (bestOctave * 12);
    inversionToUse = bestInversion;
    currentInversion = inversionToUse;
#if CHOCO_LOG_LEVEL >= CHOCO_LOG_LEVEL_DEBUG
    Serial.print("Auto-voicing (forced): degree="); Serial.print(degree);
    Serial.print(" root="); Serial.print(root);
    Serial.print(" octave="); Serial.print(bestOctave);
    Serial.print(" inversion="); Serial.println(bestInversion);
#endif
  } else {
    currentInversion = inversionToUse;
  }

  // Play with the provided intervals
  playChord(root, forcedIntervals, forcedSize, String(forcedName));

  if (currentChord.size > 0) {
    lastPlayedNote = currentChord.notes[0];
  }
}

void playChord(int root, const int* intervals, int size, String name) {
  stopCurrentChord();
  
  // Play bass note if bass mode is enabled
  if (bassMode) {
    bassNote = root + BASS_OCTAVE_OFFSET;
    if (bassNote < 0) bassNote = 0;
    midiNoteOn(bassNote, 80);
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
  currentChord.root = root;
  
  // Build chord name with slash notation for inversions
  currentChord.name = getNoteName(root) + " " + name;
  
  // Add slash chord notation if inverted
  if (currentInversion > 0 && size > 0) {
    // Find the bass note (lowest note after inversion)
    int bassNoteNumber = root + intervals[currentInversion % size];
    currentChord.name += "/" + getNoteName(bassNoteNumber);
  }

#if LOG_CHORDS
  // Determine scale degree by matching root offset to current scale intervals
  int interval = (root - rootNote) % 12;
  if (interval < 0) interval += 12;
  const uint8_t* scale = nullptr;
  switch (currentScale) {
    case SCALE_IONIAN: scale = IONIAN_INTERVALS; break;
    case SCALE_DORIAN: scale = DORIAN_INTERVALS; break;
    case SCALE_PHRYGIAN: scale = PHRYGIAN_INTERVALS; break;
    case SCALE_LYDIAN: scale = LYDIAN_INTERVALS; break;
    case SCALE_MIXOLYDIAN: scale = MIXOLYDIAN_INTERVALS; break;
    case SCALE_AEOLIAN: scale = AEOLIAN_INTERVALS; break;
    case SCALE_LOCRIAN: scale = LOCRIAN_INTERVALS; break;
    case SCALE_HARMONIC_MINOR: scale = HARM_MIN_INTERVALS; break;
    case SCALE_MELODIC_MINOR: scale = MELO_MIN_INTERVALS; break;
    default: scale = IONIAN_INTERVALS; break;
  }
  int degree = -1;
  for (int i = 0; i < 7; i++) {
    if (scale[i] == interval) { degree = i; break; }
  }
  // Roman numeral helper based on triad quality
  auto romanFor = [&](int deg) -> const char* {
    // recompute triad quality for logging
    int d = deg % 7; if (d < 0) d += 7;
    int third = scale[(d + 2) % 7];
    int fifth = scale[(d + 4) % 7];
    int thirdInt = (third - scale[d] + 12) % 12;
    int fifthInt = (fifth - scale[d] + 12) % 12;
    static const char* R_UP[7] = {"I","II","III","IV","V","VI","VII"};
    static const char* R_LO[7] = {"i","ii","iii","iv","v","vi","vii"};
    if (thirdInt == 4 && fifthInt == 7) return R_UP[d];
    if (thirdInt == 3 && fifthInt == 7) return R_LO[d];
    if (thirdInt == 3 && fifthInt == 6) return "vii(dim)"; // diminished
    if (thirdInt == 4 && fifthInt == 8) return "III+"; // rough indicator for aug
    return R_UP[d];
  };
  const char* rn = (degree >= 0) ? romanFor(degree) : "?";

  // Log chord details with slash notation
  Serial.print("Playing chord: ");
  Serial.print(currentChord.name);
  Serial.print(" | Degree: ");
  Serial.print(degree >= 0 ? degree : -1);
  Serial.print(" ("); Serial.print(rn); Serial.print(")");
  Serial.print(" | Root: ");
  Serial.print(getNoteName(root));
  Serial.print(" ("); Serial.print(root); Serial.print(")");
  
  // Show inversion type
  if (currentInversion > 0) {
    Serial.print(" | Inv: ");
    Serial.print(currentInversion);
    Serial.print(" (bass: ");
    Serial.print(getNoteName(root + intervals[currentInversion % size]));
    Serial.print(")");
  }
  
  Serial.print(" | Notes: ");
  for (int i = 0; i < currentChord.size; i++) {
    if (i) Serial.print(", ");
    Serial.print(getNoteName(currentChord.notes[i]));
    Serial.print(" ("); Serial.print(currentChord.notes[i]); Serial.print(")");
  }
  if (bassNote >= 0) {
    Serial.print(" | Bass pedal: ");
    Serial.print(getNoteName(bassNote));
    Serial.print(" ("); Serial.print(bassNote); Serial.print(")");
  }
  Serial.println();
#endif

  // Add to history (store root note name + chord name + roman numeral if available)
  String historyEntry = currentChord.name;
  #if LOG_CHORDS
  if (degree >= 0) {
    historyEntry += " (";
    historyEntry += rn;
    historyEntry += ")";
    chordHistoryDegrees[historyWriteIndex] = degree; // Store degree for display
  } else {
    chordHistoryDegrees[historyWriteIndex] = -1; // Unknown degree
  }
  #else
  chordHistoryDegrees[historyWriteIndex] = -1;
  #endif
  chordHistory[historyWriteIndex] = historyEntry;
  historyWriteIndex = (historyWriteIndex + 1) % CHORD_HISTORY_SIZE;
  if (historyCount < CHORD_HISTORY_SIZE) historyCount++;
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
  if (currentChord.size == 0) {
    return "";
  }
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

ScaleType getScaleType() { return currentScale; }
void setScaleType(ScaleType type) { currentScale = type; }
void cycleScaleType(int step) {
  int t = wrapScaleIndex((int)currentScale, step, (int)SCALE_COUNT);
  currentScale = (ScaleType)t;
#if CHOCO_LOG_LEVEL >= CHOCO_LOG_LEVEL_DEBUG
  Serial.print("Scale: "); Serial.println(getCurrentScaleName());
#endif
}

void setCurrentInversion(int inversion) {
  currentInversion = inversion;
}

int getCurrentInversion() {
  return currentInversion;
}

void setInversionForDegree(int degree, int inversion) {
  if (isValidDegree(degree)) {
    chordInversions[degree] = normalizeTriadInversion(inversion);
#if CHOCO_LOG_LEVEL >= CHOCO_LOG_LEVEL_DEBUG
    Serial.print("Set inversion for degree ");
    Serial.print(degree);
    Serial.print(" to ");
    Serial.println(chordInversions[degree]);
#endif
  }
}

int getInversionForDegree(int degree) {
  if (isValidDegree(degree)) {
    return chordInversions[degree];
  }
  return 0;
}

void cycleInversionForDegree(int degree) {
  if (isValidDegree(degree)) {
    chordInversions[degree] = (chordInversions[degree] + 1) % 3;
#if CHOCO_LOG_LEVEL >= CHOCO_LOG_LEVEL_DEBUG
    Serial.print("Cycled inversion for degree ");
    Serial.print(degree);
    Serial.print(" to ");
    Serial.println(chordInversions[degree]);
#endif
  }
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

bool isAutoVoicingMode() {
  return autoVoicingMode;
}

void setAutoVoicingMode(bool enabled) {
  autoVoicingMode = enabled;
  Serial.print("Auto-voicing mode: ");
  Serial.println(autoVoicingMode ? "ON" : "OFF");
}

void toggleAutoVoicingMode() {
  autoVoicingMode = !autoVoicingMode;
  Serial.print("Auto-voicing mode: ");
  Serial.println(autoVoicingMode ? "ON" : "OFF");
}

int getOctaveOffset() {
  return octaveOffset;
}

void setOctaveOffset(int offset) {
  // Clamp to reasonable range (-2 to +2 octaves)
  if (offset < -2) offset = -2;
  if (offset > 2) offset = 2;
  octaveOffset = offset;
  Serial.print("Octave offset: ");
  Serial.println(octaveOffset);
}

void incrementOctave() {
  setOctaveOffset(octaveOffset + 1);
}

void decrementOctave() {
  setOctaveOffset(octaveOffset - 1);
}

const char* getCurrentScaleName() {
  switch (currentScale) {
    case SCALE_IONIAN: return "Ionian";
    case SCALE_DORIAN: return "Dorian";
    case SCALE_PHRYGIAN: return "Phrygian";
    case SCALE_LYDIAN: return "Lydian";
    case SCALE_MIXOLYDIAN: return "Mixolyd";
    case SCALE_AEOLIAN: return "Aeolian";
    case SCALE_LOCRIAN: return "Locrian";
    case SCALE_HARMONIC_MINOR: return "HarmMin";
    case SCALE_MELODIC_MINOR: return "MelMin";
    default: return "Ionian";
  }
}

// ---- Chord History ----
void printChordHistory() {
  if (historyCount == 0) {
    Serial.println("Chord history: (empty)");
    return;
  }
  Serial.println("=== Chord History (most recent first) ===");
  for (int i = 0; i < historyCount; i++) {
    // Read backwards from most recent
    int index = (historyWriteIndex - 1 - i + CHORD_HISTORY_SIZE) % CHORD_HISTORY_SIZE;
    Serial.print(i + 1);
    Serial.print(". ");
    Serial.println(chordHistory[index]);
  }
  Serial.println("=========================================");
}

int getChordHistoryCount() {
  return historyCount;
}

String getChordHistoryEntry(int index) {
  if (index < 0 || index >= historyCount) return "";
  // 0 = most recent
  int arrayIndex = (historyWriteIndex - 1 - index + CHORD_HISTORY_SIZE) % CHORD_HISTORY_SIZE;
  return chordHistory[arrayIndex];
}

int getChordHistoryDegree(int index) {
  if (index < 0 || index >= historyCount) return -1;
  // 0 = most recent
  int arrayIndex = (historyWriteIndex - 1 - index + CHORD_HISTORY_SIZE) % CHORD_HISTORY_SIZE;
  return chordHistoryDegrees[arrayIndex];
}
