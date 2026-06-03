#include "app.h"
#include "window.h"
#include "timer.h"
#include "animation.h"

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
        auto& anim = AnimationManager::instance();
        DWORD timeout = anim.hasActive() ? 1 : INFINITE;

        DWORD result = MsgWaitForMultipleObjects(0, nullptr, FALSE, timeout, QS_ALLINPUT);
        if (result == WAIT_OBJECT_0) {
            while (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE)) {
                if (msg.message == WM_QUIT) {
                    running_ = false;
                    break;
                }
                TranslateMessage(&msg);
                DispatchMessage(&msg);
            }
        }
        processEvents();
    }
#elif defined(LTGUI_PLATFORM_LINUX)
    while (running_) {
        X11Window::processAllPending();
        processEvents();
        if (!AnimationManager::instance().hasActive()) {
            usleep(16000);
        } else {
            usleep(1000);
        }
    }
#elif defined(LTGUI_PLATFORM_MACOS)
    [NSApplication sharedApplication];
    [NSApp setActivationPolicy:NSApplicationActivationPolicyRegular];
    [NSApp activateIgnoringOtherApps:YES];
    [NSApp finishLaunching];

    while (running_) {
        NSEvent* event = [NSApp nextEventMatchingMask:NSEventMaskAny
                            untilDate:[NSDate dateWithTimeIntervalSinceNow:0.001]
                               inMode:NSDefaultRunLoopMode
                              dequeue:YES];
        if (event) {
            [NSApp sendEvent:event];
        }
        AnimationManager::instance().tick();
        processEvents();
    }
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
    auto& anim = AnimationManager::instance();
    anim.tick();

    uint64_t now = anim.nowMs();

    // Tick timers (iterate backwards so stopped timers can be removed safely)
    for (size_t i = timers_.size(); i > 0; i--) {
        // Timer may stop itself during tick(), invalidating the pointer.
        // But stop() removes from timers_, so we re-fetch each time.
        if (i - 1 < timers_.size()) {
            Timer* t = timers_[i - 1];
            if (t) t->tick(now);
        }
    }

    if (anim.hasActive()) {
        for (auto* w : windows_) {
            w->update();
        }
    }
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

void Application::registerTimer(Timer* timer) {
    if (std::find(timers_.begin(), timers_.end(), timer) == timers_.end())
        timers_.push_back(timer);
}

void Application::unregisterTimer(Timer* timer) {
    auto it = std::find(timers_.begin(), timers_.end(), timer);
    if (it != timers_.end()) {
        timers_.erase(it);
    }
}

} // namespace ltgui
