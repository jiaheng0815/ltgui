// FileDialog example: open / save / folder modes with a filter list.
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
  if (!window.create(520, 320, "ltgui FileDialog")) {
    std::cerr << "Failed to create window." << std::endl;
    return 1;
  }

  auto root = std::make_unique<Widget>();
  root->style().bgColor = currentTheme().bgPrimary;
  auto layout = std::make_unique<BoxLayout>(BoxLayout::TopToBottom, 12, 10);

  auto *openBtn = root->makeChild<Button>("Open file...");
  auto *saveBtn = root->makeChild<Button>("Save file as...");
  auto *folderBtn = root->makeChild<Button>("Select folder...");
  auto *result = root->makeChild<Label>("Result: <none>");
  result->style().fgColor = currentTheme().textSecondary;

  // Standard filter list, reused by the open/save dialogs
  auto makeFilters = []() {
    std::vector<FileFilter> f;
    f.push_back({"All files", "*.*"});
    f.push_back({"Text files", "*.txt"});
    f.push_back({"Source files", "*.cpp;*.h"});
    return f;
  };

  openBtn->onClicked.connect([&]() {
    FileDialog dlg(openBtn);
    dlg.setMode(FileDialogMode::OpenFile);
    dlg.addFilter(makeFilters()[2]);
    dlg.addFilter(makeFilters()[0]);
    DialogResult r = dlg.exec();
    if (r == DialogResult::OK)
      result->setText("Result: open -> " + dlg.selectedPath());
    else
      result->setText("Result: open cancelled");
  });

  saveBtn->onClicked.connect([&]() {
    FileDialog dlg(openBtn);
    dlg.setMode(FileDialogMode::SaveFile);
    dlg.addFilter(makeFilters()[0]);
    dlg.setDefaultPath("output.txt");
    DialogResult r = dlg.exec();
    if (r == DialogResult::OK)
      result->setText("Result: save -> " + dlg.selectedPath());
    else
      result->setText("Result: save cancelled");
  });

  folderBtn->onClicked.connect([&]() {
    FileDialog dlg(openBtn);
    dlg.setMode(FileDialogMode::SelectFolder);
    DialogResult r = dlg.exec();
    if (r == DialogResult::OK)
      result->setText("Result: folder -> " + dlg.selectedPath());
    else
      result->setText("Result: folder cancelled");
  });

  layout->addStretch(0);
  layout->addStretch(1);

  root->setLayout(std::move(layout));
  window.setCentralWidget(std::move(root));
  window.show();

  return Application::instance().run();
}
