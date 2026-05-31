#pragma once
#include "color.h"

namespace ltgui {

struct Theme {
    Color bgPrimary;
    Color bgSecondary;
    Color bgTertiary;
    Color textPrimary;
    Color textSecondary;
    Color textDisabled;
    Color accent;
    Color accentHover;
    Color accentPressed;
    Color border;
    Color borderFocus;
    Color scrollbarTrack;
    Color scrollbarThumb;
    Color selectionBg;

    static Theme Light();
    static Theme Dark();

    bool operator==(const Theme& o) const {
        return bgPrimary == o.bgPrimary && bgSecondary == o.bgSecondary &&
               bgTertiary == o.bgTertiary && textPrimary == o.textPrimary &&
               textSecondary == o.textSecondary && textDisabled == o.textDisabled &&
               accent == o.accent && accentHover == o.accentHover &&
               accentPressed == o.accentPressed && border == o.border &&
               borderFocus == o.borderFocus && scrollbarTrack == o.scrollbarTrack &&
               scrollbarThumb == o.scrollbarThumb && selectionBg == o.selectionBg;
    }
    bool operator!=(const Theme& o) const { return !(*this == o); }
};

// Global theme access
Theme currentTheme();
void setTheme(const Theme& theme);

} // namespace ltgui
