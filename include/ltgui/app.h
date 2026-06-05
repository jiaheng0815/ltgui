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
inline bool isMainThread() {
    static std::thread::id s_mainThreadId = std::this_thread::get_id();
    return std::this_thread::get_id() == s_mainThreadId;
}
inline void setMainThread() {
    isMainThread(); // force static init on the calling thread
}
#endif

class Application {
public:
    static Application& instance();

    int run();
    void quit();
    void processEvents();

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
