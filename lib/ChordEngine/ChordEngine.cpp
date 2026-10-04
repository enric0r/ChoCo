#include "ChordEngine.h"
#include "ChordLogic.h"
#include "ChordPlayback.h"
#include <stdio.h>
#include <string.h>

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

constexpr uint16_t BIT(uint8_t n) { return (1u << n); }

const uint16_t TRIAD_MAJ = BIT(0) | BIT(4) | BIT(7); // 1-3-5
const uint16_t TRIAD_MIN = BIT(0) | BIT(3) | BIT(7); // 1-b3-5
const uint16_t TRIAD_DIM = BIT(0) | BIT(3) | BIT(6); // 1-b3-b5
const uint16_t TRIAD_AUG = BIT(0) | BIT(4) | BIT(8); // 1-3-#5
const uint16_t TRIAD_SUS2 = BIT(0) | BIT(2) | BIT(7); // 1-2-5
const uint16_t TRIAD_SUS4 = BIT(0) | BIT(5) | BIT(7); // 1-4-5

static constexpr int kMaxChordNotes = 5;

static Chord currentChord = {{0}, 0, 0, ""};
static int rootNote = BASE_NOTE;
static ScaleType currentScale = SCALE_IONIAN;
static int currentInversion = 0;
static bool bassMode = BASS_MODE_ENABLED_DEFAULT;
static int bassNote = -1; // Track active bass note (-1 = none)
static bool chordLatchMode = CHORD_LATCH_ENABLED_DEFAULT;
static bool strumMode = STRUM_MODE_ENABLED_DEFAULT;
static bool singleNoteMode = SINGLE_NOTE_MODE_ENABLED_DEFAULT;
static int activeChordDegree = -1; // Degree (0..6) of currently active chord when known

// Per-chord inversion memory: stores inversion for each degree (0-6)
static int chordInversions[7] = {0, 0, 0, 0, 0, 0, 0};

// Voicing mode.
static VoicingMode voicingMode = AUTO_VOICING_ENABLED_DEFAULT ? VOICING_SMART : VOICING_CLASSIC;
static int previousVoicedNotes[kMaxChordNotes] = {0};
static int previousVoicedSize = 0;
static bool hasPreviousVoicing = false;
// Octave offset: adjusts all chords up/down by octaves (-2 to +2)
static int octaveOffset = 0;
// Chord history: circular buffer storing last 6 chords
static char chordHistory[CHORD_HISTORY_SIZE][CHORD_NAME_CAPACITY];
static int chordHistoryDegrees[CHORD_HISTORY_SIZE]; // Store degree numbers for display
static int historyWriteIndex = 0;
static int historyCount = 0;

// Chord suggestions: weighted next-step degrees, refreshed after each played chord.
struct DegreeWeight {
  int degree;
  uint8_t weight;
};
static int chordSuggestions[CHORD_SUGGESTION_SIZE] = {0, 4, 3, 5};
static int chordSuggestionCount = 4;
static bool chordSuggestionsDirty = true;
static int chordSuggestionContextDegree = -1;
static uint32_t chordSuggestionRng = 0x53454747u;

const int CHORD_MAJ[] = {0, 4, 7};
const int CHORD_MIN[] = {0, 3, 7};
const int CHORD_DIM[] = {0, 3, 6};
const int CHORD_AUG[] = {0, 4, 8};
const int CHORD_SUS4[] = {0, 5, 7};
const int CHORD_MAJ7[] = {0, 4, 7, 11};
const int CHORD_MIN7[] = {0, 3, 7, 10};
const int CHORD_SUS2[] = {0, 2, 7};
const int CHORD_MAJ6[] = {0, 4, 7, 9};
const int CHORD_MAJ9[] = {0, 4, 7, 11, 14};
const int CHORD_DOM7[] = {0, 4, 7, 10};
const int CHORD_DOM9[] = {0, 4, 7, 10, 14};
const int CHORD_DOM11[] = {0, 4, 7, 10, 17};
const int CHORD_MIN9[] = {0, 3, 7, 10, 14};
const int CHORD_SUS4_7[] = {0, 5, 7, 10};
const int CHORD_ADD11[] = {0, 4, 7, 17};
const int CHORD_HALFDIM7[] = {0, 3, 6, 10};
const int CHORD_DOM7_SHARP9[] = {0, 4, 7, 10, 15};
const int CHORD_ADD9[] = {0, 4, 7, 14};
const int CHORD_MIN11[] = {0, 3, 7, 10, 17};
const int CHORD_MIN_MAJ7[] = {0, 3, 7, 11};
const int CHORD_MAJ13[] = {0, 4, 7, 11, 21};
const int CHORD_SIX_NINE[] = {0, 4, 7, 9, 14};
const int CHORD_MAJ7_SHARP11[] = {0, 4, 7, 11, 18};
const int CHORD_DOM13[] = {0, 4, 7, 10, 21};
const int CHORD_DOM7_FLAT9[] = {0, 4, 7, 10, 13};
const int CHORD_DOM7_ALT[] = {0, 4, 8, 10, 13};

static bool isMinorFamilyScale(ScaleType type) {
  switch (type) {
    case SCALE_DORIAN:
    case SCALE_PHRYGIAN:
    case SCALE_AEOLIAN:
    case SCALE_LOCRIAN:
    case SCALE_HARMONIC_MINOR:
    case SCALE_MELODIC_MINOR:
      return true;
    case SCALE_IONIAN:
    case SCALE_LYDIAN:
    case SCALE_MIXOLYDIAN:
    default:
      return false;
  }
}

static uint32_t nextSuggestionRandom() {
  chordSuggestionRng ^= (uint32_t)millis() + 0x9E3779B9u;
  chordSuggestionRng ^= chordSuggestionRng << 13;
  chordSuggestionRng ^= chordSuggestionRng >> 17;
  chordSuggestionRng ^= chordSuggestionRng << 5;
  return chordSuggestionRng;
}

static void invalidateChordSuggestions() {
  chordSuggestionsDirty = true;
}

static const DegreeWeight* getSuggestionTable(bool minorFamily, int degree, int& count) {
  static const DegreeWeight kMajorSeed[] = {{0, 36}, {4, 20}, {3, 18}, {5, 14}, {1, 12}};
  static const DegreeWeight kMinorSeed[] = {{0, 34}, {5, 18}, {3, 18}, {4, 16}, {6, 14}};
  static const DegreeWeight kMajorTransitions[7][6] = {
    {{4, 26}, {3, 20}, {5, 16}, {1, 16}, {2, 8}, {0, 12}},
    {{4, 30}, {6, 18}, {0, 18}, {3, 14}, {5, 10}, {1, 10}},
    {{5, 26}, {3, 20}, {1, 18}, {4, 12}, {0, 10}, {2, 8}},
    {{4, 28}, {0, 20}, {1, 18}, {5, 12}, {6, 10}, {3, 12}},
    {{0, 34}, {5, 18}, {3, 14}, {1, 14}, {6, 10}, {4, 10}},
    {{1, 22}, {3, 20}, {4, 18}, {0, 18}, {2, 10}, {5, 12}},
    {{0, 40}, {5, 18}, {1, 16}, {3, 10}, {4, 8}, {6, 8}}
  };
  static const DegreeWeight kMinorTransitions[7][6] = {
    {{5, 22}, {3, 20}, {4, 18}, {6, 14}, {1, 12}, {0, 14}},
    {{4, 28}, {6, 20}, {0, 18}, {3, 16}, {5, 10}, {1, 8}},
    {{5, 22}, {3, 18}, {6, 16}, {0, 16}, {4, 12}, {2, 10}},
    {{4, 26}, {0, 20}, {6, 16}, {5, 14}, {1, 12}, {3, 12}},
    {{0, 32}, {5, 18}, {3, 16}, {6, 12}, {1, 10}, {4, 12}},
    {{3, 20}, {4, 18}, {0, 18}, {6, 16}, {2, 12}, {5, 10}},
    {{0, 28}, {5, 18}, {3, 16}, {4, 14}, {1, 12}, {6, 12}}
  };

  if (!isValidDegree(degree)) {
    count = 5;
    return minorFamily ? kMinorSeed : kMajorSeed;
  }

  count = 6;
  return minorFamily ? kMinorTransitions[degree] : kMajorTransitions[degree];
}

static int pickWeightedUniqueDegree(const DegreeWeight* table, int count, const bool used[7]) {
  int totalWeight = 0;
  for (int i = 0; i < count; ++i) {
    const int degree = table[i].degree;
    if (isValidDegree(degree) && !used[degree]) {
      totalWeight += table[i].weight;
    }
  }

  if (totalWeight <= 0) {
    return -1;
  }

  int roll = (int)(nextSuggestionRandom() % (uint32_t)totalWeight);
  for (int i = 0; i < count; ++i) {
    const int degree = table[i].degree;
    if (!isValidDegree(degree) || used[degree]) {
      continue;
    }
    if (roll < table[i].weight) {
      return degree;
    }
    roll -= table[i].weight;
  }

  return -1;
}

static void recomputeChordSuggestions(int contextDegree) {
  const bool minorFamily = isMinorFamilyScale(currentScale);
  int tableCount = 0;
  const DegreeWeight* table = getSuggestionTable(minorFamily, contextDegree, tableCount);
  bool used[7] = {false, false, false, false, false, false, false};

  chordSuggestionCount = 0;
  chordSuggestionContextDegree = contextDegree;

  int strongestIndex = 0;
  for (int i = 1; i < tableCount; ++i) {
    if (table[i].weight > table[strongestIndex].weight) {
      strongestIndex = i;
    }
  }

  if (tableCount > 0 && isValidDegree(table[strongestIndex].degree)) {
    chordSuggestions[chordSuggestionCount++] = table[strongestIndex].degree;
    used[table[strongestIndex].degree] = true;
  }

  while (chordSuggestionCount < CHORD_SUGGESTION_SIZE) {
    const int picked = pickWeightedUniqueDegree(table, tableCount, used);
    if (!isValidDegree(picked)) {
      break;
    }
    chordSuggestions[chordSuggestionCount++] = picked;
    used[picked] = true;
  }

  for (int i = 0; i < tableCount && chordSuggestionCount < CHORD_SUGGESTION_SIZE; ++i) {
    const int degree = table[i].degree;
    if (isValidDegree(degree) && !used[degree]) {
      chordSuggestions[chordSuggestionCount++] = degree;
      used[degree] = true;
    }
  }

  chordSuggestionsDirty = false;
}

static void refreshChordSuggestionsForDegree(int degree) {
  recomputeChordSuggestions(degree);
}

static const uint8_t* getScaleIntervals(ScaleType t) {
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
}

static TriadQuality triadQualityFromScaleDegree(int degree, const uint8_t* scale) {
  if (scale == nullptr) return TRIAD_QUALITY_UNKNOWN;
  int d = degree % 7;
  if (d < 0) d += 7;

  const int third = scale[(d + 2) % 7];
  const int fifth = scale[(d + 4) % 7];
  const int thirdInt = (third - scale[d] + 12) % 12;
  const int fifthInt = (fifth - scale[d] + 12) % 12;

  if (thirdInt == 4 && fifthInt == 7) return TRIAD_QUALITY_MAJOR;
  if (thirdInt == 3 && fifthInt == 7) return TRIAD_QUALITY_MINOR;
  if (thirdInt == 3 && fifthInt == 6) return TRIAD_QUALITY_DIMINISHED;
  if (thirdInt == 4 && fifthInt == 8) return TRIAD_QUALITY_AUGMENTED;
  return TRIAD_QUALITY_UNKNOWN;
}

static void triadFromScaleDegree(int degree, const uint8_t* scale, const int*& intervals, int& size, const char*& name) {
  switch (triadQualityFromScaleDegree(degree, scale)) {
    case TRIAD_QUALITY_MAJOR:
      intervals = CHORD_MAJ; size = 3; name = "Maj"; break;
    case TRIAD_QUALITY_MINOR:
      intervals = CHORD_MIN; size = 3; name = "Min"; break;
    case TRIAD_QUALITY_DIMINISHED:
      intervals = CHORD_DIM; size = 3; name = "Dim"; break;
    case TRIAD_QUALITY_AUGMENTED:
      intervals = CHORD_AUG; size = 3; name = "Aug"; break;
    default:
      intervals = CHORD_MIN; size = 3; name = "Min"; break;
  }
}

bool isValidDegree(int degree) {
  return isValidDegreeIndex(degree);
}

static ChordPlayback playback;
static uint8_t noteOwners[128] = {};
static void retainNote(int note, uint8_t velocity) {
  if (noteOwners[note]++ == 0) midiNoteOn(note, velocity);
}
static void noteOn(int note) { retainNote(note, 100); }
static void noteOff(int note) {
  if (noteOwners[note] && --noteOwners[note] == 0) midiNoteOff(note, 0);
}

void updateChordPlayback() { playback.tick(millis(), noteOn); }
bool isChordPlaybackPending() { return playback.pending(); }

static bool buildVoicedCandidate(int root, const int* intervals, int size, int inversion, int* outNotes) {
  if (intervals == nullptr || outNotes == nullptr || size <= 0 || size > kMaxChordNotes) {
    return false;
  }
  if (inversion < 0 || inversion >= size) {
    return false;
  }

  for (int i = 0; i < size; i++) {
    int note = root + intervals[i];
    if (i < inversion) {
      note += 12;
    }
    if (note < 0 || note > 127) {
      return false;
    }
    outNotes[i] = note;
  }
  return true;
}

static int nearestDistanceToSet(int note, const int* setNotes, int setSize) {
  if (setSize <= 0) return 12;
  int best = 127;
  for (int i = 0; i < setSize; i++) {
    int d = abs(note - setNotes[i]);
    if (d < best) {
      best = d;
    }
  }
  return best;
}

static int computeMotionCost(const int* candidate, int candSize) {
  if (!hasPreviousVoicing || previousVoicedSize <= 0) return 0;

  int cost = 0;
  for (int i = 0; i < candSize; i++) {
    cost += nearestDistanceToSet(candidate[i], previousVoicedNotes, previousVoicedSize);
  }
  for (int j = 0; j < previousVoicedSize; j++) {
    cost += nearestDistanceToSet(previousVoicedNotes[j], candidate, candSize);
  }
  return cost;
}

static int countCommonTones(const int* candidate, int candSize) {
  if (!hasPreviousVoicing || previousVoicedSize <= 0) return 0;

  bool matchedPrev[kMaxChordNotes] = {false, false, false, false, false};
  int common = 0;
  for (int i = 0; i < candSize; i++) {
    for (int j = 0; j < previousVoicedSize; j++) {
      if (!matchedPrev[j] && candidate[i] == previousVoicedNotes[j]) {
        matchedPrev[j] = true;
        common++;
        break;
      }
    }
  }
  return common;
}

static int computeRangePenalty(const int* candidate, int candSize) {
  int penalty = 0;
  for (int i = 0; i < candSize; i++) {
    if (candidate[i] < SMART_VOICING_RANGE_MIN) {
      penalty += (SMART_VOICING_RANGE_MIN - candidate[i]);
    } else if (candidate[i] > SMART_VOICING_RANGE_MAX) {
      penalty += (candidate[i] - SMART_VOICING_RANGE_MAX);
    }
  }
  return penalty;
}

static int computeSpanPenalty(const int* candidate, int candSize) {
  if (candSize <= 1) return 0;
  int low = candidate[0];
  int high = candidate[0];
  for (int i = 1; i < candSize; i++) {
    if (candidate[i] < low) low = candidate[i];
    if (candidate[i] > high) high = candidate[i];
  }
  const int span = high - low;
  return abs(span - SMART_TARGET_SPAN);
}

static int scoreVoicedCandidate(const int* candidate, int candSize) {
  const int motion = computeMotionCost(candidate, candSize);
  const int commonTones = countCommonTones(candidate, candSize);
  const int rangePenalty = computeRangePenalty(candidate, candSize);
  const int spanPenalty = computeSpanPenalty(candidate, candSize);

  int score = 0;
  score -= motion * SMART_WEIGHT_MOTION;
  score += commonTones * SMART_WEIGHT_COMMON_TONE;
  score -= rangePenalty * SMART_WEIGHT_RANGE;
  score -= spanPenalty * SMART_WEIGHT_SPAN;
  return score;
}

static void chooseSmartVoicing(int baseRoot, const int* intervals, int size, int& selectedRoot, int& selectedInversion) {
  if (intervals == nullptr || size <= 0 || size > kMaxChordNotes) {
    selectedRoot = baseRoot;
    selectedInversion = 0;
    return;
  }

  int bestScore = -1000000000;
  int bestRoot = baseRoot;
  int bestInversion = 0;
  int candidateNotes[kMaxChordNotes] = {0};

  for (int octaveShift = SMART_VOICING_OCTAVE_MIN; octaveShift <= SMART_VOICING_OCTAVE_MAX; octaveShift++) {
    const int testRoot = baseRoot + (octaveShift * 12);

    for (int inversion = 0; inversion < size; inversion++) {
      if (!buildVoicedCandidate(testRoot, intervals, size, inversion, candidateNotes)) {
        continue;
      }

      const int score = scoreVoicedCandidate(candidateNotes, size);
      if (score > bestScore) {
        bestScore = score;
        bestRoot = testRoot;
        bestInversion = inversion;
      }
    }
  }

  selectedRoot = bestRoot;
  selectedInversion = bestInversion;
}

static bool playVoicedIntervalsForDegree(int degree, int baseRoot, const int* intervals, int size, const char* chordName, const char* debugTag) {
  (void)debugTag;
  if (!isValidDegree(degree) || intervals == nullptr || size <= 0 || size > kMaxChordNotes) {
    return false;
  }

  int root = baseRoot;
  int inversionToUse = chordInversions[degree];

  if (voicingMode == VOICING_SMART) {
    chooseSmartVoicing(root, intervals, size, root, inversionToUse);

#if CHOCO_LOG_LEVEL >= CHOCO_LOG_LEVEL_DEBUG
    Serial.print(debugTag);
    Serial.print(" degree=");
    Serial.print(degree);
    Serial.print(" root=");
    Serial.print(root);
    Serial.print(" inversion=");
    Serial.println(inversionToUse);
#endif
  }
  int candidate[kMaxChordNotes];
  inversionToUse %= size;
  if (!buildVoicedCandidate(root, intervals, size, inversionToUse, candidate)) return false;
  currentInversion = inversionToUse;
  playChord(root, intervals, size, chordName);
  return true;
}

void playChordForDegree(int degree) {
  if (!isValidDegree(degree)) {
    return;
  }

  if (singleNoteMode) {
    playSingleNoteForDegree(degree);
    return;
  }
  
  const uint8_t* scale = getScaleIntervals(currentScale);
  int root = rootNote + scale[degree];
  
  // Apply octave offset
  root += (octaveOffset * 12);

  const int* triad = nullptr; int triadSize = 0; const char* triadName = "";
  triadFromScaleDegree(degree, scale, triad, triadSize, triadName);

  // Play triad determined by current scale through shared voicing pipeline.
  if (!playVoicedIntervalsForDegree(degree, root, triad, triadSize, triadName, "Smart voicing")) return;
  activeChordDegree = degree;
  refreshChordSuggestionsForDegree(degree);
}

// Variant: play a specific degree but with custom chord intervals (e.g., Maj7/Sus)
void playChordForDegreeWithIntervals(int degree, const int* forcedIntervals, int forcedSize, const char* forcedName) {
  if (!isValidDegree(degree)) {
    return;
  }

  if (singleNoteMode) {
    playSingleNoteForDegree(degree);
    return;
  }

  if (forcedIntervals == nullptr || forcedSize <= 0 || forcedSize > kMaxChordNotes) {
    return;
  }

  const uint8_t* scale = getScaleIntervals(currentScale);
  int root = rootNote + scale[degree];
  
  // Apply octave offset
  root += (octaveOffset * 12);

  // Play with the provided intervals through shared voicing pipeline.
  if (!playVoicedIntervalsForDegree(degree, root, forcedIntervals, forcedSize, forcedName, "Smart voicing (forced)")) return;
  activeChordDegree = degree;
  refreshChordSuggestionsForDegree(degree);
}

void playChord(int root, const int* intervals, int size, const char* name) {
  if (intervals == nullptr || size <= 0) {
    stopCurrentChord();
    return;
  }
  if (size > kMaxChordNotes) {
    size = kMaxChordNotes;
  }
  int inversionToApply = currentInversion;
  if (inversionToApply < 0) {
    inversionToApply = 0;
  } else if (size > 0) {
    inversionToApply %= size;
  }

  int targetNotes[kMaxChordNotes] = {0};
  for (int i = 0; i < size; i++) {
    int note = root + intervals[i];
    if (i < inversionToApply) {
      note += 12;
    }
    if (note < 0 || note > 127) {
      return;
    }
    targetNotes[i] = note;
  }
  currentInversion = inversionToApply;

  int nextBassNote = -1;
  if (bassMode) {
    nextBassNote = root + BASS_OCTAVE_OFFSET;
    if (nextBassNote < 0) nextBassNote = 0;
    if (nextBassNote > 127) nextBassNote = 127;
  }

  const bool legato = voicingMode == VOICING_SMART && currentChord.size > 0;
  if (bassNote >= 0 && (bassNote != nextBassNote || !legato)) noteOff(bassNote);
  if (nextBassNote >= 0 && (nextBassNote != bassNote || !legato)) retainNote(nextBassNote, 80);
  bassNote = nextBassNote;
  playback.start(targetNotes, size, legato, strumMode ? STRUM_NOTE_DELAY_MS : 0,
                 millis(), noteOn, noteOff);

  for (int i = 0; i < size; i++) {
    currentChord.notes[i] = targetNotes[i];
  }
  currentChord.size = size;
  currentChord.root = root;

  const int storedSize = size;
  for (int i = 0; i < storedSize; i++) {
    previousVoicedNotes[i] = currentChord.notes[i];
  }
  previousVoicedSize = storedSize;
  hasPreviousVoicing = (storedSize > 0);
  
  // Build chord name with slash notation for inversions
  const char* chordLabel = (name != nullptr) ? name : "";
  snprintf(currentChord.name, sizeof(currentChord.name), "%s %s", getNoteName(root), chordLabel);
  int lowestNote = targetNotes[0];
  for (int i = 1; i < size; ++i) if (targetNotes[i] < lowestNote) lowestNote = targetNotes[i];
  if (bassNote >= 0 && bassNote < lowestNote) lowestNote = bassNote;
  if ((lowestNote % 12) != (root % 12)) {
    const size_t used = strlen(currentChord.name);
    snprintf(currentChord.name + used, sizeof(currentChord.name) - used, "/%s", getNoteName(lowestNote));
  }

  // Determine scale degree by matching root offset to current scale intervals
  int interval = (root - rootNote) % 12;
  if (interval < 0) interval += 12;
  const uint8_t* scale = getScaleIntervals(currentScale);
  int degree = -1;
  for (int i = 0; i < 7; i++) {
    if (scale[i] == interval) { degree = i; break; }
  }

#if LOG_CHORDS
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
    static char alteredRoman[12];
    if (thirdInt == 3 && fifthInt == 6) {
      snprintf(alteredRoman, sizeof(alteredRoman), "%s(dim)", R_LO[d]);
      return alteredRoman;
    }
    if (thirdInt == 4 && fifthInt == 8) {
      snprintf(alteredRoman, sizeof(alteredRoman), "%s+", R_UP[d]);
      return alteredRoman;
    }
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
    Serial.print(getNoteName(lowestNote));
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

  // History content is independent of the serial logging level.
  chordHistoryDegrees[historyWriteIndex] = degree;
  activeChordDegree = degree;
  snprintf(chordHistory[historyWriteIndex], CHORD_NAME_CAPACITY, "%s", currentChord.name);
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
    noteOff(bassNote);
    bassNote = -1;
  }
  
  playback.stop(noteOff);
  currentChord.size = 0;
  activeChordDegree = -1;
}

const char* getCurrentChordName() {
  return currentChord.size > 0 ? currentChord.name : "";
}

int getCurrentRootNote() {
  return rootNote;
}

void setCurrentRootNote(int newRoot) {
  // Clamp to 0..11 offset from BASE_NOTE
  int offset = (newRoot - BASE_NOTE) % 12;
  if (offset < 0) offset += 12;
  rootNote = BASE_NOTE + offset;
  invalidateChordSuggestions();
}

ScaleType getScaleType() { return currentScale; }
void setScaleType(ScaleType type) {
  if (type >= SCALE_COUNT) return;
  currentScale = type;
  invalidateChordSuggestions();
}
void cycleScaleType(int step) {
  int t = wrapScaleIndex((int)currentScale, step, (int)SCALE_COUNT);
  currentScale = (ScaleType)t;
  invalidateChordSuggestions();
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

TriadQuality getTriadQualityForDegree(int degree) {
  if (!isValidDegree(degree)) return TRIAD_QUALITY_UNKNOWN;
  const uint8_t* scale = getScaleIntervals(currentScale);
  return triadQualityFromScaleDegree(degree, scale);
}

bool isChordActive() {
  return currentChord.size > 0;
}

int getActiveChordRoot() {
  if (currentChord.size > 0) return currentChord.root;
  return getCurrentRootNote();
}

int getActiveChordDegree() {
  if (!isChordActive()) return -1;
  return activeChordDegree;
}

bool isBassMode() {
  return bassMode;
}

void setBassMode(bool enabled) {
  if (bassMode == enabled) return;
  bassMode = enabled;
  if (bassNote >= 0) { noteOff(bassNote); bassNote = -1; }
  if (bassMode && isChordActive() && !singleNoteMode) {
    bassNote = constrain(currentChord.root + BASS_OCTAVE_OFFSET, 0, 127);
    retainNote(bassNote, 80);
  }
  if (isChordActive() && !singleNoteMode) {
    // Only the optional slash suffix changes when the root pedal toggles.
    // 6/9 is a chord quality, so find only the final slash followed by a note.
    char* slash = strrchr(currentChord.name, '/');
    if (slash && slash[1] >= 'A' && slash[1] <= 'G') *slash = '\0';
    int lowest = currentChord.notes[0];
    for (int i = 1; i < currentChord.size; ++i) if (currentChord.notes[i] < lowest) lowest = currentChord.notes[i];
    if (bassNote >= 0 && bassNote < lowest) lowest = bassNote;
    if (lowest % 12 != currentChord.root % 12) {
      size_t used = strlen(currentChord.name);
      snprintf(currentChord.name + used, sizeof(currentChord.name) - used, "/%s", getNoteName(lowest));
    }
  }
}

void toggleBassMode() {
  setBassMode(!bassMode);
#if CHOCO_LOG_LEVEL >= CHOCO_LOG_LEVEL_INFO
  Serial.print("Bass mode: ");
  Serial.println(bassMode ? "ON" : "OFF");
#endif
}

bool isSingleNoteMode() {
  return singleNoteMode;
}

void setSingleNoteMode(bool enabled) {
  if (singleNoteMode == enabled) {
    return;
  }
  singleNoteMode = enabled;
  // Stop any currently sounding chord/note to avoid mismatched state.
  stopCurrentChord();
#if CHOCO_LOG_LEVEL >= CHOCO_LOG_LEVEL_INFO
  Serial.print("Single note mode: ");
  Serial.println(singleNoteMode ? "ON" : "OFF");
#endif
}

void toggleSingleNoteMode() {
  setSingleNoteMode(!singleNoteMode);
}

void playSingleNoteForDegree(int degree) {
  if (!isValidDegree(degree)) {
    return;
  }

  const uint8_t* scale = getScaleIntervals(currentScale);
  int note = rootNote + scale[degree];
  note += (octaveOffset * 12);

  if (note < 0) note = 0;
  if (note > 127) note = 127;

  if (currentChord.size == 1 && currentChord.notes[0] == note && activeChordDegree == degree) {
    return;
  }

  stopCurrentChord();
  playback.start(&note, 1, false, 0, millis(), noteOn, noteOff);

  currentChord.notes[0] = note;
  currentChord.size = 1;
  currentChord.root = note;
  snprintf(currentChord.name, sizeof(currentChord.name), "%s", getNoteName(note));
  currentInversion = 0;
  activeChordDegree = degree;

  previousVoicedNotes[0] = note;
  previousVoicedSize = 1;
  hasPreviousVoicing = true;

#if LOG_CHORDS
  Serial.print("Playing note: ");
  Serial.print(getNoteName(note));
  Serial.print(" ("); Serial.print(note); Serial.print(")");
  Serial.print(" | Degree: ");
  Serial.println(degree);
#endif

  chordHistoryDegrees[historyWriteIndex] = degree;
  snprintf(chordHistory[historyWriteIndex], CHORD_NAME_CAPACITY, "%s", currentChord.name);
  historyWriteIndex = (historyWriteIndex + 1) % CHORD_HISTORY_SIZE;
  if (historyCount < CHORD_HISTORY_SIZE) historyCount++;
  refreshChordSuggestionsForDegree(degree);
}

void stopSingleNote() {
  stopCurrentChord();
}

bool isChordLatchMode() {
  return chordLatchMode;
}

void setChordLatchMode(bool enabled) {
  chordLatchMode = enabled;
#if CHOCO_LOG_LEVEL >= CHOCO_LOG_LEVEL_INFO
  Serial.print("Chord latch mode: ");
  Serial.println(chordLatchMode ? "ON" : "OFF");
#endif
}

void toggleChordLatchMode() {
  setChordLatchMode(!chordLatchMode);
}

bool isStrumMode() {
  return strumMode;
}

void setStrumMode(bool enabled) {
  strumMode = enabled;
#if CHOCO_LOG_LEVEL >= CHOCO_LOG_LEVEL_INFO
  Serial.print("Strum mode: ");
  Serial.println(strumMode ? "ON" : "OFF");
#endif
}

void toggleStrumMode() {
  setStrumMode(!strumMode);
}

bool isAutoVoicingMode() {
  return voicingMode == VOICING_SMART;
}

void setAutoVoicingMode(bool enabled) {
  voicingMode = enabled ? VOICING_SMART : VOICING_CLASSIC;
#if CHOCO_LOG_LEVEL >= CHOCO_LOG_LEVEL_INFO
  Serial.print("Smart voicing mode: ");
  Serial.println(isAutoVoicingMode() ? "ON" : "OFF");
#endif
}

void toggleAutoVoicingMode() {
  voicingMode = (voicingMode == VOICING_SMART) ? VOICING_CLASSIC : VOICING_SMART;
#if CHOCO_LOG_LEVEL >= CHOCO_LOG_LEVEL_INFO
  Serial.print("Smart voicing mode: ");
  Serial.println(isAutoVoicingMode() ? "ON" : "OFF");
#endif
}

VoicingMode getVoicingMode() {
  return voicingMode;
}

void setVoicingMode(VoicingMode mode) {
  voicingMode = mode;
#if CHOCO_LOG_LEVEL >= CHOCO_LOG_LEVEL_INFO
  Serial.print("Voicing mode: ");
  Serial.println(voicingMode == VOICING_SMART ? "Smart" : "Classic");
#endif
}

int getOctaveOffset() {
  return octaveOffset;
}

void setOctaveOffset(int offset) {
  // Clamp to reasonable range (-2 to +2 octaves)
  if (offset < -2) offset = -2;
  if (offset > 2) offset = 2;
  octaveOffset = offset;
#if CHOCO_LOG_LEVEL >= CHOCO_LOG_LEVEL_INFO
  Serial.print("Octave offset: ");
  Serial.println(octaveOffset);
#endif
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
#if CHOCO_LOG_LEVEL >= CHOCO_LOG_LEVEL_INFO
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
#endif
}

int getChordHistoryCount() {
  return historyCount;
}

const char* getChordHistoryEntry(int index) {
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

int getChordSuggestionCount() {
  if (chordSuggestionsDirty) {
    recomputeChordSuggestions(chordSuggestionContextDegree);
  }
  return chordSuggestionCount;
}

int getChordSuggestionDegree(int index) {
  if (chordSuggestionsDirty) {
    recomputeChordSuggestions(chordSuggestionContextDegree);
  }
  if (index < 0 || index >= chordSuggestionCount) {
    return -1;
  }
  return chordSuggestions[index];
}
