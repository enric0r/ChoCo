#ifndef MIDI_H
#define MIDI_H

#include <Arduino.h>
#include <Adafruit_TinyUSB.h>

extern Adafruit_USBD_MIDI usb_midi;

void setupMIDI();
void midiNoteOn(byte pitch, byte velocity);
void midiNoteOff(byte pitch, byte velocity);
void flushMIDI();
// Optional: send a short test so hosts can verify MIDI reception
void midiSendTestSequence();

#endif