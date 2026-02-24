#include <cassert>
#include "JoystickDirection.h"

static constexpr JoystickClassifierConfig kCfg = {
  512,
  512,
  72,
  55,
  0,
  1023
};

int main() {
  // Inside engage radius stays centered in instant mode.
  assert(classifyJoystickDirectionInstantWithConfig(512, 512, kCfg) == JoystickDirection::Center);
  assert(classifyJoystickDirectionInstantWithConfig(712, 512, kCfg) == JoystickDirection::Center);

  // Cardinal directions.
  assert(classifyJoystickDirectionInstantWithConfig(512, 1023, kCfg) == JoystickDirection::Up);
  assert(classifyJoystickDirectionInstantWithConfig(512, 0, kCfg) == JoystickDirection::Down);
  assert(classifyJoystickDirectionInstantWithConfig(1023, 512, kCfg) == JoystickDirection::Right);
  assert(classifyJoystickDirectionInstantWithConfig(0, 512, kCfg) == JoystickDirection::Left);

  // Diagonals.
  assert(classifyJoystickDirectionInstantWithConfig(850, 850, kCfg) == JoystickDirection::UpRight);
  assert(classifyJoystickDirectionInstantWithConfig(180, 850, kCfg) == JoystickDirection::UpLeft);
  assert(classifyJoystickDirectionInstantWithConfig(850, 180, kCfg) == JoystickDirection::DownRight);
  assert(classifyJoystickDirectionInstantWithConfig(180, 180, kCfg) == JoystickDirection::DownLeft);

  // Hysteresis: hold direction in the band between release and engage.
  const JoystickDirection previous = JoystickDirection::Right;
  assert(classifyJoystickDirectionLatchedWithConfig(800, 512, previous, kCfg) == JoystickDirection::Right);

  // Drop below release radius resets to center.
  assert(classifyJoystickDirectionLatchedWithConfig(740, 512, previous, kCfg) == JoystickDirection::Center);

  // If previous is center and we're in the hysteresis band, remain centered.
  assert(classifyJoystickDirectionLatchedWithConfig(800, 512, JoystickDirection::Center, kCfg) == JoystickDirection::Center);

  return 0;
}
