#pragma once
#include "widget.h"
#include "animation.h"
#include <string>
#include <functional>

namespace ltgui {

class TextBox;

enum class DialogResult { None, OK, Cancel, Yes, No };

enum class DialogButton {
    OK     = 1,
    Cancel = 2,
    Yes    = 4,
    No     = 8
};
inline DialogButton operator|(DialogButton a, DialogButton b) {
    return static_cast<DialogButton>(static_cast<int>(a) | static_cast<int>(b));
}
inline int operator&(DialogButton a, DialogButton b) {
    return static_cast<int>(a) & static_cast<int>(b);
}

class Dialog : public Widget {
public:
    explicit Dialog(Widget* parent = nullptr);

    virtual DialogResult exec();
    void done(DialogResult result);
    DialogResult result() const { return result_; }

    void setTitle(const std::string& title) { title_ = title; }

    using ResultCallback = std::function<void(DialogResult)>;
    void onFinished(ResultCallback cb) { resultCallback_ = std::move(cb); }

    WidgetType widgetType() const override { return WidgetType::Dialog; }
    bool canAcceptFocus() const override { return true; }
    Size sizeHint() const override;

    // Allow Window to detect and redirect events to the active dialog
    bool isModal() const { return running_; }

protected:
    void paintSelf(NativeCanvas* canvas) override;
    bool handleEvent(Event& event) override;
    void positionPanel();

    void addButton(const std::string& text, DialogResult res, bool isDefault = false);

    std::string title_;
    DialogResult result_ = DialogResult::None;
    ResultCallback resultCallback_;
    bool running_ = false;
    AnimatedFloat fadeAnim_{0.0f};
    Widget* overlay_ = nullptr;
    Widget* panel_ = nullptr;
    int panelW_ = 360;
    int panelH_ = 140;
};

class MessageBox : public Dialog {
public:
    enum Icon { None, Info, Warning, Error, Question };

    explicit MessageBox(Widget* parent = nullptr);

    void setMessage(const std::string& msg) { message_ = msg; }
    void setIcon(Icon icon) { icon_ = icon; }
    void setButtons(int buttonFlags);

    static DialogResult show(Widget* parent,
                             const std::string& title,
                             const std::string& message,
                             int buttons = static_cast<int>(DialogButton::OK),
                             Icon icon = Icon::None);

    WidgetType widgetType() const override { return WidgetType::MessageBox; }
    bool canAcceptFocus() const override { return true; }

protected:
    void paintSelf(NativeCanvas* canvas) override;

private:
    std::string message_;
    Icon icon_ = Icon::None;
    int buttonFlags_ = 0;
    void rebuild();
};

class InputDialog : public Dialog {
public:
    explicit InputDialog(Widget* parent = nullptr);

    void setLabel(const std::string& label) { label_ = label; }
    void setText(const std::string& text);
    std::string text() const;

    static std::string getText(Widget* parent,
                               const std::string& title,
                               const std::string& label,
                               const std::string& defaultText = "");

    WidgetType widgetType() const override { return WidgetType::InputDialog; }
    bool canAcceptFocus() const override { return true; }

protected:
    bool handleEvent(Event& event) override;

private:
    std::string label_;
    TextBox* input_ = nullptr;
    void rebuild();
};

} // namespace ltgui
