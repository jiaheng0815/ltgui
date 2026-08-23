// TableView example: virtual table with sorting, selection and row highlight.
#include "ltgui.h"
#include <cstring>
#include <iostream>

using namespace ltgui;

int main(int argc, char *argv[]) {
  for (int i = 1; i < argc; i++) {
    if (std::strcmp(argv[i], "--debug") == 0)
      Logger::instance().setGlobalDebug(true);
  }

  Window window;
  if (!window.create(700, 480, "ltgui TableView")) {
    std::cerr << "Failed to create window." << std::endl;
    return 1;
  }

  auto root = std::make_unique<Widget>();
  root->style().bgColor = currentTheme().bgPrimary;
  auto layout = std::make_unique<BoxLayout>(BoxLayout::TopToBottom, 8, 8);

  // A small toolbar explaining how to use the table
  auto *tip = root->makeChild<Label>(
      "Click a header to sort, drag a header edge to resize, click a row to select.");
  tip->style().fgColor = currentTheme().textSecondary;

  auto *table = root->makeChild<TableView>();
  table->addColumn(TableColumn{"Name", 200, 80, true, true});
  table->addColumn(TableColumn{"Role", 150, 80, true, true});
  table->addColumn(TableColumn{"Version", 120, 60, true, true});
  table->addColumn(TableColumn{"Stars", 80, 50, true, true});

  auto model = std::make_shared<SimpleTableModel>(0, 4);
  model->addRow({"ltgui", "GUI framework", "1.0.0", "276"});
  model->addRow({"clang", "Compiler", "22.1", "15K"});
  model->addRow({"doctest", "Test framework", "2.4", "2K"});
  model->addRow({"cmake", "Build system", "4.1", "20K"});
  table->setModel(model);

  // Selection feedback
  auto *selLabel = root->makeChild<Label>("Selection: <none>");
  int clickCount = 0;
  table->onRowSelected.connect([&](int row) {
    (void)row;
    clickCount++;
    selLabel->setText("Selection: #" + std::to_string(clickCount) +
                      " -> " + model->cellText(row, 0) + " (" +
                      model->cellText(row, 1) + ")");
  });

  // Sort feedback (header click already sorts via setSortColumn logic)
  table->onHeaderClicked.connect([&](int col) {
    LOG_INFO("Table", "header %d clicked", col);
  });

  // Programmatic selection + sort demo: start sorted by Name ascending
  model->sort(0, true);
  table->setCurrentIndex(0);

  layout->addStretch(0);
  layout->addStretch(1);

  root->setLayout(std::move(layout));
  window.setCentralWidget(std::move(root));
  window.show();

  return Application::instance().run();
}
