// Menus example: MenuBar (menus, submenus, checkable/shortcut items) and a
// ContextMenu opened from a button.
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
  if (!window.create(640, 400, "ltgui Menus")) {
    std::cerr << "Failed to create window." << std::endl;
    return 1;
  }

  auto root = std::make_unique<Widget>();
  root->style().bgColor = currentTheme().bgPrimary;
  auto layout = std::make_unique<BoxLayout>(BoxLayout::TopToBottom, 4, 8);

  // Menu bar: File + View + Help. Keyboard navigation works out of the box
  // (arrows, Enter, Esc) once the menu is open or focused.
  auto *menuBar = root->makeChild<MenuBar>();
  int fileMenu = menuBar->addMenu("File");
  menuBar->addItem(fileMenu, "New", []() { LOG_INFO("Demo", "File -> New"); });
  menuBar->addItem(fileMenu, "Open...", []() { LOG_INFO("Demo", "File -> Open"); });
  menuBar->addSeparator(fileMenu);
  menuBar->addItem(fileMenu, "Exit", []() { LOG_INFO("Demo", "File -> Exit"); });

  int viewMenu = menuBar->addMenu("View");
  menuBar->addItem(viewMenu, "Show Grid", []() { LOG_INFO("Demo", "View -> Grid"); });
  menuBar->setItemCheckable(viewMenu, 0, true);
  menuBar->setItemChecked(viewMenu, 0, true);

  int helpMenu = menuBar->addMenu("Help");
  int docsIdx = menuBar->addItem(helpMenu, "Documentation", []() {
    LOG_INFO("Demo", "Help -> Documentation");
  });
  menuBar->setItemShortcut(helpMenu, docsIdx, "F1");
  int aboutSub = menuBar->addSubmenu(helpMenu, docsIdx, "More");
  menuBar->addSubItem(helpMenu, docsIdx, aboutSub, "Version info",
                      []() { LOG_INFO("Demo", "Help -> More -> Version"); });

  // Context menu: popup takes screen coordinates; W0 uses window coordinates
  // here, which is fine for menu display at a fixed point.
  ContextMenu contextMenu(root.get());
  int copyIdx = contextMenu.addItem("Copy", []() { LOG_INFO("Demo", "ctx -> Copy"); });
  (void)copyIdx;
  contextMenu.addItem("Paste", []() { LOG_INFO("Demo", "ctx -> Paste"); });
  contextMenu.addSeparator();
  contextMenu.addItem("Delete", []() { LOG_INFO("Demo", "ctx -> Delete"); });

  auto *ctxBtn = root->makeChild<Button>("Show context menu");
  ctxBtn->onClicked.connect([&]() {
    // The menu panels are positioned in the window coordinate space (0,0 is
    // the top-left client area corner).
    contextMenu.popup({30, 60});
  });

  auto *hint = root->makeChild<Label>(
      "Open the File menu, check View -> Show Grid, hover Help -> More.");
  hint->style().fgColor = currentTheme().textSecondary;

  layout->addStretch(0);
  layout->addStretch(0);

  root->setLayout(std::move(layout));
  window.setCentralWidget(std::move(root));
  window.show();

  return Application::instance().run();
}
