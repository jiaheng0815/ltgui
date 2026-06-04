#include "widgets/checkbox.h"
#include "window.h"
#include "theme.h"
#include "platform/native_canvas.h"

namespace ltgui {

CheckBox::CheckBox(const std::string& text, Widget* parent)
    : Widget(parent), text_(text) {
    style().bgColor = Color::Transparent;
    style().borderWidth = 0;
    style().fgColor = currentTheme().textPrimary;
}

void CheckBox::setText(const std::string& text) {
    text_ = text;
    invalidateSizeHint();
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
    if (!sizeHintDirty()) return cachedSizeHint();
    if (auto* win = window()) {
        if (auto* c = win->canvas()) {
            Size textSize = c->measureText(text_);
            setCachedSizeHint({textSize.width + 24 + style().paddingHorz(),
                               std::max(textSize.height, 16) + style().paddingVert()});
            return cachedSizeHint();
        }
    }
    setCachedSizeHint({100, 22});
    return cachedSizeHint();
}

void CheckBox::paintSelf(NativeCanvas* canvas) {
    Rect r = absoluteRect();
    Theme t = currentTheme();

    int boxSize = 14;
    int boxY = r.y + (r.height - boxSize) / 2;
    Rect boxRect(r.x + 2, boxY, boxSize, boxSize);

    canvas->setColor(t.bgSecondary);
    canvas->fillRoundedRect(boxRect, 3);

    canvas->setColor(checked_ ? t.accent : t.border);
    canvas->strokeRoundedRect(boxRect, 3);

    if (checked_) {
        // Fill entire box with accent
        canvas->setColor(t.accent);
        canvas->fillRoundedRect(boxRect, 3);
        // White checkmark ✓ — short stem up-right, long tail down-right
        canvas->setColor(Color::White);
        int bx = boxRect.x, by = boxRect.y;
        canvas->drawLine({bx + 3, by + 8},  {bx + 6, by + 11}, 2);
        canvas->drawLine({bx + 6, by + 11}, {bx + 11, by + 3}, 2);
    }

    canvas->setColor(isEnabled() ? style().fgColor : t.textDisabled);
    canvas->setFont(style().font);
    Rect textRect(r.x + boxSize + 6, r.y, r.width - boxSize - 6, r.height);
    canvas->drawText(text_, textRect,
                     NativeCanvas::AlignLeft | NativeCanvas::AlignVCenter | NativeCanvas::SingleLine);
}

bool CheckBox::handleEvent(Event& event) {
    if (!isEnabled()) return false;

    if (event.type == EventType::MouseDown && event.button == MouseButton::Left) {
        setChecked(!checked_);
        event.accepted = true;
        return true;
    }
    return false;
}

} // namespace ltgui
