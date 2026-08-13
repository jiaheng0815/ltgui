#pragma once
#include "widget.h"
#include "signal.h"

namespace ltgui {

// Base class for widgets with an integer value bounded to [minimum, maximum].
// setValue() clamps to the range and emits onValueChanged on actual change.
// Shared by Slider and ProgressBar.
class Range : public Widget {
public:
    explicit Range(Widget* parent = nullptr) : Widget(parent) {}

    int value() const { return value_; }
    virtual void setValue(int value);

    int minimum() const { return min_; }
    int maximum() const { return max_; }
    virtual void setRange(int min, int max);

    // Emitted whenever value() changes (after clamping).
    Signal<int> onValueChanged;

protected:
    int min_ = 0;
    int max_ = 100;
    int value_ = 0;
};

} // namespace ltgui
