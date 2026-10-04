#include <cassert>
#include "DebouncedInput.h"

int main() {
  DebouncedInput key;
  assert(!key.update(true, 100, 10));
  assert(!key.update(false, 102, 10));
  assert(!key.update(true, 104, 10));
  assert(!key.update(true, 113, 10));
  assert(key.update(true, 114, 10) && key.held());
  assert(!key.update(false, 115, 10));
  assert(!key.update(true, 117, 10));
  assert(!key.update(true, 140, 10)); // no repeated press after release bounce
  assert(!key.update(false, 150, 10));
  assert(key.update(false, 160, 10) && !key.held());
  DebouncedInput rollover;
  assert(!rollover.update(true, UINT32_MAX - 5, 10));
  assert(!rollover.update(true, 3, 10));
  assert(rollover.update(true, 4, 10));
}
