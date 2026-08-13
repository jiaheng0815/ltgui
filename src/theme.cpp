#include "theme.h"
#include "app.h"
#include "window.h"
#include <cstring>
#include <unordered_map>

namespace ltgui {

// --- Theme preset color data (table-driven) ---
// Each preset is a flat array of 28 RGBA tuples matching the field order:
//   bgPrimary, bgSecondary, bgTertiary, textPrimary, textSecondary,
//   textDisabled, accent, accentHover, accentPressed, border, borderFocus,
//   scrollbarTrack, scrollbarThumb, selectionBg, dialogBg, dialogTitleBg,
//   dialogBorder, tableHeaderBg, tableRowAlt, tableBorder, menuBarBg,
//   menuItemSelected, tooltipBg, tooltipBorder, scrollbarHover,
//   scrollbarActive, progressBarFill, progressBarTrack

namespace {

// Each theme preset is 28 packed ARGB colors (A in high byte, R/G/B in low
// bytes). Field order must match the Color member declaration order in Theme.
struct PresetDef {
  const char *name;
  uint32_t c[28];
};

// Helper to decode ARGB uint32_t → Color (matches Color::toARGB layout)
inline Color decodeARGB(uint32_t v) {
  return Color((v >> 16) & 0xFF, (v >> 8) & 0xFF, v & 0xFF, (v >> 24) & 0xFF);
}

// Macros only for compact color literals: ARGB(a,r,g,b) packs to 0xAARRGGBB
#define C(r, g, b)                                                             \
  0xFF000000u | ((uint32_t)(r) << 16) | ((uint32_t)(g) << 8) | (uint32_t)(b)
#define CA(r, g, b, a)                                                         \
  ((uint32_t)(a) << 24) | ((uint32_t)(r) << 16) | ((uint32_t)(g) << 8) |       \
      (uint32_t)(b)

// Note: single braces for the array, not double. PresetDef = {name,
// {c0,c1,...,c27}}
constexpr PresetDef kPresets[] = {
    {"Light",
     {
         C(248, 248, 248), C(255, 255, 255), C(238, 238, 238), C(30, 30, 30),
         C(120, 120, 120), C(180, 180, 180), C(0, 120, 215),   C(0, 140, 235),
         C(0, 90, 175),    C(200, 200, 200), C(0, 120, 215),   C(240, 240, 240),
         C(190, 190, 190), C(0, 120, 215),   C(255, 255, 255), C(240, 240, 240),
         C(200, 200, 200), C(240, 240, 240), C(248, 248, 248), C(220, 220, 220),
         C(248, 248, 248), C(0, 120, 215),   C(255, 255, 225), C(200, 200, 180),
         C(160, 160, 160), C(120, 120, 120), C(0, 120, 215),   C(230, 230, 230),
     }},
    {"Dark",
     {
         C(30, 30, 30),    C(45, 45, 45),    C(55, 55, 55),  C(220, 220, 220),
         C(150, 150, 150), C(90, 90, 90),    C(0, 140, 235), C(0, 160, 255),
         C(0, 110, 205),   C(80, 80, 80),    C(0, 140, 235), C(50, 50, 50),
         C(100, 100, 100), C(0, 100, 190),   C(45, 45, 45),  C(35, 35, 35),
         C(80, 80, 80),    C(50, 50, 50),    C(40, 40, 40),  C(70, 70, 70),
         C(35, 35, 35),    C(0, 140, 235),   C(60, 60, 40),  C(100, 100, 80),
         C(130, 130, 130), C(160, 160, 160), C(0, 140, 235), C(60, 60, 60),
     }},
    {"DarkBlue",
     {
         C(20, 25, 35),    C(28, 34, 46),    C(36, 43, 58),   C(210, 218, 235),
         C(140, 150, 170), C(80, 85, 95),    C(70, 140, 220), C(90, 160, 240),
         C(50, 110, 190),  C(55, 65, 85),    C(70, 140, 220), C(30, 36, 50),
         C(60, 70, 95),    C(60, 100, 180),  C(28, 34, 46),   C(22, 28, 38),
         C(55, 65, 85),    C(32, 40, 54),    C(26, 32, 42),   C(50, 60, 78),
         C(22, 28, 38),    C(70, 140, 220),  C(40, 45, 58),   C(65, 75, 95),
         C(80, 92, 118),   C(100, 112, 138), C(70, 140, 220), C(36, 43, 58),
     }},
    {"HighContrast",
     {
         C(0, 0, 0),       C(15, 15, 15),    C(30, 30, 30),  C(255, 255, 255),
         C(200, 255, 0),   C(100, 100, 100), C(0, 255, 255), C(80, 255, 255),
         C(0, 200, 200),   C(255, 255, 255), C(0, 255, 255), C(30, 30, 30),
         C(200, 200, 200), C(0, 100, 200),   C(15, 15, 15),  C(0, 0, 0),
         C(255, 255, 255), C(30, 30, 30),    C(20, 20, 20),  C(255, 255, 255),
         C(0, 0, 0),       C(0, 255, 255),   C(40, 40, 0),   C(200, 255, 0),
         C(230, 230, 230), C(255, 255, 255), C(0, 255, 255), C(30, 30, 30),
     }},
    {"Solarized",
     {
         C(0, 43, 54),    C(7, 54, 66),     C(0, 43, 54),    C(131, 148, 150),
         C(88, 110, 117), C(50, 65, 70),    C(38, 139, 210), C(60, 160, 230),
         C(20, 110, 180), C(55, 78, 85),    C(38, 139, 210), C(0, 43, 54),
         C(55, 78, 85),   C(38, 139, 210),  C(7, 54, 66),    C(0, 43, 54),
         C(55, 78, 85),   C(0, 43, 54),     C(5, 48, 60),    C(55, 78, 85),
         C(0, 43, 54),    C(38, 139, 210),  C(7, 54, 66),    C(55, 78, 85),
         C(80, 105, 112), C(100, 125, 132), C(38, 139, 210), C(0, 43, 54),
     }},
    {"Nord",
     {
         C(46, 52, 64),    C(59, 66, 82),    C(67, 76, 94),    C(236, 239, 244),
         C(129, 142, 165), C(76, 86, 106),   C(136, 192, 208), C(158, 212, 228),
         C(114, 172, 188), C(67, 76, 94),    C(136, 192, 208), C(46, 52, 64),
         C(76, 86, 106),   C(94, 129, 172),  C(59, 66, 82),    C(46, 52, 64),
         C(67, 76, 94),    C(46, 52, 64),    C(56, 62, 78),    C(67, 76, 94),
         C(46, 52, 64),    C(136, 192, 208), C(59, 66, 82),    C(76, 86, 106),
         C(94, 104, 124),  C(112, 122, 142), C(136, 192, 208), C(46, 52, 64),
     }},
};

#undef C
#undef CA

// Pointer-to-member table: maps field index → Color member in Theme.
// The order MUST match the kPresets color array order.
using ColorMemberPtr = Color Theme::*;
constexpr ColorMemberPtr kColorFields[28] = {
    &Theme::bgPrimary,        &Theme::bgSecondary,     &Theme::bgTertiary,
    &Theme::textPrimary,      &Theme::textSecondary,   &Theme::textDisabled,
    &Theme::accent,           &Theme::accentHover,     &Theme::accentPressed,
    &Theme::border,           &Theme::borderFocus,     &Theme::scrollbarTrack,
    &Theme::scrollbarThumb,   &Theme::selectionBg,     &Theme::dialogBg,
    &Theme::dialogTitleBg,    &Theme::dialogBorder,    &Theme::tableHeaderBg,
    &Theme::tableRowAlt,      &Theme::tableBorder,     &Theme::menuBarBg,
    &Theme::menuItemSelected, &Theme::tooltipBg,       &Theme::tooltipBorder,
    &Theme::scrollbarHover,   &Theme::scrollbarActive, &Theme::progressBarFill,
    &Theme::progressBarTrack,
};

Theme makeTheme(const PresetDef &p) {
  Theme t;
  t.name = p.name;
  for (int i = 0; i < 28; ++i) {
    t.*(kColorFields[i]) = decodeARGB(p.c[i]);
  }
  return t;
}

// Preset factory map: name → index into kPresets
const std::unordered_map<std::string, int> &presetMap() {
  static const std::unordered_map<std::string, int> m = {
      {"Light", 0},        {"Dark", 1},      {"DarkBlue", 2},
      {"HighContrast", 3}, {"Solarized", 4}, {"Nord", 5},
  };
  return m;
}

} // anonymous namespace

// --- Theme preset factory methods (delegate to table) ---

Theme Theme::Light() { return makeTheme(kPresets[0]); }
Theme Theme::Dark() { return makeTheme(kPresets[1]); }
Theme Theme::DarkBlue() { return makeTheme(kPresets[2]); }
Theme Theme::HighContrast() { return makeTheme(kPresets[3]); }
Theme Theme::Solarized() { return makeTheme(kPresets[4]); }
Theme Theme::Nord() { return makeTheme(kPresets[5]); }

// --- Theme comparison (memcmp for Color block, then compare name) ---

bool Theme::operator==(const Theme &o) const {
  static_assert(sizeof(Color) == 4, "Color must be 4 bytes for memcmp");
  // All 28 Color fields are contiguous from bgPrimary to progressBarTrack
  constexpr size_t kColorBytes = 28 * sizeof(Color);
  return std::memcmp(&bgPrimary, &o.bgPrimary, kColorBytes) == 0 &&
         name == o.name;
}

// --- ThemeManager ---

ThemeManager::ThemeManager() : currentTheme_(Theme::Light()) {}

ThemeManager &ThemeManager::instance() {
  static ThemeManager mgr;
  return mgr;
}

void ThemeManager::setTheme(const Theme &theme) {
  if (currentTheme_ == theme)
    return;
  currentTheme_ = theme;
  onThemeChanged.emit(currentTheme_);
  for (auto *win : Application::instance().windows()) {
    win->update();
  }
}

void ThemeManager::setThemeByName(const std::string &name) {
  auto it = presetMap().find(name);
  if (it != presetMap().end()) {
    setTheme(makeTheme(kPresets[it->second]));
    return;
  }
  auto jt = customThemes_.find(name);
  if (jt != customThemes_.end()) {
    setTheme(jt->second);
  }
}

void ThemeManager::registerTheme(const std::string &name, const Theme &theme) {
  customThemes_[name] = theme;
}

bool ThemeManager::unregisterTheme(const std::string &name) {
  return customThemes_.erase(name) > 0;
}

std::vector<std::string> ThemeManager::availableThemes() const {
  static const char *kPresetNames[] = {"Light",        "Dark",      "DarkBlue",
                                       "HighContrast", "Solarized", "Nord"};
  std::vector<std::string> names;
  names.reserve(6 + customThemes_.size());
  for (const char *n : kPresetNames)
    names.push_back(n);
  for (auto &kv : customThemes_)
    names.push_back(kv.first);
  return names;
}

// --- Global helpers ---

void setTheme(const Theme &theme) { ThemeManager::instance().setTheme(theme); }

} // namespace ltgui
