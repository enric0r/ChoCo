#include <cassert>
#include <vector>
#include "ChordPlayback.h"

int main() {
  ChordPlayback playback;
  std::vector<int> events;
  auto on = [&](int n) { events.push_back(n); };
  auto off = [&](int n) { events.push_back(-n); };
  const int triad[] = {67, 60, 64};
  playback.start(triad, 3, false, 14, 0, on, off);
  assert(events == std::vector<int>({60}));
  playback.tick(13, on);
  assert(events.size() == 1);
  playback.tick(14, on);
  assert(events == std::vector<int>({60, 64}));
  playback.stop(off);
  playback.tick(1000, on);
  assert(events == std::vector<int>({60, 64, -60, -64}));
  assert(!playback.pending());

  events.clear();
  playback.start(triad, 3, false, 14, UINT32_MAX - 5, on, off);
  playback.tick(8, on); // uint32 rollover: 14ms
  assert(events == std::vector<int>({60, 64}));
  const int next[] = {60, 65, 69};
  playback.start(next, 3, true, 14, 9, on, off);
  assert(events == std::vector<int>({60, 64, -64, 65}));
  playback.tick(23, on);
  assert(events.back() == 69); // old queued 67 must never play
  assert(!playback.pending());
  playback.stop(off);
  events.clear();
  playback.start(triad, 3, false, 0, 100, on, off);
  assert(events == std::vector<int>({60, 64, 67}));
  playback.stop(off);
  events.clear();
  playback.start(triad, 3, false, 14, 100, on, off);
  playback.tick(200, on);
  assert(events == std::vector<int>({60, 64})); // overdue notes are not bunched
  playback.tick(201, on);
  assert(events.size() == 2);
  playback.tick(214, on);
  assert(events.back() == 67);
}
