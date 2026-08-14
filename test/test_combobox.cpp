#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest/doctest.h"
#include "event.h"
#include "layout.h"
#include "widget.h"
#include "widgets/button.h"
#include "widgets/combobox.h"
#include "widgets/label.h"
#include <memory>

using namespace ltgui;

// Regression test for the ComboBox dropdown.
//
// Background: openDropdown() raises the combo to the top of its parent's
// children vector (so the dropdown paints over siblings), but layouts
// position children by vector order. If the raise is never undone, any
// relayout while raised (e.g. a sibling label's setText) permanently
// moves the combo to the end of its row — the dropdown then appears
// "broken" because clicks at the original position miss it.

static int childIndex(Widget *parent, Widget *child) {
  const auto &kids = parent->children();
  for (int i = 0; i < static_cast<int>(kids.size()); i++) {
    if (kids[i].get() == child)
      return i;
  }
  return -1;
}

// ComboBox::handleEvent is protected — dispatch through the public Widget
// base (virtual dispatch still reaches the override).
static void clickButton(ComboBox *combo) {
  Event ev;
  ev.type = EventType::MouseDown;
  ev.button = MouseButton::Left;
  ev.pos = {combo->x() + combo->width() / 2, combo->y() + combo->height() / 2};
  static_cast<Widget &>(*combo).handleEvent(ev);
}

static void clickItem(ComboBox *combo, int index) {
  Event ev;
  ev.type = EventType::MouseDown;
  ev.button = MouseButton::Left;
  ev.pos = {combo->x() + 20, combo->y() + combo->height() + 1 + index * 26 + 13};
  static_cast<Widget &>(*combo).handleEvent(ev);
}

static bool isOpen(ComboBox *combo) {
  return combo->effectiveGeometry().height > combo->height();
}

TEST_CASE("ComboBox open/close preserves layout order and geometry") {
  auto parent = std::make_unique<Widget>();
  parent->setGeometry(Rect(0, 0, 400, 80));
  parent->setLayout(std::make_unique<BoxLayout>(BoxLayout::LeftToRight, 8, 4));

  parent->makeChild<Button>("a");
  auto *combo = parent->makeChild<ComboBox>();
  combo->addItem("One");
  combo->addItem("Two");
  combo->addItem("Three");
  combo->setCurrentIndex(1);
  parent->makeChild<Button>("c");
  parent->layout()->layout(parent.get());

  int slotBefore = childIndex(parent.get(), combo);
  int xBefore = combo->x();
  CHECK(slotBefore == 1);

  SUBCASE("raising on open then restoring on close keeps the slot") {
    clickButton(combo);
    CHECK(isOpen(combo));
    // While open the combo is raised to the end of the child list.
    CHECK(childIndex(parent.get(), combo) == 2);

    clickItem(combo, 0);
    CHECK_FALSE(isOpen(combo));
    CHECK(combo->currentIndex() == 0);
    // Z-order restored and geometry unchanged after a layout pass.
    CHECK(childIndex(parent.get(), combo) == slotBefore);
    CHECK(combo->x() == xBefore);
  }

  SUBCASE("reopening works after a full open/select/close cycle") {
    clickButton(combo);
    clickItem(combo, 2);
    CHECK(combo->currentIndex() == 2);

    // Second cycle: open again — the click must still land on the combo.
    clickButton(combo);
    CHECK(isOpen(combo));
    clickItem(combo, 1);
    CHECK_FALSE(isOpen(combo));
    CHECK(combo->currentIndex() == 1);
    CHECK(childIndex(parent.get(), combo) == slotBefore);
    CHECK(combo->x() == xBefore);
  }
}

TEST_CASE("ComboBox stays put when a sibling resizes during selection") {
  // Reproduces the original bug: selecting an item updates a label,
  // scheduleRelayout() re-lays out the parent while the combo is raised,
  // and the combo used to end up at the END of the row.
  auto parent = std::make_unique<Widget>();
  parent->setGeometry(Rect(0, 0, 500, 60));
  parent->setLayout(std::make_unique<BoxLayout>(BoxLayout::LeftToRight, 8, 4));

  parent->makeChild<Button>("a");
  auto *combo = parent->makeChild<ComboBox>();
  combo->addItem("Small");
  combo->addItem("Medium");
  combo->addItem("Large");
  combo->setCurrentIndex(1);
  auto *label = parent->makeChild<Label>("Size: Medium");
  combo->onSelectionChanged.connect([&](int) {
    label->setText("Size: " + combo->currentText());
  });
  parent->layout()->layout(parent.get());

  int slotBefore = childIndex(parent.get(), combo);
  int xBefore = combo->x();

  clickButton(combo);
  CHECK(isOpen(combo));
  clickItem(combo, 0); // label shrinks ("Medium" -> "Small") mid-close

  CHECK(combo->currentIndex() == 0);
  CHECK(label->text() == "Size: Small");
  CHECK(childIndex(parent.get(), combo) == slotBefore);
  CHECK(combo->x() == xBefore);

  // And the combo is still fully usable afterwards.
  clickButton(combo);
  CHECK(isOpen(combo));
  clickItem(combo, 2);
  CHECK(combo->currentIndex() == 2);
  CHECK(label->text() == "Size: Large");
  CHECK(childIndex(parent.get(), combo) == slotBefore);
}

TEST_CASE("clearing items while open closes the dropdown and restores") {
  auto parent = std::make_unique<Widget>();
  parent->setGeometry(Rect(0, 0, 400, 60));
  parent->setLayout(std::make_unique<BoxLayout>(BoxLayout::LeftToRight, 8, 4));
  auto *combo = parent->makeChild<ComboBox>();
  combo->addItem("One");
  combo->addItem("Two");
  parent->makeChild<Button>("b");
  parent->layout()->layout(parent.get());

  int slotBefore = childIndex(parent.get(), combo);

  clickButton(combo);
  CHECK(isOpen(combo));
  combo->clear(); // triggers onItemsStructureChanged -> closeDropdown
  CHECK_FALSE(isOpen(combo));
  CHECK(childIndex(parent.get(), combo) == slotBefore);
}
