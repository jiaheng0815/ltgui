#pragma once
#include "widget.h"
#include <functional>

namespace ltgui {

class Slider : public Widget {
public:
    explicit Slider(Widget* parent = nullptr);

    int value() const { return value_; }
    void setValue(int value);
    int minimum() const { return min_; }
    int maximum() const { return max_; }
    void setRange(int min, int max);

    using ValueChangedCallback = std::function<void(int)>;
    void onValueChanged(ValueChangedCallback cb) { valueChangedCallback_ = std::move(cb); }

    WidgetType widgetType() const override { return WidgetType::Slider; }
    Size sizeHint() const override;

protected:
    void paintSelf(NativeCanvas* canvas) override;
    bool handleEvent(Event& event) override;

private:
    int min_ = 0;
    int max_ = 100;
    int value_ = 0;
    bool dragging_ = false;
    bool hovered_ = false;
    ValueChangedCallback valueChangedCallback_;

    int thumbPos() const;
};

} // namespace ltgui
