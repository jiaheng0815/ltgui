#include "timer.h"
#include "app.h"
#include <algorithm>

namespace ltgui {

int Timer::nextId_ = 0;

void Timer::start(int ms, bool repeating, Callback cb) {
    stop();
    if (ms <= 0 || !cb) return;

    id_ = nextId_++;
    cb_ = std::move(cb);
    interval_ = ms;
    repeating_ = repeating;
    nextFireMs_ = 0; // set on first tick

    Application::instance().registerTimer(this);
}

void Timer::stop() {
    if (id_ >= 0) {
        Application::instance().unregisterTimer(this);
        id_ = -1;
        cb_ = nullptr;
    }
}

bool Timer::tick(uint64_t nowMs) {
    if (id_ < 0 || !cb_) return false;

    // Lazy-init nextFireMs on first tick
    if (nextFireMs_ == 0) {
        nextFireMs_ = nowMs + interval_;
        return false;
    }

    if (nowMs >= nextFireMs_) {
        cb_();
        if (repeating_) {
            nextFireMs_ = nowMs + interval_;
        } else {
            stop();
        }
        return true;
    }
    return false;
}

} // namespace ltgui
