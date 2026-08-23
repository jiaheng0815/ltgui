// i18n example: in-memory TranslationTables, locale switch, plural selection.
// Note: tr(key, n) returns the plural *form* for n; it does not substitute
// "%d" — format the number yourself if the table uses placeholders.
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
  if (!window.create(520, 340, "ltgui i18n")) {
    std::cerr << "Failed to create window." << std::endl;
    return 1;
  }

  // English table (plural forms keyed by CLDR categories; en uses one/other).
  TranslationTable en;
  en.add("hello", "Hello, ltgui!");
  en.add("file", "File");
  en.addPlural("files", "0 files", "exactly 1 file", "2 files", "a few files",
               "many files", "N files");

  // Chinese table: one form for every count (zh uses only `other`).
  TranslationTable zh;
  zh.add("hello", "你好,ltgui！");
  zh.add("file", "文件");
  zh.addPlural("files", "%d 个文件", "%d 个文件", "%d 个文件", "%d 个文件",
               "%d 个文件", "%d 个文件");

  I18n &i18n = I18n::instance();
  i18n.addTable(Locale("en"), en);
  i18n.addTable(Locale("zh", "CN"), zh);

  auto root = std::make_unique<Widget>();
  root->style().bgColor = currentTheme().bgPrimary;
  auto layout = std::make_unique<BoxLayout>(BoxLayout::TopToBottom, 12, 10);

  auto *title = root->makeChild<Label>("");
  title->style().font = Font("Segoe UI", 18, FontWeight::Bold);
  title->style().fgColor = currentTheme().accent;

  auto *countLabel = root->makeChild<Label>("");
  auto *hint = root->makeChild<Label>("plural(1) / plural(3) below");
  hint->style().fgColor = currentTheme().textSecondary;

  auto *btnRow = root->makeChild<Widget>();
  btnRow->style().bgColor = Color::Transparent;
  auto row = std::make_unique<BoxLayout>(BoxLayout::LeftToRight, 8, 0);
  auto *enBtn = btnRow->makeChild<Button>("EN");
  auto *zhBtn = btnRow->makeChild<Button>("中文");
  row->addStretch(0);
  row->addStretch(1);
  btnRow->setLayout(std::move(row));

  auto applyLocale = [&](const Locale &locale) {
    i18n.setLocale(locale);
    title->setText(i18n.tr("hello"));
    countLabel->setText(i18n.tr("files", 1) + "  /  " + i18n.tr("files", 3));
  };

  enBtn->onClicked.connect([&]() { applyLocale(Locale("en")); });
  zhBtn->onClicked.connect([&]() { applyLocale(Locale("zh", "CN")); });

  applyLocale(Locale("en"));

  layout->addStretch(0);
  layout->addStretch(0);

  root->setLayout(std::move(layout));
  window.setCentralWidget(std::move(root));
  window.show();

  return Application::instance().run();
}
