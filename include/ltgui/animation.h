#pragma once
#include "color.h"
#include "signal.h"
#include <cstdint>
#include <functional>
#include <vector>

namespace ltgui {

class Widget;

enum class Easing {
  Linear,
  EaseIn,
  EaseOut,
  EaseInOut,
  EaseInQuad,
  EaseOutQuad,
  EaseInOutQuad,
  EaseInCubic,
  EaseOutCubic,
  EaseInOutCubic,
  EaseInQuart,
  EaseOutQuart,
  EaseInOutQuart,
  EaseInQuint,
  EaseOutQuint,
  EaseInOutQuint,
  EaseInSine,
  EaseOutSine,
  EaseInOutSine,
  EaseInExpo,
  EaseOutExpo,
  EaseInOutExpo,
  EaseInCirc,
  EaseOutCirc,
  EaseInOutCirc,
  EaseInBack,
  EaseOutBack,
  EaseInOutBack,
  EaseInElastic,
  EaseOutElastic,
  EaseInOutElastic,
  EaseInBounce,
  EaseOutBounce,
  EaseInOutBounce,
  StepStart,
  StepEnd
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

  void setLoop(bool loop) { loop_ = loop; }
  void setRepeatCount(int n) { repeatCount_ = n; }
  void setYoyo(bool yoyo) { yoyo_ = yoyo; }

  Signal<> onFinished;

private:
  float current_ = 0.0f;
  float startValue_ = 0.0f;
  float target_ = 0.0f;
  int durationMs_ = 200;
  Easing easing_ = Easing::EaseOut;
  uint64_t startTickMs_ = 0;
  bool animating_ = false;
  bool loop_ = false;
  int repeatCount_ = 0;
  int repeatsDone_ = 0;
  bool yoyo_ = false;
  bool yoyoDir_ = true;
};

class WidgetAnimation {
public:
  using ValueCallback = std::function<void(float)>;

  WidgetAnimation() = default;
  ~WidgetAnimation();

  // Non-copyable (AnimationManager tracks by raw pointer)
  WidgetAnimation(const WidgetAnimation &) = delete;
  WidgetAnimation &operator=(const WidgetAnimation &) = delete;
  WidgetAnimation(WidgetAnimation &&other) noexcept;
  WidgetAnimation &operator=(WidgetAnimation &&other) noexcept;

  void setDuration(int ms) { durationMs_ = ms; }
  void setEasing(Easing e) { easing_ = e; }
  void setDelay(int ms) { delayMs_ = ms; }
  void setLoop(bool loop) { loop_ = loop; }
  void setYoyo(bool yoyo) { yoyo_ = yoyo; }

  void setStartValue(float v) { startVal_ = v; }
  void setEndValue(float v) { endVal_ = v; }
  void setValueCallback(ValueCallback cb) { onValue_ = std::move(cb); }

  float currentValue() { return anim_.value(); }
  bool isPlaying() const { return playing_; }

  void play();
  void pause();
  void stop();
  Signal<> onFinished;

private:
  friend class AnimationManager;
  AnimatedFloat anim_;
  int durationMs_ = 200;
  int delayMs_ = 0;
  Easing easing_ = Easing::EaseOut;
  bool loop_ = false;
  bool yoyo_ = false;
  bool playing_ = false;
  uint64_t startTickMs_ = 0;
  float startVal_ = 0.0f;
  float endVal_ = 1.0f;
  ValueCallback onValue_;
  bool delayPhase_ = false;
};

// Animates a color toward a target over time (hover transitions, etc.).
// Backed by WidgetAnimation, so it is driven by the AnimationManager tick.
class AnimatedColor {
public:
  AnimatedColor() = default;
  ~AnimatedColor() = default;

  // Non-copyable (wraps WidgetAnimation, which AnimationManager tracks
  // by raw pointer).
  AnimatedColor(const AnimatedColor &) = delete;
  AnimatedColor &operator=(const AnimatedColor &) = delete;
  AnimatedColor(AnimatedColor &&) = delete;
  AnimatedColor &operator=(AnimatedColor &&) = delete;

  // Animate from the current value to `to` over durationMs.
  void setTarget(const Color &to, int durationMs = 200,
                 Easing e = Easing::EaseOut);

  // Jump immediately to a value (no animation).
  void setImmediate(const Color &value) {
    // Stop any in-flight animation first so the next tick cannot overwrite
    // this value with a stale interpolation (B2-05).
    if (anim_.isPlaying())
      anim_.stop();
    current_ = value;
    if (onValue_)
      onValue_(current_);
  }

  Color current() const { return current_; }
  bool isAnimating() const { return anim_.isPlaying(); }

  // Invoked on every animation tick with the interpolated color.
  void setValueCallback(std::function<void(const Color &)> cb) {
    onValue_ = std::move(cb);
  }

private:
  WidgetAnimation anim_;
  Color current_ = Color::Black;
  Color start_ = Color::Black;
  Color end_ = Color::Black;
  std::function<void(const Color &)> onValue_;
};

struct Keyframe {
  float time = 0.0f;
  float value = 0.0f;
  Easing easing = Easing::Linear;
};

class KeyframeAnimation {
public:
  using ValueCallback = std::function<void(float)>;

  KeyframeAnimation() = default;
  ~KeyframeAnimation();

  // Non-copyable
  KeyframeAnimation(const KeyframeAnimation &) = delete;
  KeyframeAnimation &operator=(const KeyframeAnimation &) = delete;
  KeyframeAnimation(KeyframeAnimation &&other) noexcept;
  KeyframeAnimation &operator=(KeyframeAnimation &&other) noexcept;

  void addKeyframe(const Keyframe &kf);
  void setDuration(int ms) {
    if (ms > 0)
      durationMs_ = ms;
  }
  void setLoop(bool loop) { loop_ = loop; }
  void setValueCallback(ValueCallback cb) { onValue_ = std::move(cb); }

  float currentValue() const;
  bool isPlaying() const { return playing_; }

  void play();
  void stop();
  Signal<> onFinished;

private:
  friend class AnimationManager;
  float interpolate(float t) const;

  std::vector<Keyframe> keyframes_;
  int durationMs_ = 1000;
  bool loop_ = false;
  bool playing_ = false;
  uint64_t startTickMs_ = 0;
  ValueCallback onValue_;
};

class AnimationManager {
public:
  static AnimationManager &instance();

  void tick();
  bool hasActive() const { return activeCount_ > 0; }
  uint64_t nowMs() const { return nowMs_; }

  void registerAnimation(WidgetAnimation *anim);
  void unregisterAnimation(WidgetAnimation *anim);
  void registerKeyframe(KeyframeAnimation *kf);
  void unregisterKeyframe(KeyframeAnimation *kf);

  void onAnimStarted();
  void onAnimStopped();

private:
  int activeCount_ = 0;
  uint64_t nowMs_ = 0;
  std::vector<WidgetAnimation *> widgetAnims_;
  std::vector<KeyframeAnimation *> keyframeAnims_;
};

} // namespace ltgui
