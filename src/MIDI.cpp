#include "MIDI.h"
#include "Config.h"

Adafruit_USBD_MIDI usb_midi;

void setupMIDI() {
    // Set a friendly USB device name that will appear on the host
    // Use the Adafruit_TinyUSB API to set string descriptors.
    // You can change these strings to whatever you prefer.
    TinyUSBDevice.setManufacturerDescriptor("ChoCo");
    TinyUSBDevice.setProductDescriptor("ChoCo");

    usb_midi.begin();
}

void midiNoteOn(byte pitch, byte velocity) {
    uint8_t note_on[3] = {0x90, pitch, velocity};
    usb_midi.write(note_on, 3);
    flushMIDI();
}

void midiNoteOff(byte pitch, byte velocity) {
    uint8_t note_off[3] = {0x80, pitch, velocity};
    usb_midi.write(note_off, 3);
    flushMIDI();
}

void flushMIDI() {
    // Let TinyUSB background task run to push packets promptly
    yield();
}

void midiSendTestSequence() {
    // Send C major triad briefly
    const byte notes[3] = {60, 64, 67};
    for (byte i = 0; i < 3; i++) {
        midiNoteOn(notes[i], 100);
        delay(5);
    }
    delay(150);
    for (byte i = 0; i < 3; i++) {
        midiNoteOff(notes[i], 0);
        delay(5);
    }
}

void midiPanic() {
    // Send All Notes Off (CC 123) on all 16 MIDI channels
    for (byte channel = 0; channel < 16; channel++) {
        uint8_t all_notes_off[3] = {(uint8_t)(0xB0 | channel), 123, 0};
        usb_midi.write(all_notes_off, 3);
    }
    
    // Also send Note Off for all possible notes on channel 1 as a fallback
    for (byte note = 0; note < 128; note++) {
        midiNoteOff(note, 0);
    }
    
    flushMIDI();
    Serial.println("MIDI PANIC: All notes off");
}