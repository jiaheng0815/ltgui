#include "animation.h"
#include <chrono>
#include <cmath>
#include <algorithm>

#ifndef M_PI
#define M_PI 3.14159265358979323846f
#endif

namespace ltgui {

static uint64_t steadyNowMs() {
    auto now = std::chrono::steady_clock::now().time_since_epoch();
    return static_cast<uint64_t>(
        std::chrono::duration_cast<std::chrono::milliseconds>(now).count());
}

// --- Easing functions (Robert Penner) ---

static inline float easeInQuad(float t)   { return t * t; }
static inline float easeOutQuad(float t)  { return t * (2.0f - t); }
static inline float easeInOutQuad(float t) {
    return t < 0.5f ? 2.0f * t * t : -1.0f + (4.0f - 2.0f * t) * t;
}
static inline float easeInCubic(float t)  { return t * t * t; }
static inline float easeOutCubic(float t) {
    float t1 = t - 1.0f; return t1 * t1 * t1 + 1.0f;
}
static inline float easeInOutCubic(float t) {
    return t < 0.5f ? 4.0f * t * t * t : (t - 1.0f) * (2.0f * t - 2.0f) * (2.0f * t - 2.0f) + 1.0f;
}
static inline float easeInQuart(float t)  { return t * t * t * t; }
static inline float easeOutQuart(float t) {
    float t1 = t - 1.0f; return 1.0f - t1 * t1 * t1 * t1;
}
static inline float easeInOutQuart(float t) {
    return t < 0.5f ? 8.0f * t * t * t * t : 1.0f - std::pow(-2.0f * t + 2.0f, 4.0f) / 2.0f;
}
static inline float easeInQuint(float t)  { return t * t * t * t * t; }
static inline float easeOutQuint(float t) {
    float t1 = t - 1.0f; return 1.0f + t1 * t1 * t1 * t1 * t1;
}
static inline float easeInOutQuint(float t) {
    return t < 0.5f ? 16.0f * t * t * t * t * t : 1.0f - std::pow(-2.0f * t + 2.0f, 5.0f) / 2.0f;
}
static inline float easeInSine(float t)   { return 1.0f - std::cos(t * M_PI / 2.0f); }
static inline float easeOutSine(float t)  { return std::sin(t * M_PI / 2.0f); }
static inline float easeInOutSine(float t){ return -(std::cos(M_PI * t) - 1.0f) / 2.0f; }
static inline float easeInExpo(float t)   { return t <= 0.0f ? 0.0f : std::pow(2.0f, 10.0f * t - 10.0f); }
static inline float easeOutExpo(float t)  { return t >= 1.0f ? 1.0f : 1.0f - std::pow(2.0f, -10.0f * t); }
static inline float easeInOutExpo(float t) {
    return t <= 0.0f ? 0.0f : t >= 1.0f ? 1.0f :
           t < 0.5f ? std::pow(2.0f, 20.0f * t - 10.0f) / 2.0f
                    : (2.0f - std::pow(2.0f, -20.0f * t + 10.0f)) / 2.0f;
}
static inline float easeInCirc(float t)   { return 1.0f - std::sqrt(1.0f - t * t); }
static inline float easeOutCirc(float t)  { return std::sqrt(1.0f - (t - 1.0f) * (t - 1.0f)); }
static inline float easeInOutCirc(float t) {
    return t < 0.5f ? (1.0f - std::sqrt(1.0f - 4.0f * t * t)) / 2.0f
                    : (std::sqrt(1.0f - std::pow(-2.0f * t + 2.0f, 2.0f)) + 1.0f) / 2.0f;
}
static inline float easeInBack(float t)   {
    const float c1 = 1.70158f; return (c1 + 1.0f) * t * t * t - c1 * t * t;
}
static inline float easeOutBack(float t)  {
    const float c1 = 1.70158f; float t1 = t - 1.0f;
    return 1.0f + (c1 + 1.0f) * t1 * t1 * t1 + c1 * t1 * t1;
}
static inline float easeInOutBack(float t) {
    const float c1 = 1.70158f, c2 = c1 * 1.525f;
    return t < 0.5f ? (4.0f * t * t * ((c2 + 1.0f) * 2.0f * t - c2)) / 2.0f
                    : (std::pow(2.0f * t - 2.0f, 2.0f) * ((c2 + 1.0f) * (t * 2.0f - 2.0f) + c2) + 2.0f) / 2.0f;
}
static inline float easeInElastic(float t) {
    const float c4 = 2.0943951023931953f; // 2*PI/3
    return t <= 0.0f ? 0.0f : t >= 1.0f ? 1.0f
         : -std::pow(2.0f, 10.0f * t - 10.0f) * std::sin((t * 10.0f - 10.75f) * c4);
}
static inline float easeOutElastic(float t) {
    const float c4 = 2.0943951023931953f;
    return t <= 0.0f ? 0.0f : t >= 1.0f ? 1.0f
         : std::pow(2.0f, -10.0f * t) * std::sin((t * 10.0f - 0.75f) * c4) + 1.0f;
}
static inline float easeInOutElastic(float t) {
    const float c5 = 1.3962634015954636f; // 2*PI/4.5
    return t <= 0.0f ? 0.0f : t >= 1.0f ? 1.0f
         : t < 0.5f ? -(std::pow(2.0f, 20.0f * t - 10.0f) * std::sin((20.0f * t - 11.125f) * c5)) / 2.0f
                    :  (std::pow(2.0f, -20.0f * t + 10.0f) * std::sin((20.0f * t - 11.125f) * c5)) / 2.0f + 1.0f;
}
static inline float easeOutBounce(float t) {
    const float n1 = 7.5625f, d1 = 2.75f;
    if (t < 1.0f / d1)       return n1 * t * t;
    if (t < 2.0f / d1)       { t -= 1.5f / d1;  return n1 * t * t + 0.75f; }
    if (t < 2.5f / d1)       { t -= 2.25f / d1; return n1 * t * t + 0.9375f; }
    t -= 2.625f / d1; return n1 * t * t + 0.984375f;
}
static inline float easeInBounce(float t)  { return 1.0f - easeOutBounce(1.0f - t); }
static inline float easeInOutBounce(float t) {
    return t < 0.5f ? (1.0f - easeOutBounce(1.0f - 2.0f * t)) / 2.0f
                    : (1.0f + easeOutBounce(2.0f * t - 1.0f)) / 2.0f;
}

float easeValue(Easing e, float t) {
    if (t <= 0.0f) return 0.0f;
    if (t >= 1.0f) return 1.0f;

    switch (e) {
    case Easing::Linear:          return t;
    case Easing::EaseIn:          return t * t;
    case Easing::EaseOut:         return t * (2.0f - t);
    case Easing::EaseInOut:       return t < 0.5f ? 2.0f * t * t : -1.0f + (4.0f - 2.0f * t) * t;
    case Easing::EaseInQuad:      return easeInQuad(t);
    case Easing::EaseOutQuad:     return easeOutQuad(t);
    case Easing::EaseInOutQuad:   return easeInOutQuad(t);
    case Easing::EaseInCubic:     return easeInCubic(t);
    case Easing::EaseOutCubic:    return easeOutCubic(t);
    case Easing::EaseInOutCubic:  return easeInOutCubic(t);
    case Easing::EaseInQuart:     return easeInQuart(t);
    case Easing::EaseOutQuart:    return easeOutQuart(t);
    case Easing::EaseInOutQuart:  return easeInOutQuart(t);
    case Easing::EaseInQuint:     return easeInQuint(t);
    case Easing::EaseOutQuint:    return easeOutQuint(t);
    case Easing::EaseInOutQuint:  return easeInOutQuint(t);
    case Easing::EaseInSine:      return easeInSine(t);
    case Easing::EaseOutSine:     return easeOutSine(t);
    case Easing::EaseInOutSine:   return easeInOutSine(t);
    case Easing::EaseInExpo:      return easeInExpo(t);
    case Easing::EaseOutExpo:     return easeOutExpo(t);
    case Easing::EaseInOutExpo:   return easeInOutExpo(t);
    case Easing::EaseInCirc:      return easeInCirc(t);
    case Easing::EaseOutCirc:     return easeOutCirc(t);
    case Easing::EaseInOutCirc:   return easeInOutCirc(t);
    case Easing::EaseInBack:      return easeInBack(t);
    case Easing::EaseOutBack:     return easeOutBack(t);
    case Easing::EaseInOutBack:   return easeInOutBack(t);
    case Easing::EaseInElastic:   return easeInElastic(t);
    case Easing::EaseOutElastic:  return easeOutElastic(t);
    case Easing::EaseInOutElastic:return easeInOutElastic(t);
    case Easing::EaseInBounce:    return easeInBounce(t);
    case Easing::EaseOutBounce:   return easeOutBounce(t);
    case Easing::EaseInOutBounce: return easeInOutBounce(t);
    case Easing::StepStart:       return 0.0f;
    case Easing::StepEnd:         return 1.0f;
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
        // Target is always the endpoint of the current segment regardless of
        // yoyo direction — when yoyoDir_ is false, target_ was swapped to
        // the "return" value by the previous completion.
        current_ = target_;
        if (loop_ && (repeatCount_ <= 0 || repeatsDone_ < repeatCount_ - 1)) {
            repeatsDone_++;
            startTickMs_ = now;
            if (yoyo_) {
                yoyoDir_ = !yoyoDir_;
                std::swap(startValue_, target_);
            } else {
                current_ = startValue_;
            }
            return yoyoDir_ ? current_ : startValue_;
        }
        animating_ = false;
        AnimationManager::instance().onAnimStopped();
        // Copy to stack before emitting — onFinished callbacks may destroy
        // this AnimatedFloat (or its owning WidgetAnimation), making `this`
        // dangling.  Returning a local avoids use-after-free.
        float result = current_;
        onFinished.emit();
        return result;
    }

    float t = static_cast<float>(elapsed) / static_cast<float>(durationMs_);
    current_ = startValue_ + (target_ - startValue_) * easeValue(easing_, t);
    return current_;
}

void AnimatedFloat::setTarget(float v, int durationMs, Easing e) {
    // Reject non-finite targets (inf/nan) — they produce garbage animation values
    if (!std::isfinite(v)) return;
    // Only skip when NOTHING changed — target, duration, and easing all match.
    if (target_ == v && durationMs_ == durationMs && easing_ == e && animating_) return;

    repeatsDone_ = 0;
    yoyoDir_ = true;
    uint64_t now = AnimationManager::instance().nowMs();

    if (animating_) {
        uint64_t elapsed = now - startTickMs_;
        if (elapsed < static_cast<uint64_t>(durationMs_)) {
            float t = static_cast<float>(elapsed) / static_cast<float>(durationMs_);
            current_ = startValue_ + (target_ - startValue_) * easeValue(easing_, t);
        } else {
            current_ = target_;
        }
    }
    startValue_ = current_;

    target_ = v;
    durationMs_ = durationMs;
    easing_ = e;
    startTickMs_ = now;

    if (durationMs <= 0 || startValue_ == target_) {
        current_ = target_;
        // If we were previously animating, stop cleanly.  Without this,
        // AnimationManager keeps tracking an animation that will never
        // complete, wasting CPU and preventing the idle-state transition.
        if (animating_) {
            animating_ = false;
            AnimationManager::instance().onAnimStopped();
        }
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
    repeatsDone_ = 0;
    if (animating_) {
        animating_ = false;
        AnimationManager::instance().onAnimStopped();
    }
}

void AnimatedFloat::complete() {
    if (animating_) {
        current_ = target_;
        animating_ = false;
        repeatsDone_ = 0;
        AnimationManager::instance().onAnimStopped();
        onFinished.emit();
    }
}

// --- WidgetAnimation ---

WidgetAnimation::~WidgetAnimation() {
    // Auto-unregister on destruction so AnimationManager doesn't hold
    // a dangling pointer after the animation is destroyed.
    if (playing_) {
        AnimationManager::instance().unregisterAnimation(this);
    }
}

WidgetAnimation::WidgetAnimation(WidgetAnimation&& other) noexcept
    : onFinished(std::move(other.onFinished)),
      anim_(std::move(other.anim_)), durationMs_(other.durationMs_),
      delayMs_(other.delayMs_), easing_(other.easing_),
      loop_(other.loop_), yoyo_(other.yoyo_),
      playing_(other.playing_), startTickMs_(other.startTickMs_),
      startVal_(other.startVal_), endVal_(other.endVal_),
      onValue_(std::move(other.onValue_)),
      delayPhase_(other.delayPhase_) {
    // Transfer registration to the moved-to object
    if (playing_) {
        AnimationManager::instance().unregisterAnimation(&other);
        AnimationManager::instance().registerAnimation(this);
        other.playing_ = false;
    }
}

WidgetAnimation& WidgetAnimation::operator=(WidgetAnimation&& other) noexcept {
    if (this != &other) {
        if (playing_) AnimationManager::instance().unregisterAnimation(this);
        anim_ = std::move(other.anim_);
        durationMs_ = other.durationMs_;
        delayMs_ = other.delayMs_;
        easing_ = other.easing_;
        loop_ = other.loop_;
        yoyo_ = other.yoyo_;
        playing_ = other.playing_;
        startTickMs_ = other.startTickMs_;
        startVal_ = other.startVal_;
        endVal_ = other.endVal_;
        onValue_ = std::move(other.onValue_);
        onFinished = std::move(other.onFinished);
        delayPhase_ = other.delayPhase_;
        if (playing_) {
            AnimationManager::instance().unregisterAnimation(&other);
            AnimationManager::instance().registerAnimation(this);
            other.playing_ = false;
        }
    }
    return *this;
}

void WidgetAnimation::play() {
    if (playing_) return;
    playing_ = true;
    startTickMs_ = AnimationManager::instance().nowMs();
    delayPhase_ = delayMs_ > 0;
    anim_.setImmediate(startVal_);

    if (!delayPhase_) {
        anim_.setTarget(endVal_, durationMs_, easing_);
        anim_.setLoop(loop_);
        anim_.setYoyo(yoyo_);
    }

    AnimationManager::instance().registerAnimation(this);
}

void WidgetAnimation::pause() {
    playing_ = false;
    AnimationManager::instance().unregisterAnimation(this);
}

void WidgetAnimation::stop() {
    if (playing_) {
        playing_ = false;
        AnimationManager::instance().unregisterAnimation(this);
        onFinished.emit();
    }
}

// --- KeyframeAnimation ---

KeyframeAnimation::~KeyframeAnimation() {
    // Auto-unregister on destruction to prevent dangling pointer
    if (playing_) {
        AnimationManager::instance().unregisterKeyframe(this);
    }
}

KeyframeAnimation::KeyframeAnimation(KeyframeAnimation&& other) noexcept
    : onFinished(std::move(other.onFinished)),
      keyframes_(std::move(other.keyframes_)),
      durationMs_(other.durationMs_), loop_(other.loop_),
      playing_(other.playing_), startTickMs_(other.startTickMs_),
      onValue_(std::move(other.onValue_)) {
    if (playing_) {
        AnimationManager::instance().unregisterKeyframe(&other);
        AnimationManager::instance().registerKeyframe(this);
        other.playing_ = false;
    }
}

KeyframeAnimation& KeyframeAnimation::operator=(KeyframeAnimation&& other) noexcept {
    if (this != &other) {
        if (playing_) AnimationManager::instance().unregisterKeyframe(this);
        keyframes_ = std::move(other.keyframes_);
        durationMs_ = other.durationMs_;
        loop_ = other.loop_;
        playing_ = other.playing_;
        startTickMs_ = other.startTickMs_;
        onValue_ = std::move(other.onValue_);
        onFinished = std::move(other.onFinished);
        if (playing_) {
            AnimationManager::instance().unregisterKeyframe(&other);
            AnimationManager::instance().registerKeyframe(this);
            other.playing_ = false;
        }
    }
    return *this;
}

void KeyframeAnimation::addKeyframe(const Keyframe& kf) {
    keyframes_.push_back(kf);
    std::sort(keyframes_.begin(), keyframes_.end(),
              [](const Keyframe& a, const Keyframe& b) { return a.time < b.time; });
}

float KeyframeAnimation::currentValue() const {
    if (keyframes_.empty()) return 0.0f;
    if (keyframes_.size() == 1) return keyframes_[0].value;
    if (durationMs_ <= 0) return keyframes_.back().value;

    uint64_t now = AnimationManager::instance().nowMs();
    uint64_t elapsed = now - startTickMs_;

    float t = static_cast<float>(elapsed) / static_cast<float>(durationMs_);
    if (t >= 1.0f) t = loop_ ? std::fmod(t, 1.0f) : 1.0f;

    return interpolate(t);
}

float KeyframeAnimation::interpolate(float t) const {
    if (keyframes_.empty()) return 0.0f;
    if (t <= keyframes_[0].time) return keyframes_[0].value;
    if (t >= keyframes_.back().time) return keyframes_.back().value;

    for (size_t i = 1; i < keyframes_.size(); i++) {
        if (t <= keyframes_[i].time) {
            float segT = (t - keyframes_[i - 1].time) /
                         (keyframes_[i].time - keyframes_[i - 1].time);
            float eased = easeValue(keyframes_[i].easing, segT);
            return keyframes_[i - 1].value +
                   (keyframes_[i].value - keyframes_[i - 1].value) * eased;
        }
    }
    return keyframes_.back().value;
}

void KeyframeAnimation::play() {
    if (playing_) return;
    playing_ = true;
    startTickMs_ = AnimationManager::instance().nowMs();
    AnimationManager::instance().registerKeyframe(this);
}

void KeyframeAnimation::stop() {
    if (playing_) {
        playing_ = false;
        AnimationManager::instance().unregisterKeyframe(this);
        onFinished.emit();
    }
}

// --- AnimationManager ---

AnimationManager& AnimationManager::instance() {
    static AnimationManager mgr;
    return mgr;
}

void AnimationManager::tick() {
    nowMs_ = steadyNowMs();

    // Tick widget animations.  We iterate by index because a callback
    // (onValue_ or onFinished) may modify widgetAnims_ (via stop() /
    // play()), so a range-for with iterators is not safe.
    for (size_t i = 0; i < widgetAnims_.size(); ) {
        WidgetAnimation* anim = widgetAnims_[i];
        if (!anim->isPlaying()) {
            i++;
            continue;
        }

        uint64_t elapsed = nowMs_ - anim->startTickMs_;
        if (anim->delayPhase_) {
            if (elapsed >= static_cast<uint64_t>(anim->delayMs_)) {
                anim->delayPhase_ = false;
                anim->startTickMs_ = nowMs_;
                anim->anim_.setTarget(anim->endVal_, anim->durationMs_, anim->easing_);
                anim->anim_.setLoop(anim->loop_);
                anim->anim_.setYoyo(anim->yoyo_);
                elapsed = 0;
            } else {
                i++;
                continue;
            }
        }

        float v = anim->anim_.value();
        if (anim->onValue_) anim->onValue_(v);

        // If the callback destroyed this animation (auto-unregisters it
        // via ~WidgetAnimation), anim is now dangling. Verify liveness.
        {
            auto it = std::find(widgetAnims_.begin(), widgetAnims_.end(), anim);
            if (it == widgetAnims_.end()) continue; // destroyed; vector shifted
        }

        if (!anim->anim_.isAnimating() && !anim->delayPhase_) {
            anim->playing_ = false;
            // Shell the completed animation out; if the callback adds/removes
            // animations, our index is safe because we use the stale size
            // and move the element to the end before erasing.
            size_t oldSize = widgetAnims_.size();
            widgetAnims_.erase(
                std::remove(widgetAnims_.begin(), widgetAnims_.end(), anim),
                widgetAnims_.end());
            // If the vector changed (element was removed), don't advance i
            // because the current slot now holds the next element.
            if (widgetAnims_.size() == oldSize) i++;
            // Move the signal to a local before emitting so that even if
            // a connected slot destroys the WidgetAnimation during the
            // emit, the stack-local Signal remains valid and the emit
            // runs to completion safely.
            auto finishedSignal = std::move(anim->onFinished);
            finishedSignal.emit();
        } else {
            i++;
        }
    }

    for (size_t i = 0; i < keyframeAnims_.size(); ) {
        KeyframeAnimation* kf = keyframeAnims_[i];
        if (!kf->isPlaying()) {
            i++;
            continue;
        }
        float v = kf->currentValue();
        if (kf->onValue_) kf->onValue_(v);

        // If the callback destroyed this animation (auto-unregisters it
        // via ~KeyframeAnimation), kf is now dangling. Verify liveness.
        {
            auto it = std::find(keyframeAnims_.begin(), keyframeAnims_.end(), kf);
            if (it == keyframeAnims_.end()) continue; // destroyed; vector shifted
        }

        uint64_t elapsed = nowMs_ - kf->startTickMs_;
        if (elapsed >= static_cast<uint64_t>(kf->durationMs_) && !kf->loop_) {
            kf->playing_ = false;
            size_t oldSize = keyframeAnims_.size();
            keyframeAnims_.erase(
                std::remove(keyframeAnims_.begin(), keyframeAnims_.end(), kf),
                keyframeAnims_.end());
            if (keyframeAnims_.size() == oldSize) i++;
            // Move the signal to a local before emitting so that even if
            // a connected slot destroys the KeyframeAnimation during the
            // emit, the stack-local Signal remains valid.
            auto finishedSignal = std::move(kf->onFinished);
            finishedSignal.emit();
        } else {
            i++;
        }
    }
}

void AnimationManager::registerAnimation(WidgetAnimation* anim) {
    auto it = std::find(widgetAnims_.begin(), widgetAnims_.end(), anim);
    if (it == widgetAnims_.end()) widgetAnims_.push_back(anim);
}

void AnimationManager::unregisterAnimation(WidgetAnimation* anim) {
    auto it = std::find(widgetAnims_.begin(), widgetAnims_.end(), anim);
    if (it != widgetAnims_.end()) widgetAnims_.erase(it);
}

void AnimationManager::registerKeyframe(KeyframeAnimation* kf) {
    auto it = std::find(keyframeAnims_.begin(), keyframeAnims_.end(), kf);
    if (it == keyframeAnims_.end()) keyframeAnims_.push_back(kf);
}

void AnimationManager::unregisterKeyframe(KeyframeAnimation* kf) {
    auto it = std::find(keyframeAnims_.begin(), keyframeAnims_.end(), kf);
    if (it != keyframeAnims_.end()) keyframeAnims_.erase(it);
}

void AnimationManager::onAnimStarted() {
    activeCount_++;
}

void AnimationManager::onAnimStopped() {
    if (activeCount_ > 0) activeCount_--;
}

// --- AnimatedColor ---

void AnimatedColor::setTarget(const Color& to, int durationMs, Easing e) {
    start_ = current_;
    end_ = to;
    anim_.setStartValue(0.0f);
    anim_.setEndValue(1.0f);
    anim_.setDuration(durationMs);
    anim_.setEasing(e);
    anim_.setValueCallback([this](float t) {
        current_ = Color::lerp(start_, end_, t);
        if (onValue_) onValue_(current_);
    });
    anim_.play();
}

} // namespace ltgui
