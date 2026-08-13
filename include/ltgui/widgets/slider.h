#pragma once
#include "widgets/range.h"

namespace ltgui {

class Slider : public Range {
public:
    explicit Slider(Widget* parent = nullptr);

    LTGUI_DECLARE_WIDGET_TYPE(Slider)
    bool canAcceptFocus() const override { return true; }
    Size sizeHint() const override;

protected:
    void paintSelf(NativeCanvas* canvas) override;
    bool handleEvent(Event& event) override;

private:
    bool dragging_ = false;

    int thumbPos() const;
};

} // namespace ltgui
