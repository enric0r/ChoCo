#ifndef ARPEGGIATOR_H
#define ARPEGGIATOR_H

#include <Arduino.h>

// Arpeggiator pattern types
enum ArpPattern {
  ARP_UP,         // Low to high
  ARP_DOWN,       // High to low
  ARP_UP_DOWN,    // Low to high and back
  ARP_RANDOM,     // Random order
  ARP_STRUM_UP,   // Strum upward (quick succession)
  ARP_STRUM_DOWN  // Strum downward (quick succession)
};

// Initialize arpeggiator
void setupArpeggiator();

// Update arpeggiator state (call in main loop)
void updateArpeggiator();

// Enable/disable arpeggiator
void setArpeggiatorEnabled(bool enabled);

// Check if arpeggiator is enabled
bool isArpeggiatorEnabled();

// Set arpeggiator pattern
void setArpPattern(ArpPattern pattern);

// Get current pattern
ArpPattern getArpPattern();

// Set note division (1=quarter, 2=eighth, 4=sixteenth, etc.)
void setArpNoteDivision(int division);

// Get note division
int getArpNoteDivision();

// Start arpeggiating the current chord
void startArpeggio(const int* notes, int noteCount);

// Stop arpeggio
void stopArpeggio();

// Check if arpeggio is active
bool isArpeggioActive();

#endif
