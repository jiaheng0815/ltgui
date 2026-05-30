#pragma once
#include <string>

namespace ltgui {

enum class FontWeight {
    Thin = 100,
    ExtraLight = 200,
    Light = 300,
    Normal = 400,
    Medium = 500,
    SemiBold = 600,
    Bold = 700,
    ExtraBold = 800,
    Black = 900
};

enum class FontStyle {
    Normal,
    Italic
};

struct Font {
    std::string family;
    int size = 12;
    FontWeight weight = FontWeight::Normal;
    FontStyle style = FontStyle::Normal;

    Font() = default;
    Font(const std::string& family, int size,
         FontWeight weight = FontWeight::Normal,
         FontStyle style = FontStyle::Normal)
        : family(family), size(size), weight(weight), style(style) {}

    bool operator==(const Font& o) const {
        return family == o.family && size == o.size && weight == o.weight && style == o.style;
    }
    bool operator!=(const Font& o) const { return !(*this == o); }
};

} // namespace ltgui
