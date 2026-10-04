#pragma once
#include <stdint.h>

// Fixed storage for sounding notes and cancellable, staggered note-ons.
class ChordPlayback {
 public:
  template<class On, class Off>
  void start(const int* notes, int count, bool legato, uint32_t spacing,
             uint32_t now, On on, Off off) {
    pendingCount_ = 0;
    for (int i = 0; i < soundingCount_;) {
      if (!legato || !contains(notes, count, sounding_[i])) {
        off(sounding_[i]);
        sounding_[i] = sounding_[--soundingCount_];
      } else {
        ++i;
      }
    }
    for (int i = 0; i < count && i < 5; ++i) {
      if (!contains(sounding_, soundingCount_, notes[i]) &&
          !contains(pending_, pendingCount_, notes[i])) {
        pending_[pendingCount_++] = notes[i];
      }
    }
    for (int i = 1; i < pendingCount_; ++i) {
      int j = i;
      while (j > 0 && pending_[j] < pending_[j - 1]) {
        int tmp = pending_[j]; pending_[j] = pending_[j - 1]; pending_[j - 1] = tmp;
        --j;
      }
    }
    spacing_ = spacing;
    lastOn_ = now - spacing;
    tick(now, on);
  }
  template<class On>
  void tick(uint32_t now, On on) {
    while (pendingCount_ && uint32_t(now - lastOn_) >= spacing_) {
      int note = pending_[0];
      for (int i = 1; i < pendingCount_; ++i) pending_[i - 1] = pending_[i];
      --pendingCount_;
      sounding_[soundingCount_++] = note;
      on(note);
      lastOn_ = now;
      // Never bunch overdue strum notes after an expensive display frame.
      if (spacing_) break;
    }
  }
  template<class Off>
  void stop(Off off) {
    pendingCount_ = 0;
    for (int i = 0; i < soundingCount_; ++i) off(sounding_[i]);
    soundingCount_ = 0;
  }
  bool pending() const { return pendingCount_ > 0; }
 private:
  static bool contains(const int* notes, int count, int note) {
    for (int i = 0; i < count; ++i) if (notes[i] == note) return true;
    return false;
  }
  int sounding_[5] = {};
  int pending_[5] = {};
  int soundingCount_ = 0;
  int pendingCount_ = 0;
  uint32_t spacing_ = 0;
  uint32_t lastOn_ = 0;
};
