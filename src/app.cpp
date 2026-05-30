#include "app.h"
#include "window.h"

#ifdef LTGUI_PLATFORM_WINDOWS
#include <windows.h>
#endif

namespace ltgui {

Application& Application::instance() {
    static Application app;
    return app;
}

int Application::run() {
    running_ = true;

#ifdef LTGUI_PLATFORM_WINDOWS
    MSG msg;
    while (running_) {
        while (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE)) {
            if (msg.message == WM_QUIT) {
                running_ = false;
                break;
            }
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
        processEvents();
    }
#endif

#ifdef LTGUI_PLATFORM_LINUX
    // TODO: X11 event loop
#endif

#ifdef LTGUI_PLATFORM_MACOS
    // TODO: Cocoa event loop
#endif

    return 0;
}

void Application::quit() {
    running_ = false;
#ifdef LTGUI_PLATFORM_WINDOWS
    PostQuitMessage(0);
#endif
}

void Application::processEvents() {
    // Process any pending tasks (timers, idle callbacks, etc.)
}

void Application::registerWindow(Window* window) {
    windows_.push_back(window);
}

void Application::unregisterWindow(Window* window) {
    auto it = std::find(windows_.begin(), windows_.end(), window);
    if (it != windows_.end()) {
        windows_.erase(it);
    }
}

} // namespace ltgui
