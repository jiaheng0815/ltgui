#include "style.h"
#include "theme.h"

namespace ltgui {

Style Style::fromTheme(const Theme& theme) {
    Style s;
    s.bgColor = theme.bgSecondary;
    s.fgColor = theme.textPrimary;
    s.borderColor = theme.border;
    s.accent = theme.accent;
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
    out.bgColor     = pick(bgColor, theme.bgSecondary);
    out.fgColor     = pick(fgColor, theme.textPrimary);
    out.borderColor = pick(borderColor, theme.border);
    out.accent      = pick(accent, theme.accent);
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
