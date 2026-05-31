#pragma once
#include <cstdint>
#include <vector>

namespace ltgui {

class Window;

enum class Easing {
    Linear,
    EaseIn,
    EaseOut,
    EaseInOut
};

float easeValue(Easing e, float t);

class AnimatedFloat {
public:
    AnimatedFloat() = default;
    explicit AnimatedFloat(float value);

    float value();
    float target() const { return target_; }

    void setTarget(float v, int durationMs = 200, Easing e = Easing::EaseOut);
    void setImmediate(float v);
    bool isAnimating() const { return animating_; }
    void complete();

private:
    float current_ = 0.0f;
    float startValue_ = 0.0f;
    float target_ = 0.0f;
    int durationMs_ = 200;
    Easing easing_ = Easing::EaseOut;
    uint64_t startTickMs_ = 0;
    bool animating_ = false;
};

class AnimationManager {
public:
    static AnimationManager& instance();

    void tick();
    bool hasActive() const { return activeCount_ > 0; }
    uint64_t nowMs() const { return nowMs_; }

    void onAnimStarted();
    void onAnimStopped();

private:
    int activeCount_ = 0;
    uint64_t nowMs_ = 0;
};

} // namespace ltgui
