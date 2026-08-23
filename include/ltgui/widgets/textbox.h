#pragma once
#include "signal.hpp"
#include "widgets/textwidget.h"
#include <vector>

namespace ltgui {

class TextBox : public TextWidget {
public:
  explicit TextBox(const std::string &text = "", Widget *parent = nullptr);

  void setText(const std::string &text) override;

  LTGUI_DECLARE_WIDGET_TYPE(TextBox)
  bool canAcceptFocus() const override { return true; }
  Size sizeHint() const override;

  // Multi-line mode
  void setMultiLine(bool multiLine);
  bool isMultiLine() const { return multiLine_; }

  // Clipboard
  void copy();
  void cut();
  void paste();
  void selectAll();

  // Undo/Redo
  void undo();
  void redo();
  bool canUndo() const { return !undoStack_.empty(); }
  bool canRedo() const { return !redoStack_.empty(); }

  // Emitted whenever the text content changes (programmatic or user input).
  Signal<const std::string &> onTextChanged;

protected:
  void paintSelf(NativeCanvas *canvas) override;
  bool handleEvent(Event &event) override;

private:
  int cursorPos_ = 0;
  int selectionStart_ = -1; // -1 = no selection
  bool focused_ = false;
  bool dragging_ = false;
  bool multiLine_ = false;

  // IME composition state
  std::string imePreedit_;
  int imeCursor_ = 0;

  // Pending high UTF-16 surrogate from a preceding charCode event
  // (Windows WM_CHAR delivers emoji as surrogate pairs); combined with
  // the following low surrogate into a single code point.
  unsigned int pendingHighSurrogate_ = 0;

  // Undo/Redo stack
  struct UndoEntry {
    std::string text;
    int cursorPos;
    int selectionStart;
    bool operator==(const UndoEntry &o) const {
      return text == o.text && cursorPos == o.cursorPos &&
             selectionStart == o.selectionStart;
    }
  };
  std::vector<UndoEntry> undoStack_;
  std::vector<UndoEntry> redoStack_;
  bool undoing_ = false;

  void pushUndo();
  void insertText(const std::string &str);
  void deleteCharBefore();
  void deleteCharAt();
  void moveCursorByChar(int delta, bool preserveSelection = false);
  void moveCursorByLine(int delta, bool preserveSelection = false);
  void deleteSelection();
  std::string selectedText() const;

  // Clamps cursorPos_ into [0, text_.size()] and aligns it to a UTF-8
  // character boundary (or text end).
  void clampCursorPos();

  // Line handling for multi-line
  std::vector<std::string> lines() const;
  int totalLines() const;
};

} // namespace ltgui
