#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "animation.h"
#include "doctest/doctest.h"
#include <cmath>
#include <limits>

using namespace ltgui;

// ============================================================
// 刁钻角度：动画系统的边界和恶意输入
// ============================================================

TEST_CASE("AnimatedFloat edge: zero and negative durations") {
  SUBCASE("duration zero acts like setImmediate") {
    AnimatedFloat af(10.0f);
    af.setTarget(50.0f, 0);
    CHECK(af.value() == doctest::Approx(50.0f));
  }

  SUBCASE("negative duration acts like setImmediate") {
    AnimatedFloat af(10.0f);
    af.setTarget(50.0f, -100);
    // Negative duration → immediately at target
    CHECK(af.value() == doctest::Approx(50.0f));
  }

  SUBCASE("very large duration still works") {
    AnimatedFloat af(0.0f);
    af.setTarget(100.0f, 999999);
    float v = af.value();
    CHECK(v >= 0.0f);
    CHECK(v <= 100.0f);
  }
}

TEST_CASE("AnimatedFloat edge: extreme values") {
  SUBCASE("very large target") {
    AnimatedFloat af(0.0f);
    af.setTarget(1e10f, 1000);
    float v = af.value();
    CHECK(v >= 0.0f);
    CHECK(v <= 1e10f);
  }

  SUBCASE("very small target") {
    AnimatedFloat af(100.0f);
    af.setTarget(-1e10f, 1000);
    float v = af.value();
    CHECK(v <= 100.0f);
  }

  SUBCASE("NaN target — should not explode") {
    AnimatedFloat af(0.0f);
    float nan = std::numeric_limits<float>::quiet_NaN();
    af.setTarget(nan, 100);
    // Just verify no crash; value may be NaN but that's expected
    // (framework doesn't guard against NaN)
  }

  SUBCASE("Inf target") {
    AnimatedFloat af(0.0f);
    float inf = std::numeric_limits<float>::infinity();
    af.setTarget(inf, 100);
    // Infinity targets are rejected — value stays at initial
    CHECK(af.value() == 0.0f);
  }
}

TEST_CASE("AnimatedFloat edge: rapid re-targeting") {
  SUBCASE("many rapid setTarget calls") {
    AnimatedFloat af(0.0f);
    for (int i = 0; i < 100; i++) {
      af.setTarget(static_cast<float>(i * 10), 100);
    }
    float v = af.value();
    // Should be somewhere in the range, not crashed
    CHECK(v >= 0.0f);
    CHECK(v <= 1000.0f);
  }

  SUBCASE("setTarget then immediately complete") {
    AnimatedFloat af(0.0f);
    af.setTarget(42.0f, 1000);
    af.complete();
    CHECK(af.value() == doctest::Approx(42.0f));
    // complete() should leave animating=false
    af.complete(); // double-complete should be safe
    CHECK(af.value() == doctest::Approx(42.0f));
  }

  SUBCASE("setImmediate then setTarget") {
    AnimatedFloat af(0.0f);
    af.setImmediate(10.0f);
    af.setTarget(100.0f, 500);
    float v = af.value();
    CHECK(v >= 10.0f); // somewhere between 10 and 100
  }
}

TEST_CASE("Easing edge: out of range t values") {
  SUBCASE("t < 0 clamps to 0") {
    CHECK(easeValue(Easing::EaseIn, -100.0f) == doctest::Approx(0.0f));
  }

  SUBCASE("t > 1 clamps to 1") {
    CHECK(easeValue(Easing::EaseOut, 100.0f) == doctest::Approx(1.0f));
  }

  SUBCASE("all easings return 0 at t=0") {
    CHECK(easeValue(Easing::Linear, 0.0f) == doctest::Approx(0.0f));
    CHECK(easeValue(Easing::EaseIn, 0.0f) == doctest::Approx(0.0f));
    CHECK(easeValue(Easing::EaseOut, 0.0f) == doctest::Approx(0.0f));
    CHECK(easeValue(Easing::EaseInOut, 0.0f) == doctest::Approx(0.0f));
  }

  SUBCASE("all easings return 1 at t=1") {
    CHECK(easeValue(Easing::Linear, 1.0f) == doctest::Approx(1.0f));
    CHECK(easeValue(Easing::EaseIn, 1.0f) == doctest::Approx(1.0f));
    CHECK(easeValue(Easing::EaseOut, 1.0f) == doctest::Approx(1.0f));
    CHECK(easeValue(Easing::EaseInOut, 1.0f) == doctest::Approx(1.0f));
  }
}

TEST_CASE("AnimatedFloat edge: concurrent start/stop") {
  SUBCASE("setImmediate while animating") {
    AnimatedFloat af(0.0f);
    af.setTarget(100.0f, 500);
    af.setImmediate(50.0f);
    // After setImmediate, current_ and target_ both = 50, animating_ = false
    CHECK(af.value() == doctest::Approx(50.0f));
  }

  SUBCASE("setTarget to same value while animating") {
    AnimatedFloat af(0.0f);
    af.setTarget(100.0f, 500);
    af.setTarget(100.0f, 500);
    // Second call sees target_ == v && animating_, returns early
    float v = af.value();
    CHECK(v >= 0.0f);
  }
}
