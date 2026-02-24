#include <cassert>
#include "ChordLogic.h"

int main() {
  // Degree bounds
  assert(!isValidDegreeIndex(-1));
  assert(isValidDegreeIndex(0));
  assert(isValidDegreeIndex(6));
  assert(!isValidDegreeIndex(7));

  // Scale wraparound
  assert(wrapScaleIndex(0, 1, 9) == 1);
  assert(wrapScaleIndex(8, 1, 9) == 0);
  assert(wrapScaleIndex(0, -1, 9) == 8);
  assert(wrapScaleIndex(2, -4, 9) == 7);

  // Inversion normalization
  assert(normalizeTriadInversion(0) == 0);
  assert(normalizeTriadInversion(1) == 1);
  assert(normalizeTriadInversion(2) == 2);
  assert(normalizeTriadInversion(3) == 0);
  assert(normalizeTriadInversion(-1) == 2);
  assert(normalizeTriadInversion(-4) == 2);

  return 0;
}
