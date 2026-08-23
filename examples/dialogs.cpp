// Dialog example: MessageBox, InputDialog, and a custom Dialog subclass.
#include "ltgui.h"
#include <cstring>
#include <iostream>

using namespace ltgui;

// Custom dialog: content is built on the protected panel_ in the constructor.
class NameDialog : public Dialog {
public:
  explicit NameDialog(Widget *parent = nullptr) : Dialog(parent) {
    panelW_ = 360;
    panelH_ = 150;
    panel_ = makeChild<Widget>();
    panel_->style().bgColor = Color::Transparent;
    panel_->style().borderWidth = 0;
    panel_->setStyle(Style::defaultStyle());
    panel_->style().bgColor = currentTheme().dialogBg;
    panel_->style().borderWidth = 1;
    panel_->style().borderRadius = 6;

    auto layout = std::make_unique<BoxLayout>(BoxLayout::TopToBottom, 8, 12);

    auto *title = panel_->makeChild<Label>("Your name?");
    title->style().font = Font("Segoe UI", 16, FontWeight::Bold);

    name_ = panel_->makeChild<TextBox>("Ada");

    auto *btnRow = panel_->makeChild<Widget>();
    btnRow->style().bgColor = Color::Transparent;
    auto rowLayout = std::make_unique<BoxLayout>(BoxLayout::LeftToRight, 8, 0);
    auto *ok = btnRow->makeChild<Button>("OK");
    ok->onClicked.connect([this]() { done(DialogResult::OK); });
    auto *cancel = btnRow->makeChild<Button>("Cancel");
    cancel->onClicked.connect([this]() { done(DialogResult::Cancel); });
    rowLayout->addStretch(1);
    btnRow->setLayout(std::move(rowLayout));
    layout->addStretch(0);

    panel_->setLayout(std::move(layout));
  }

  std::string name() const { return name_->text(); }

private:
  TextBox *name_ = nullptr;
};

int main(int argc, char *argv[]) {
  for (int i = 1; i < argc; i++) {
    if (std::strcmp(argv[i], "--debug") == 0)
      Logger::instance().setGlobalDebug(true);
  }

  Window window;
  if (!window.create(500, 380, "ltgui Dialogs")) {
    std::cerr << "Failed to create window." << std::endl;
    return 1;
  }

  auto root = std::make_unique<Widget>();
  root->style().bgColor = currentTheme().bgPrimary;
  auto layout = std::make_unique<BoxLayout>(BoxLayout::TopToBottom, 16, 12);

  auto *msgBtn = root->makeChild<Button>("MessageBox (OK)");
  auto *askBtn = root->makeChild<Button>("MessageBox (Yes/No)");
  auto *inputBtn = root->makeChild<Button>("InputDialog");
  auto *customBtn = root->makeChild<Button>("Custom Dialog");
  auto *resultLabel = root->makeChild<Label>("Result: <none>");
  resultLabel->style().fgColor = currentTheme().accent;

  msgBtn->onClicked.connect([&]() {
    DialogResult r = MessageBox::show(
        msgBtn, "Information", "This is a static message box.",
        static_cast<int>(DialogButton::OK), MessageBox::Icon::Info);
    resultLabel->setText("Result: " + std::to_string(static_cast<int>(r)));
  });

  askBtn->onClicked.connect([&]() {
    DialogResult r = MessageBox::show(
        msgBtn, "Question", "Do you really want to quit?",
        static_cast<int>(DialogButton::Yes | DialogButton::No),
        MessageBox::Icon::Question);
    resultLabel->setText(r == DialogResult::Yes ? "Result: Yes" : "Result: No");
  });

  inputBtn->onClicked.connect([&]() {
    std::string answer = InputDialog::getText(
        msgBtn, "Input", "Enter your name:", "Ada");
    resultLabel->setText("Input: " + answer);
  });

  customBtn->onClicked.connect([&]() {
    NameDialog dlg(msgBtn);
    DialogResult r = dlg.exec();
    resultLabel->setText(
        r == DialogResult::OK ? "Custom: OK -> " + dlg.name() : "Custom: Cancel");
  });

  layout->addStretch(0);
  layout->addStretch(0);

  root->setLayout(std::move(layout));
  window.setCentralWidget(std::move(root));
  window.show();

  return Application::instance().run();
}
