#pragma once
#include "widgets/range.h"
#include "animation.h"

namespace ltgui {

class ProgressBar : public Range {
public:
    explicit ProgressBar(Widget* parent = nullptr);

    void setValue(int value) override;

    bool indeterminate() const { return indeterminate_; }
    void setIndeterminate(bool on);

    WidgetType widgetType() const override { return WidgetType::ProgressBar; }
    Size sizeHint() const override;

protected:
    void paintSelf(NativeCanvas* canvas) override;

private:
    bool indeterminate_ = false;
    AnimatedFloat displayValue_{0.0f};
    AnimatedFloat indeterminatePhase_{0.0f};
};

} // namespace ltgui
