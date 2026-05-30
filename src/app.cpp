#include "app.h"
#include "window.h"

#ifdef LTGUI_PLATFORM_WINDOWS
#include <windows.h>
#elif defined(LTGUI_PLATFORM_LINUX)
#include "platform/x11/x11_window.h"
#include <X11/Xlib.h>
#include <unistd.h>
#elif defined(LTGUI_PLATFORM_MACOS)
#import <Cocoa/Cocoa.h>
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
#elif defined(LTGUI_PLATFORM_LINUX)
    while (running_) {
        if (!X11Window::processAllPending()) {
            usleep(5000);
        }
        processEvents();
    }
#elif defined(LTGUI_PLATFORM_MACOS)
    [NSApplication sharedApplication];
    [NSApp setActivationPolicy:NSApplicationActivationPolicyRegular];
    [NSApp activateIgnoringOtherApps:YES];
    [NSApp run];
#endif

    return 0;
}

void Application::quit() {
    running_ = false;
#ifdef LTGUI_PLATFORM_WINDOWS
    PostQuitMessage(0);
#elif defined(LTGUI_PLATFORM_MACOS)
    [NSApp terminate:nil];
#endif
}

void Application::processEvents() {
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
