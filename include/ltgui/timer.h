#pragma once
#include <cstdint>
#include <functional>

namespace ltgui {

// Lightweight timer integrated with the Application event loop.
// Timers are NOT widget-owned — they live independently and fire
// callbacks on the main thread. Call stop() to cancel, or let the
// Timer go out of scope (destructor auto-cancels).
//
// Usage:
//   Timer t;
//   t.start(1000, true, []{ LOG_INFO("Tick", "fired"); });
//   // ... later:
//   t.stop();

class Timer {
public:
  using Callback = std::function<void()>;

  Timer() = default;
  ~Timer() { stop(); }

  Timer(const Timer &) = delete;
  Timer &operator=(const Timer &) = delete;
  Timer(Timer &&other) noexcept;

  // Start the timer. If `repeating` is true, fires every `ms` milliseconds.
  // If false, fires once then auto-stops.
  void start(int ms, bool repeating, Callback cb);

  // Convenience: fire once after `ms` milliseconds.
  // Self-managing — the timer is heap-allocated and auto-deletes after firing.
  static void singleShot(int ms, Callback cb) {
    auto *t = new Timer();
    auto wrapped = [t, cb = std::move(cb)]() mutable {
      cb();
      delete t;
    };
    t->start(ms, false, std::move(wrapped));
  }

  // Stop the timer. Safe to call multiple times.
  void stop();

  bool isActive() const { return id_ >= 0; }

private:
  friend class Application;
  // Called each frame by Application::processEvents().
  // Returns true if the timer fired.
  bool tick(uint64_t nowMs);

  int id_ = -1;
  Callback cb_;
  int interval_ = 0;
  bool repeating_ = false;
  uint64_t nextFireMs_ = 0;

  static int nextId_;
  friend class Application;
};

} // namespace ltgui
