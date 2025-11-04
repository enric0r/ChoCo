#ifndef CHORD_ENGINE_H
#define CHORD_ENGINE_H

#include <Arduino.h>
#include "Config.h"
#include "MIDI.h"

struct Chord {
  int notes[5];
  int size;
  int root; // MIDI note of the chord root used when the chord was started (pre-inversion)
  String name;
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

void playChordForDegree(int degree);
void playChord(int root, const int* intervals, int size, String name);
void stopCurrentChord();
String getCurrentChordName();
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
// True if a chord is currently sounding (notes are on)
bool isChordActive();
int getActiveChordRoot();

// Bass mode functions
bool isBassMode();
void setBassMode(bool enabled);
void toggleBassMode();

// Auto-voicing mode functions
bool isAutoVoicingMode();
void setAutoVoicingMode(bool enabled);
void toggleAutoVoicingMode();

// Chord history functions
void printChordHistory();
int getChordHistoryCount();
String getChordHistoryEntry(int index); // 0 = most recent
int getChordHistoryDegree(int index);   // Get degree number for display (0-6, or -1 if unknown)

extern const int CHORD_MAJ[];
extern const int CHORD_MIN[];
extern const int CHORD_DIM[];
extern const int CHORD_AUG[];
extern const int CHORD_SUS4[];
extern const int CHORD_MAJ7[];
extern const int CHORD_MIN7[];
extern const int CHORD_SUS2[];
extern const int CHORD_DOM7[];
extern const int CHORD_DOM9[];
extern const int CHORD_DOM11[];
extern const int CHORD_MIN9[];

#endif