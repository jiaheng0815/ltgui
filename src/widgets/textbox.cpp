#include "widgets/textbox.h"
#include "window.h"
#include "theme.h"
#include "utf8.h"
#include "platform/native_canvas.h"
#include <algorithm>

#ifdef LTGUI_PLATFORM_WINDOWS
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#endif

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
    selectionStart_ = -1;
    scrollOffset_ = 0;
    update();
    if (textChangedCallback_) textChangedCallback_(text_);
}

Size TextBox::sizeHint() const {
    if (!sizeHintDirty()) return cachedSizeHint();
    setCachedSizeHint({150, 30});
    return cachedSizeHint();
}

void TextBox::deleteSelection() {
    if (selectionStart_ < 0) return;
    int selStart = std::min(cursorPos_, selectionStart_);
    int selEnd   = std::max(cursorPos_, selectionStart_);
    text_.erase(selStart, selEnd - selStart);
    cursorPos_ = selStart;
    selectionStart_ = -1;
}

std::string TextBox::selectedText() const {
    if (selectionStart_ < 0) return {};
    int selStart = std::min(cursorPos_, selectionStart_);
    int selEnd   = std::max(cursorPos_, selectionStart_);
    return text_.substr(selStart, selEnd - selStart);
}

void TextBox::insertText(const std::string& str) {
    deleteSelection();
    text_.insert(cursorPos_, str);
    cursorPos_ += static_cast<int>(str.size());
    update();
    if (textChangedCallback_) textChangedCallback_(text_);
}

void TextBox::deleteCharBefore() {
    if (selectionStart_ >= 0) {
        deleteSelection();
        update();
        if (textChangedCallback_) textChangedCallback_(text_);
        return;
    }
    if (cursorPos_ > 0) {
        int prev = utf8::prevPos(text_, cursorPos_);
        text_.erase(prev, cursorPos_ - prev);
        cursorPos_ = prev;
        update();
        if (textChangedCallback_) textChangedCallback_(text_);
    }
}

void TextBox::deleteCharAt() {
    if (selectionStart_ >= 0) {
        deleteSelection();
        update();
        if (textChangedCallback_) textChangedCallback_(text_);
        return;
    }
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
    selectionStart_ = -1;
    update();
}

// --- Clipboard helpers ---

void TextBox::setClipboardText(const std::string& text) {
#ifdef LTGUI_PLATFORM_WINDOWS
    if (!OpenClipboard(nullptr)) return;
    EmptyClipboard();
    int wlen = MultiByteToWideChar(CP_UTF8, 0, text.c_str(), -1, nullptr, 0);
    HGLOBAL hMem = GlobalAlloc(GMEM_MOVEABLE, wlen * sizeof(wchar_t));
    if (hMem) {
        wchar_t* p = static_cast<wchar_t*>(GlobalLock(hMem));
        MultiByteToWideChar(CP_UTF8, 0, text.c_str(), -1, p, wlen);
        GlobalUnlock(hMem);
        SetClipboardData(CF_UNICODETEXT, hMem);
    }
    CloseClipboard();
#else
    (void)text;
#endif
}

std::string TextBox::getClipboardText() {
#ifdef LTGUI_PLATFORM_WINDOWS
    if (!OpenClipboard(nullptr)) return {};
    HANDLE hData = GetClipboardData(CF_UNICODETEXT);
    if (!hData) { CloseClipboard(); return {}; }
    wchar_t* p = static_cast<wchar_t*>(GlobalLock(hData));
    std::string result;
    if (p) {
        int len = WideCharToMultiByte(CP_UTF8, 0, p, -1, nullptr, 0, nullptr, nullptr);
        if (len > 0) {
            result.resize(len - 1);
            WideCharToMultiByte(CP_UTF8, 0, p, -1, &result[0], len, nullptr, nullptr);
        }
        GlobalUnlock(hData);
    }
    CloseClipboard();
    return result;
#else
    return {};
#endif
}

void TextBox::copy() {
    std::string sel = selectedText();
    if (sel.empty()) sel = text_; // copy all if no selection
    if (!sel.empty()) setClipboardText(sel);
}

void TextBox::cut() {
    std::string sel = selectedText();
    if (sel.empty()) sel = text_; // cut all if no selection
    if (!sel.empty()) {
        setClipboardText(sel);
        if (selectionStart_ >= 0) {
            deleteSelection();
        } else {
            text_.clear();
            cursorPos_ = 0;
        }
        selectionStart_ = -1;
        update();
        if (textChangedCallback_) textChangedCallback_(text_);
    }
}

void TextBox::paste() {
    std::string clip = getClipboardText();
    if (!clip.empty()) {
        insertText(clip);
    }
}

void TextBox::selectAll() {
    selectionStart_ = 0;
    cursorPos_ = static_cast<int>(text_.size());
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

    int pad = style().paddingLeft;
    Rect textRect(r.x + pad, r.y, r.width - pad * 2, r.height);

    // Paint selection highlight
    if (focused_ && selectionStart_ >= 0 && selectionStart_ != cursorPos_) {
        int selStart = std::min(cursorPos_, selectionStart_);
        int selEnd   = std::max(cursorPos_, selectionStart_);
        std::string before = text_.substr(0, selStart);
        std::string sel    = text_.substr(selStart, selEnd - selStart);
        int selX = r.x + pad + canvas->measureText(before).width;
        int selW = canvas->measureText(sel).width;
        canvas->setColor(t.accent);
        canvas->fillRect(Rect(selX, r.y + 2, selW, r.height - 4));
    }

    canvas->setColor(isEnabled() ? style().fgColor : t.textDisabled);
    canvas->setFont(style().font);

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
        selectionStart_ = -1;
        int pad = style().paddingLeft;
        int clickX = event.pos.x - x() - pad;
        if (clickX <= 0 || text_.empty()) {
            cursorPos_ = 0;
        } else {
            auto* can = window() ? window()->canvas() : nullptr;
            if (can) {
                int lo = 0, hi = static_cast<int>(text_.size());
                while (lo < hi) {
                    int mid = (lo + hi) / 2;
                    int w = can->measureText(text_.substr(0, mid)).width;
                    if (w < clickX) lo = mid + 1;
                    else hi = mid;
                }
                cursorPos_ = lo;
            } else {
                cursorPos_ = static_cast<int>(text_.size());
            }
        }
        update();
        event.accepted = true;
        return true;
    }
    case EventType::MouseMove: {
        if (event.button == MouseButton::Left && focused_) {
            // Extend selection
            if (selectionStart_ < 0) selectionStart_ = cursorPos_;
            int pad = style().paddingLeft;
            int clickX = event.pos.x - x() - pad;
            auto* can = window() ? window()->canvas() : nullptr;
            if (can && !text_.empty()) {
                int lo = 0, hi = static_cast<int>(text_.size());
                while (lo < hi) {
                    int mid = (lo + hi) / 2;
                    int w = can->measureText(text_.substr(0, mid)).width;
                    if (w < clickX) lo = mid + 1;
                    else hi = mid;
                }
                if (lo != cursorPos_) {
                    cursorPos_ = lo;
                    update();
                }
            }
            event.accepted = true;
            return true;
        }
        return false;
    }
    case EventType::FocusOut:
        focused_ = false;
        selectionStart_ = -1;
        update();
        event.accepted = true;
        return true;
    case EventType::KeyDown:
        if (focused_) {
            // Check for Ctrl+key combinations
            bool ctrl = (event.modifiers & 2) != 0; // Control modifier

            if (ctrl && event.key == Key::C) {
                copy();
                event.accepted = true;
                return true;
            }
            if (ctrl && event.key == Key::X) {
                cut();
                event.accepted = true;
                return true;
            }
            if (ctrl && event.key == Key::V) {
                paste();
                event.accepted = true;
                return true;
            }
            if (ctrl && event.key == Key::A) {
                selectAll();
                event.accepted = true;
                return true;
            }

            // Shift+arrow extends selection
            bool shift = (event.modifiers & 1) != 0;

            if (event.charCode >= 32 && event.charCode != 127) {
                insertText(utf8::encode(event.charCode));
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
                if (shift && selectionStart_ < 0) selectionStart_ = cursorPos_;
                moveCursorByChar(-1);
                if (!shift) selectionStart_ = -1;
                event.accepted = true;
                return true;
            case Key::Right:
                if (shift && selectionStart_ < 0) selectionStart_ = cursorPos_;
                moveCursorByChar(1);
                if (!shift) selectionStart_ = -1;
                event.accepted = true;
                return true;
            case Key::Home:
                if (shift && selectionStart_ < 0) selectionStart_ = cursorPos_;
                cursorPos_ = 0;
                if (!shift) selectionStart_ = -1;
                update();
                event.accepted = true;
                return true;
            case Key::End:
                if (shift && selectionStart_ < 0) selectionStart_ = cursorPos_;
                cursorPos_ = static_cast<int>(text_.size());
                if (!shift) selectionStart_ = -1;
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
    return false;
}

} // namespace ltgui
