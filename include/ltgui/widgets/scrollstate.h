#pragma once
#include "animation.h"
#include <algorithm>

namespace ltgui {

// Mixin for widgets with vertically scrolling item content (ListBox,
// TreeView). Provides the animated scroll offset, target clamping, and
// wheel handling shared by those widgets. NOT a Widget subclass — combine
// via multiple inheritance. Widgets whose scrolling differs too much
// (TableView's plain int offset, TextBox) keep their own implementation.
class ScrollState {
public:
    // Current animated scroll offset (in item rows).
    int scrollOffset() { return static_cast<int>(scrollAnim_.value()); }
    int scrollTarget() const { return scrollTarget_; }
    bool isScrolling() const { return scrollAnim_.isAnimating(); }

    // Animate toward `target`, clamped to [0, maxOffset].
    void setScrollTarget(int target, int maxOffset) {
        scrollTarget_ = std::max(0, std::min(maxOffset, target));
        scrollAnim_.setTarget(static_cast<float>(scrollTarget_), 200, Easing::EaseOut);
    }

    // Jump immediately (no animation).
    void setScrollImmediate(int offset, int maxOffset) {
        scrollTarget_ = std::max(0, std::min(maxOffset, offset));
        scrollAnim_.setImmediate(static_cast<float>(scrollTarget_));
    }

    // Mouse wheel: scroll by `pageRows` rows per wheel notch.
    void handleWheel(int wheelDelta, int maxOffset, int pageRows) {
        setScrollTarget(scrollTarget_ - wheelDelta * pageRows, maxOffset);
    }

protected:
    AnimatedFloat scrollAnim_{0.0f};
    int scrollTarget_ = 0;
};

} // namespace ltgui
