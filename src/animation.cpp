#include "animation.h"
#include <chrono>
#include <cmath>

namespace ltgui {

static uint64_t steadyNowMs() {
    auto now = std::chrono::steady_clock::now().time_since_epoch();
    return static_cast<uint64_t>(
        std::chrono::duration_cast<std::chrono::milliseconds>(now).count());
}

float easeValue(Easing e, float t) {
    if (t <= 0.0f) return 0.0f;
    if (t >= 1.0f) return 1.0f;

    switch (e) {
    case Easing::Linear:
        return t;
    case Easing::EaseIn:
        return t * t;
    case Easing::EaseOut:
        return t * (2.0f - t);
    case Easing::EaseInOut:
        return t < 0.5f ? 2.0f * t * t : -1.0f + (4.0f - 2.0f * t) * t;
    }
    return t;
}

// --- AnimatedFloat ---

AnimatedFloat::AnimatedFloat(float value)
    : current_(value), target_(value) {}

float AnimatedFloat::value() {
    if (!animating_) return current_;

    uint64_t now = AnimationManager::instance().nowMs();
    uint64_t elapsed = now - startTickMs_;

    if (elapsed >= static_cast<uint64_t>(durationMs_) || durationMs_ <= 0) {
        current_ = target_;
        animating_ = false;
        AnimationManager::instance().onAnimStopped();
        return current_;
    }

    float t = static_cast<float>(elapsed) / static_cast<float>(durationMs_);
    current_ = startValue_ + (target_ - startValue_) * easeValue(easing_, t);
    return current_;
}

void AnimatedFloat::setTarget(float v, int durationMs, Easing e) {
    if (target_ == v && animating_) return;

    uint64_t now = AnimationManager::instance().nowMs();

    if (animating_) {
        // Start from current interpolated position
        uint64_t elapsed = now - startTickMs_;
        if (elapsed < static_cast<uint64_t>(durationMs_)) {
            float t = static_cast<float>(elapsed) / static_cast<float>(durationMs_);
            startValue_ = startValue_ + (target_ - startValue_) * easeValue(easing_, t);
        } else {
            startValue_ = target_;
        }
    } else {
        startValue_ = current_;
    }

    target_ = v;
    durationMs_ = durationMs;
    easing_ = e;
    startTickMs_ = now;

    if (durationMs <= 0) {
        current_ = target_;
        return;
    }

    if (!animating_) {
        animating_ = true;
        AnimationManager::instance().onAnimStarted();
    }
}

void AnimatedFloat::setImmediate(float v) {
    current_ = v;
    target_ = v;
    if (animating_) {
        animating_ = false;
        AnimationManager::instance().onAnimStopped();
    }
}

void AnimatedFloat::complete() {
    if (animating_) {
        current_ = target_;
        animating_ = false;
        AnimationManager::instance().onAnimStopped();
    }
}

// --- AnimationManager ---

AnimationManager& AnimationManager::instance() {
    static AnimationManager mgr;
    return mgr;
}

void AnimationManager::tick() {
    nowMs_ = steadyNowMs();
}

void AnimationManager::onAnimStarted() {
    activeCount_++;
}

void AnimationManager::onAnimStopped() {
    if (activeCount_ > 0) activeCount_--;
}

} // namespace ltgui
