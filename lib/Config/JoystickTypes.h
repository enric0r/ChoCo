#pragma once
#include <stdint.h>

enum class JoystickDirection : uint8_t {
  Center = 0,
  Up,
  UpRight,
  Right,
  DownRight,
  Down,
  DownLeft,
  Left,
  UpLeft
};
