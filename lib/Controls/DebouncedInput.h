#pragma once
#include <stdint.h>

// Both edges must be stable. A short release bounce never rearms a held key.
class DebouncedInput {
 public:
  bool update(bool raw, uint32_t now, uint32_t interval) {
    if (raw != candidate_) {
      candidate_ = raw;
      changedAt_ = now;
    }
    if (stable_ != candidate_ && uint32_t(now - changedAt_) >= interval) {
      stable_ = candidate_;
      return true;
    }
    return false;
  }
  bool held() const { return stable_; }
 private:
  bool candidate_ = false;
  bool stable_ = false;
  uint32_t changedAt_ = 0;
};
