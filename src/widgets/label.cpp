#include "widgets/label.h"
#include "window.h"
#include "platform/native_canvas.h"

namespace ltgui {

Label::Label(const std::string& text, Widget* parent)
    : Widget(parent), text_(text) {
    style().bgColor = Color::Transparent;
    style().borderWidth = 0;
}

void Label::setText(const std::string& text) {
    text_ = text;
    update();
}

Size Label::sizeHint() const {
    Widget* w = const_cast<Label*>(this);
    auto* win = w->window();
    if (win && win->canvas()) {
        Size textSize = win->canvas()->measureText(text_);
        return {textSize.width + style().paddingHorz(),
                textSize.height + style().paddingVert()};
    }
    return {60, 20};
}

void Label::paintSelf(NativeCanvas* canvas) {
    canvas->setColor(style().fgColor);
    canvas->setFont(style().font);
    int flags = NativeCanvas::AlignLeft | NativeCanvas::AlignVCenter | NativeCanvas::SingleLine;
    canvas->drawText(text_, absoluteRect(), flags);
}

} // namespace ltgui
