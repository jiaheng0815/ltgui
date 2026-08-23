#pragma once
#include "platform/platform.h"

#ifdef LTGUI_PLATFORM_MACOS

#include "platform/native_canvas.h"
#include <string>

#ifdef __OBJC__
@class NSFont;
@class NSColor;
#else
typedef struct objc_object NSFont;
typedef struct objc_object NSColor;
#endif

namespace ltgui {

class CocoaCanvas : public NativeCanvas {
public:
  CocoaCanvas(void *nsView);
  ~CocoaCanvas() override;

  // Non-copyable (owns CGContextRef / ObjC resources — accidental copy would
  // double-free)
  CocoaCanvas(const CocoaCanvas &) = delete;
  CocoaCanvas &operator=(const CocoaCanvas &) = delete;

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
  void *nsView_ = nullptr;     // CocoaView*
  void *context_ = nullptr;    // CGContextRef
  void *backbuffer_ = nullptr; // CGContextRef (bitmap)

  int currentR_ = 0, currentG_ = 0, currentB_ = 0, currentA_ = 255;
  NSFont *currentFont_ = nullptr;
  NSColor *currentColor_ = nullptr;
  Font currentFontDesc_;

  int canvasWidth_ = 0;
  int canvasHeight_ = 0;
};

} // namespace ltgui

#endif // LTGUI_PLATFORM_MACOS
