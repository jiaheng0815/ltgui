#include "widgets/button.h"
#include "window.h"
#include "platform/native_canvas.h"

namespace ltgui {

Button::Button(const std::string& text, Widget* parent)
    : Widget(parent), text_(text) {
    style().borderWidth = 1;
    style().borderRadius = 3;
    style().borderColor = Color::ButtonShadow;
    style().bgColor = Color::ButtonFace;
}

void Button::setText(const std::string& text) {
    text_ = text;
    update();
}

Size Button::sizeHint() const {
    Widget* w = const_cast<Button*>(this);
    auto* win = w->window();
    if (win && win->canvas()) {
        Size textSize = win->canvas()->measureText(text_);
        return {textSize.width + style().paddingHorz() + 20,
                textSize.height + style().paddingVert() + 8};
    }
    return {80, 28};
}

void Button::paintSelf(NativeCanvas* canvas) {
    Rect r = absoluteRect();

    // Background
    if (pressed_) {
        canvas->setColor(Color(220, 220, 220));
    } else if (hovered_) {
        canvas->setColor(Color(230, 230, 255));
    } else {
        canvas->setColor(style().bgColor);
    }

    if (style().borderRadius > 0) {
        canvas->fillEllipse(r);
    } else {
        canvas->fillRect(r);
    }

    // Border
    if (style().borderWidth > 0) {
        canvas->setColor(pressed_ ? Color::DarkGray : style().borderColor);
        if (style().borderRadius > 0) {
            canvas->strokeEllipse(r, style().borderWidth);
        } else {
            canvas->strokeRect(r, style().borderWidth);
        }
    }

    // Text
    canvas->setColor(style().fgColor);
    canvas->setFont(style().font);
    int flags = NativeCanvas::AlignCenter | NativeCanvas::AlignVCenter | NativeCanvas::SingleLine;
    Rect textRect = r.adjusted(0, pressed_ ? 1 : 0, 0, pressed_ ? 1 : 0);
    canvas->drawText(text_, textRect, flags);
}

bool Button::handleEvent(Event& event) {
    if (!isEnabled()) return false;

    switch (event.type) {
    case EventType::MouseDown:
        if (event.button == MouseButton::Left) {
            pressed_ = true;
            update();
            event.accepted = true;
            return true;
        }
        break;
    case EventType::MouseUp:
        if (event.button == MouseButton::Left && pressed_) {
            pressed_ = false;
            update();
            if (clickCallback_) clickCallback_();
            event.accepted = true;
            return true;
        }
        break;
    case EventType::MouseMove:
        // Track hover (position is already relative to parent)
        break;
    default:
        break;
    }
    return Widget::handleEvent(event);
}

} // namespace ltgui
