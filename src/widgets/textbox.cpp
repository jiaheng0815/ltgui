#include "widgets/textbox.h"
#include "window.h"
#include "platform/native_canvas.h"
#include <algorithm>

namespace ltgui {

// --- UTF-8 helpers ---

static int utf8Len(unsigned char c) {
    if ((c & 0x80) == 0) return 1;
    if ((c & 0xE0) == 0xC0) return 2;
    if ((c & 0xF0) == 0xE0) return 3;
    if ((c & 0xF8) == 0xF0) return 4;
    return 1; // invalid byte, treat as single
}

static std::string encodeUtf8(unsigned int cp) {
    std::string out;
    if (cp < 0x80) {
        out += static_cast<char>(cp);
    } else if (cp < 0x800) {
        out += static_cast<char>(0xC0 | (cp >> 6));
        out += static_cast<char>(0x80 | (cp & 0x3F));
    } else if (cp < 0x10000) {
        out += static_cast<char>(0xE0 | (cp >> 12));
        out += static_cast<char>(0x80 | ((cp >> 6) & 0x3F));
        out += static_cast<char>(0x80 | (cp & 0x3F));
    } else if (cp < 0x110000) {
        out += static_cast<char>(0xF0 | (cp >> 18));
        out += static_cast<char>(0x80 | ((cp >> 12) & 0x3F));
        out += static_cast<char>(0x80 | ((cp >> 6) & 0x3F));
        out += static_cast<char>(0x80 | (cp & 0x3F));
    }
    return out;
}

static int prevCharPos(const std::string& s, int pos) {
    if (pos <= 0) return 0;
    int p = pos - 1;
    while (p > 0 && (static_cast<unsigned char>(s[p]) & 0xC0) == 0x80) p--;
    return p;
}

static int nextCharPos(const std::string& s, int pos) {
    int len = static_cast<int>(s.size());
    if (pos >= len) return len;
    return pos + utf8Len(static_cast<unsigned char>(s[pos]));
}

// --- TextBox ---

TextBox::TextBox(const std::string& text, Widget* parent)
    : Widget(parent), text_(text), cursorPos_(static_cast<int>(text.size())) {
    style().bgColor = Color::White;
    style().borderWidth = 1;
    style().borderColor = Color::Gray;
    style().borderRadius = 2;
    style().setPadding(4);
}

void TextBox::setText(const std::string& text) {
    text_ = text;
    cursorPos_ = static_cast<int>(text.size());
    scrollOffset_ = 0;
    update();
    if (textChangedCallback_) textChangedCallback_(text_);
}

Size TextBox::sizeHint() const {
    return {150, 28};
}

int TextBox::visibleCursorPos() const {
    return cursorPos_ - scrollOffset_;
}

void TextBox::insertText(const std::string& str) {
    text_.insert(cursorPos_, str);
    cursorPos_ += static_cast<int>(str.size());
    update();
    if (textChangedCallback_) textChangedCallback_(text_);
}

void TextBox::deleteCharBefore() {
    if (cursorPos_ > 0) {
        int prev = prevCharPos(text_, cursorPos_);
        text_.erase(prev, cursorPos_ - prev);
        cursorPos_ = prev;
        update();
        if (textChangedCallback_) textChangedCallback_(text_);
    }
}

void TextBox::deleteCharAt() {
    if (cursorPos_ < static_cast<int>(text_.size())) {
        int nxt = nextCharPos(text_, cursorPos_);
        text_.erase(cursorPos_, nxt - cursorPos_);
        update();
        if (textChangedCallback_) textChangedCallback_(text_);
    }
}

void TextBox::moveCursorByChar(int delta) {
    if (delta < 0) {
        cursorPos_ = prevCharPos(text_, cursorPos_);
    } else if (delta > 0) {
        cursorPos_ = nextCharPos(text_, cursorPos_);
    }
    update();
}

void TextBox::paintSelf(NativeCanvas* canvas) {
    Rect r = absoluteRect();

    // Background
    canvas->setColor(style().bgColor);
    canvas->fillRect(r);

    // Border
    if (style().borderWidth > 0) {
        canvas->setColor(focused_ ? Color::DarkBlue : style().borderColor);
        canvas->strokeRect(r, style().borderWidth);
    }

    // Text
    canvas->setColor(style().fgColor);
    canvas->setFont(style().font);

    int pad = style().paddingLeft;
    Rect textRect(r.x + pad, r.y + 3, r.width - pad * 2, r.height - 6);

    if (!text_.empty()) {
        canvas->drawText(text_, textRect,
                         NativeCanvas::AlignLeft | NativeCanvas::AlignVCenter | NativeCanvas::SingleLine);
    }

    // Cursor
    if (focused_) {
        std::string before = text_.substr(0, cursorPos_);
        Size textBefore = canvas->measureText(before);
        int cursorX = r.x + pad + textBefore.width;

        canvas->setColor(style().fgColor);
        canvas->drawLine({cursorX, r.y + 4}, {cursorX, r.bottom() - 4});
    }
}

bool TextBox::handleEvent(Event& event) {
    if (!isEnabled()) return false;

    int localX = event.pos.x - x();
    int localY = event.pos.y - y();

    switch (event.type) {
    case EventType::MouseDown: {
        focused_ = true;
        claimFocus();
        int pad = style().paddingLeft;
        if (localX < pad + 10) {
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
            // Printable characters (from WM_CHAR) — convert Unicode codepoint to UTF-8
            if (event.charCode >= 32 && event.charCode != 127) {
                insertText(encodeUtf8(event.charCode));
                event.accepted = true;
                return true;
            }
            switch (event.key) {
            case Key::Backspace:
                deleteCharBefore();
                event.accepted = true;
                return true;
            case Key::Delete:
                deleteCharAt();
                event.accepted = true;
                return true;
            case Key::Left:
                moveCursorByChar(-1);
                event.accepted = true;
                return true;
            case Key::Right:
                moveCursorByChar(1);
                event.accepted = true;
                return true;
            case Key::Home:
                cursorPos_ = 0;
                update();
                event.accepted = true;
                return true;
            case Key::End:
                cursorPos_ = static_cast<int>(text_.size());
                update();
                event.accepted = true;
                return true;
            default:
                break;
            }
        }
        break;
    default:
        break;
    }
    return Widget::handleEvent(event);
}

} // namespace ltgui
