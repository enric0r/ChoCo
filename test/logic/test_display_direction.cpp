#include <cassert>
#include "DisplayDirection.h"

int main() {
  const JoystickDirection right[] = {JoystickDirection::Right, JoystickDirection::UpRight, JoystickDirection::DownRight};
  const JoystickDirection left[] = {JoystickDirection::Left, JoystickDirection::UpLeft, JoystickDirection::DownLeft};
  for (auto d : right) assert(displayDirectionVector(d).x == 1);
  for (auto d : left) assert(displayDirectionVector(d).x == -1);
  // Existing vertical orientation must survive the horizontal correction.
  assert(displayDirectionVector(JoystickDirection::Up).y == 1);
  assert(displayDirectionVector(JoystickDirection::Down).y == -1);
  assert(displayDirectionVector(JoystickDirection::UpRight).y == 1);
  assert(displayDirectionVector(JoystickDirection::DownLeft).y == -1);
  assert(displayDirectionVector(JoystickDirection::Center).x == 0);
  assert(displayDirectionVector(JoystickDirection::Center).y == 0);
}
