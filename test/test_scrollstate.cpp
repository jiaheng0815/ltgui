#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "animation.h"
#include "doctest/doctest.h"
#include "widgets/scrollstate.h"
#include <chrono>
#include <thread>

using namespace ltgui;

// ScrollState is a plain mixin (no Widget), so it tests headless directly.

TEST_CASE("ScrollState target clamping") {
  ScrollState s;

  SUBCASE("clamps negative targets to zero") {
    s.setScrollTarget(-10, 50);
    CHECK(s.scrollTarget() == 0);
  }

  SUBCASE("clamps above maxOffset") {
    s.setScrollTarget(100, 20);
    CHECK(s.scrollTarget() == 20);
  }

  SUBCASE("keeps targets within range") {
    s.setScrollTarget(15, 20);
    CHECK(s.scrollTarget() == 15);
  }

  SUBCASE("setScrollImmediate jumps without animation") {
    s.setScrollImmediate(10, 50);
    CHECK(s.scrollOffset() == 10);
    CHECK(s.scrollTarget() == 10);
    CHECK_FALSE(s.isScrolling());
  }
}

TEST_CASE("ScrollState wheel handling") {
  ScrollState s;
  s.setScrollImmediate(5, 50);

  SUBCASE("positive wheel scrolls up") {
    s.handleWheel(3, 50, 1); // wheelDelta 3 -> scroll back 3 rows
    CHECK(s.scrollTarget() == 2);
  }

  SUBCASE("negative wheel scrolls down") {
    s.handleWheel(-3, 50, 1);
    CHECK(s.scrollTarget() == 8);
  }

  SUBCASE("cannot scroll past the end") {
    s.setScrollImmediate(48, 50);
    s.handleWheel(-10, 50, 1);
    CHECK(s.scrollTarget() == 50);
  }

  SUBCASE("cannot scroll above the start") {
    s.setScrollImmediate(2, 50);
    s.handleWheel(10, 50, 1);
    CHECK(s.scrollTarget() == 0);
  }
}

TEST_CASE("ScrollState animates toward target") {
  ScrollState s;
  s.setScrollImmediate(0, 100);
  s.setScrollTarget(50, 100);
  CHECK(s.isScrolling());

  // AnimatedFloat's clock is AnimationManager::nowMs(), which advances
  // on tick(); the value itself advances lazily on scrollOffset(). Drive
  // both with real time until the 200ms animation completes.
  auto &mgr = AnimationManager::instance();
  for (int i = 0; i < 30; i++) {
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
    mgr.tick();
    (void)s.scrollOffset();
  }
  CHECK_FALSE(s.isScrolling());
  CHECK(s.scrollOffset() == 50);
}
