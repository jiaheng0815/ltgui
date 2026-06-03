#pragma once
#include "platform/platform.h"
#include "geometry.h"
#include "event.h"
#include <vector>

namespace ltgui {

class Window;
class Timer;

class Application {
public:
    static Application& instance();

    int run();
    void quit();
    void processEvents();

    void registerWindow(Window* window);
    void unregisterWindow(Window* window);

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
