#include "ltgui.h"
#include <cstring>
#include <iostream>

using namespace ltgui;

int main(int argc, char *argv[]) {
  // Enable full debug logging when --debug is passed
  for (int i = 1; i < argc; i++) {
    if (std::strcmp(argv[i], "--debug") == 0) {
      Logger::instance().setGlobalDebug(true);
    }
  }

  Window window;
  if (!window.create(640, 480, "ltgui Demo")) {
    std::cerr << "Failed to create window." << std::endl;
    return 1;
  }

  auto root = std::make_unique<Widget>();
  root->style().bgColor = currentTheme().bgPrimary;

  auto mainLayout = std::make_unique<BoxLayout>(BoxLayout::TopToBottom, 4, 8);

  // Menu bar (File + View with a checkable item)
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

  // Title
  auto *title = root->makeChild<Label>("ltgui Widget Demo");
  title->style().font = Font("Segoe UI", 20, FontWeight::Bold);
  title->style().fgColor = currentTheme().accent;

  // Button row
  auto *buttonRow = root->makeChild<Widget>();
  buttonRow->style().bgColor = Color::Transparent;
  auto btnLayout = std::make_unique<BoxLayout>(BoxLayout::LeftToRight, 8, 4);

  auto *btn1 = buttonRow->makeChild<Button>("Normal");
  auto *btn2 = buttonRow->makeChild<Button>("Disabled");
  btn2->setEnabled(false);
  int clicks = 0;
  auto *btn3 = buttonRow->makeChild<Button>("Counter: 0");
  btn3->onClicked.connect([&]() {
    clicks++;
    btn3->setText("Counter: " + std::to_string(clicks));
  });

  buttonRow->setLayout(std::move(btnLayout));

  // Textbox row
  auto *textRow = root->makeChild<Widget>();
  textRow->style().bgColor = Color::Transparent;
  auto textLayout = std::make_unique<BoxLayout>(BoxLayout::LeftToRight, 8, 4);
  auto *tbLabel = textRow->makeChild<Label>("Text:");
  auto *textBox = textRow->makeChild<TextBox>("Edit me!");
  textLayout->addStretch(1);
  textRow->setLayout(std::move(textLayout));

  // Checkbox + Radio row
  auto *checkRow = root->makeChild<Widget>();
  checkRow->style().bgColor = Color::Transparent;
  auto checkLayout = std::make_unique<BoxLayout>(BoxLayout::LeftToRight, 12, 4);

  auto *cb1 = checkRow->makeChild<CheckBox>("Option A");
  auto *cb2 = checkRow->makeChild<CheckBox>("Option B");
  cb2->setChecked(true);
  auto *cb3 = checkRow->makeChild<CheckBox>("Option C");

  auto *rbGroup = checkRow->makeChild<Widget>();
  rbGroup->style().bgColor = Color::Transparent;
  auto rbLayout = std::make_unique<BoxLayout>(BoxLayout::LeftToRight, 4, 0);
  auto *rb1 = rbGroup->makeChild<RadioButton>("Red");
  auto *rb2 = rbGroup->makeChild<RadioButton>("Green");
  auto *rb3 = rbGroup->makeChild<RadioButton>("Blue");
  rb1->setChecked(true);
  rbGroup->setLayout(std::move(rbLayout));

  checkRow->setLayout(std::move(checkLayout));

  // Slider row
  auto *sliderRow = root->makeChild<Widget>();
  sliderRow->style().bgColor = Color::Transparent;
  auto sliderLayout = std::make_unique<BoxLayout>(BoxLayout::LeftToRight, 8, 4);
  auto *slLabel = sliderRow->makeChild<Label>("Volume:");
  auto *slider = sliderRow->makeChild<Slider>();
  slider->setValue(50);
  auto *slValue = sliderRow->makeChild<Label>(" 50");
  slider->onValueChanged.connect([&](int v) {
    // Fixed-width formatting so the label width never changes,
    // preventing the layout from resizing the slider track.
    if (v >= 100)
      slValue->setText(std::to_string(v));
    else if (v >= 10)
      slValue->setText(" " + std::to_string(v));
    else
      slValue->setText("  " + std::to_string(v));
  });
  sliderLayout->addStretch(1);
  sliderRow->setLayout(std::move(sliderLayout));

  // List + Combo row
  auto *listRow = root->makeChild<Widget>();
  listRow->style().bgColor = Color::Transparent;
  auto listRowLayout =
      std::make_unique<BoxLayout>(BoxLayout::LeftToRight, 8, 4);

  auto *listBox = listRow->makeChild<ListBox>();
  listBox->addItem("Apple");
  listBox->addItem("Banana");
  listBox->addItem("Cherry");
  listBox->addItem("Date");
  listBox->addItem("Elderberry");
  listBox->addItem("Fig");
  listBox->addItem("Grape");

  auto *comboBox = listRow->makeChild<ComboBox>();
  comboBox->addItem("Small");
  comboBox->addItem("Medium");
  comboBox->addItem("Large");
  comboBox->setCurrentIndex(1);

  listRowLayout->addStretch(1);
  listRow->setLayout(std::move(listRowLayout));

  // Table + Tree row
  auto *tableRow = root->makeChild<Widget>();
  tableRow->style().bgColor = Color::Transparent;
  auto tableRowLayout = std::make_unique<BoxLayout>(BoxLayout::LeftToRight, 8, 4);

  auto *table = tableRow->makeChild<TableView>();
  table->addColumn(TableColumn{"Name", 150, 60, true, true});
  table->addColumn(TableColumn{"Value", 90, 50, true, true});
  auto tmodel = std::make_shared<SimpleTableModel>(0, 2);
  tmodel->addRow({"One", "1"});
  tmodel->addRow({"Two", "2"});
  tmodel->addRow({"Three", "3"});
  table->setModel(tmodel);

  auto *tree = tableRow->makeChild<TreeView>();
  auto *catA = tree->rootItem()->addChild("Section A");
  catA->setExpanded(true);
  catA->addChild("Item 1");
  catA->addChild("Item 2");
  auto *catB = tree->rootItem()->addChild("Section B");
  catB->addChild("Item 3");

  tableRowLayout->addStretch(1);
  tableRowLayout->addStretch(1);
  tableRow->setLayout(std::move(tableRowLayout));

  // Tab row: ProgressBar + long scroll list in two tabs
  auto *tabs = root->makeChild<TabWidget>();
  tabs->addTab("Progress");
  tabs->addTab("Scroller");

  auto *progressPage = tabs->tabContent(0);
  auto *progress = progressPage->makeChild<ProgressBar>();
  progress->setValue(42);
  auto *progressLabel = progressPage->makeChild<Label>("42/100");
  progress->onValueChanged.connect([&](int v) {
    progressLabel->setText(std::to_string(v) + "/100");
  });
  auto *progressBtn = progressPage->makeChild<Button>("Increment");
  progressBtn->onClicked.connect(
      [&]() { progress->setValue(progress->value() + 10); });
  auto progressLayout = std::make_unique<BoxLayout>(BoxLayout::TopToBottom, 4, 4);
  progressPage->setLayout(std::move(progressLayout));

  auto *scrollPage = tabs->tabContent(1);
  auto *area = scrollPage->makeChild<ScrollArea>();
  auto longContent = std::make_unique<Widget>();
  longContent->style().bgColor = currentTheme().bgSecondary;
  auto longLayout = std::make_unique<BoxLayout>(BoxLayout::TopToBottom, 2, 2);
  for (int i = 0; i < 25; i++)
    longContent->makeChild<Label>("Scroll row " + std::to_string(i + 1));
  longLayout->addStretch(1);
  longContent->setLayout(std::move(longLayout));
  area->setWidget(std::move(longContent));
  auto scrollLayout = std::make_unique<BoxLayout>(BoxLayout::TopToBottom, 0, 0);
  scrollPage->setLayout(std::move(scrollLayout));

  // Stretch factors — the last one absorbs the leftover space
  mainLayout->addStretch(1);

  root->setLayout(std::move(mainLayout));
  window.setCentralWidget(std::move(root));
  window.show();

  return Application::instance().run();
}
