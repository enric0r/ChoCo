#ifndef JOYSTICK_DIRECTION_H
#define JOYSTICK_DIRECTION_H

#include <stdint.h>
#include "Controls.h"

struct JoystickClassifierConfig {
  int centerX;
  int centerY;
  int engagePercent;
  int releasePercent;
  int minValue;
  int maxValue;
};

constexpr int32_t joystickAbs32(int32_t value) {
  return value < 0 ? -value : value;
}

constexpr int32_t joystickMin32(int32_t a, int32_t b) {
  return a < b ? a : b;
}

constexpr int32_t joystickSq32(int32_t value) {
  return value * value;
}

constexpr int32_t joystickBaseRadius(const JoystickClassifierConfig& cfg) {
  int32_t left = cfg.centerX - cfg.minValue;
  int32_t right = cfg.maxValue - cfg.centerX;
  int32_t down = cfg.centerY - cfg.minValue;
  int32_t up = cfg.maxValue - cfg.centerY;

  int32_t radius = joystickMin32(joystickMin32(left, right), joystickMin32(down, up));
  return radius < 1 ? 1 : radius;
}

constexpr int32_t joystickPercentRadius(int32_t baseRadius, int percent) {
  return (baseRadius * percent) / 100;
}

constexpr JoystickDirection joystickDirectionFromDelta(int32_t dx, int32_t dy) {
  if (dx == 0 && dy == 0) {
    return JoystickDirection::Center;
  }

  const int32_t absX = joystickAbs32(dx);
  const int32_t absY = joystickAbs32(dy);
  const int32_t yScaled = absY * 1000;

  // Octant boundaries at 22.5 deg and 67.5 deg without floating point.
  // tan(22.5 deg) ~= 0.414 and tan(67.5 deg) ~= 2.414.
  if (yScaled >= absX * 2414) {
    return dy >= 0 ? JoystickDirection::Up : JoystickDirection::Down;
  }

  if (yScaled <= absX * 414) {
    return dx >= 0 ? JoystickDirection::Right : JoystickDirection::Left;
  }

  if (dx >= 0 && dy >= 0) return JoystickDirection::UpRight;
  if (dx < 0 && dy >= 0) return JoystickDirection::UpLeft;
  if (dx >= 0 && dy < 0) return JoystickDirection::DownRight;
  return JoystickDirection::DownLeft;
}

constexpr JoystickDirection classifyJoystickDirectionInstantWithConfig(
    int x, int y, const JoystickClassifierConfig& cfg) {
  const int32_t dx = x - cfg.centerX;
  const int32_t dy = y - cfg.centerY;
  const int32_t radiusSq = joystickSq32(dx) + joystickSq32(dy);
  const int32_t engageRadius = joystickPercentRadius(joystickBaseRadius(cfg), cfg.engagePercent);
  const int32_t engageSq = joystickSq32(engageRadius);

  if (radiusSq < engageSq) {
    return JoystickDirection::Center;
  }
  return joystickDirectionFromDelta(dx, dy);
}

constexpr JoystickDirection classifyJoystickDirectionLatchedWithConfig(
    int x, int y, JoystickDirection previous, const JoystickClassifierConfig& cfg) {
  const int32_t dx = x - cfg.centerX;
  const int32_t dy = y - cfg.centerY;
  const int32_t radiusSq = joystickSq32(dx) + joystickSq32(dy);

  const int32_t baseRadius = joystickBaseRadius(cfg);
  const int32_t engageRadius = joystickPercentRadius(baseRadius, cfg.engagePercent);
  const int32_t releaseRadius = joystickPercentRadius(baseRadius, cfg.releasePercent);
  const int32_t engageSq = joystickSq32(engageRadius);
  const int32_t releaseSq = joystickSq32(releaseRadius);

  if (radiusSq < releaseSq) {
    return JoystickDirection::Center;
  }
  if (radiusSq >= engageSq) {
    return joystickDirectionFromDelta(dx, dy);
  }
  return previous;
}

#endif
