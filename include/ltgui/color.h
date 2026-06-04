#pragma once
#include <cstdint>

namespace ltgui {

struct Color {
    uint8_t r = 0;
    uint8_t g = 0;
    uint8_t b = 0;
    uint8_t a = 255;

    Color() = default;
    Color(uint8_t r, uint8_t g, uint8_t b, uint8_t a = 255) : r(r), g(g), b(b), a(a) {}

    static Color fromRGB(uint8_t r, uint8_t g, uint8_t b) { return {r, g, b, 255}; }
    static Color fromARGB(uint8_t a, uint8_t r, uint8_t g, uint8_t b) { return {r, g, b, a}; }

    bool operator==(const Color& o) const { return r == o.r && g == o.g && b == o.b && a == o.a; }
    bool operator!=(const Color& o) const { return !(*this == o); }

    uint32_t toARGB() const { return (a << 24) | (r << 16) | (g << 8) | b; }

    // GPU vertex color: little-endian RGBA byte order.
    // DXGI_FORMAT_R8G8B8A8_UNORM and GL_RGBA+GL_UNSIGNED_BYTE both
    // read bytes at increasing offsets as [R][G][B][A], which in LE
    // corresponds to the uint32_t layout 0xAABBGGRR.
    uint32_t toABGR() const { return (a << 24) | (b << 16) | (g << 8) | r; }

    static const Color Transparent;
    static const Color Black;
    static const Color White;
    static const Color Red;
    static const Color Green;
    static const Color Blue;
    static const Color Yellow;
    static const Color Cyan;
    static const Color Magenta;
    static const Color Gray;
    static const Color LightGray;
    static const Color DarkGray;
    static const Color DarkBlue;
    static const Color ButtonFace;
    static const Color ButtonHighlight;
    static const Color ButtonShadow;
    static const Color WindowBg;
    static const Color TextColor;
};

} // namespace ltgui
