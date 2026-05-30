#include "style.h"

namespace ltgui {

Style Style::defaultStyle() {
    Style s;
    s.bgColor = Color::ButtonFace;
    s.fgColor = Color::TextColor;
    s.borderColor = Color::ButtonShadow;
    s.borderWidth = 1;
    s.borderRadius = 0;
    s.font = Font("Segoe UI", 12);
    s.setPadding(6, 4);
    s.setMargin(2);
    return s;
}

} // namespace ltgui
