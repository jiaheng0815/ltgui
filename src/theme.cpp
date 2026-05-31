#include "theme.h"
#include "app.h"
#include "window.h"

namespace ltgui {

static Theme g_currentTheme = Theme::Light();

Theme Theme::Light() {
    Theme t;
    t.bgPrimary      = Color(248, 248, 248);
    t.bgSecondary    = Color(255, 255, 255);
    t.bgTertiary     = Color(238, 238, 238);
    t.textPrimary    = Color(30, 30, 30);
    t.textSecondary  = Color(120, 120, 120);
    t.textDisabled   = Color(180, 180, 180);
    t.accent         = Color(0, 120, 215);
    t.accentHover    = Color(0, 140, 235);
    t.accentPressed  = Color(0, 90, 175);
    t.border         = Color(200, 200, 200);
    t.borderFocus    = Color(0, 120, 215);
    t.scrollbarTrack = Color(240, 240, 240);
    t.scrollbarThumb = Color(190, 190, 190);
    t.selectionBg    = Color(0, 120, 215);
    return t;
}

Theme Theme::Dark() {
    Theme t;
    t.bgPrimary      = Color(30, 30, 30);
    t.bgSecondary    = Color(45, 45, 45);
    t.bgTertiary     = Color(55, 55, 55);
    t.textPrimary    = Color(220, 220, 220);
    t.textSecondary  = Color(150, 150, 150);
    t.textDisabled   = Color(90, 90, 90);
    t.accent         = Color(0, 140, 235);
    t.accentHover    = Color(0, 160, 255);
    t.accentPressed  = Color(0, 110, 205);
    t.border         = Color(80, 80, 80);
    t.borderFocus    = Color(0, 140, 235);
    t.scrollbarTrack = Color(50, 50, 50);
    t.scrollbarThumb = Color(100, 100, 100);
    t.selectionBg    = Color(0, 100, 190);
    return t;
}

Theme currentTheme() {
    return g_currentTheme;
}

void setTheme(const Theme& theme) {
    if (g_currentTheme == theme) return;
    g_currentTheme = theme;
    // Propagate repaint to all existing windows
    for (auto* win : Application::instance().windows()) {
        win->update();
    }
}

} // namespace ltgui
