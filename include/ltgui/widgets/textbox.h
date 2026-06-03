#pragma once
#include "widget.h"
#include <string>
#include <functional>

namespace ltgui {

class TextBox : public Widget {
public:
    explicit TextBox(const std::string& text = "", Widget* parent = nullptr);

    std::string text() const { return text_; }
    void setText(const std::string& text);

    WidgetType widgetType() const override { return WidgetType::TextBox; }
    Size sizeHint() const override;

    // Clipboard
    void copy();
    void cut();
    void paste();
    void selectAll();

    using TextChangedCallback = std::function<void(const std::string&)>;
    void onTextChanged(TextChangedCallback cb) { textChangedCallback_ = std::move(cb); }

protected:
    void paintSelf(NativeCanvas* canvas) override;
    bool handleEvent(Event& event) override;

private:
    std::string text_;
    int cursorPos_ = 0;
    int selectionStart_ = -1; // -1 = no selection
    int scrollOffset_ = 0;
    bool focused_ = false;
    TextChangedCallback textChangedCallback_;

    void insertText(const std::string& str);
    void deleteCharBefore();
    void deleteCharAt();
    void moveCursorByChar(int delta);
    void deleteSelection();
    std::string selectedText() const;
    void setClipboardText(const std::string& text);
    std::string getClipboardText();
};

} // namespace ltgui
