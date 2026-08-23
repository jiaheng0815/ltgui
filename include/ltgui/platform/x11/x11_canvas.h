#pragma once
#include "platform/platform.h"

#ifdef LTGUI_PLATFORM_LINUX

#include "platform/native_canvas.h"
#include <X11/Xft/Xft.h>
#include <X11/Xlib.h>

// Clean up X11 macro pollution
#undef None
#undef FocusIn
#undef FocusOut
#undef ButtonPress
#undef ButtonRelease
#undef Button4
#undef Button5

#include <string>
#include <unordered_map>

namespace ltgui {

class X11Canvas : public NativeCanvas {
public:
  X11Canvas(Display *display, ::Window window, int screen);
  ~X11Canvas() override;

  // Non-copyable (owns X server resources — accidental copy would double-free)
  X11Canvas(const X11Canvas &) = delete;
  X11Canvas &operator=(const X11Canvas &) = delete;

  void resize(int width, int height) override;
  void beginPaint() override;
  void endPaint() override;

  void setColor(const Color &color) override;
  void setFont(const Font &font) override;

  void fillRect(const Rect &rect) override;
  void strokeRect(const Rect &rect, int lineWidth = 1) override;
  void fillRoundedRect(const Rect &rect, int radius) override;
  void strokeRoundedRect(const Rect &rect, int radius,
                         int lineWidth = 1) override;
  // Gradient with optional fullBounds mapping (uses the new contract
  // signature from native_canvas.h — depends on that header being updated
  // to the 5-parameter fillLinearGradient in the same change set).
  void fillLinearGradient(const Rect &rect, const Color &from, const Color &to,
                          bool vertical = false,
                          const Rect &fullBounds = Rect{}) override;
  void drawText(const std::string &text, const Rect &rect,
                int flags = 0) override;
  void drawLine(const Point &p1, const Point &p2, int lineWidth = 1) override;
  void fillEllipse(const Rect &rect) override;
  void strokeEllipse(const Rect &rect, int lineWidth = 1) override;

  Size measureText(const std::string &text) override;

private:
  unsigned long allocColor(const Color &c);
  int textAlignToX11(int flags) const;

  Display *display_ = nullptr;
  ::Window window_ = 0;
  int screen_ = 0;
  int depth_ = 24;

  Pixmap backbuffer_ = 0;
  GC gc_ = nullptr;
  Colormap colormap_ = 0;

  // Xft
  XftDraw *xftDraw_ = nullptr;
  XftFont *xftFont_ = nullptr;
  XftColor xftColor_;
  bool xftColorSet_ = false;

  unsigned long currentPixel_ = 0;
  uint8_t currentAlpha_ = 255;
  Font currentFontDesc_;
  float dpiScale_ = 1.0f;
  int canvasWidth_ = 0;
  int canvasHeight_ = 0;

  // ARGB key → pixel value, for cells obtained from XAllocColor.  Cached so
  // repeated setColor calls don't leak color cells; freed in the destructor.
  std::unordered_map<uint32_t, unsigned long> allocColorCache_;
};

} // namespace ltgui

#endif // LTGUI_PLATFORM_LINUX
