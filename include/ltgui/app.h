#pragma once
#include "platform/platform.h"
#include "geometry.h"
#include "event.h"
#include <vector>

namespace ltgui {

class Window;

class Application {
public:
    static Application& instance();

    int run();
    void quit();
    void processEvents();

    void registerWindow(Window* window);
    void unregisterWindow(Window* window);

    const std::vector<Window*>& windows() const { return windows_; }

    Application(const Application&) = delete;
    Application& operator=(const Application&) = delete;

private:
    Application() = default;
    ~Application() = default;

    bool running_ = false;
    std::vector<Window*> windows_;
};

} // namespace ltgui
