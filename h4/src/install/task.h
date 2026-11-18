// A long installer step (download, unpacking, checking) as the screen sees
// it: a phase line, bytes done of total, and a cancel flag. Written by the
// worker thread, read by the screen.
#pragma once

#include <atomic>
#include <cstdint>
#include <mutex>
#include <string>

namespace h4::install {

struct Task {
  std::atomic<uint64_t> done{0}, total{0};
  std::atomic<bool> cancel{false};

  void SetPhase(const std::string& s) {
    std::lock_guard<std::mutex> l(mu_);
    phase_ = s;
  }
  std::string Phase() const {
    std::lock_guard<std::mutex> l(mu_);
    return phase_;
  }

 private:
  mutable std::mutex mu_;
  std::string phase_;
};

}  // namespace h4::install
