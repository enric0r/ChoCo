#include "MIDI.h"
#include "Config.h"

Adafruit_USBD_MIDI usb_midi;

void setupMIDI() {
    // Optional custom USB strings. Keeping this disabled by default preserves
    // core defaults, which are usually the most host-compatible.
#if USB_USE_CUSTOM_DESCRIPTORS
    TinyUSBDevice.setManufacturerDescriptor(USB_MANUFACTURER);
    TinyUSBDevice.setProductDescriptor(USB_PRODUCT);
#endif

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
