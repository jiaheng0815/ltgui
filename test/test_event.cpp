#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest/doctest.h"
#include "event.h"

using namespace ltgui;

TEST_CASE("Event default construction") {
  Event e;
  CHECK(e.type == EventType::None);
  CHECK(e.pos == Point(0, 0));
  CHECK(e.button == MouseButton::None);
  CHECK(e.key == Key::Unknown);
  CHECK(e.modifiers == 0);
  CHECK(e.wheelDelta == 0);
  CHECK(e.charCode == 0);
  CHECK(e.accepted == false);
}

TEST_CASE("EventType enum values") {
  // compile-time check that enum values exist
  CHECK(static_cast<int>(EventType::None) == 0);
  CHECK(static_cast<int>(EventType::MouseDown) == 1);
  CHECK(static_cast<int>(EventType::MouseUp) == 2);
  CHECK(static_cast<int>(EventType::MouseMove) == 3);
  CHECK(static_cast<int>(EventType::MouseWheel) == 4);
  CHECK(static_cast<int>(EventType::KeyDown) == 5);
  CHECK(static_cast<int>(EventType::KeyUp) == 6);
  CHECK(static_cast<int>(EventType::Paint) == 7);
  CHECK(static_cast<int>(EventType::Resize) == 8);
  CHECK(static_cast<int>(EventType::Close) == 9);
  CHECK(static_cast<int>(EventType::FocusIn) == 10);
  CHECK(static_cast<int>(EventType::FocusOut) == 11);
}

TEST_CASE("MouseButton enum values") {
  CHECK(static_cast<int>(MouseButton::None) == 0);
  CHECK(static_cast<int>(MouseButton::Left) == 1);
  CHECK(static_cast<int>(MouseButton::Right) == 2);
  CHECK(static_cast<int>(MouseButton::Middle) == 3);
}

TEST_CASE("KeyModifier operators") {
  SUBCASE("bitwise OR") {
    auto mod = KeyModifier::Shift | KeyModifier::Control;
    CHECK(static_cast<int>(mod) == 3);
  }
  SUBCASE("bitwise AND") {
    auto mod = KeyModifier::Shift | KeyModifier::Control;
    CHECK(hasModifier(static_cast<int>(mod), KeyModifier::Shift));
    CHECK(!hasModifier(static_cast<int>(mod), KeyModifier::Alt));
  }
}

TEST_CASE("Key enum values") {
  CHECK(static_cast<int>(Key::Unknown) == 0);
  CHECK(static_cast<int>(Key::A) == 1);
  CHECK(static_cast<int>(Key::Z) == 26);
  CHECK(static_cast<int>(Key::Escape) > 0);
  CHECK(static_cast<int>(Key::Enter) > 0);
  CHECK(static_cast<int>(Key::Space) > 0);
  CHECK(static_cast<int>(Key::Backspace) > 0);
  CHECK(static_cast<int>(Key::Tab) > 0);
  CHECK(static_cast<int>(Key::Left) > 0);
  CHECK(static_cast<int>(Key::Right) > 0);
  CHECK(static_cast<int>(Key::Up) > 0);
  CHECK(static_cast<int>(Key::Down) > 0);
  CHECK(static_cast<int>(Key::F1) > 0);
  CHECK(static_cast<int>(Key::F12) > 0);
}

TEST_CASE("Event struct members") {
  SUBCASE("set and read position") {
    Event e;
    e.pos = Point(42, 99);
    CHECK(e.pos.x == 42);
    CHECK(e.pos.y == 99);
  }
  SUBCASE("set and read key") {
    Event e;
    e.key = Key::Enter;
    CHECK(e.key == Key::Enter);
  }
  SUBCASE("set and read charCode") {
    Event e;
    e.charCode = 0x4E2D;
    CHECK(e.charCode == 0x4E2D);
  }
  SUBCASE("set and read modifiers") {
    Event e;
    e.modifiers = 3; // Shift | Control
    CHECK(e.modifiers == 3);
  }
  SUBCASE("set and read accepted") {
    Event e;
    e.accepted = true;
    CHECK(e.accepted == true);
  }
  SUBCASE("mouse button") {
    Event e;
    e.button = MouseButton::Right;
    CHECK(e.button == MouseButton::Right);
  }
  SUBCASE("wheelDelta") {
    Event e;
    e.wheelDelta = 120;
    CHECK(e.wheelDelta == 120);
  }
  SUBCASE("resize dimensions") {
    Event e;
    e.width = 800;
    e.height = 600;
    CHECK(e.width == 800);
    CHECK(e.height == 600);
  }
}
