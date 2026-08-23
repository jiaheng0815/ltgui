// TreeView example: two-level tree with selection tracking.
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
  if (!window.create(500, 420, "ltgui TreeView")) {
    std::cerr << "Failed to create window." << std::endl;
    return 1;
  }

  auto root = std::make_unique<Widget>();
  root->style().bgColor = currentTheme().bgPrimary;
  auto layout = std::make_unique<BoxLayout>(BoxLayout::TopToBottom, 8, 8);

  auto *tree = root->makeChild<TreeView>();

  // Two-level tree: categories -> items
  auto *animals = tree->rootItem()->addChild("Animals");
  animals->setExpanded(true);
  animals->addChild("Dog");
  animals->addChild("Cat");
  auto *birds = tree->rootItem()->addChild("Birds");
  birds->setExpanded(false);
  birds->addChild("Eagle");
  birds->addChild("Sparrow");
  auto *fruit = tree->rootItem()->addChild("Fruit");
  fruit->setExpanded(true);
  fruit->addChild("Apple");
  fruit->addChild("Banana");

  auto *selLabel = root->makeChild<Label>("Selection: <none>");
  selLabel->style().fgColor = currentTheme().textSecondary;

  tree->onSelectionChanged.connect([&](TreeViewItem *item) {
    selLabel->setText(item ? "Selection: " + item->text() : "Selection: <none>");
  });

  auto *btnRow = root->makeChild<Widget>();
  btnRow->style().bgColor = Color::Transparent;
  auto rowLayout = std::make_unique<BoxLayout>(BoxLayout::LeftToRight, 8, 0);
  auto *expandBtn = btnRow->makeChild<Button>("Expand all");
  expandBtn->onClicked.connect([&]() {
    bool state = !birds->expanded();
    birds->setExpanded(state);
    fruit->setExpanded(state);
    animals->setExpanded(state);
  });
  auto *selectBtn = btnRow->makeChild<Button>("Select Eagle");
  selectBtn->onClicked.connect([&]() {
    tree->setSelectedItem(birds->childCount() > 0 ? birds->children()[0].get() : nullptr);
  });
  rowLayout->addStretch(1);
  btnRow->setLayout(std::move(rowLayout));

  layout->addStretch(0);
  layout->addStretch(1);

  root->setLayout(std::move(layout));
  window.setCentralWidget(std::move(root));
  window.show();

  return Application::instance().run();
}
