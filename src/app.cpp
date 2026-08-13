#include "app.h"
#include "animation.h"
#include "log.h"
#include "timer.h"
#include "window.h"

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

Application &Application::instance() {
  static Application app;
  return app;
}

bool Application::pumpPlatformEvents(int timeoutMs) {
#ifdef LTGUI_PLATFORM_WINDOWS
  MSG msg;
  DWORD timeout = (timeoutMs < 0) ? INFINITE : (DWORD)timeoutMs;
  DWORD result =
      MsgWaitForMultipleObjects(0, nullptr, FALSE, timeout, QS_ALLINPUT);
  if (result == WAIT_OBJECT_0) {
    while (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE)) {
      if (msg.message == WM_QUIT) {
        running_ = false;
        return false;
      }
      TranslateMessage(&msg);
      DispatchMessage(&msg);
    }
  }
  return running_;
#elif defined(LTGUI_PLATFORM_LINUX)
  X11Window::processAllPending();
  int x11Fd = X11Window::displayFd();
  if (x11Fd >= 0 && timeoutMs != 0) {
    fd_set fds;
    FD_ZERO(&fds);
    FD_SET(x11Fd, &fds);
    struct timeval tv;
    tv.tv_sec = timeoutMs < 0 ? 60 : timeoutMs / 1000;
    tv.tv_usec = timeoutMs < 0 ? 0 : (timeoutMs % 1000) * 1000;
    select(x11Fd + 1, &fds, nullptr, nullptr, &tv);
    X11Window::processAllPending();
  } else if (timeoutMs > 0) {
    usleep(timeoutMs * 1000);
  }
  return running_;
#elif defined(LTGUI_PLATFORM_MACOS)
  double interval =
      (timeoutMs <= 0) ? 0.0 : (timeoutMs < 0 ? 60.0 : timeoutMs / 1000.0);
  @autoreleasepool {
    NSEvent *event = [NSApp
        nextEventMatchingMask:NSEventMaskAny
                    untilDate:[NSDate dateWithTimeIntervalSinceNow:interval]
                       inMode:NSDefaultRunLoopMode
                      dequeue:YES];
    if (event) {
      [NSApp sendEvent:event];
    }
  }
  return running_;
#endif
}

int Application::run() {
  setMainThread(); // record the UI thread for debug assertions
  running_ = true;

  // Helper: compute the wakeup timeout in milliseconds.
  auto computeWakeupMs = [this]() -> int {
    auto &anim = AnimationManager::instance();
    bool hasAnim = anim.hasActive();
    int animTimeout = hasAnim ? 16 : 500;
    int64_t timerWakeup = nextTimerWakeupMs();
    if (timerWakeup == INT64_MAX)
      return animTimeout;
    if (timerWakeup <= 0)
      return 0;
    if (timerWakeup < animTimeout)
      return (int)timerWakeup;
    return animTimeout;
  };

#ifdef LTGUI_PLATFORM_MACOS
  [NSApplication sharedApplication];
  [NSApp setActivationPolicy:NSApplicationActivationPolicyRegular];
  [NSApp activateIgnoringOtherApps:YES];
  [NSApp finishLaunching];
#endif

  while (running_) {
    if (!pumpPlatformEvents(computeWakeupMs()))
      break;
    processEvents();
  }

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
  auto &anim = AnimationManager::instance();
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
  for (auto *t : timersSnapshot) {
    // Guard: only tick if still registered and active
    if (std::find(timers_.begin(), timers_.end(), t) != timers_.end() &&
        t->isActive()) {
      t->tick(now);
    }
  }

  if (anim.hasActive()) {
    // Snapshot the window list — w->update() may trigger window
    // destruction during iteration (e.g. closing a dialog), which
    // would invalidate iterators into windows_.
    auto windowsSnapshot = windows_;
    for (auto *w : windowsSnapshot) {
      // Only update if still registered (not destroyed mid-iteration)
      if (std::find(windows_.begin(), windows_.end(), w) != windows_.end()) {
        w->update();
      }
    }
  }
}

void Application::registerWindow(Window *window) { windows_.push_back(window); }

void Application::unregisterWindow(Window *window) {
  auto it = std::find(windows_.begin(), windows_.end(), window);
  if (it != windows_.end()) {
    windows_.erase(it);
  }
}

void Application::closeWindow(Window *window) {
  if (!window)
    return;
  LOG_DEBUG("App", "closeWindow: destroying native window (windows_.size=%zu)",
            windows_.size());
  window->close();
  LOG_DEBUG("App", "closeWindow: native window destroyed, unregistering");
  // Remove from the window list so the last-window check can trigger quit().
  // The Window destructor also calls unregisterWindow — this is idempotent.
  unregisterWindow(window);
  LOG_DEBUG("App", "closeWindow: unregistered, windows_.size=%zu",
            windows_.size());
  // If it was the last window, quit the application
  if (windows_.empty()) {
    LOG_DEBUG("App", "closeWindow: last window closed, calling quit()");
    quit();
  }
}

void Application::registerTimer(Timer *timer) {
  if (std::find(timers_.begin(), timers_.end(), timer) == timers_.end())
    timers_.push_back(timer);
}

void Application::unregisterTimer(Timer *timer) {
  auto it = std::find(timers_.begin(), timers_.end(), timer);
  if (it != timers_.end()) {
    timers_.erase(it);
  }
}

int64_t Application::nextTimerWakeupMs() const {
  if (timers_.empty())
    return INT64_MAX;

  uint64_t now = AnimationManager::instance().nowMs();
  int64_t best = INT64_MAX;

  for (const auto *t : timers_) {
    if (!t->isActive())
      continue;
    // nextFireMs_ may be UINT64_MAX if not yet initialised (first tick pending)
    if (t->nextFireMs_ == UINT64_MAX)
      continue;
    if (t->nextFireMs_ <= now)
      return 0; // timer is due right now
    int64_t remaining = (int64_t)(t->nextFireMs_ - now);
    if (remaining < best)
      best = remaining;
  }
  return best;
}

bool Application::tick(int timeoutMs) {
  if (!running_)
    return false;
  pumpPlatformEvents(timeoutMs);
  if (running_)
    processEvents();
  return running_;
}

} // namespace ltgui
