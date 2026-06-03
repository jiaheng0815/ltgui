#include "widgets/button.h"
#include "window.h"
#include "theme.h"
#include "platform/native_canvas.h"

namespace ltgui {

Button::Button(const std::string& text, Widget* parent)
    : Widget(parent), text_(text) {
    style().bgColor = currentTheme().bgSecondary;
    style().fgColor = currentTheme().textPrimary;
    style().borderColor = currentTheme().border;
    style().borderWidth = 1;
    style().borderRadius = 4;
}

void Button::setText(const std::string& text) {
    text_ = text;
    invalidateSizeHint();
    update();
}

Size Button::sizeHint() const {
    if (!sizeHintDirty()) return cachedSizeHint();
    if (auto* win = window()) {
        if (auto* c = win->canvas()) {
            Size textSize = c->measureText(text_);
            setCachedSizeHint({textSize.width + style().paddingHorz() + 24,
                               textSize.height + style().paddingVert() + 10});
            return cachedSizeHint();
        }
    }
    setCachedSizeHint({80, 30});
    return cachedSizeHint();
}

static Color lerpColor(const Color& a, const Color& b, float t) {
    return {
        static_cast<uint8_t>(a.r + (b.r - a.r) * t),
        static_cast<uint8_t>(a.g + (b.g - a.g) * t),
        static_cast<uint8_t>(a.b + (b.b - a.b) * t),
        static_cast<uint8_t>(a.a + (b.a - a.a) * t)
    };
}

void Button::paintSelf(NativeCanvas* canvas) {
    Rect r = absoluteRect();
    Theme t = currentTheme();

    // Compute color based on state
    Color baseColor = style().bgColor;
    Color targetColor;
    if (pressed_) {
        targetColor = t.accentPressed;
    } else if (hovered_) {
        targetColor = t.accentHover;
    } else {
        targetColor = t.accent;
    }
    // Use accent for interactive buttons, or keep custom bg
    bool useAccent = !pressed_ && !hovered_;
    if (useAccent && style().bgColor == t.bgSecondary) {
        targetColor = baseColor; // default bg
    }

    if (pressed_ || hovered_) {
        canvas->setColor(targetColor);
    } else {
        canvas->setColor(baseColor);
    }

    if (style().borderRadius > 0) {
        canvas->fillRoundedRect(r, style().borderRadius);
    } else {
        canvas->fillRect(r);
    }

    // Border
    if (style().borderWidth > 0) {
        canvas->setColor(pressed_ ? t.accentPressed : (hovered_ ? t.accentHover : style().borderColor));
        if (style().borderRadius > 0) {
            canvas->strokeRoundedRect(r, style().borderRadius, style().borderWidth);
        } else {
            canvas->strokeRect(r, style().borderWidth);
        }
    }

    // Text
    canvas->setColor(hovered_ || pressed_ ? Color::White : style().fgColor);
    canvas->setFont(style().font);
    int flags = NativeCanvas::AlignCenter | NativeCanvas::AlignVCenter | NativeCanvas::SingleLine;
    canvas->drawText(text_, r, flags);
}

bool Button::handleEvent(Event& event) {
    if (!isEnabled()) return false;

    switch (event.type) {
    case EventType::MouseMove: {
        bool inBounds = geometry().contains(event.pos);
        if (inBounds && !hovered_) {
            hovered_ = true;
            update();
        } else if (!inBounds && hovered_) {
            hovered_ = false;
            if (pressed_) {
                pressed_ = false;
            }
            update();
        }
        if (inBounds) event.accepted = true;
        return inBounds;
    }
    case EventType::MouseDown:
        if (event.button == MouseButton::Left) {
            pressed_ = true;
            claimFocus();
            update();
            event.accepted = true;
            return true;
        }
        break;
    case EventType::MouseUp:
        if (event.button == MouseButton::Left && pressed_) {
            pressed_ = false;
            update();
            if (clickCallback_ && hovered_) clickCallback_();
            event.accepted = true;
            return true;
        }
        break;
    case EventType::KeyDown:
        if (event.key == Key::Enter || event.key == Key::Space) {
            pressed_ = true;
            update();
            event.accepted = true;
            return true;
        }
        break;
    case EventType::KeyUp:
        if (event.key == Key::Enter || event.key == Key::Space) {
            pressed_ = false;
            update();
            if (clickCallback_) clickCallback_();
            event.accepted = true;
            return true;
        }
        break;
    default:
        break;
    }
    return false;
}

} // namespace ltgui
