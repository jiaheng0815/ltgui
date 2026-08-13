#include "widgets/label.h"
#include "window.h"
#include "theme.h"
#include "platform/native_canvas.h"

namespace ltgui {

Label::Label(const std::string& text, Widget* parent)
    : TextWidget(text, parent) {
    style().bgColor = Color::Transparent;
    style().borderWidth = 0;
    style().fgColor = currentTheme().textPrimary;
}

Size Label::sizeHint() const {
    if (!sizeHintDirty()) return cachedSizeHint();
    setCachedSizeHint(textSizeHint({60, 20},
                                   style().paddingHorz(),
                                   style().paddingVert()));
    return cachedSizeHint();
}

void Label::paintSelf(NativeCanvas* canvas) {
    Rect r = absoluteRect();
    canvas->setColor(isEnabled() ? style().fgColor : currentTheme().textDisabled);
    canvas->setFont(style().font);
    int flags = NativeCanvas::AlignLeft | NativeCanvas::AlignVCenter | NativeCanvas::SingleLine;
    canvas->drawText(text_, r, flags);
}

} // namespace ltgui
