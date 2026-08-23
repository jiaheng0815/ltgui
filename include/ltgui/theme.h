#pragma once
#include "api.h"
#include "color.h"
#include "signal.h"
#include <string>
#include <unordered_map>
#include <vector>

namespace ltgui {

struct Theme {
  // Backgrounds
  Color bgPrimary;
  Color bgSecondary;
  Color bgTertiary;
  // Text
  Color textPrimary;
  Color textSecondary;
  Color textDisabled;
  // Accent
  Color accent;
  Color accentHover;
  Color accentPressed;
  // Borders
  Color border;
  Color borderFocus;
  // Scrollbar
  Color scrollbarTrack;
  Color scrollbarThumb;
  // Selection
  Color selectionBg;
  // Dialog
  Color dialogBg;
  Color dialogTitleBg;
  Color dialogBorder;
  // Table
  Color tableHeaderBg;
  Color tableRowAlt;
  Color tableBorder;
  // Menu
  Color menuBarBg;
  Color menuItemSelected;
  // Tooltip
  Color tooltipBg;
  Color tooltipBorder;
  // Scrollbar interaction
  Color scrollbarHover;
  Color scrollbarActive;
  // ProgressBar
  Color progressBarFill;
  Color progressBarTrack;

  std::string name;

  static Theme Light();
  static Theme Dark();
  static Theme DarkBlue();
  static Theme HighContrast();
  static Theme Solarized();
  static Theme Nord();

  bool operator==(const Theme &o) const;
  bool operator!=(const Theme &o) const { return !(*this == o); }
};

class LTGUI_API ThemeManager {
public:
  static ThemeManager &instance();

  void setTheme(const Theme &theme);
  void setThemeByName(const std::string &name);
  const Theme &currentTheme() const { return currentTheme_; }
  std::string currentThemeName() const { return currentTheme_.name; }

  void registerTheme(const std::string &name, const Theme &theme);
  bool unregisterTheme(const std::string &name);
  std::vector<std::string> availableThemes() const;

  Signal<const Theme &> onThemeChanged;

  ThemeManager(const ThemeManager &) = delete;
  ThemeManager &operator=(const ThemeManager &) = delete;

private:
  ThemeManager();
  Theme currentTheme_;
  std::unordered_map<std::string, Theme> customThemes_;
};

// Convenience — delegates to ThemeManager. Returns const ref to avoid
// copying the entire Theme struct (28 Colors + string) on every call.
inline const Theme &currentTheme() {
  return ThemeManager::instance().currentTheme();
}
void setTheme(const Theme &theme);

} // namespace ltgui
