#include "widgets/checkbox.h"
#include "window.h"
#include "platform/native_canvas.h"

namespace ltgui {

CheckBox::CheckBox(const std::string& text, Widget* parent)
    : Widget(parent), text_(text) {
    style().bgColor = Color::Transparent;
    style().borderWidth = 0;
}

void CheckBox::setText(const std::string& text) {
    text_ = text;
    update();
}

void CheckBox::setChecked(bool checked) {
    if (checked_ != checked) {
        checked_ = checked;
        update();
        if (toggleCallback_) toggleCallback_(checked_);
    }
}

Size CheckBox::sizeHint() const {
    Widget* w = const_cast<CheckBox*>(this);
    auto* win = w->window();
    if (win && win->canvas()) {
        Size textSize = win->canvas()->measureText(text_);
        return {textSize.width + 24 + style().paddingHorz(),
                std::max(textSize.height, 16) + style().paddingVert()};
    }
    return {100, 22};
}

void CheckBox::paintSelf(NativeCanvas* canvas) {
    Rect r = absoluteRect();

    // Check box
    int boxSize = 14;
    int boxY = r.y + (r.height - boxSize) / 2;
    Rect boxRect(r.x + 2, boxY, boxSize, boxSize);

    canvas->setColor(Color::White);
    canvas->fillRect(boxRect);

    canvas->setColor(Color::Gray);
    canvas->strokeRect(boxRect);

    if (checked_) {
        // Draw check mark
        canvas->setColor(Color::DarkBlue);
        canvas->drawLine({boxRect.x + 2, boxRect.y + boxSize / 2},
                         {boxRect.x + boxSize / 2, boxRect.y + boxSize - 2});
        canvas->drawLine({boxRect.x + boxSize / 2, boxRect.y + boxSize - 2},
                         {boxRect.x + boxSize - 2, boxRect.y + 2});
    }

    // Text label
    canvas->setColor(style().fgColor);
    canvas->setFont(style().font);
    Rect textRect(r.x + boxSize + 6, r.y, r.width - boxSize - 6, r.height);
    canvas->drawText(text_, textRect,
                     NativeCanvas::AlignLeft | NativeCanvas::AlignVCenter | NativeCanvas::SingleLine);
}

bool CheckBox::handleEvent(Event& event) {
    if (!isEnabled()) return false;

    if (event.type == EventType::MouseDown && event.button == MouseButton::Left) {
        setChecked(!checked_);
        return true;
    }
    return Widget::handleEvent(event);
}

} // namespace ltgui
