#include "widgets/textbox.h"
#include "window.h"
#include "theme.h"
#include "utf8.h"
#include "platform/native_canvas.h"
#include <algorithm>

namespace ltgui {

TextBox::TextBox(const std::string& text, Widget* parent)
    : Widget(parent), text_(text), cursorPos_(static_cast<int>(text.size())) {
    style().bgColor = currentTheme().bgSecondary;
    style().fgColor = currentTheme().textPrimary;
    style().borderWidth = 1;
    style().borderColor = currentTheme().border;
    style().borderRadius = 4;
    style().setPadding(8, 4);
}

void TextBox::setText(const std::string& text) {
    text_ = text;
    cursorPos_ = static_cast<int>(text.size());
    scrollOffset_ = 0;
    update();
    if (textChangedCallback_) textChangedCallback_(text_);
}

Size TextBox::sizeHint() const {
    if (!sizeHintDirty()) return cachedSizeHint();
    setCachedSizeHint({150, 30});
    return cachedSizeHint();
}

void TextBox::insertText(const std::string& str) {
    text_.insert(cursorPos_, str);
    cursorPos_ += static_cast<int>(str.size());
    update();
    if (textChangedCallback_) textChangedCallback_(text_);
}

void TextBox::deleteCharBefore() {
    if (cursorPos_ > 0) {
        int prev = utf8::prevPos(text_, cursorPos_);
        text_.erase(prev, cursorPos_ - prev);
        cursorPos_ = prev;
        update();
        if (textChangedCallback_) textChangedCallback_(text_);
    }
}

void TextBox::deleteCharAt() {
    if (cursorPos_ < static_cast<int>(text_.size())) {
        int nxt = utf8::nextPos(text_, cursorPos_);
        text_.erase(cursorPos_, nxt - cursorPos_);
        update();
        if (textChangedCallback_) textChangedCallback_(text_);
    }
}

void TextBox::moveCursorByChar(int delta) {
    if (delta < 0) {
        cursorPos_ = utf8::prevPos(text_, cursorPos_);
    } else if (delta > 0) {
        cursorPos_ = utf8::nextPos(text_, cursorPos_);
    }
    update();
}

void TextBox::paintSelf(NativeCanvas* canvas) {
    Rect r = absoluteRect();
    Theme t = currentTheme();

    canvas->setColor(style().bgColor);
    canvas->fillRoundedRect(r, style().borderRadius);

    Color borderColor = focused_ ? t.accent : style().borderColor;
    canvas->setColor(borderColor);
    canvas->strokeRoundedRect(r, style().borderRadius, focused_ ? 2 : style().borderWidth);

    canvas->setColor(isEnabled() ? style().fgColor : t.textDisabled);
    canvas->setFont(style().font);

    int pad = style().paddingLeft;
    Rect textRect(r.x + pad, r.y, r.width - pad * 2, r.height);

    if (!text_.empty()) {
        canvas->drawText(text_, textRect,
                         NativeCanvas::AlignLeft | NativeCanvas::AlignVCenter | NativeCanvas::SingleLine);
    }

    if (focused_) {
        std::string before = text_.substr(0, cursorPos_);
        Size textBefore = canvas->measureText(before);
        int cursorX = r.x + pad + textBefore.width;

        canvas->setColor(t.accent);
        canvas->drawLine({cursorX, r.y + 5}, {cursorX, r.bottom() - 5}, 2);
    }
}

bool TextBox::handleEvent(Event& event) {
    if (!isEnabled()) return false;

    switch (event.type) {
    case EventType::MouseDown: {
        focused_ = true;
        claimFocus();
        int pad = style().paddingLeft;
        int textBtnW = event.pos.x - x() - pad;
        if (textBtnW <= 0) {
            cursorPos_ = 0;
        } else {
            cursorPos_ = static_cast<int>(text_.size());
        }
        update();
        event.accepted = true;
        return true;
    }
    case EventType::FocusOut:
        focused_ = false;
        update();
        event.accepted = true;
        return true;
    case EventType::KeyDown:
        if (focused_) {
            if (event.charCode >= 32 && event.charCode != 127) {
                insertText(utf8::encode(event.charCode));
                event.accepted = true;
                return true;
            }
            switch (event.key) {
            case Key::Backspace: deleteCharBefore(); event.accepted = true; return true;
            case Key::Delete:    deleteCharAt();    event.accepted = true; return true;
            case Key::Left:      moveCursorByChar(-1); event.accepted = true; return true;
            case Key::Right:     moveCursorByChar(1);  event.accepted = true; return true;
            case Key::Home:      cursorPos_ = 0; update(); event.accepted = true; return true;
            case Key::End:       cursorPos_ = static_cast<int>(text_.size()); update(); event.accepted = true; return true;
            default: break;
            }
        }
        break;
    default:
        break;
    }
    return false;
}

} // namespace ltgui
