#include "widgets/progressbar.h"
#include "window.h"
#include "theme.h"
#include "platform/native_canvas.h"
#include <algorithm>

namespace ltgui {

ProgressBar::ProgressBar(Widget* parent) : Widget(parent) {
    style().bgColor = currentTheme().bgTertiary;
    style().borderRadius = 4;
}

void ProgressBar::setValue(int value) {
    value = std::max(min_, std::min(max_, value));
    if (value_ != value) {
        value_ = value;
        float pct = (max_ > min_) ? static_cast<float>(value_ - min_) / static_cast<float>(max_ - min_) : 1.0f;
        displayValue_.setTarget(pct, 300, Easing::EaseOut);
        update();
    }
}

void ProgressBar::setRange(int min, int max) {
    min_ = min;
    max_ = max;
    if (value_ < min_) value_ = min_;
    if (value_ > max_) value_ = max_;
    update();
}

void ProgressBar::setIndeterminate(bool on) {
    indeterminate_ = on;
    if (on) {
        indeterminatePhase_.setTarget(1.0f, 1200, Easing::Linear);
    } else {
        indeterminatePhase_.setImmediate(0.0f);
    }
    update();
}

Size ProgressBar::sizeHint() const {
    if (!sizeHintDirty()) return cachedSizeHint();
    float dpi = window() ? window()->dpiScale() : 1.0f;
    setCachedSizeHint({static_cast<int>(200 * dpi), static_cast<int>(20 * dpi)});
    return cachedSizeHint();
}

void ProgressBar::paintSelf(NativeCanvas* canvas) {
    Rect r = absoluteRect();
    Theme t = currentTheme();

    // Track
    canvas->setColor(style().bgColor);
    canvas->fillRoundedRect(r, style().borderRadius);

    if (indeterminate_) {
        // Indeterminate: sliding bar
        float phase = indeterminatePhase_.value();
        if (phase >= 0.999f) {
            indeterminatePhase_.setImmediate(0.0f);
            indeterminatePhase_.setTarget(1.0f, 1200, Easing::Linear);
        }

        int barW = r.width / 3;
        int offset = static_cast<int>(phase * (r.width + barW) - barW);

        Rect barRect(r.x + offset, r.y, barW, r.height);
        if (barRect.x < r.x) {
            barRect.width -= r.x - barRect.x;
            barRect.x = r.x;
        }
        if (barRect.right() > r.right()) {
            barRect.width = r.right() - barRect.x;
        }

        canvas->setColor(t.accent);
        canvas->fillRoundedRect(barRect, style().borderRadius);
    } else {
        // Determinate: filled portion
        float pct = displayValue_.value();
        int fillW = static_cast<int>(r.width * pct);
        if (fillW > 0) {
            Rect fillRect(r.x, r.y, fillW, r.height);
            canvas->setColor(t.accent);
            canvas->fillRoundedRect(fillRect, style().borderRadius);

            // If not full, cover the right rounded corner on the fill end
            if (fillW < r.width - style().borderRadius) {
                canvas->fillRect(Rect(r.x + fillW - style().borderRadius, r.y,
                                      style().borderRadius, r.height));
            }
        }

        // Percentage text
        if (r.width > 60) {
            canvas->setColor(t.textPrimary);
            canvas->setFont(Font::systemDefault(11));
            int pctInt = static_cast<int>(pct * 100.0f + 0.5f);
            std::string pctText = std::to_string(pctInt) + "%";
            canvas->drawText(pctText, r,
                             NativeCanvas::AlignCenter | NativeCanvas::AlignVCenter | NativeCanvas::SingleLine);
        }
    }
}

} // namespace ltgui
