#pragma once
#include "widget.h"
#include <string>

namespace ltgui {

// Base class for widgets that display or edit a single text string.
// Provides the text_ storage, the text()/setText() pair, and a shared
// measureText-based sizeHint helper — this kills the duplicated
// "if (window()) { canvas()->setFont + measureText }" boilerplate that
// used to live in every text-bearing widget's sizeHint().
class TextWidget : public Widget {
public:
    explicit TextWidget(const std::string& text = "", Widget* parent = nullptr)
        : Widget(parent), text_(text) {}

    std::string text() const { return text_; }

    // Replaces the widget's text. Subclasses that need extra behaviour
    // (e.g. TextBox cursor/undo reset) override this.
    virtual void setText(const std::string& text) {
        text_ = text;
        invalidateSizeHint();
        scheduleRelayout();
        update();
    }

    // Measures text_ through the window canvas when available and returns
    // (text + extraW/H). Falls back to `fallback` when no window or canvas
    // is attached (headless tests, pre-layout). Safe to call from sizeHint().
    Size textSizeHint(Size fallback, int extraW = 0, int extraH = 0) const;

protected:
    std::string text_;
};

} // namespace ltgui
