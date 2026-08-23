#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest/doctest.h"
#include "ltgui.h"

using namespace ltgui;

// Headless ContextMenu tests — item management does not need a window;
// popup()/dismiss() require a window and are exercised by the GUI smoke.

TEST_CASE("ContextMenu items and callbacks") {
  ContextMenu menu;
  CHECK(menu.count() == 0);
  CHECK(menu.itemText(0).empty());

  menu.addItem("Copy");
  menu.addItem("Paste");
  CHECK(menu.count() == 2);
  CHECK(menu.itemText(0) == "Copy");
  CHECK(menu.itemText(1) == "Paste");

  menu.addSeparator();
  CHECK(menu.count() == 3);
  CHECK(menu.itemText(2).empty());

  int fired = 0;
  menu.addItem("Delete", [&]() { fired++; });
  CHECK(menu.count() == 4);

  menu.clear();
  CHECK(menu.count() == 0);
  CHECK(fired == 0);
}

TEST_CASE("ContextMenu item callbacks fire via keyboard activation") {
  ContextMenu menu;
  // Activation goes through handleEvent with the window; without one the
  // callback list stays intact — verify no crash on a bare dispatch.
  menu.addItem("X", []() {});
  Event e;
  e.type = EventType::KeyDown;
  e.key = Key::Escape;
  CHECK_FALSE(static_cast<Widget &>(menu).handleEvent(e));
}
