#include "style.h"
#include "theme.h"

namespace ltgui {

Style Style::defaultStyle() {
    const Theme& t = currentTheme();
    Style s;
    s.bgColor = t.bgSecondary;
    s.fgColor = t.textPrimary;
    s.borderColor = t.border;
    s.borderWidth = 1;
    s.borderRadius = 4;
    s.font = Font::systemDefault(12);
    s.setPadding(8, 4);
    return s;
}

} // namespace ltgui
