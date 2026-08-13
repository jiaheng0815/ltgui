#pragma once
#include "color.h"
#include "font.h"
#include <cstdint>
#include <algorithm>
#include <optional>

namespace ltgui {

class Theme;

// Visual state used to resolve a widget's effective style.
// Derivation order (highest wins): Disabled > Pressed > Hovered > Focused.
enum class WidgetState { Normal, Hovered, Pressed, Focused, Disabled };

// Optional two-color linear gradient; consumed by paintBackground().
struct Gradient {
    Color from;
    Color to;
    bool vertical = false;
};

// Effective style after resolution — concrete values only, no optionals.
// Widgets' paintSelf() reads this, never Style directly.
struct ResolvedStyle {
    Color bgColor;
    Color fgColor;
    Color borderColor;
    Color accent;
    int borderWidth = 0;
    int borderRadius = 0;
    Font font;
    int16_t paddingLeft = 0;
    int16_t paddingTop = 0;
    int16_t paddingRight = 0;
    int16_t paddingBottom = 0;
    std::optional<Gradient> gradient;
};

// Per-widget style. Fields default to transparent/zero so that
// resolve() can fall back to theme colors for anything unset.
struct Style {
    Color bgColor;
    Color fgColor;
    Color borderColor;
    Color accent;                      // semantic accent (fills the theme accent role)
    int borderWidth = 0;
    int borderRadius = 0;
    Font font;
    int16_t paddingLeft = 0;
    int16_t paddingTop = 0;
    int16_t paddingRight = 0;
    int16_t paddingBottom = 0;
    std::optional<Gradient> gradient;  // when set, paintBackground fills with it

    // Per-state overrides. A patch field that is set wins over the base
    // style field; anything unset falls back to base style, then theme.
    struct StatePatch {
        std::optional<Color> bgColor;
        std::optional<Color> fgColor;
        std::optional<Color> borderColor;
        std::optional<Color> accent;
    } hovered, pressed, focused, disabled;

    static Style defaultStyle();
    // Theme-derived defaults (bgSecondary/textPrimary/border/accent +
    // standard border/padding/font). Widgets override individual fields.
    static Style fromTheme(const Theme& theme);

    // Resolve the effective style for a widget state. Pure function —
    // resolution order: state patch > base style > theme default.
    ResolvedStyle resolve(WidgetState state, const Theme& theme) const;

    void setPadding(int all) {
        all = std::clamp(all, 0, static_cast<int>(INT16_MAX));
        paddingLeft = paddingTop = paddingRight = paddingBottom = static_cast<int16_t>(all);
    }
    void setPadding(int h, int v) {
        h = std::clamp(h, 0, static_cast<int>(INT16_MAX));
        v = std::clamp(v, 0, static_cast<int>(INT16_MAX));
        paddingLeft = paddingRight = static_cast<int16_t>(h);
        paddingTop = paddingBottom = static_cast<int16_t>(v);
    }

    int paddingHorz() const { return paddingLeft + paddingRight; }
    int paddingVert() const { return paddingTop + paddingBottom; }
};

} // namespace ltgui
