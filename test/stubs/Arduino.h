#pragma once
#include <stdint.h>
#include <cstdlib>
#include <cstdio>
using byte = uint8_t;
constexpr int INPUT = 0, OUTPUT = 1, INPUT_PULLUP = 2;
constexpr int LOW = 0, HIGH = 1, A0 = 26, A1 = 27;
unsigned long millis();
void pinMode(int, int);
void digitalWrite(int, int);
int digitalRead(int);
int analogRead(int);
void analogReadResolution(int);
void delayMicroseconds(unsigned int);
struct TestSerial { void begin(int) {} };
extern TestSerial Serial;
template<class T> T constrain(T value, T low, T high) {
  return value < low ? low : value > high ? high : value;
}
