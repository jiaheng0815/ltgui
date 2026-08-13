#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest/doctest.h"
#include "ltgui.h"

using namespace ltgui;

// Headless MenuBar tests — drive keyboard navigation through real events.
// handleEvent() is protected, so tests dispatch through the Widget&
// interface (virtual dispatch still lands in MenuBar::handleEvent).

namespace {

bool key(Widget &w, Key k) {
  Event e;
  e.type = EventType::KeyDown;
  e.key = k;
  return static_cast<Widget &>(w).handleEvent(e);
}

} // namespace

TEST_CASE("MenuBar addMenu/addItem structure") {
  MenuBar mb;
  int m = mb.addMenu("File");
  CHECK(m == 0);
  CHECK(mb.addItem(m, "Open") >= 0);
  CHECK(mb.addItem(m, "Quit") >= 0);
  mb.addSeparator(m);
  CHECK(mb.addItem(m, "Extra") >= 0);
  mb.addMenu("Edit");
  CHECK(mb.count() == 2);
}

TEST_CASE("MenuBar keyboard navigation") {
  MenuBar mb;
  int file = mb.addMenu("File");
  mb.addItem(file, "Open");
  mb.addItem(file, "Save");
  int edit = mb.addMenu("Edit");
  mb.addItem(edit, "Copy");

  // Open the File menu by pressing Down on the menubar.
  CHECK(key(mb, Key::Down));
  CHECK(mb.currentIndex() == 0);
  CHECK(mb.hoveredItem() == 0);

  // Navigate down to Save.
  CHECK(key(mb, Key::Down));
  CHECK(mb.hoveredItem() == 1);

  // Right switches to the Edit menu.
  CHECK(key(mb, Key::Right));
  CHECK(mb.currentIndex() == 1);
  CHECK(mb.hoveredItem() == 0);

  // Left goes back to File.
  CHECK(key(mb, Key::Left));
  CHECK(mb.currentIndex() == 0);

  // Escape closes the menu.
  CHECK(key(mb, Key::Escape));
  CHECK(mb.currentIndex() == -1);
}

TEST_CASE("MenuBar item activation invokes callback") {
  MenuBar mb;
  int file = mb.addMenu("File");
  int fired = 0;
  mb.addItem(file, "Open", [&]() { fired++; });

  CHECK(key(mb, Key::Down));  // open menu, hover Open
  CHECK(key(mb, Key::Enter)); // activate
  CHECK(fired == 1);
  CHECK(mb.currentIndex() == -1); // menu closed after activation
}

TEST_CASE("MenuBar checkable and radio items") {
  MenuBar mb;
  int view = mb.addMenu("View");
  mb.addItem(view, "Status Bar");
  mb.setItemCheckable(view, 0, true);
  mb.addItem(view, "Mode A");
  mb.addItem(view, "Mode B");
  mb.setItemRadio(view, 1, 1);
  mb.setItemRadio(view, 2, 1);

  // Toggle the checkable item via keyboard.
  CHECK(key(mb, Key::Down));
  CHECK(key(mb, Key::Enter));
  CHECK(mb.isItemChecked(view, 0));

  // Checkable toggles back off.
  CHECK(key(mb, Key::Down));
  CHECK(key(mb, Key::Enter));
  CHECK_FALSE(mb.isItemChecked(view, 0));
}

TEST_CASE("MenuBar submenu keyboard navigation") {
  MenuBar mb;
  int file = mb.addMenu("File");
  mb.addItem(file, "New");
  mb.addItem(file, "Recent");
  int fired = 0;
  // Each submenu entry: addSubmenu creates a slot, addSubItem fills it.
  int s0 = mb.addSubmenu(file, 1, "Recent");
  mb.addSubItem(file, 1, s0, "Doc A", [&]() { fired++; });
  int s1 = mb.addSubmenu(file, 1, "Recent");
  mb.addSubItem(file, 1, s1, "Doc B");

  CHECK(key(mb, Key::Down));  // open menu, hover New (item 0)
  CHECK(key(mb, Key::Down));  // hover Recent (item 1)
  CHECK(key(mb, Key::Right)); // open the submenu

  // Down navigates within the submenu.
  CHECK(key(mb, Key::Down));
  CHECK(key(mb, Key::Enter)); // activate Doc B
  CHECK(fired == 0);          // Doc B has no callback

  // Reopen (Enter closed the menu), navigate to Recent, open submenu,
  // activate Doc A (hoveredSub_ starts at 0).
  CHECK(key(mb, Key::Down));
  CHECK(key(mb, Key::Down));
  CHECK(key(mb, Key::Right));
  CHECK(key(mb, Key::Enter));
  CHECK(fired == 1);
  CHECK(mb.currentIndex() == -1); // closed after activation
}
