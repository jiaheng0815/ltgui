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

    Size sizeHint() const override;

    using TextChangedCallback = std::function<void(const std::string&)>;
    void onTextChanged(TextChangedCallback cb) { textChangedCallback_ = std::move(cb); }

protected:
    void paintSelf(NativeCanvas* canvas) override;
    bool handleEvent(Event& event) override;

private:
    std::string text_;
    int cursorPos_ = 0;
    int scrollOffset_ = 0;
    bool focused_ = false;
    TextChangedCallback textChangedCallback_;

    void insertText(const std::string& str);
    void deleteCharBefore();
    void deleteCharAt();
    void moveCursorByChar(int delta);
    int visibleCursorPos() const;
};

} // namespace ltgui
