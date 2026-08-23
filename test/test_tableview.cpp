#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest/doctest.h"
#include "ltgui.h"

using namespace ltgui;

// Headless TableView tests — model management, sorting, selection API.

TEST_CASE("SimpleTableModel rows/cols") {
  SimpleTableModel model(3, 2);
  CHECK(model.rowCount() == 3);
  CHECK(model.columnCount() == 2);

  model.setCellText(1, 1, "hello");
  CHECK(model.cellText(1, 1) == "hello");
  CHECK(model.cellText(5, 5).empty()); // out of bounds is safe

  model.addRow({"a", "b"});
  CHECK(model.rowCount() == 4);

  model.removeRow(0);
  CHECK(model.rowCount() == 3);

  model.clear();
  CHECK(model.rowCount() == 0);
}

TEST_CASE("SimpleTableModel sort") {
  SimpleTableModel model(0, 1);
  model.addRow({"banana"});
  model.addRow({"apple"});
  model.addRow({"cherry"});

  model.sort(0, true);
  CHECK(model.cellText(0, 0) == "apple");
  CHECK(model.cellText(1, 0) == "banana");
  CHECK(model.cellText(2, 0) == "cherry");

  model.sort(0, false);
  CHECK(model.cellText(0, 0) == "cherry");
  CHECK(model.cellText(2, 0) == "apple");
}

TEST_CASE("TableView selection API") {
  TableView tv;
  auto model = std::make_shared<SimpleTableModel>(5, 2);
  for (int i = 0; i < 5; i++) {
    model->setCellText(i, 0, "row" + std::to_string(i));
  }
  tv.setModel(model);

  int selected = -1;
  tv.onRowSelected.connect([&](int row) { selected = row; });

  CHECK(tv.currentIndex() == -1);

  tv.setCurrentIndex(2);
  CHECK(tv.currentIndex() == 2);
  CHECK(selected == 2);

  // Legacy alias still works.
  CHECK(tv.currentIndex() == 2);

  tv.clearSelection();
  CHECK(tv.currentIndex() == -1);
}

TEST_CASE("TableView columns and sorting state") {
  TableView tv;
  TableColumn col{"Name", 120};
  tv.addColumn(col);
  tv.addColumn({"Value", 80});
  CHECK(tv.model() == nullptr);

  tv.setSortColumn(0, true);
  // No crash; sort state stored for header rendering.
  tv.setSortColumn(0, false);
}

// --- In-place editing ---

TEST_CASE("TableView cell editing: double-click enters edit, Enter commits") {
  TableView tv;
  tv.setGeometry(Rect(0, 0, 300, 200));
  auto model = std::make_shared<SimpleTableModel>(0, 2);
  model->addRow({"alpha", "1"});
  model->addRow({"beta", "2"});
  tv.addColumn({"A", 120});
  tv.addColumn({"B", 100});
  tv.setModel(model);

  int edited = 0;
  tv.onCellEdited.connect([&](int, int) { edited++; });

  // Click cell (0,0) twice — same position within 400ms → beginEdit.
  Event down;
  down.type = EventType::MouseDown;
  down.button = MouseButton::Left;
  down.pos = Point(30, 30 + 13);
  CHECK(static_cast<Widget &>(tv).handleEvent(down));
  CHECK(static_cast<Widget &>(tv).handleEvent(down));
  CHECK(tv.isEditing());
  CHECK(tv.editingRow() == 0);
  CHECK(tv.editingCol() == 0);

  // Enter commits: nothing changed (text untouched) → no signal.
  Event enter;
  enter.type = EventType::KeyDown;
  enter.key = Key::Enter;
  CHECK(static_cast<Widget &>(tv).handleEvent(enter));
  CHECK_FALSE(tv.isEditing());
  CHECK(edited == 0);
  CHECK(model->cellText(0, 0) == "alpha");
}

TEST_CASE("TableView cell editing: Escape cancels, outside click commits") {
  TableView tv;
  tv.setGeometry(Rect(0, 0, 300, 200));
  auto model = std::make_shared<SimpleTableModel>(0, 2);
  model->addRow({"alpha", "1"});
  model->addRow({"beta", "2"});
  tv.addColumn({"A", 120});
  tv.addColumn({"B", 100});
  tv.setModel(model);

  Event down;
  down.type = EventType::MouseDown;
  down.button = MouseButton::Left;
  down.pos = Point(30, 30 + 13);
  CHECK(static_cast<Widget &>(tv).handleEvent(down));
  CHECK(static_cast<Widget &>(tv).handleEvent(down));
  CHECK(tv.isEditing());

  Event esc;
  esc.type = EventType::KeyDown;
  esc.key = Key::Escape;
  CHECK(static_cast<Widget &>(tv).handleEvent(esc));
  CHECK_FALSE(tv.isEditing());
  CHECK(model->cellText(0, 0) == "alpha");

  // Re-enter editing, then click another row — commits and selects the row.
  CHECK(static_cast<Widget &>(tv).handleEvent(down));
  CHECK(static_cast<Widget &>(tv).handleEvent(down));
  CHECK(tv.isEditing());
  Event other;
  other.type = EventType::MouseDown;
  other.button = MouseButton::Left;
  other.pos = Point(30, 30 + 13 + 26);  // row 1
  CHECK(static_cast<Widget &>(tv).handleEvent(other));
  CHECK_FALSE(tv.isEditing());
  CHECK(tv.currentIndex() == 1);
}

TEST_CASE("TableView cell editing: read-only model refuses edit") {
  // A model without supportsEdit() must not enter editing via double-click.
  struct ReadOnly : TableModel {
    int rowCount() const override { return 1; }
    int columnCount() const override { return 1; }
    std::string cellText(int, int) const override { return "ro"; }
  };
  TableView tv;
  tv.setGeometry(Rect(0, 0, 300, 200));
  tv.addColumn({"A", 120});
  tv.setModel(std::make_shared<ReadOnly>());

  Event down;
  down.type = EventType::MouseDown;
  down.button = MouseButton::Left;
  down.pos = Point(30, 30 + 13);
  CHECK(static_cast<Widget &>(tv).handleEvent(down));
  CHECK(static_cast<Widget &>(tv).handleEvent(down));
  CHECK_FALSE(tv.isEditing());
}
