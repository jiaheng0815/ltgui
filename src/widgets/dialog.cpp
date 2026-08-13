#include "widgets/dialog.h"
#include "widgets/button.h"
#include "widgets/label.h"
#include "widgets/textbox.h"
#include "window.h"
#include "layout.h"
#include "theme.h"
#include "app.h"
#include "animation.h"
#include "platform/native_canvas.h"

namespace ltgui {

// --- Dialog ---

Dialog::Dialog(Widget* parent) : Widget(parent) {
    style().bgColor = Color::Transparent;
    style().borderWidth = 0;
    setVisible(false);
}

Size Dialog::sizeHint() const {
    return {panelW_, panelH_};
}

void Dialog::addButton(const std::string& text, DialogResult res, bool isDefault) {
    if (!panel_) return;
    auto* btn = panel_->makeChild<Button>(text);
    btn->onClicked.connect([this, res]() { done(res); });
    if (isDefault) {
        btn->style().borderWidth = 2;
        btn->style().borderColor = currentTheme().accent;
    }
}

DialogResult Dialog::exec() {
    if (running_) return DialogResult::None;
    running_ = true;
    result_ = DialogResult::None;
    setVisible(true);
    fadeAnim_.setImmediate(0.0f);
    fadeAnim_.setTarget(1.0f, 150, Easing::EaseOut);
    claimFocus();

    auto* win = window();
    if (!win) {
        running_ = false;
        return DialogResult::None;
    }

    // Position the content panel once before entering the event loop,
    // instead of re-computing it every frame inside paintSelf().
    positionPanel();

    // Inner event loop: pump platform events and drive animations/timers.
    // Uses Application::pumpPlatformEvents() shared with run() and tick().
    while (running_) {
        Application::instance().processEvents();

        // Block with adaptive timeout: ~16ms during animations, 500ms idle.
        bool hasAnim = AnimationManager::instance().hasActive();
        int wakeMs = hasAnim ? 16 : 500;
        if (!Application::instance().pumpPlatformEvents(wakeMs)) {
            running_ = false;
            break;
        }

        // Drive the fade-in animation so the overlay appears immediately.
        fadeAnim_.value();
    }

    setVisible(false);
    return result_;
}

void Dialog::done(DialogResult result) {
    result_ = result;
    running_ = false;
    onFinished.emit(result);
    update();
}

void Dialog::positionPanel() {
    int ww = 640, wh = 480;
    if (auto* win = window()) {
        Size sz = win->size();
        ww = sz.width; wh = sz.height;
    }
    if (panel_) {
        int px = (ww - panelW_) / 2;
        int py = (wh - panelH_) / 2;
        panel_->setGeometry(Rect(px + 12, py + (title_.empty() ? 12 : 36),
                                 panelW_ - 24, panelH_ - (title_.empty() ? 24 : 48)));
    }
}

void Dialog::paintSelf(NativeCanvas* canvas) {
    float alpha = fadeAnim_.value();
    if (alpha <= 0.0f) return;

    int ww = 640, wh = 480;
    if (auto* win = window()) {
        Size sz = win->size();
        ww = sz.width; wh = sz.height;
    }

    Color overlay(0, 0, 0, static_cast<uint8_t>(alpha * 100));
    canvas->setColor(overlay);
    canvas->fillRect(Rect(0, 0, ww, wh));

    int px = (ww - panelW_) / 2;
    int py = (wh - panelH_) / 2;
    Rect panelRect(px, py, panelW_, panelH_);

    const Theme& t = currentTheme();
    canvas->setColor(t.dialogBg);
    canvas->fillRoundedRect(panelRect, 8);
    canvas->setColor(t.dialogBorder);
    canvas->strokeRoundedRect(panelRect, 8, 1);

    if (!title_.empty()) {
        canvas->setColor(t.dialogTitleBg);
        canvas->fillRoundedRect(Rect(px, py, panelW_, 32), 8);
        canvas->setColor(t.dialogTitleBg);
        canvas->fillRect(Rect(px, py + 24, panelW_, 8));
        canvas->setColor(t.textPrimary);
        canvas->setFont(style().font);
        canvas->drawText(title_, Rect(px + 12, py, panelW_ - 24, 32),
                         NativeCanvas::AlignLeft | NativeCanvas::AlignVCenter);
    }
}

bool Dialog::handleEvent(Event& event) {
    if (!running_) return false;
    if (event.type == EventType::KeyDown && event.key == Key::Escape) {
        done(DialogResult::Cancel);
        return true;
    }
    return Widget::handleEvent(event);
}

// --- MessageBox ---

MessageBox::MessageBox(Widget* parent) : Dialog(parent) {
    rebuild();
}

void MessageBox::rebuild() {
    while (!children().empty())
        removeChild(children().back().get());
    panel_ = nullptr;

    panelW_ = 300;
    panelH_ = 130;
    panel_ = makeChild<Widget>();
    panel_->style().bgColor = Color::Transparent;
    panel_->style().borderWidth = 0;
    panel_->setLayout(std::make_unique<BoxLayout>(BoxLayout::TopToBottom, 8, 12));

    auto* msg = panel_->makeChild<Label>(message_);
    msg->style().fgColor = currentTheme().textSecondary;
    msg->style().setPadding(8, 8);

    auto* btnRow = panel_->makeChild<Widget>();
    btnRow->style().bgColor = Color::Transparent;
    auto bl = std::make_unique<BoxLayout>(BoxLayout::LeftToRight, 8, 0);
    bl->addStretch(1);
    btnRow->setLayout(std::move(bl));

    if (buttonFlags_ & static_cast<int>(DialogButton::OK)) {
        auto* b = btnRow->makeChild<Button>("OK");
        b->onClicked.connect([this]() { done(DialogResult::OK); });
    }
    if (buttonFlags_ & static_cast<int>(DialogButton::Cancel)) {
        auto* b = btnRow->makeChild<Button>("Cancel");
        b->onClicked.connect([this]() { done(DialogResult::Cancel); });
    }
    if (buttonFlags_ & static_cast<int>(DialogButton::Yes)) {
        auto* b = btnRow->makeChild<Button>("Yes");
        b->onClicked.connect([this]() { done(DialogResult::Yes); });
    }
    if (buttonFlags_ & static_cast<int>(DialogButton::No)) {
        auto* b = btnRow->makeChild<Button>("No");
        b->onClicked.connect([this]() { done(DialogResult::No); });
    }

    if (!message_.empty())
        panelW_ = std::max(300, std::min(500, (int)message_.size() * 8 + 80));
    panelH_ = 130;
}

void MessageBox::setButtons(int buttonFlags) {
    buttonFlags_ = buttonFlags;
    rebuild();
}

DialogResult MessageBox::show(Widget* parent, const std::string& title,
                               const std::string& message, int buttons, Icon icon) {
    (void)icon;
    MessageBox mb(parent);
    mb.setTitle(title);
    mb.setMessage(message);
    mb.setButtons(buttons);
    return mb.exec();
}

void MessageBox::paintSelf(NativeCanvas* canvas) {
    Dialog::paintSelf(canvas);
}

// --- InputDialog ---

InputDialog::InputDialog(Widget* parent) : Dialog(parent) {
    rebuild();
}

void InputDialog::rebuild() {
    while (!children().empty())
        removeChild(children().back().get());
    panel_ = nullptr;

    panelW_ = 400;
    panelH_ = 160;
    panel_ = makeChild<Widget>();
    panel_->style().bgColor = Color::Transparent;
    panel_->style().borderWidth = 0;
    panel_->setLayout(std::make_unique<BoxLayout>(BoxLayout::TopToBottom, 8, 12));

    auto* lbl = panel_->makeChild<Label>(label_);
    lbl->style().fgColor = currentTheme().textPrimary;

    input_ = panel_->makeChild<TextBox>("");
    input_->setMultiLine(false);

    auto* btnRow = panel_->makeChild<Widget>();
    btnRow->style().bgColor = Color::Transparent;
    auto bl = std::make_unique<BoxLayout>(BoxLayout::LeftToRight, 8, 0);
    bl->addStretch(1);
    btnRow->setLayout(std::move(bl));

    auto* okBtn = btnRow->makeChild<Button>("OK");
    okBtn->onClicked.connect([this]() { done(DialogResult::OK); });
    auto* cancelBtn = btnRow->makeChild<Button>("Cancel");
    cancelBtn->onClicked.connect([this]() { done(DialogResult::Cancel); });
}

void InputDialog::setText(const std::string& text) {
    if (input_) input_->setText(text);
}

std::string InputDialog::text() const {
    return input_ ? input_->text() : "";
}

std::string InputDialog::getText(Widget* parent, const std::string& title,
                                  const std::string& label, const std::string& defaultText) {
    InputDialog dlg(parent);
    dlg.setTitle(title);
    dlg.setLabel(label);
    dlg.setText(defaultText);
    if (dlg.exec() == DialogResult::OK) return dlg.text();
    return "";
}

bool InputDialog::handleEvent(Event& event) {
    if (event.type == EventType::KeyDown && event.key == Key::Enter) {
        done(DialogResult::OK);
        return true;
    }
    return Dialog::handleEvent(event);
}

} // namespace ltgui
