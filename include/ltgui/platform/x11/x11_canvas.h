#pragma once
#include "platform/platform.h"

#ifdef LTGUI_PLATFORM_LINUX

#include "platform/native_canvas.h"
#include <X11/Xlib.h>
#include <X11/Xft/Xft.h>
#include <string>

namespace ltgui {

class X11Canvas : public NativeCanvas {
public:
    X11Canvas(Display* display, ::Window window, int screen);
    ~X11Canvas() override;

    void resize(int width, int height) override;
    void beginPaint() override;
    void endPaint() override;

    void setColor(const Color& color) override;
    void setFont(const Font& font) override;

    void fillRect(const Rect& rect) override;
    void strokeRect(const Rect& rect, int lineWidth = 1) override;
    void drawText(const std::string& text, const Rect& rect, int flags = 0) override;
    void drawLine(const Point& p1, const Point& p2, int lineWidth = 1) override;
    void fillEllipse(const Rect& rect) override;
    void strokeEllipse(const Rect& rect, int lineWidth = 1) override;

    Size measureText(const std::string& text) override;

private:
    unsigned long allocColor(const Color& c);
    int textAlignToX11(int flags) const;

    Display* display_ = nullptr;
    ::Window window_ = 0;
    int screen_ = 0;
    int depth_ = 24;

    Pixmap backbuffer_ = 0;
    GC gc_ = nullptr;
    Colormap colormap_ = 0;

    // Xft
    XftDraw* xftDraw_ = nullptr;
    XftFont* xftFont_ = nullptr;
    XftColor xftColor_;
    bool xftColorSet_ = false;

    unsigned long currentPixel_ = 0;
    Font currentFontDesc_;
    int canvasWidth_ = 0;
    int canvasHeight_ = 0;
};

} // namespace ltgui

#endif // LTGUI_PLATFORM_LINUX
