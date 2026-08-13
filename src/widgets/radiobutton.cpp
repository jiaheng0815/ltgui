#include "widgets/radiobutton.h"
#include "window.h"
#include "theme.h"
#include "platform/native_canvas.h"

namespace ltgui {

RadioButton::RadioButton(const std::string& text, Widget* parent)
    : Widget(parent), text_(text) {
    style().bgColor = Color::Transparent;
    style().borderWidth = 0;
    style().fgColor = currentTheme().textPrimary;
}

void RadioButton::setText(const std::string& text) {
    text_ = text;
    invalidateSizeHint();
    scheduleRelayout();
    update();
}

void RadioButton::setChecked(bool checked) {
    // Radio buttons in a group cannot be unchecked by direct user action.
    // Only allow unchecking programmatically or when another button becomes checked.
    if (checked_ == checked) return;

    if (checked) {
        // Uncheck all sibling radio buttons
        if (parent()) {
            for (auto& child : parent()->children()) {
                if (child.get() != this && child->widgetType() == WidgetType::RadioButton) {
                    auto* rb = static_cast<RadioButton*>(child.get());
                    if (rb->isChecked()) {
                        rb->checked_ = false;
                        rb->update();
                        if (rb->toggleCallback_) rb->toggleCallback_(false);
                    }
                }
            }
        }
    }
    // If trying to uncheck the only checked button in group, ignore
    if (!checked && checked_) {
        return;
    }

    checked_ = checked;
    update();
    if (toggleCallback_) toggleCallback_(checked_);
}

Size RadioButton::sizeHint() const {
    if (!sizeHintDirty()) return cachedSizeHint();
    if (auto* win = window()) {
        if (auto* c = win->canvas()) {
            c->setFont(style().font);
            Size textSize = c->measureText(text_);
            setCachedSizeHint({textSize.width + 24 + style().paddingHorz(),
                               std::max(textSize.height, 16) + style().paddingVert()});
            return cachedSizeHint();
        }
    }
    setCachedSizeHint({100, 22});
    return cachedSizeHint();
}

void RadioButton::paintSelf(NativeCanvas* canvas) {
    Rect r = absoluteRect();
    const Theme& t = currentTheme();

    int circleSize = 14;
    int circleY = r.y + (r.height - circleSize) / 2;
    Rect circleRect(r.x + 2, circleY, circleSize, circleSize);

    canvas->setColor(t.bgSecondary);
    canvas->fillEllipse(circleRect);
    canvas->setColor(checked_ ? t.accent : t.border);
    canvas->strokeEllipse(circleRect, checked_ ? 2 : 1);

    if (checked_) {
        Rect dotRect(r.x + 5, circleY + 3, circleSize - 6, circleSize - 6);
        canvas->setColor(t.accent);
        canvas->fillEllipse(dotRect);
    }

    canvas->setColor(isEnabled() ? style().fgColor : t.textDisabled);
    canvas->setFont(style().font);
    Rect textRect(r.x + circleSize + 6, r.y, r.width - circleSize - 6, r.height);
    canvas->drawText(text_, textRect,
                     NativeCanvas::AlignLeft | NativeCanvas::AlignVCenter | NativeCanvas::SingleLine);
}

bool RadioButton::handleEvent(Event& event) {
    if (!isEnabled()) return false;

    if (event.type == EventType::MouseDown && event.button == MouseButton::Left) {
        setChecked(true);
        event.accepted = true;
        return true;
    }
    return false;
}

} // namespace ltgui
