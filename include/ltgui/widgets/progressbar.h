#pragma once
#include "widget.h"
#include "animation.h"

namespace ltgui {

class ProgressBar : public Widget {
public:
    explicit ProgressBar(Widget* parent = nullptr);

    int value() const { return value_; }
    void setValue(int value);

    int minimum() const { return min_; }
    int maximum() const { return max_; }
    void setRange(int min, int max);

    bool indeterminate() const { return indeterminate_; }
    void setIndeterminate(bool on);

    Size sizeHint() const override;

protected:
    void paintSelf(NativeCanvas* canvas) override;

private:
    int min_ = 0;
    int max_ = 100;
    int value_ = 0;
    bool indeterminate_ = false;
    AnimatedFloat displayValue_{0.0f};
    AnimatedFloat indeterminatePhase_{0.0f};
};

} // namespace ltgui
