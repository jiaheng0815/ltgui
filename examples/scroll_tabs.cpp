// Scroll + Tab example: a ScrollArea holding a tall column of items, and a
// TabWidget whose pages each host their own content.
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
  if (!window.create(640, 480, "ltgui Scroll & Tabs")) {
    std::cerr << "Failed to create window." << std::endl;
    return 1;
  }

  auto root = std::make_unique<Widget>();
  root->style().bgColor = currentTheme().bgPrimary;
  auto layout = std::make_unique<BoxLayout>(BoxLayout::TopToBottom, 8, 8);

  // --- TabWidget with two pages ---
  auto *tabs = root->makeChild<TabWidget>();
  tabs->addTab("Scroll area");
  tabs->addTab("Buttons");

  // Page 0: scrollable long content
  auto *page0 = tabs->tabContent(0);
  if (page0) {
    auto *area = page0->makeChild<ScrollArea>();
    area->style().borderRadius = 4;
    auto content = std::make_unique<Widget>();
    content->style().bgColor = currentTheme().bgSecondary;
    auto col = std::make_unique<BoxLayout>(BoxLayout::TopToBottom, 4, 4);
    for (int i = 0; i < 40; i++) {
      content->makeChild<Label>("Row " + std::to_string(i + 1) +
                                " — scroll this long list with the wheel");
    }
    col->addStretch(0);
    col->addStretch(1);
    content->setLayout(std::move(col));
    area->setWidget(std::move(content));

    auto fill = std::make_unique<BoxLayout>(BoxLayout::TopToBottom, 0, 0);
    fill->addStretch(0);
    fill->addStretch(1);
    page0->setLayout(std::move(fill));
  }

  // Page 1: simple button rows
  auto *page1 = tabs->tabContent(1);
  if (page1) {
    page1->style().bgColor = currentTheme().bgSecondary;
    auto col = std::make_unique<BoxLayout>(BoxLayout::TopToBottom, 8, 8);
    auto *b1 = page1->makeChild<Button>("Alpha");
    b1->onClicked.connect([]() { LOG_INFO("Demo", "tab1 -> Alpha"); });
    auto *b2 = page1->makeChild<Button>("Beta");
    b2->onClicked.connect([]() { LOG_INFO("Demo", "tab1 -> Beta"); });
    col->addStretch(0);
    col->addStretch(1);
    page1->setLayout(std::move(col));
  }

  auto *hint = root->makeChild<Label>("Switch tabs; the first page scrolls with the wheel.");
  hint->style().fgColor = currentTheme().textSecondary;

  layout->addStretch(0);
  layout->addStretch(1);

  root->setLayout(std::move(layout));
  window.setCentralWidget(std::move(root));
  window.show();

  return Application::instance().run();
}
