#include "widgets/textbox.h"
#include "platform/native_canvas.h"
#include "platform/native_window.h"
#include "theme.h"
#include "utf8.h"
#include "window.h"
#include <algorithm>
#include <sstream>

namespace ltgui {

namespace {

// Returns the next UTF-8 character boundary at or after `pos`, clamped to
// s.size(). Advances past the character a position points into — including
// positions inside a multi-byte sequence — so the caret never sits in the
// middle of a glyph. (utf8::nextPos() treats a continuation byte start as a
// one-byte char and thus needs this wrapper for mid-sequence positions.)
int nextBoundary(const std::string &s, int pos) {
  int len = static_cast<int>(s.size());
  if (pos < 0 || pos >= len)
    return len;
  int p = pos;
  // Skip any continuation bytes the position points into.
  while (p < len && (static_cast<unsigned char>(s[p]) & 0xC0) == 0x80)
    ++p;
  if (p >= len)
    return len;
  // Step over the character itself.
  p += utf8::codePointLen(static_cast<unsigned char>(s[p]));
  return p < len ? p : len;
}

} // namespace

TextBox::TextBox(const std::string &text, Widget *parent)
    : TextWidget(text, parent), cursorPos_(static_cast<int>(text.size())) {}

void TextBox::clampCursorPos() { cursorPos_ = nextBoundary(text_, cursorPos_); }

void TextBox::setText(const std::string &text) {
  text_ = text;
  cursorPos_ = static_cast<int>(text.size());
  selectionStart_ = -1;
  pendingHighSurrogate_ = 0;
  undoStack_.clear();
  redoStack_.clear();
  invalidateSizeHint();
  scheduleRelayout();
  update();
  onTextChanged.emit(text_);
}

void TextBox::setMultiLine(bool multiLine) {
  multiLine_ = multiLine;
  invalidateSizeHint();
  update();
}

Size TextBox::sizeHint() const {
  if (!sizeHintDirty())
    return cachedSizeHint();
  // Apply DPI scale to the default size
  if (multiLine_) {
    setCachedSizeHint(dpiScaleSize(150, 80));
  } else {
    setCachedSizeHint(dpiScaleSize(150, 30));
  }
  return cachedSizeHint();
}

void TextBox::pushUndo() {
  if (undoing_)
    return;
  UndoEntry entry{text_, cursorPos_, selectionStart_};
  if (!undoStack_.empty() && undoStack_.back() == entry)
    return;
  undoStack_.push_back(entry);
  redoStack_.clear();
  // Limit undo stack to 100 entries
  if (undoStack_.size() > 100)
    undoStack_.erase(undoStack_.begin());
}

void TextBox::undo() {
  if (undoStack_.empty())
    return;
  undoing_ = true;
  redoStack_.push_back({text_, cursorPos_, selectionStart_});
  UndoEntry entry = undoStack_.back();
  undoStack_.pop_back();
  text_ = entry.text;
  cursorPos_ = entry.cursorPos;
  selectionStart_ = entry.selectionStart;
  undoing_ = false;
  update();
  onTextChanged.emit(text_);
}

void TextBox::redo() {
  if (redoStack_.empty())
    return;
  undoing_ = true;
  undoStack_.push_back({text_, cursorPos_, selectionStart_});
  UndoEntry entry = redoStack_.back();
  redoStack_.pop_back();
  text_ = entry.text;
  cursorPos_ = entry.cursorPos;
  selectionStart_ = entry.selectionStart;
  undoing_ = false;
  update();
  onTextChanged.emit(text_);
}

void TextBox::deleteSelection() {
  if (selectionStart_ < 0)
    return;
  int selStart = std::clamp(std::min(cursorPos_, selectionStart_), 0,
                            static_cast<int>(text_.size()));
  int selEnd = std::clamp(std::max(cursorPos_, selectionStart_), 0,
                          static_cast<int>(text_.size()));
  if (selEnd <= selStart) {
    selectionStart_ = -1;
    return;
  }
  text_.erase(selStart, selEnd - selStart);
  cursorPos_ = selStart;
  selectionStart_ = -1;
}

std::string TextBox::selectedText() const {
  if (selectionStart_ < 0)
    return {};
  int selStart = std::clamp(std::min(cursorPos_, selectionStart_), 0,
                            static_cast<int>(text_.size()));
  int selEnd = std::clamp(std::max(cursorPos_, selectionStart_), 0,
                          static_cast<int>(text_.size()));
  if (selEnd <= selStart)
    return {};
  return text_.substr(selStart, selEnd - selStart);
}

void TextBox::insertText(const std::string &str) {
  clampCursorPos();
  pushUndo();
  deleteSelection();
  text_.insert(cursorPos_, str);
  cursorPos_ += static_cast<int>(str.size());
  invalidateSizeHint();
  update();
  onTextChanged.emit(text_);
}

void TextBox::deleteCharBefore() {
  pushUndo();
  if (selectionStart_ >= 0) {
    deleteSelection();
    update();
    onTextChanged.emit(text_);
    return;
  }
  if (cursorPos_ > 0) {
    int prev = utf8::prevPos(text_, cursorPos_);
    text_.erase(prev, cursorPos_ - prev);
    cursorPos_ = prev;
    update();
    onTextChanged.emit(text_);
  }
}

void TextBox::deleteCharAt() {
  clampCursorPos();
  pushUndo();
  if (selectionStart_ >= 0) {
    deleteSelection();
    update();
    onTextChanged.emit(text_);
    return;
  }
  if (cursorPos_ < static_cast<int>(text_.size())) {
    int nxt = utf8::nextPos(text_, cursorPos_);
    text_.erase(cursorPos_, nxt - cursorPos_);
    update();
    onTextChanged.emit(text_);
  }
}

void TextBox::moveCursorByChar(int delta, bool preserveSelection) {
  if (delta < 0) {
    cursorPos_ = utf8::prevPos(text_, cursorPos_);
  } else if (delta > 0) {
    cursorPos_ = utf8::nextPos(text_, cursorPos_);
  }
  if (!preserveSelection)
    selectionStart_ = -1;
  update();
}

void TextBox::moveCursorByLine(int delta, bool preserveSelection) {
  if (!multiLine_) {
    // In single-line mode, up/down = home/end
    if (delta < 0)
      cursorPos_ = 0;
    else
      cursorPos_ = static_cast<int>(text_.size());
    if (!preserveSelection)
      selectionStart_ = -1;
    update();
    return;
  }
  // Multi-line: find current line and move up/down
  auto ls = lines();
  int curLine = -1, offset = 0;
  for (int i = 0; i < static_cast<int>(ls.size()); i++) {
    int lineLen = static_cast<int>(ls[i].size());
    if (cursorPos_ >= offset && cursorPos_ <= offset + lineLen) {
      curLine = i;
      break;
    }
    offset += lineLen + 1; // +1 for newline
  }
  if (curLine < 0)
    return;

  int targetLine = curLine + delta;
  if (targetLine < 0 || targetLine >= static_cast<int>(ls.size()))
    return;

  // Find byte offset of target line start
  int targetOffset = 0;
  for (int i = 0; i < targetLine; i++) {
    targetOffset += static_cast<int>(ls[i].size()) + 1;
  }
  // Clamp to target line length
  int colOffset = cursorPos_ - offset;
  if (colOffset < 0)
    colOffset = 0;
  int maxCol = static_cast<int>(ls[targetLine].size());
  if (colOffset > maxCol)
    colOffset = maxCol;
  cursorPos_ = targetOffset + colOffset;
  if (!preserveSelection)
    selectionStart_ = -1;
  update();
}

std::vector<std::string> TextBox::lines() const {
  std::vector<std::string> result;
  std::istringstream ss(text_);
  std::string line;
  while (std::getline(ss, line)) {
    result.push_back(line);
  }
  // Handle trailing newline
  if (!text_.empty() && text_.back() == '\n') {
    result.push_back("");
  }
  if (result.empty())
    result.push_back("");
  return result;
}

int TextBox::totalLines() const { return static_cast<int>(lines().size()); }

// --- Clipboard (uses NativeWindow abstraction) ---

void TextBox::copy() {
  std::string sel = selectedText();
  if (sel.empty())
    sel = text_; // copy all if no selection
  if (!sel.empty()) {
    if (auto *w = window()) {
      if (auto *nw = w->nativeWindow()) {
        nw->setClipboardText(sel);
      }
    }
  }
}

void TextBox::cut() {
  std::string sel = selectedText();
  if (sel.empty())
    sel = text_; // cut all if no selection
  if (!sel.empty()) {
    if (auto *w = window()) {
      if (auto *nw = w->nativeWindow()) {
        nw->setClipboardText(sel);
      }
    }
    pushUndo();
    if (selectionStart_ >= 0) {
      deleteSelection();
    } else {
      text_.clear();
      cursorPos_ = 0;
    }
    selectionStart_ = -1;
    update();
    onTextChanged.emit(text_);
  }
}

void TextBox::paste() {
  if (auto *w = window()) {
    if (auto *nw = w->nativeWindow()) {
      std::string clip = nw->getClipboardText();
      if (!clip.empty()) {
        insertText(clip);
      }
    }
  }
}

void TextBox::selectAll() {
  selectionStart_ = 0;
  cursorPos_ = static_cast<int>(text_.size());
  update();
}

void TextBox::paintSelf(NativeCanvas *canvas) {
  Rect r = absoluteRect();
  ResolvedStyle st = resolvedStyle();
  const Theme &t = currentTheme();

  canvas->setColor(st.bgColor);
  canvas->fillRoundedRect(r, st.borderRadius);

  // Focus border uses the theme's dedicated borderFocus color.
  Color borderColor = focused_ ? t.borderFocus : st.borderColor;
  canvas->setColor(borderColor);
  canvas->strokeRoundedRect(r, st.borderRadius, focused_ ? 2 : st.borderWidth);

  // Set font early — required before any measureText() call
  canvas->setFont(st.font);

  int pad = st.paddingLeft;
  Rect textRect(r.x + pad, r.y, r.width - pad * 2, r.height);

  // Defensive: alignment edits may have left a stale cursor — normalize
  // before any substr()/insert() below.
  clampCursorPos();

  // Build display text: combine IME preedit with actual text
  std::string displayText = text_;
  int displayCursor = cursorPos_;
  int imeLen = static_cast<int>(imePreedit_.size());
  if (focused_ && imeLen > 0) {
    displayText.insert(displayCursor, imePreedit_);
    displayCursor = displayCursor + imeLen;
  }
  displayCursor = std::clamp(displayCursor, 0, static_cast<int>(displayText.size()));

  // Paint selection highlight
  if (focused_ && selectionStart_ >= 0 && selectionStart_ != cursorPos_) {
    int selStart = std::clamp(std::min(cursorPos_, selectionStart_), 0,
                              static_cast<int>(displayText.size()));
    int selEnd = std::clamp(std::max(cursorPos_, selectionStart_), 0,
                            static_cast<int>(displayText.size()));
    std::string before = displayText.substr(0, selStart);
    std::string sel = displayText.substr(selStart, selEnd - selStart);
    int selX = r.x + pad + canvas->measureText(before).width;
    int selW = canvas->measureText(sel).width;
    canvas->setColor(st.accent);
    canvas->fillRect(Rect(selX, r.y + 2, selW, r.height - 4));
  }

  // Paint IME preedit underline
  if (focused_ && imeLen > 0) {
    std::string before = text_.substr(0, cursorPos_);
    std::string preedit = imePreedit_;
    int preeditX = r.x + pad + canvas->measureText(before).width;
    int preeditW = canvas->measureText(preedit).width;
    // Draw underline for preedit text
    int underlineY = r.bottom() - 4;
    canvas->setColor(st.accent);
    canvas->drawLine({preeditX, underlineY}, {preeditX + preeditW, underlineY},
                     1);
  }

  canvas->setColor(st.fgColor);

  int flags = NativeCanvas::AlignLeft | NativeCanvas::AlignVCenter;
  if (!multiLine_)
    flags |= NativeCanvas::SingleLine;
  else
    flags |= NativeCanvas::WordWrap;

  if (!displayText.empty()) {
    canvas->drawText(displayText, textRect, flags);
  }

  if (focused_) {
    std::string before = displayText.substr(0, displayCursor);
    Size textBefore = canvas->measureText(before);
    int cursorX = r.x + pad + textBefore.width;

    canvas->setColor(st.accent);
    canvas->drawLine({cursorX, r.y + 5}, {cursorX, r.bottom() - 5}, 2);
  }
}

bool TextBox::handleEvent(Event &event) {
  if (!isEnabled())
    return false;

  switch (event.type) {
  case EventType::MouseDown: {
    focused_ = true;
    dragging_ = (event.button == MouseButton::Left);
    claimFocus();
    selectionStart_ = -1;
    int pad = style().paddingLeft;
    int clickX = event.pos.x - x() - pad;
    if (clickX <= 0 || text_.empty()) {
      cursorPos_ = 0;
    } else {
      auto *can = window() ? window()->canvas() : nullptr;
      if (can) {
        can->setFont(style().font);
        int lo = 0, hi = static_cast<int>(text_.size());
        while (lo < hi) {
          int mid = (lo + hi) / 2;
          int w = can->measureText(text_.substr(0, mid)).width;
          if (w < clickX)
            lo = mid + 1;
          else
            hi = mid;
        }
        // Align to the next character boundary so the caret never lands
        // inside a multi-byte UTF-8 sequence.
        cursorPos_ =
            (lo >= static_cast<int>(text_.size()))
                ? static_cast<int>(text_.size())
                : nextBoundary(text_, lo);
      } else {
        cursorPos_ = static_cast<int>(text_.size());
      }
    }
    update();
    event.accepted = true;
    return true;
  }
  case EventType::MouseMove: {
    if (dragging_ && focused_) {
      // Extend selection
      if (selectionStart_ < 0)
        selectionStart_ = cursorPos_;
      int pad = style().paddingLeft;
      int clickX = event.pos.x - x() - pad;
      auto *can = window() ? window()->canvas() : nullptr;
      if (can && !text_.empty()) {
        can->setFont(style().font);
        int lo = 0, hi = static_cast<int>(text_.size());
        while (lo < hi) {
          int mid = (lo + hi) / 2;
          int w = can->measureText(text_.substr(0, mid)).width;
          if (w < clickX)
            lo = mid + 1;
          else
            hi = mid;
        }
        // Align to the next character boundary (see MouseDown).
        int aligned =
            (lo >= static_cast<int>(text_.size()))
                ? static_cast<int>(text_.size())
                : nextBoundary(text_, lo);
        if (aligned != cursorPos_) {
          cursorPos_ = aligned;
          update();
        }
      }
      event.accepted = true;
      return true;
    }
    return false;
  }
  case EventType::MouseUp:
    dragging_ = false;
    event.accepted = true;
    return true;
  case EventType::FocusOut:
    focused_ = false;
    dragging_ = false;
    selectionStart_ = -1;
    imePreedit_.clear();
    pendingHighSurrogate_ = 0;
    update();
    event.accepted = true;
    return true;
  case EventType::ImeComposition: {
    // Update IME preedit display
    imePreedit_ = event.imeText;
    imeCursor_ = std::clamp(event.imeCursor, 0,
                            static_cast<int>(imePreedit_.size()));
    // Sanitize the insertion point before painting the preedit.
    clampCursorPos();
    update();
    event.accepted = true;
    return true;
  }
  case EventType::KeyDown:
    if (focused_) {
      // Check for Ctrl+key combinations
      bool ctrl = hasModifier(event.modifiers, KeyModifier::Control);
      bool shift = hasModifier(event.modifiers, KeyModifier::Shift);

      if (ctrl && event.key == Key::Z) {
        if (shift)
          redo();
        else
          undo();
        event.accepted = true;
        return true;
      }
      if (ctrl && event.key == Key::Y) {
        redo();
        event.accepted = true;
        return true;
      }
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

      // Handle printable characters (including IME-committed multi-byte UTF-8)
      if (event.charCode >= 32 && event.charCode != 127) {
        // Clear IME preedit when actual text arrives
        imePreedit_.clear();
        unsigned int cp = event.charCode;
        if (cp >= 0xD800 && cp <= 0xDBFF) {
          // High surrogate (e.g. emoji from Windows WM_CHAR surrogate
          // pairs) — defer insertion until the low half arrives.
          pendingHighSurrogate_ = cp;
        } else if (cp >= 0xDC00 && cp <= 0xDFFF && pendingHighSurrogate_ != 0) {
          // Combine the pending pair into a single code point.
          cp = 0x10000 + ((pendingHighSurrogate_ - 0xD800) << 10) +
               (cp - 0xDC00);
          pendingHighSurrogate_ = 0;
          insertText(utf8::encode(cp));
        } else {
          pendingHighSurrogate_ = 0;
          insertText(utf8::encode(cp));
        }
        event.accepted = true;
        return true;
      }
      switch (event.key) {
      case Key::Enter:
        if (multiLine_) {
          insertText("\n");
          event.accepted = true;
          return true;
        }
        break;
      case Key::Backspace:
        deleteCharBefore();
        event.accepted = true;
        return true;
      case Key::Delete:
        deleteCharAt();
        event.accepted = true;
        return true;
      case Key::Left:
        if (shift && selectionStart_ < 0)
          selectionStart_ = cursorPos_;
        moveCursorByChar(-1, shift);
        event.accepted = true;
        return true;
      case Key::Right:
        if (shift && selectionStart_ < 0)
          selectionStart_ = cursorPos_;
        moveCursorByChar(1, shift);
        event.accepted = true;
        return true;
      case Key::Up:
        if (shift && selectionStart_ < 0)
          selectionStart_ = cursorPos_;
        moveCursorByLine(-1, shift);
        event.accepted = true;
        return true;
      case Key::Down:
        if (shift && selectionStart_ < 0)
          selectionStart_ = cursorPos_;
        moveCursorByLine(1, shift);
        event.accepted = true;
        return true;
      case Key::Home:
        if (shift && selectionStart_ < 0)
          selectionStart_ = cursorPos_;
        cursorPos_ = 0;
        if (!shift)
          selectionStart_ = -1;
        update();
        event.accepted = true;
        return true;
      case Key::End:
        if (shift && selectionStart_ < 0)
          selectionStart_ = cursorPos_;
        cursorPos_ = static_cast<int>(text_.size());
        if (!shift)
          selectionStart_ = -1;
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
