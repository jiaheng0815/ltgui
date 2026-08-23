#pragma once
#include <cstdint>

namespace ltgui {

struct Color {
  uint8_t r = 0;
  uint8_t g = 0;
  uint8_t b = 0;
  uint8_t a = 255;

  Color() = default;
  Color(uint8_t r, uint8_t g, uint8_t b, uint8_t a = 255)
      : r(r), g(g), b(b), a(a) {}

  static Color fromRGB(uint8_t r, uint8_t g, uint8_t b) {
    return {r, g, b, 255};
  }
  static Color fromARGB(uint8_t a, uint8_t r, uint8_t g, uint8_t b) {
    return {r, g, b, a};
  }

  bool operator==(const Color &o) const {
    return r == o.r && g == o.g && b == o.b && a == o.a;
  }
  bool operator!=(const Color &o) const { return !(*this == o); }

  uint32_t toARGB() const {
    return (static_cast<uint32_t>(a) << 24) |
           (static_cast<uint32_t>(r) << 16) | (static_cast<uint32_t>(g) << 8) |
           static_cast<uint32_t>(b);
  }

  // Linear interpolation per channel (alpha included). t=0 -> a, t=1 -> b.
  static Color lerp(const Color &a, const Color &b, float t) {
    t = t < 0.0f ? 0.0f : (t > 1.0f ? 1.0f : t);
    auto mix = [t](uint8_t x, uint8_t y) {
      return static_cast<uint8_t>(x + static_cast<int>((y - x) * t));
    };
    return {mix(a.r, b.r), mix(a.g, b.g), mix(a.b, b.b), mix(a.a, b.a)};
  }

  // GPU vertex color: little-endian RGBA byte order.
  // DXGI_FORMAT_R8G8B8A8_UNORM and GL_RGBA+GL_UNSIGNED_BYTE both
  // read bytes at increasing offsets as [R][G][B][A], which in LE
  // corresponds to the uint32_t layout 0xAABBGGRR.
  uint32_t toABGR() const {
    return (static_cast<uint32_t>(a) << 24) |
           (static_cast<uint32_t>(b) << 16) | (static_cast<uint32_t>(g) << 8) |
           static_cast<uint32_t>(r);
  }

  static const Color Transparent;
  static const Color Black;
  static const Color White;
  static const Color Red;
};

} // namespace ltgui
