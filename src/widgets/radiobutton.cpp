#include "widgets/radiobutton.h"
#include "window.h"
#include "platform/native_canvas.h"

namespace ltgui {

RadioButton::RadioButton(const std::string& text, Widget* parent)
    : Widget(parent), text_(text) {
    style().bgColor = Color::Transparent;
    style().borderWidth = 0;
}

void RadioButton::setText(const std::string& text) {
    text_ = text;
    update();
}

void RadioButton::setChecked(bool checked) {
    if (checked_ != checked) {
        // Uncheck siblings in the same parent
        if (checked && parent()) {
            for (auto* child : parent()->children()) {
                if (child != this) {
                    auto* rb = dynamic_cast<RadioButton*>(child);
                    if (rb && rb->isChecked()) {
                        rb->checked_ = false;
                        rb->update();
                    }
                }
            }
        }
        checked_ = checked;
        update();
        if (toggleCallback_) toggleCallback_(checked_);
    }
}

Size RadioButton::sizeHint() const {
    Widget* w = const_cast<RadioButton*>(this);
    auto* win = w->window();
    if (win && win->canvas()) {
        Size textSize = win->canvas()->measureText(text_);
        return {textSize.width + 24 + style().paddingHorz(),
                std::max(textSize.height, 16) + style().paddingVert()};
    }
    return {100, 22};
}

void RadioButton::paintSelf(NativeCanvas* canvas) {
    Rect r = absoluteRect();

    // Radio circle
    int circleSize = 14;
    int circleY = r.y + (r.height - circleSize) / 2;
    Rect circleRect(r.x + 2, circleY, circleSize, circleSize);

    canvas->setColor(Color::White);
    canvas->fillEllipse(circleRect);

    canvas->setColor(Color::Gray);
    canvas->strokeEllipse(circleRect);

    if (checked_) {
        // Draw filled dot inside
        Rect dotRect(r.x + 5, circleY + 3, circleSize - 6, circleSize - 6);
        canvas->setColor(Color::DarkBlue);
        canvas->fillEllipse(dotRect);
    }

    // Text label
    canvas->setColor(style().fgColor);
    canvas->setFont(style().font);
    Rect textRect(r.x + circleSize + 6, r.y, r.width - circleSize - 6, r.height);
    canvas->drawText(text_, textRect,
                     NativeCanvas::AlignLeft | NativeCanvas::AlignVCenter | NativeCanvas::SingleLine);
}

bool RadioButton::handleEvent(Event& event) {
    if (!isEnabled()) return false;

    if (event.type == EventType::MouseDown && event.button == MouseButton::Left) {
        setChecked(true);
        return true;
    }
    return Widget::handleEvent(event);
}

} // namespace ltgui
