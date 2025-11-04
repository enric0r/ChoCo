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

void playChordForDegree(int degree);
void playChord(int root, const int* intervals, int size, String name);
void stopCurrentChord();
String getCurrentChordName();
int getCurrentRootNote();
void setCurrentRootNote(int rootNote);
bool isMinorScale();
void setMinorScale(bool isMinor);
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