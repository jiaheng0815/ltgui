#pragma once
#include "platform/platform.h"

#ifdef LTGUI_PLATFORM_WINDOWS

#ifndef NOMINMAX
#define NOMINMAX
#endif
#include "platform/native_canvas.h"
// windows.h MUST precede gdiplus.h (GDI+ needs PROPID etc. from wtypes.h).
#include <windows.h>
#include <gdiplus.h>
#include <unordered_map>

namespace ltgui {

class Win32Canvas : public NativeCanvas {
public:
  explicit Win32Canvas(HWND hwnd);
  ~Win32Canvas() override;

  // Non-copyable (owns raw GDI+ resources — accidental copy would
  // double-delete)
  Win32Canvas(const Win32Canvas &) = delete;
  Win32Canvas &operator=(const Win32Canvas &) = delete;

  void resize(int width, int height) override;
  void beginPaint() override;
  void endPaint() override;

  void setColor(const Color &color) override;
  void setFont(const Font &font) override;

  void fillRect(const Rect &rect) override;
  void strokeRect(const Rect &rect, int lineWidth = 1) override;
  void fillRoundedRect(const Rect &rect, int radius) override;
  void fillLinearGradient(const Rect &rect, const Color &from, const Color &to,
                          bool vertical) override;
  void strokeRoundedRect(const Rect &rect, int radius,
                         int lineWidth = 1) override;
  void drawText(const std::string &text, const Rect &rect,
                int flags = 0) override;
  void drawLine(const Point &p1, const Point &p2, int lineWidth = 1) override;
  void fillEllipse(const Rect &rect) override;
  void strokeEllipse(const Rect &rect, int lineWidth = 1) override;
  void drawImage(const std::string &path, const Rect &rect) override;
  Size imageSize(const std::string &path) override;
  void drawPixelBuffer(const uint8_t *rgba, int w, int h,
                       const Rect &rect) override;

  Size measureText(const std::string &text) override;

  // Load a font file for use with non-system fonts (e.g. "Deng").
  // Uses AddFontResourceExW with FR_PRIVATE so the font is visible
  // to GDI+ for the lifetime of this process.
  bool loadFontFile(const Font &font, const char *ttfPath) override;

private:
  Gdiplus::Color toGdiColor(const Color &c);
  Gdiplus::Font *getOrCreateFont(const Font &f);
  int toGdiFontStyle(FontWeight w, FontStyle s);
  void flushFontCache();

  HWND hwnd_ = nullptr;
  Gdiplus::Bitmap *backbuffer_ = nullptr;
  Gdiplus::Graphics *graphics_ = nullptr;

  Gdiplus::Color currentColor_;
  Gdiplus::Font *currentFont_ = nullptr;
  Font currentFontDesc_;
  Gdiplus::SolidBrush *brush_ = nullptr;
  Gdiplus::Pen *pen_ = nullptr;

  int canvasWidth_ = 0;
  int canvasHeight_ = 0;

  // Caches
  std::unordered_map<Font, Gdiplus::Font *> fontCache_;
  std::unordered_map<std::string, Gdiplus::Image *> imageCache_;
  Gdiplus::Bitmap *measureBitmap_ = nullptr;
  Gdiplus::Graphics *measureGraphics_ = nullptr;
};

} // namespace ltgui

#endif // LTGUI_PLATFORM_WINDOWS
