#include "ltgui.h"
#include <cstring>
#include <iostream>

using namespace ltgui;

int main(int argc, char *argv[]) {
  for (int i = 1; i < argc; i++) {
    if (std::strcmp(argv[i], "--debug") == 0) {
      Logger::instance().setGlobalDebug(true);
    }
  }

  Window window;
  if (!window.create(400, 300, "Hello, ltgui!")) {
    std::cerr << "Failed to create window." << std::endl;
    return 1;
  }

  auto root = std::make_unique<Widget>();
  root->style().bgColor = currentTheme().bgPrimary;

  auto layout = std::make_unique<BoxLayout>(BoxLayout::TopToBottom, 8, 12);

  auto *label = root->makeChild<Label>("Welcome to ltgui!");
  label->style().font = Font("Segoe UI", 18, FontWeight::Bold);
  label->style().fgColor = currentTheme().accent;

  auto *button = root->makeChild<Button>("Click Me!");
  int clickCount = 0;
  button->onClicked.connect([&]() {
    clickCount++;
    button->setText("Clicked: " + std::to_string(clickCount) + " times");
  });

  auto *quitBtn = root->makeChild<Button>("Quit");
  quitBtn->onClicked.connect([&]() { window.close(); });

  root->setLayout(std::move(layout));

  window.setCentralWidget(std::move(root));
  window.show();

  return Application::instance().run();
}
