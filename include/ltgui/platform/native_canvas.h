#pragma once
#include "color.h"
#include "font.h"
#include "geometry.h"
#include <string>

namespace ltgui {

class NativeCanvas {
public:
  virtual ~NativeCanvas() = default;

  virtual void resize(int width, int height) = 0;
  virtual void beginPaint() = 0;
  virtual void endPaint() = 0;

  virtual void setColor(const Color &color) = 0;
  virtual void setFont(const Font &font) = 0;

  virtual void fillRect(const Rect &rect) = 0;
  virtual void strokeRect(const Rect &rect, int lineWidth = 1) = 0;
  virtual void fillRoundedRect(const Rect &rect, int radius);
  virtual void strokeRoundedRect(const Rect &rect, int radius,
                                 int lineWidth = 1);
  // Two-color linear gradient. Default implementation bands fillRect
  // rows/columns with lerped colors; backends override with native
  // gradient primitives where available.
  virtual void fillLinearGradient(const Rect &rect, const Color &from,
                                  const Color &to, bool vertical = false);
  virtual void drawText(const std::string &text, const Rect &rect,
                        int flags = 0) = 0;
  virtual void drawLine(const Point &p1, const Point &p2,
                        int lineWidth = 1) = 0;
  virtual void fillEllipse(const Rect &rect) = 0;
  virtual void strokeEllipse(const Rect &rect, int lineWidth = 1) = 0;
  virtual void drawImage(const std::string &path, const Rect &rect);
  virtual Size imageSize(const std::string &path);
  virtual void drawPixelBuffer(const uint8_t *rgba, int w, int h,
                               const Rect &rect);

  virtual Size measureText(const std::string &text) = 0;

  // Load a TrueType/OpenType font file so it's available by family name.
  // Default: no-op. GPU backend uses FontAtlas; Win32 backend uses
  // AddFontResourceExW to register the font for this process.
  virtual bool loadFontFile(const Font & /*font*/, const char * /*ttfPath*/) {
    return false;
  }

  enum TextFlag {
    AlignLeft = 0,
    AlignCenter = 1,
    AlignRight = 2,
    AlignTop = 0,
    AlignVCenter = 4,
    AlignBottom = 8,
    SingleLine = 16,
    WordWrap = 32
  };
};

// Default implementations — fall back to basic shapes
inline void NativeCanvas::fillRoundedRect(const Rect &rect, int /*radius*/) {
  fillRect(rect);
}
inline void NativeCanvas::strokeRoundedRect(const Rect &rect, int /*radius*/,
                                            int lineWidth) {
  strokeRect(rect, lineWidth);
}
inline void NativeCanvas::fillLinearGradient(const Rect &rect,
                                             const Color &from, const Color &to,
                                             bool vertical) {
  int steps = vertical ? rect.height : rect.width;
  if (steps <= 0)
    return;
  for (int i = 0; i < steps; i++) {
    float t = steps > 1 ? static_cast<float>(i) / static_cast<float>(steps - 1)
                        : 0.0f;
    setColor(Color::lerp(from, to, t));
    if (vertical) {
      fillRect(Rect(rect.x, rect.y + i, rect.width, 1));
    } else {
      fillRect(Rect(rect.x + i, rect.y, 1, rect.height));
    }
  }
}
inline void NativeCanvas::drawImage(const std::string & /*path*/,
                                    const Rect & /*rect*/) {
  // No-op default: platforms override when image support is available
}
inline void NativeCanvas::drawPixelBuffer(const uint8_t * /*rgba*/, int /*w*/,
                                          int /*h*/, const Rect & /*rect*/) {
  // No-op default: platforms override when pixel-buffer support is available
}
inline Size NativeCanvas::imageSize(const std::string & /*path*/) { return {}; }

} // namespace ltgui
