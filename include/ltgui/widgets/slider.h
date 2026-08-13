#pragma once
#include "widgets/range.h"

namespace ltgui {

class Slider : public Range {
public:
    explicit Slider(Widget* parent = nullptr);

    WidgetType widgetType() const override { return WidgetType::Slider; }
    bool canAcceptFocus() const override { return true; }
    Size sizeHint() const override;

protected:
    void paintSelf(NativeCanvas* canvas) override;
    bool handleEvent(Event& event) override;

private:
    bool dragging_ = false;
    bool hovered_ = false;

    int thumbPos() const;
};

} // namespace ltgui
