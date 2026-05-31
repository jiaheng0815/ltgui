#include "widgets/label.h"
#include "window.h"
#include "theme.h"
#include "platform/native_canvas.h"

namespace ltgui {

Label::Label(const std::string& text, Widget* parent)
    : Widget(parent), text_(text) {
    style().bgColor = Color::Transparent;
    style().borderWidth = 0;
    style().fgColor = currentTheme().textPrimary;
}

void Label::setText(const std::string& text) {
    text_ = text;
    invalidateSizeHint();
    update();
}

Size Label::sizeHint() const {
    if (!sizeHintDirty()) return cachedSizeHint();
    if (auto* win = window()) {
        if (auto* c = win->canvas()) {
            Size textSize = c->measureText(text_);
            setCachedSizeHint({textSize.width + style().paddingHorz(),
                               textSize.height + style().paddingVert()});
            return cachedSizeHint();
        }
    }
    setCachedSizeHint({60, 20});
    return cachedSizeHint();
}

void Label::paintSelf(NativeCanvas* canvas) {
    canvas->setColor(isEnabled() ? style().fgColor : currentTheme().textDisabled);
    canvas->setFont(style().font);
    int flags = NativeCanvas::AlignLeft | NativeCanvas::AlignVCenter | NativeCanvas::SingleLine;
    canvas->drawText(text_, absoluteRect(), flags);
}

} // namespace ltgui
