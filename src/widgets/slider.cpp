#include "widgets/slider.h"
#include "platform/native_canvas.h"
#include <algorithm>

namespace ltgui {

Slider::Slider(Widget* parent) : Widget(parent) {
    style().bgColor = Color::Transparent;
}

void Slider::setValue(int value) {
    value = std::max(min_, std::min(max_, value));
    if (value_ != value) {
        value_ = value;
        update();
        if (valueChangedCallback_) valueChangedCallback_(value_);
    }
}

void Slider::setRange(int min, int max) {
    min_ = min;
    max_ = max;
    if (value_ < min_) value_ = min_;
    if (value_ > max_) value_ = max_;
    update();
}

Size Slider::sizeHint() const {
    return {150, 28};
}

int Slider::thumbPos() const {
    int range = max_ - min_;
    if (range == 0) return 0;
    int trackW = width() - 16;
    return 8 + (trackW * (value_ - min_)) / range;
}

void Slider::paintSelf(NativeCanvas* canvas) {
    Rect r = absoluteRect();

    // Track
    int trackY = r.y + r.height / 2 - 2;
    Rect trackRect(r.x + 8, trackY, r.width - 16, 4);
    canvas->setColor(Color::LightGray);
    canvas->fillRect(trackRect);
    canvas->setColor(Color::Gray);
    canvas->strokeRect(trackRect);

    // Filled portion
    int tpos = thumbPos();
    Rect filledRect(r.x + 8, trackY, tpos - 8, 4);
    canvas->setColor(Color(0, 120, 215));
    canvas->fillRect(filledRect);

    // Thumb
    int thumbSize = 14;
    Rect thumbRect(r.x + tpos - thumbSize / 2, r.y + (r.height - thumbSize) / 2, thumbSize, thumbSize);
    canvas->setColor(Color::ButtonFace);
    canvas->fillEllipse(thumbRect);
    canvas->setColor(Color::Gray);
    canvas->strokeEllipse(thumbRect);
}

bool Slider::handleEvent(Event& event) {
    if (!isEnabled()) return false;

    int localX = event.pos.x - x();
    int localY = event.pos.y - y();

    switch (event.type) {
    case EventType::MouseDown:
        if (event.button == MouseButton::Left) {
            dragging_ = true;
            // Calculate value from click position
            int trackW = width() - 16;
            int clickX = localX - 8;
            clickX = std::max(0, std::min(trackW, clickX));
            int range = max_ - min_;
            if (range > 0) {
                setValue(min_ + (clickX * range) / trackW);
            }
            event.accepted = true;
            return true;
        }
        break;
    case EventType::MouseUp:
        dragging_ = false;
        event.accepted = true;
        return true;
    case EventType::MouseMove:
        if (dragging_) {
            int trackW = width() - 16;
            int clickX = localX - 8;
            clickX = std::max(0, std::min(trackW, clickX));
            int range = max_ - min_;
            if (range > 0) {
                setValue(min_ + (clickX * range) / trackW);
            }
            event.accepted = true;
            return true;
        }
        break;
    default:
        break;
    }
    return Widget::handleEvent(event);
}

} // namespace ltgui
