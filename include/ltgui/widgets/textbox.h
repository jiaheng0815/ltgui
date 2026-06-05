#pragma once
#include "widget.h"
#include <string>
#include <vector>
#include <functional>

namespace ltgui {

class TextBox : public Widget {
public:
    explicit TextBox(const std::string& text = "", Widget* parent = nullptr);

    std::string text() const { return text_; }
    void setText(const std::string& text);

    WidgetType widgetType() const override { return WidgetType::TextBox; }
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
    bool multiLine_ = false;

    // IME composition state
    std::string imePreedit_;
    int imeCursor_ = 0;

    TextChangedCallback textChangedCallback_;

    // Undo/Redo stack
    struct UndoEntry {
        std::string text;
        int cursorPos;
        int selectionStart;
        bool operator==(const UndoEntry& o) const {
            return text == o.text && cursorPos == o.cursorPos && selectionStart == o.selectionStart;
        }
    };
    std::vector<UndoEntry> undoStack_;
    std::vector<UndoEntry> redoStack_;
    bool undoing_ = false;

    void pushUndo();
    void insertText(const std::string& str);
    void deleteCharBefore();
    void deleteCharAt();
    void moveCursorByChar(int delta);
    void moveCursorByLine(int delta);
    void deleteSelection();
    std::string selectedText() const;

    // Line handling for multi-line
    std::vector<std::string> lines() const;
    int totalLines() const;
};

} // namespace ltgui
