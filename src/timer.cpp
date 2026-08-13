#include "timer.h"
#include "app.h"
#include <cstdint>

namespace ltgui {

int Timer::nextId_ = 0;

Timer::Timer(Timer &&other) noexcept
    : id_(other.id_), cb_(std::move(other.cb_)), interval_(other.interval_),
      repeating_(other.repeating_), nextFireMs_(other.nextFireMs_) {
  if (other.id_ >= 0) {
    Application::instance().unregisterTimer(&other);
    Application::instance().registerTimer(this);
  }
  other.id_ = -1;
}

void Timer::start(int ms, bool repeating, Callback cb) {
  stop();
  if (ms <= 0 || !cb)
    return;

  id_ = nextId_++;
  cb_ = std::move(cb);
  interval_ = ms;
  repeating_ = repeating;
  nextFireMs_ = UINT64_MAX; // unset until first tick

  Application::instance().registerTimer(this);
}

void Timer::stop() {
  if (id_ >= 0) {
    Application::instance().unregisterTimer(this);
    id_ = -1;
    cb_ = nullptr;
  }
}

bool Timer::tick(uint64_t nowMs) {
  if (id_ < 0 || !cb_)
    return false;

  // Lazy-init on first tick
  if (nextFireMs_ == UINT64_MAX) {
    nextFireMs_ = nowMs + interval_;
    return false;
  }

  if (nowMs >= nextFireMs_) {
    bool wasRepeating = repeating_;
    if (wasRepeating) {
      nextFireMs_ = nowMs + interval_;
    } else {
      // Unregister before the callback so that even if the callback
      // destroys this Timer, we never access freed memory afterward.
      if (id_ >= 0) {
        Application::instance().unregisterTimer(this);
        id_ = -1;
      }
    }
    // Capture the callback locally: if cb_() destroys `this`, the
    // subsequent return statement still executes on valid stack.
    Callback savedCb = std::move(cb_);
    savedCb();
    return true;
  }
  return false;
}

} // namespace ltgui
