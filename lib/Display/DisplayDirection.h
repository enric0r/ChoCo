#pragma once
#include "../Config/JoystickTypes.h"

struct DirectionVector { int x; int y; };

// Screen-space mapping for the current physical mounting. Keep the existing
// vertical mapping; removing the horizontal mirror fixes left/right and diagonals.
constexpr DirectionVector displayDirectionVector(JoystickDirection direction) {
  switch (direction) {
    case JoystickDirection::Up: return {0, 1};
    case JoystickDirection::UpRight: return {1, 1};
    case JoystickDirection::Right: return {1, 0};
    case JoystickDirection::DownRight: return {1, -1};
    case JoystickDirection::Down: return {0, -1};
    case JoystickDirection::DownLeft: return {-1, -1};
    case JoystickDirection::Left: return {-1, 0};
    case JoystickDirection::UpLeft: return {-1, 1};
    default: return {0, 0};
  }
}
