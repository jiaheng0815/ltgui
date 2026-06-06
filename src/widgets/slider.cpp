#include "widgets/slider.h"
#include "window.h"
#include "theme.h"
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
    if (!sizeHintDirty()) return cachedSizeHint();
    float dpi = window() ? window()->dpiScale() : 1.0f;
    setCachedSizeHint({static_cast<int>(150 * dpi), static_cast<int>(28 * dpi)});
    return cachedSizeHint();
}

int Slider::thumbPos() const {
    int range = max_ - min_;
    if (range == 0) return 16;
    // Thumb center moves from 16 to width-16 (usable track = width - 32).
    // This keeps the 16px thumb fully within the widget bounds at both ends.
    int usableW = width() - 32;
    return 16 + (usableW * (value_ - min_)) / range;
}

void Slider::paintSelf(NativeCanvas* canvas) {
    Rect r = absoluteRect();
    Theme t = currentTheme();

    // Track spans from x+8 to x+width-8 (= width-16 wide)
    int trackW = r.width - 16;
    int trackX = r.x + 8;
    int trackY = r.y + r.height / 2 - 3;

    // Track background
    Rect trackRect(trackX, trackY, trackW, 6);
    canvas->setColor(t.bgTertiary);
    canvas->fillRoundedRect(trackRect, 3);

    // Filled portion — plain fillRect, no rounded corners.
    // tpos is widget-local (origin = widget left edge).  The track starts
    // at widget edge + 8, so the thumb centre relative to the track start
    // is tpos - 8.  At max value, fill the entire track.
    int tpos = thumbPos();
    int fillW = tpos - 8;
    if (fillW < 0) fillW = 0;
    if (value_ == max_) fillW = trackW;

    if (fillW > 0) {
        canvas->setColor(t.accent);
        canvas->fillRect(Rect(trackX, trackY, fillW, 6));
    }

    // Thumb (tpos is widget-local, convert to absolute)
    int thumbSize = 16;
    int thumbAbsX = r.x + tpos - thumbSize / 2;
    int thumbY = r.y + (r.height - thumbSize) / 2;
    Rect thumbRect(thumbAbsX, thumbY, thumbSize, thumbSize);

    canvas->setColor(t.bgSecondary);
    canvas->fillEllipse(thumbRect);
    canvas->setColor(hovered_ || dragging_ ? t.accent : t.border);
    canvas->strokeEllipse(thumbRect, 2);
}

bool Slider::handleEvent(Event& event) {
    if (!isEnabled()) return false;

    int localX = event.pos.x - x();

    // Helper: is the cursor over the thumb?
    auto isOverThumb = [&]() -> bool {
        int thumbSize = 16;
        int tpos = thumbPos();
        int thumbCenterX = x() + tpos;
        int thumbCenterY = y() + height() / 2;
        int dx = event.pos.x - thumbCenterX;
        int dy = event.pos.y - thumbCenterY;
        return (dx * dx + dy * dy) < (thumbSize * thumbSize / 2);
    };

    // Helper: snap value to click position on the track
    auto snapToTrack = [&]() {
        int trackW = width() - 16;
        int clickX = std::max(0, std::min(trackW, localX - 8));
        int range = max_ - min_;
        if (range > 0 && trackW > 0) setValue(min_ + (clickX * range) / trackW);
    };

    switch (event.type) {
    case EventType::MouseMove: {
        bool over = isOverThumb();
        if (over != hovered_) {
            hovered_ = over;
            update();
        }
        if (dragging_) {
            snapToTrack();
            event.accepted = true;
            return true;
        }
        // Only consume MouseMove when we actually did something
        if (over) {
            event.accepted = true;
            return true;
        }
        return false;
    }
    case EventType::MouseDown:
        if (event.button == MouseButton::Left) {
            claimFocus();
            if (isOverThumb()) {
                dragging_ = true;
                event.accepted = true;
                return true;
            }
            // Click on track (not thumb): jump value to click position
            snapToTrack();
            event.accepted = true;
            return true;
        }
        break;
    case EventType::MouseUp:
        if (dragging_) {
            dragging_ = false;
            update();
            event.accepted = true;
            return true;
        }
        return false;
    default:
        break;
    }
    return false;
}

} // namespace ltgui
