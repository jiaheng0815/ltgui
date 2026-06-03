#pragma once
#include <string>
#include <functional>

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

    static Font systemDefault(int size = 12) {
#ifdef LTGUI_PLATFORM_WINDOWS
        return {"Deng", size};
#elif defined(LTGUI_PLATFORM_MACOS)
        return {".AppleSystemUIFont", size};
#else
        return {"Sans", size};
#endif
    }

    bool operator==(const Font& o) const {
        return family == o.family && size == o.size && weight == o.weight && style == o.style;
    }
    bool operator!=(const Font& o) const { return !(*this == o); }
};

} // namespace ltgui

namespace std {
template <>
struct hash<ltgui::Font> {
    size_t operator()(const ltgui::Font& f) const noexcept {
        size_t h = hash<string>{}(f.family);
        h ^= hash<int>{}(f.size) + 0x9e3779b9 + (h << 6) + (h >> 2);
        h ^= hash<int>{}(static_cast<int>(f.weight)) + 0x9e3779b9 + (h << 6) + (h >> 2);
        h ^= hash<int>{}(static_cast<int>(f.style)) + 0x9e3779b9 + (h << 6) + (h >> 2);
        return h;
    }
};
} // namespace std
