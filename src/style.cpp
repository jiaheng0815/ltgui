#include "style.h"
#include "theme.h"

namespace ltgui {

Style Style::fromTheme(const Theme& theme) {
    Style s;
    // All colors intentionally left Transparent — resolve() falls back to
    // the CURRENT theme on every call, so theme switches are picked up
    // without re-styling widgets. Only structural defaults live here.
    (void)theme;
    s.borderWidth = 1;
    s.borderRadius = 4;
    s.font = Font::systemDefault(12);
    s.setPadding(8, 4);
    return s;
}

Style Style::defaultStyle() {
    return fromTheme(currentTheme());
}

ResolvedStyle Style::resolve(WidgetState state, const Theme& theme) const {
    // Base layer: explicit style fields win over theme defaults. A fully
    // transparent color (Color::Transparent) means "not set" -> theme
    // fallback. (Note: Color's default is opaque black, so only an explicit
    // Transparent acts as "unset".)
    auto pick = [](const Color& value, const Color& fallback) {
        return value == Color::Transparent ? fallback : value;
    };
    ResolvedStyle out;
    // Disabled gets theme's tertiary bg / disabled text when not overridden.
    out.bgColor     = pick(bgColor, state == WidgetState::Disabled ? theme.bgTertiary
                                                                   : theme.bgSecondary);
    out.fgColor     = pick(fgColor, state == WidgetState::Disabled ? theme.textDisabled
                                                                   : theme.textPrimary);
    out.borderColor = pick(borderColor, theme.border);
    // State-specific theme accent fallback: hovered/pressed get their own
    // colors from the theme when no explicit style accent is set.
    if (accent != Color::Transparent) {
        out.accent = accent;
    } else {
        switch (state) {
        case WidgetState::Hovered:  out.accent = theme.accentHover;  break;
        case WidgetState::Pressed:  out.accent = theme.accentPressed; break;
        default:                    out.accent = theme.accent;       break;
        }
    }
    out.borderWidth  = borderWidth;
    out.borderRadius = borderRadius;
    out.font = font;
    out.paddingLeft   = paddingLeft;
    out.paddingTop    = paddingTop;
    out.paddingRight  = paddingRight;
    out.paddingBottom = paddingBottom;
    out.gradient = gradient;

    // State overlay layer: highest-priority state wins.
    const StatePatch* patch = nullptr;
    switch (state) {
    case WidgetState::Disabled: patch = &disabled;  break;
    case WidgetState::Pressed:  patch = &pressed;   break;
    case WidgetState::Hovered:  patch = &hovered;   break;
    case WidgetState::Focused:  patch = &focused;   break;
    default: break;
    }
    if (patch) {
        if (patch->bgColor)    out.bgColor = *patch->bgColor;
        if (patch->fgColor)    out.fgColor = *patch->fgColor;
        if (patch->borderColor) out.borderColor = *patch->borderColor;
        if (patch->accent)     out.accent = *patch->accent;
    }
    return out;
}

} // namespace ltgui
