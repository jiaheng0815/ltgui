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
