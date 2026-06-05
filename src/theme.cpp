#include "theme.h"
#include "app.h"
#include "window.h"

namespace ltgui {

// --- Theme presets ---

Theme Theme::Light() {
    Theme t;
    t.name = "Light";
    t.bgPrimary         = Color(248, 248, 248);
    t.bgSecondary       = Color(255, 255, 255);
    t.bgTertiary        = Color(238, 238, 238);
    t.textPrimary       = Color(30, 30, 30);
    t.textSecondary     = Color(120, 120, 120);
    t.textDisabled      = Color(180, 180, 180);
    t.accent            = Color(0, 120, 215);
    t.accentHover       = Color(0, 140, 235);
    t.accentPressed     = Color(0, 90, 175);
    t.border            = Color(200, 200, 200);
    t.borderFocus       = Color(0, 120, 215);
    t.scrollbarTrack    = Color(240, 240, 240);
    t.scrollbarThumb    = Color(190, 190, 190);
    t.selectionBg       = Color(0, 120, 215);
    t.dialogBg          = Color(255, 255, 255);
    t.dialogTitleBg     = Color(240, 240, 240);
    t.dialogBorder      = Color(200, 200, 200);
    t.tableHeaderBg     = Color(240, 240, 240);
    t.tableRowAlt       = Color(248, 248, 248);
    t.tableBorder       = Color(220, 220, 220);
    t.menuBarBg         = Color(248, 248, 248);
    t.menuItemSelected  = Color(0, 120, 215);
    t.tooltipBg         = Color(255, 255, 225);
    t.tooltipBorder     = Color(200, 200, 180);
    t.scrollbarHover    = Color(160, 160, 160);
    t.scrollbarActive   = Color(120, 120, 120);
    t.progressBarFill   = Color(0, 120, 215);
    t.progressBarTrack  = Color(230, 230, 230);
    return t;
}

Theme Theme::Dark() {
    Theme t;
    t.name = "Dark";
    t.bgPrimary         = Color(30, 30, 30);
    t.bgSecondary       = Color(45, 45, 45);
    t.bgTertiary        = Color(55, 55, 55);
    t.textPrimary       = Color(220, 220, 220);
    t.textSecondary     = Color(150, 150, 150);
    t.textDisabled      = Color(90, 90, 90);
    t.accent            = Color(0, 140, 235);
    t.accentHover       = Color(0, 160, 255);
    t.accentPressed     = Color(0, 110, 205);
    t.border            = Color(80, 80, 80);
    t.borderFocus       = Color(0, 140, 235);
    t.scrollbarTrack    = Color(50, 50, 50);
    t.scrollbarThumb    = Color(100, 100, 100);
    t.selectionBg       = Color(0, 100, 190);
    t.dialogBg          = Color(45, 45, 45);
    t.dialogTitleBg     = Color(35, 35, 35);
    t.dialogBorder      = Color(80, 80, 80);
    t.tableHeaderBg     = Color(50, 50, 50);
    t.tableRowAlt       = Color(40, 40, 40);
    t.tableBorder       = Color(70, 70, 70);
    t.menuBarBg         = Color(35, 35, 35);
    t.menuItemSelected  = Color(0, 140, 235);
    t.tooltipBg         = Color(60, 60, 40);
    t.tooltipBorder     = Color(100, 100, 80);
    t.scrollbarHover    = Color(130, 130, 130);
    t.scrollbarActive   = Color(160, 160, 160);
    t.progressBarFill   = Color(0, 140, 235);
    t.progressBarTrack  = Color(60, 60, 60);
    return t;
}

Theme Theme::DarkBlue() {
    Theme t;
    t.name = "DarkBlue";
    t.bgPrimary         = Color(20, 25, 35);
    t.bgSecondary       = Color(28, 34, 46);
    t.bgTertiary        = Color(36, 43, 58);
    t.textPrimary       = Color(210, 218, 235);
    t.textSecondary     = Color(140, 150, 170);
    t.textDisabled      = Color(80, 85, 95);
    t.accent            = Color(70, 140, 220);
    t.accentHover       = Color(90, 160, 240);
    t.accentPressed     = Color(50, 110, 190);
    t.border            = Color(55, 65, 85);
    t.borderFocus       = Color(70, 140, 220);
    t.scrollbarTrack    = Color(30, 36, 50);
    t.scrollbarThumb    = Color(60, 70, 95);
    t.selectionBg       = Color(60, 100, 180);
    t.dialogBg          = Color(28, 34, 46);
    t.dialogTitleBg     = Color(22, 28, 38);
    t.dialogBorder      = Color(55, 65, 85);
    t.tableHeaderBg     = Color(32, 40, 54);
    t.tableRowAlt       = Color(26, 32, 42);
    t.tableBorder       = Color(50, 60, 78);
    t.menuBarBg         = Color(22, 28, 38);
    t.menuItemSelected  = Color(70, 140, 220);
    t.tooltipBg         = Color(40, 45, 58);
    t.tooltipBorder     = Color(65, 75, 95);
    t.scrollbarHover    = Color(80, 92, 118);
    t.scrollbarActive   = Color(100, 112, 138);
    t.progressBarFill   = Color(70, 140, 220);
    t.progressBarTrack  = Color(36, 43, 58);
    return t;
}

Theme Theme::HighContrast() {
    Theme t;
    t.name = "HighContrast";
    t.bgPrimary         = Color(0, 0, 0);
    t.bgSecondary       = Color(15, 15, 15);
    t.bgTertiary        = Color(30, 30, 30);
    t.textPrimary       = Color(255, 255, 255);
    t.textSecondary     = Color(200, 255, 0);
    t.textDisabled      = Color(100, 100, 100);
    t.accent            = Color(0, 255, 255);
    t.accentHover       = Color(80, 255, 255);
    t.accentPressed     = Color(0, 200, 200);
    t.border            = Color(255, 255, 255);
    t.borderFocus       = Color(0, 255, 255);
    t.scrollbarTrack    = Color(30, 30, 30);
    t.scrollbarThumb    = Color(200, 200, 200);
    t.selectionBg       = Color(0, 100, 200);
    t.dialogBg          = Color(15, 15, 15);
    t.dialogTitleBg     = Color(0, 0, 0);
    t.dialogBorder      = Color(255, 255, 255);
    t.tableHeaderBg     = Color(30, 30, 30);
    t.tableRowAlt       = Color(20, 20, 20);
    t.tableBorder       = Color(255, 255, 255);
    t.menuBarBg         = Color(0, 0, 0);
    t.menuItemSelected  = Color(0, 255, 255);
    t.tooltipBg         = Color(40, 40, 0);
    t.tooltipBorder     = Color(200, 255, 0);
    t.scrollbarHover    = Color(230, 230, 230);
    t.scrollbarActive   = Color(255, 255, 255);
    t.progressBarFill   = Color(0, 255, 255);
    t.progressBarTrack  = Color(30, 30, 30);
    return t;
}

Theme Theme::Solarized() {
    Theme t;
    t.name = "Solarized";
    t.bgPrimary         = Color(0, 43, 54);
    t.bgSecondary       = Color(7, 54, 66);
    t.bgTertiary        = Color(0, 43, 54);
    t.textPrimary       = Color(131, 148, 150);
    t.textSecondary     = Color(88, 110, 117);
    t.textDisabled      = Color(50, 65, 70);
    t.accent            = Color(38, 139, 210);
    t.accentHover       = Color(60, 160, 230);
    t.accentPressed     = Color(20, 110, 180);
    t.border            = Color(55, 78, 85);
    t.borderFocus       = Color(38, 139, 210);
    t.scrollbarTrack    = Color(0, 43, 54);
    t.scrollbarThumb    = Color(55, 78, 85);
    t.selectionBg       = Color(38, 139, 210);
    t.dialogBg          = Color(7, 54, 66);
    t.dialogTitleBg     = Color(0, 43, 54);
    t.dialogBorder      = Color(55, 78, 85);
    t.tableHeaderBg     = Color(0, 43, 54);
    t.tableRowAlt       = Color(5, 48, 60);
    t.tableBorder       = Color(55, 78, 85);
    t.menuBarBg         = Color(0, 43, 54);
    t.menuItemSelected  = Color(38, 139, 210);
    t.tooltipBg         = Color(7, 54, 66);
    t.tooltipBorder     = Color(55, 78, 85);
    t.scrollbarHover    = Color(80, 105, 112);
    t.scrollbarActive   = Color(100, 125, 132);
    t.progressBarFill   = Color(38, 139, 210);
    t.progressBarTrack  = Color(0, 43, 54);
    return t;
}

Theme Theme::Nord() {
    Theme t;
    t.name = "Nord";
    t.bgPrimary         = Color(46, 52, 64);
    t.bgSecondary       = Color(59, 66, 82);
    t.bgTertiary        = Color(67, 76, 94);
    t.textPrimary       = Color(236, 239, 244);
    t.textSecondary     = Color(129, 142, 165);
    t.textDisabled      = Color(76, 86, 106);
    t.accent            = Color(136, 192, 208);
    t.accentHover       = Color(158, 212, 228);
    t.accentPressed     = Color(114, 172, 188);
    t.border            = Color(67, 76, 94);
    t.borderFocus       = Color(136, 192, 208);
    t.scrollbarTrack    = Color(46, 52, 64);
    t.scrollbarThumb    = Color(76, 86, 106);
    t.selectionBg       = Color(94, 129, 172);
    t.dialogBg          = Color(59, 66, 82);
    t.dialogTitleBg     = Color(46, 52, 64);
    t.dialogBorder      = Color(67, 76, 94);
    t.tableHeaderBg     = Color(46, 52, 64);
    t.tableRowAlt       = Color(56, 62, 78);
    t.tableBorder       = Color(67, 76, 94);
    t.menuBarBg         = Color(46, 52, 64);
    t.menuItemSelected  = Color(136, 192, 208);
    t.tooltipBg         = Color(59, 66, 82);
    t.tooltipBorder     = Color(76, 86, 106);
    t.scrollbarHover    = Color(94, 104, 124);
    t.scrollbarActive   = Color(112, 122, 142);
    t.progressBarFill   = Color(136, 192, 208);
    t.progressBarTrack  = Color(46, 52, 64);
    return t;
}

bool Theme::operator==(const Theme& o) const {
    return bgPrimary == o.bgPrimary && bgSecondary == o.bgSecondary &&
           bgTertiary == o.bgTertiary && textPrimary == o.textPrimary &&
           textSecondary == o.textSecondary && textDisabled == o.textDisabled &&
           accent == o.accent && accentHover == o.accentHover &&
           accentPressed == o.accentPressed && border == o.border &&
           borderFocus == o.borderFocus && scrollbarTrack == o.scrollbarTrack &&
           scrollbarThumb == o.scrollbarThumb && selectionBg == o.selectionBg &&
           dialogBg == o.dialogBg && dialogTitleBg == o.dialogTitleBg &&
           dialogBorder == o.dialogBorder && tableHeaderBg == o.tableHeaderBg &&
           tableRowAlt == o.tableRowAlt && tableBorder == o.tableBorder &&
           menuBarBg == o.menuBarBg && menuItemSelected == o.menuItemSelected &&
           tooltipBg == o.tooltipBg && tooltipBorder == o.tooltipBorder &&
           scrollbarHover == o.scrollbarHover && scrollbarActive == o.scrollbarActive &&
           progressBarFill == o.progressBarFill && progressBarTrack == o.progressBarTrack;
}

// --- ThemeManager ---

ThemeManager::ThemeManager() : currentTheme_(Theme::Light()) {}

ThemeManager& ThemeManager::instance() {
    static ThemeManager mgr;
    return mgr;
}

void ThemeManager::setTheme(const Theme& theme) {
    if (currentTheme_ == theme) return;
    currentTheme_ = theme;
    onThemeChanged.emit(currentTheme_);
    for (auto* win : Application::instance().windows()) {
        win->update();
    }
}

void ThemeManager::setThemeByName(const std::string& name) {
    // Check built-in presets
    if (name == "Light")      { setTheme(Theme::Light()); return; }
    if (name == "Dark")       { setTheme(Theme::Dark()); return; }
    if (name == "DarkBlue")   { setTheme(Theme::DarkBlue()); return; }
    if (name == "HighContrast"){ setTheme(Theme::HighContrast()); return; }
    if (name == "Solarized")  { setTheme(Theme::Solarized()); return; }
    if (name == "Nord")       { setTheme(Theme::Nord()); return; }

    auto it = customThemes_.find(name);
    if (it != customThemes_.end()) {
        setTheme(it->second);
    }
}

void ThemeManager::registerTheme(const std::string& name, const Theme& theme) {
    customThemes_[name] = theme;
}

bool ThemeManager::unregisterTheme(const std::string& name) {
    return customThemes_.erase(name) > 0;
}

std::vector<std::string> ThemeManager::availableThemes() const {
    std::vector<std::string> names = {"Light", "Dark", "DarkBlue",
                                       "HighContrast", "Solarized", "Nord"};
    for (auto& kv : customThemes_) names.push_back(kv.first);
    return names;
}

// --- Global helpers ---

void setTheme(const Theme& theme) {
    ThemeManager::instance().setTheme(theme);
}

} // namespace ltgui
