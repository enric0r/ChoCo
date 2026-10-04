#ifndef CHORD_ENGINE_H
#define CHORD_ENGINE_H

#include <Arduino.h>
#include "Config.h"
#include "MIDI.h"

struct Chord {
  int notes[5];
  int size;
  int root; // MIDI note of the chord root used when the chord was started (pre-inversion)
  char name[CHORD_NAME_CAPACITY];
};

// Supported 7-note scales (modes and common minors)
enum ScaleType : uint8_t {
  SCALE_IONIAN = 0,      // Major
  SCALE_DORIAN,
  SCALE_PHRYGIAN,
  SCALE_LYDIAN,
  SCALE_MIXOLYDIAN,
  SCALE_AEOLIAN,         // Natural Minor
  SCALE_LOCRIAN,
  SCALE_HARMONIC_MINOR,
  SCALE_MELODIC_MINOR,
  SCALE_COUNT
};

enum VoicingMode : uint8_t {
  VOICING_CLASSIC = 0,
  VOICING_SMART
};

enum TriadQuality : uint8_t {
  TRIAD_QUALITY_MAJOR = 0,
  TRIAD_QUALITY_MINOR,
  TRIAD_QUALITY_DIMINISHED,
  TRIAD_QUALITY_AUGMENTED,
  TRIAD_QUALITY_UNKNOWN
};

bool isValidDegree(int degree);
void playChordForDegree(int degree);
void playChord(int root, const int* intervals, int size, const char* name);
// Play a chord for the given degree using an explicit intervals set (e.g., Maj7, Sus).
// Applies stored inversion/auto-voicing like playChordForDegree.
void playChordForDegreeWithIntervals(int degree, const int* intervals, int size, const char* name);
void stopCurrentChord();
const char* getCurrentChordName();
void updateChordPlayback();
bool isChordPlaybackPending();
int getCurrentRootNote();
void setCurrentRootNote(int rootNote);
// Scale selection
ScaleType getScaleType();
void setScaleType(ScaleType type);
void cycleScaleType(int step = 1);
const char* getCurrentScaleName();

void setCurrentInversion(int inversion);
int getCurrentInversion();
// Per-chord inversion functions
void setInversionForDegree(int degree, int inversion);
int getInversionForDegree(int degree);
void cycleInversionForDegree(int degree);
TriadQuality getTriadQualityForDegree(int degree);
// True if a chord is currently sounding (notes are on)
bool isChordActive();
int getActiveChordRoot();
int getActiveChordDegree();

// Bass mode functions
bool isBassMode();
void setBassMode(bool enabled);
void toggleBassMode();

// Single note mode functions
bool isSingleNoteMode();
void setSingleNoteMode(bool enabled);
void toggleSingleNoteMode();
void playSingleNoteForDegree(int degree);
void stopSingleNote();

// Chord latch mode functions
bool isChordLatchMode();
void setChordLatchMode(bool enabled);
void toggleChordLatchMode();

// Strum mode functions
bool isStrumMode();
void setStrumMode(bool enabled);
void toggleStrumMode();

// Auto-voicing mode functions
bool isAutoVoicingMode();
void setAutoVoicingMode(bool enabled);
void toggleAutoVoicingMode();
VoicingMode getVoicingMode();
void setVoicingMode(VoicingMode mode);

// Octave offset functions
int getOctaveOffset();
void setOctaveOffset(int offset);
void incrementOctave();
void decrementOctave();

// Chord history functions
void printChordHistory();
int getChordHistoryCount();
const char* getChordHistoryEntry(int index); // 0 = most recent
int getChordHistoryDegree(int index);   // Get degree number for display (0-6, or -1 if unknown)

// Chord suggestion functions
int getChordSuggestionCount();
int getChordSuggestionDegree(int index); // 0 = strongest suggestion

extern const int CHORD_MAJ[];
extern const int CHORD_MIN[];
extern const int CHORD_DIM[];
extern const int CHORD_AUG[];
extern const int CHORD_SUS4[];
extern const int CHORD_MAJ7[];
extern const int CHORD_MIN7[];
extern const int CHORD_SUS2[];
extern const int CHORD_MAJ6[];
extern const int CHORD_MAJ9[];
extern const int CHORD_DOM7[];
extern const int CHORD_DOM9[];
extern const int CHORD_DOM11[];
extern const int CHORD_MIN9[];
extern const int CHORD_SUS4_7[];
extern const int CHORD_ADD11[];
extern const int CHORD_HALFDIM7[];
extern const int CHORD_DOM7_SHARP9[];
extern const int CHORD_ADD9[];
extern const int CHORD_MIN11[];
extern const int CHORD_MIN_MAJ7[];
extern const int CHORD_MAJ13[];
extern const int CHORD_SIX_NINE[];
extern const int CHORD_MAJ7_SHARP11[];
extern const int CHORD_DOM13[];
extern const int CHORD_DOM7_FLAT9[];
extern const int CHORD_DOM7_ALT[];

#endif
