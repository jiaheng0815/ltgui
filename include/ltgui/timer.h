#pragma once
#include <functional>
#include <cstdint>

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

    Timer(const Timer&) = delete;
    Timer& operator=(const Timer&) = delete;
    Timer(Timer&& other) noexcept
        : id_(other.id_), cb_(std::move(other.cb_)), interval_(other.interval_),
          repeating_(other.repeating_), nextFireMs_(other.nextFireMs_) {
        other.id_ = -1;
    }

    // Start the timer. If `repeating` is true, fires every `ms` milliseconds.
    // If false, fires once then auto-stops.
    void start(int ms, bool repeating, Callback cb);

    // Convenience: fire once after `ms` milliseconds.
    static Timer singleShot(int ms, Callback cb) {
        Timer t;
        t.start(ms, false, std::move(cb));
        return t;
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
