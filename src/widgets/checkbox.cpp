#include "widgets/checkbox.h"
#include "window.h"
#include "theme.h"
#include "platform/native_canvas.h"

namespace ltgui {

CheckBox::CheckBox(const std::string& text, Widget* parent)
    : TextWidget(text, parent), Checkable(this) {
    style().bgColor = Color::Transparent;
    style().borderWidth = 0;
}

Size CheckBox::sizeHint() const {
    if (!sizeHintDirty()) return cachedSizeHint();
    Size s = textSizeHint({100, 22});
    setCachedSizeHint({s.width + 24 + style().paddingHorz(),
                       std::max(s.height, 16 + style().paddingVert())});
    return cachedSizeHint();
}

void CheckBox::paintSelf(NativeCanvas* canvas) {
    Rect r = absoluteRect();
    ResolvedStyle st = resolvedStyle();

    int boxSize = 14;
    int boxY = r.y + (r.height - boxSize) / 2;
    Rect boxRect(r.x + 2, boxY, boxSize, boxSize);

    canvas->setColor(st.bgColor);
    canvas->fillRoundedRect(boxRect, 3);

    canvas->setColor(checked_ ? st.accent : st.borderColor);
    canvas->strokeRoundedRect(boxRect, 3);

    if (checked_) {
        // Fill entire box with accent
        canvas->setColor(st.accent);
        canvas->fillRoundedRect(boxRect, 3);
        // White checkmark ✓ — short stem up-right, long tail down-right
        canvas->setColor(Color::White);
        int bx = boxRect.x, by = boxRect.y;
        canvas->drawLine({bx + 3, by + 8},  {bx + 6, by + 11}, 2);
        canvas->drawLine({bx + 6, by + 11}, {bx + 11, by + 3}, 2);
    }

    canvas->setColor(st.fgColor);
    canvas->setFont(st.font);
    Rect textRect(r.x + boxSize + 6, r.y, r.width - boxSize - 6, r.height);
    canvas->drawText(text_, textRect,
                     NativeCanvas::AlignLeft | NativeCanvas::AlignVCenter | NativeCanvas::SingleLine);
}

bool CheckBox::handleEvent(Event& event) {
    if (!isEnabled()) return false;

    if (event.type == EventType::MouseDown && event.button == MouseButton::Left) {
        setChecked(!isChecked());
        event.accepted = true;
        return true;
    }
    return false;
}

} // namespace ltgui
