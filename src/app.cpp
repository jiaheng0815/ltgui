#include "app.h"
#include "window.h"
#include "timer.h"
#include "animation.h"

#ifdef LTGUI_PLATFORM_WINDOWS
#ifndef NOMINMAX
#define NOMINMAX
#endif
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
    setMainThread(); // record the UI thread for debug assertions
    running_ = true;

    // Helper: compute the wakeup timeout in milliseconds.
    // Returns the minimum of the animation frame interval and the next
    // timer expiry, clamped to reasonable bounds.
    auto computeWakeupMs = [this]() -> int {
        auto& anim = AnimationManager::instance();
        bool hasAnim = anim.hasActive();

        // Animation frame interval: ~16ms (60 FPS) during animations
        int animTimeout = hasAnim ? 16 : 500;

        int64_t timerWakeup = nextTimerWakeupMs();
        if (timerWakeup == INT64_MAX) {
            // No timers — use animation timeout
            return animTimeout;
        }

        // Clamp timer wakeup: at least 0, at most animTimeout
        // (we wake up at animation rate anyway; timers can fire a frame late)
        if (timerWakeup <= 0) return 0;
        if (timerWakeup < animTimeout) return (int)timerWakeup;
        return animTimeout;
    };

#ifdef LTGUI_PLATFORM_WINDOWS
    MSG msg;
    while (running_) {
        int wakeMs = computeWakeupMs();
        DWORD timeout = (wakeMs <= 0) ? 0 : (DWORD)wakeMs;

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
    int x11Fd = X11Window::displayFd();
    while (running_) {
        X11Window::processAllPending();
        processEvents();

        int wakeMs = computeWakeupMs();

        // Block on X11 connection fd until events arrive, or timeout
        if (x11Fd >= 0) {
            fd_set fds;
            FD_ZERO(&fds);
            FD_SET(x11Fd, &fds);
            struct timeval tv;
            tv.tv_sec = wakeMs / 1000;
            tv.tv_usec = (wakeMs % 1000) * 1000;
            select(x11Fd + 1, &fds, nullptr, nullptr, &tv);
        } else {
            usleep(wakeMs * 1000);
        }
    }
#elif defined(LTGUI_PLATFORM_MACOS)
    [NSApplication sharedApplication];
    [NSApp setActivationPolicy:NSApplicationActivationPolicyRegular];
    [NSApp activateIgnoringOtherApps:YES];
    [NSApp finishLaunching];

    while (running_) {
        int wakeMs = computeWakeupMs();
        // Use at least 1ms to avoid busy-wait, at most 500ms to stay responsive
        double interval = (wakeMs <= 0) ? 0.001 : (wakeMs / 1000.0);

        @autoreleasepool {
            NSEvent* event = [NSApp nextEventMatchingMask:NSEventMaskAny
                                untilDate:[NSDate dateWithTimeIntervalSinceNow:interval]
                                   inMode:NSDefaultRunLoopMode
                                  dequeue:YES];
            if (event) {
                [NSApp sendEvent:event];
            }
        }
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

    // Tick timers safely against mutations during callbacks.
    // We copy the timer list because a timer callback can:
    //   1. stop() / destroy itself (removes from timers_)
    //   2. start new timers (adds to timers_)
    //   3. destroy another timer (removes from timers_, dangling snapshot ptr)
    // All of these would invalidate iterators or skip entries if we
    // iterated the live vector directly.
    //
    // The snapshot prevents double-tick from re-entrant vector mutations,
    // but we must ALSO verify each pointer is still registered in the live
    // timers_ vector before dereferencing — a callback may have destroyed
    // a different Timer object, leaving a dangling pointer in the snapshot.
    auto timersSnapshot = timers_;
    for (auto* t : timersSnapshot) {
        // Guard: only tick if still registered and active
        if (std::find(timers_.begin(), timers_.end(), t) != timers_.end() && t->isActive()) {
            t->tick(now);
        }
    }

    if (anim.hasActive()) {
        // Snapshot the window list — w->update() may trigger window
        // destruction during iteration (e.g. closing a dialog), which
        // would invalidate iterators into windows_.
        auto windowsSnapshot = windows_;
        for (auto* w : windowsSnapshot) {
            // Only update if still registered (not destroyed mid-iteration)
            if (std::find(windows_.begin(), windows_.end(), w) != windows_.end()) {
                w->update();
            }
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

void Application::closeWindow(Window* window) {
    if (!window) return;
    // Destroy the window
    window->close();
    // If it was the last window, quit the application
    if (windows_.empty()) {
        quit();
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

int64_t Application::nextTimerWakeupMs() const {
    if (timers_.empty()) return INT64_MAX;

    uint64_t now = AnimationManager::instance().nowMs();
    int64_t best = INT64_MAX;

    for (const auto* t : timers_) {
        if (!t->isActive()) continue;
        // nextFireMs_ may be UINT64_MAX if not yet initialised (first tick pending)
        if (t->nextFireMs_ == UINT64_MAX) continue;
        if (t->nextFireMs_ <= now) return 0; // timer is due right now
        int64_t remaining = (int64_t)(t->nextFireMs_ - now);
        if (remaining < best) best = remaining;
    }
    return best;
}

} // namespace ltgui
