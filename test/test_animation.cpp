#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "animation.h"
#include "doctest/doctest.h"
#include <cmath>

using namespace ltgui;

TEST_CASE("Easing functions") {
  SUBCASE("Linear at boundaries") {
    CHECK(easeValue(Easing::Linear, 0.0f) == doctest::Approx(0.0f));
    CHECK(easeValue(Easing::Linear, 1.0f) == doctest::Approx(1.0f));
  }
  SUBCASE("Linear midpoint") {
    CHECK(easeValue(Easing::Linear, 0.5f) == doctest::Approx(0.5f));
  }
  SUBCASE("EaseIn accelerates") {
    // EaseIn: t*t, at t=0.5 → 0.25, which is less than linear
    CHECK(easeValue(Easing::EaseIn, 0.5f) == doctest::Approx(0.25f));
  }
  SUBCASE("EaseOut decelerates") {
    // EaseOut: t*(2-t), at t=0.5 → 0.5*1.5 = 0.75
    CHECK(easeValue(Easing::EaseOut, 0.5f) == doctest::Approx(0.75f));
  }
  SUBCASE("EaseInOut midpoint is 0.5") {
    CHECK(easeValue(Easing::EaseInOut, 0.5f) == doctest::Approx(0.5f));
  }
  SUBCASE("EaseInOut slower at start") {
    // At t=0.25: 2*0.25*0.25 = 0.125
    CHECK(easeValue(Easing::EaseInOut, 0.25f) == doctest::Approx(0.125f));
  }
  SUBCASE("Clamp out of bounds") {
    CHECK(easeValue(Easing::Linear, -0.5f) == doctest::Approx(0.0f));
    CHECK(easeValue(Easing::Linear, 1.5f) == doctest::Approx(1.0f));
  }
}

TEST_CASE("AnimatedFloat immediate value") {
  SUBCASE("initial value matches constructor") {
    AnimatedFloat af(42.0f);
    CHECK(af.value() == doctest::Approx(42.0f));
  }

  SUBCASE("setImmediate changes value instantly") {
    AnimatedFloat af(0.0f);
    af.setImmediate(99.0f);
    CHECK(af.value() == doctest::Approx(99.0f));
  }

  SUBCASE("setTarget with zero duration is immediate") {
    AnimatedFloat af(10.0f);
    af.setTarget(50.0f, 0);
    CHECK(af.value() == doctest::Approx(50.0f));
  }

  SUBCASE("complete finishes animation") {
    AnimatedFloat af(0.0f);
    af.setTarget(100.0f, 500);
    af.complete();
    CHECK(af.value() == doctest::Approx(100.0f));
  }
}

TEST_CASE("AnimatedFloat value interpolation over time") {
  SUBCASE("value advances toward target") {
    // This test depends on AnimationManager::tick() being called.
    // Without a real clock, verify the API contract:
    // - After setTarget with duration > 0, value() should NOT jump
    // - value() should eventually reach target
    AnimatedFloat af(0.0f);
    af.setTarget(100.0f, 1000);
    // Immediately after: should NOT have jumped (animation just started)
    float v = af.value();
    // v should be close to 0 (or exactly the start value if clock
    // hasn't advanced, or slightly beyond if it has)
    CHECK(v >= 0.0f);
    CHECK(v < 100.0f); // definitely not at target yet
  }

  SUBCASE("re-targeting mid-animation") {
    AnimatedFloat af(0.0f);
    af.setTarget(50.0f, 1000);
    af.setTarget(100.0f, 500);
    float v = af.value();
    // Should be somewhere between 0 and 100, not snapped
    CHECK(v >= 0.0f);
    CHECK(v <= 100.0f);
  }
}
