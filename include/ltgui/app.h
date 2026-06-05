#pragma once
#include "platform/platform.h"
#include "geometry.h"
#include "event.h"
#include <cstdint>
#include <vector>
#include <thread>

namespace ltgui {

class Window;
class Timer;

// ltgui is a single-threaded framework. All widget operations, painting,
// and event handling must happen on the thread that calls Application::run().
// These helpers detect cross-thread misuse at runtime in debug builds.
#ifdef NDEBUG
inline bool isMainThread() { return true; }
inline void setMainThread() {}
#else
// Use an inline variable (C++17) so setMainThread() explicitly records the
// correct thread, regardless of who calls isMainThread() first.  Before
// setMainThread() is called, isMainThread() returns false (fail-closed).
inline std::thread::id s_mainThreadId{};
inline bool isMainThread() {
    return std::this_thread::get_id() == s_mainThreadId;
}
inline void setMainThread() {
    s_mainThreadId = std::this_thread::get_id();
}
#endif

class Application {
public:
    static Application& instance();

    int run();
    void quit();
    void processEvents();

    // Single-frame tick for embedding into external event loops.
    // Pumps pending platform events, ticks timers, and drives animations.
    // Pass timeoutMs=0 to poll without blocking; pass >0 to block up to
    // that many milliseconds waiting for the next event.
    // Returns false if the application should exit (WM_QUIT received, etc.).
    bool tick(int timeoutMs = 0);

    // Returns milliseconds until the next timer fires, or INT64_MAX if idle.
    // Platform event loops use this to compute the correct blocking timeout.
    int64_t nextTimerWakeupMs() const;

    void registerWindow(Window* window);
    void unregisterWindow(Window* window);

    // Close a single window. If no windows remain, the application exits.
    void closeWindow(Window* window);

    const std::vector<Window*>& windows() const { return windows_; }

    // Timer management (internal; use Timer::start/stop instead)
    void registerTimer(Timer* timer);
    void unregisterTimer(Timer* timer);

    // DPI scale factor. Set once at startup; all widgets should
    // multiply their pixel sizes by this value.
    void setDpiScale(float scale) { dpiScale_ = scale > 0 ? scale : 1.0f; }
    float dpiScale() const { return dpiScale_; }

    Application(const Application&) = delete;
    Application& operator=(const Application&) = delete;

private:
    Application() = default;
    ~Application() = default;

    bool running_ = false;
    float dpiScale_ = 1.0f;
    std::vector<Window*> windows_;
    std::vector<Timer*> timers_;
};

} // namespace ltgui
