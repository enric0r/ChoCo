#ifndef CHORD_LOGIC_H
#define CHORD_LOGIC_H

constexpr bool isValidDegreeIndex(int degree) {
  return degree >= 0 && degree < 7;
}

constexpr int wrapScaleIndex(int current, int step, int count) {
  int value = current + step;
  while (value < 0) {
    value += count;
  }
  return value % count;
}

constexpr int normalizeTriadInversion(int inversion) {
  int normalized = inversion % 3;
  if (normalized < 0) {
    normalized += 3;
  }
  return normalized;
}

#endif
