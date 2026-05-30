#include "platform/x11/x11_canvas.h"

#ifdef LTGUI_PLATFORM_LINUX

#include <cstring>
#include <cmath>

namespace ltgui {

X11Canvas::X11Canvas(Display* display, ::Window window, int screen)
    : display_(display), window_(window), screen_(screen) {
    depth_ = DefaultDepth(display_, screen_);
    colormap_ = DefaultColormap(display_, screen_);

    gc_ = XCreateGC(display_, window_, 0, nullptr);

    // Default color
    currentPixel_ = BlackPixel(display_, screen_);
    XSetForeground(display_, gc_, currentPixel_);
}

X11Canvas::~X11Canvas() {
    if (xftFont_) {
        XftFontClose(display_, xftFont_);
        xftFont_ = nullptr;
    }
    if (xftDraw_) {
        XftDrawDestroy(xftDraw_);
        xftDraw_ = nullptr;
    }
    if (backbuffer_) {
        XFreePixmap(display_, backbuffer_);
        backbuffer_ = 0;
    }
    if (gc_) {
        XFreeGC(display_, gc_);
        gc_ = nullptr;
    }
}

void X11Canvas::resize(int width, int height) {
    if (width <= 0 || height <= 0) return;

    if (backbuffer_) {
        XFreePixmap(display_, backbuffer_);
    }
    if (xftDraw_) {
        XftDrawDestroy(xftDraw_);
        xftDraw_ = nullptr;
    }

    canvasWidth_ = width;
    canvasHeight_ = height;

    backbuffer_ = XCreatePixmap(display_, window_, width, height, depth_);
    xftDraw_ = XftDrawCreate(display_, backbuffer_,
                              DefaultVisual(display_, screen_), colormap_);
}

void X11Canvas::beginPaint() {
    if (backbuffer_) {
        XSetForeground(display_, gc_, WhitePixel(display_, screen_));
        XFillRectangle(display_, backbuffer_, gc_, 0, 0, canvasWidth_, canvasHeight_);
        XSetForeground(display_, gc_, currentPixel_);
    }
}

void X11Canvas::endPaint() {
    if (!backbuffer_ || !window_) return;

    // Blit backbuffer to window
    XCopyArea(display_, backbuffer_, window_, gc_,
              0, 0, canvasWidth_, canvasHeight_, 0, 0);
    XFlush(display_);
}

void X11Canvas::setColor(const Color& color) {
    currentPixel_ = allocColor(color);
    XSetForeground(display_, gc_, currentPixel_);

    // Xft color
    xftColor_.pixel = currentPixel_;
    xftColor_.color.red   = color.r * 0x101;
    xftColor_.color.green = color.g * 0x101;
    xftColor_.color.blue  = color.b * 0x101;
    xftColor_.color.alpha = color.a * 0x101;
    xftColorSet_ = true;
}

void X11Canvas::setFont(const Font& font) {
    currentFontDesc_ = font;

    if (xftFont_) {
        XftFontClose(display_, xftFont_);
        xftFont_ = nullptr;
    }

    // Build font name pattern: "family:size=12:weight=bold"
    std::string pattern = font.family + ":pixelsize=" + std::to_string(font.size);
    if (static_cast<int>(font.weight) >= static_cast<int>(FontWeight::Bold)) {
        pattern += ":weight=bold";
    }
    if (font.style == FontStyle::Italic) {
        pattern += ":slant=italic";
    }

    XftFont* f = XftFontOpenName(display_, screen_, pattern.c_str());
    if (f) {
        xftFont_ = f;
    } else {
        // Fallback to default font
        xftFont_ = XftFontOpenName(display_, screen_,
                                    "sans:size=12");
    }
}

unsigned long X11Canvas::allocColor(const Color& c) {
    XColor xc;
    xc.red   = c.r * 0x101;
    xc.green = c.g * 0x101;
    xc.blue  = c.b * 0x101;
    xc.flags = DoRed | DoGreen | DoBlue;
    XAllocColor(display_, colormap_, &xc);
    return xc.pixel;
}

void X11Canvas::fillRect(const Rect& rect) {
    if (!display_ || !backbuffer_) return;
    XFillRectangle(display_, backbuffer_, gc_, rect.x, rect.y, rect.width, rect.height);
}

void X11Canvas::strokeRect(const Rect& rect, int lineWidth) {
    if (!display_ || !backbuffer_) return;
    XSetLineAttributes(display_, gc_, lineWidth, LineSolid, CapButt, JoinMiter);
    XDrawRectangle(display_, backbuffer_, gc_, rect.x, rect.y, rect.width, rect.height);
    XSetLineAttributes(display_, gc_, 1, LineSolid, CapButt, JoinMiter);
}

void X11Canvas::drawText(const std::string& text, const Rect& rect, int flags) {
    if (!display_ || !backbuffer_ || !xftDraw_ || !xftFont_ || !xftColorSet_)
        return;

    // Measure text for alignment
    XGlyphInfo extents;
    XftTextExtentsUtf8(display_, xftFont_,
                        reinterpret_cast<const XftChar8*>(text.c_str()),
                        text.size(), &extents);

    int tx = rect.x;
    int ty = rect.y;

    // Horizontal alignment
    if (flags & AlignCenter) {
        tx += (rect.width - extents.width) / 2;
    } else if (flags & AlignRight) {
        tx += rect.width - extents.width;
    }

    // Vertical alignment — use font ascent/descent for accurate baseline
    ty = rect.y + (rect.height + xftFont_->ascent - xftFont_->descent) / 2;

    XftDrawStringUtf8(xftDraw_, &xftColor_, xftFont_,
                       tx, ty,
                       reinterpret_cast<const XftChar8*>(text.c_str()),
                       text.size());
}

void X11Canvas::drawLine(const Point& p1, const Point& p2, int lineWidth) {
    if (!display_ || !backbuffer_) return;
    XSetLineAttributes(display_, gc_, lineWidth, LineSolid, CapButt, JoinMiter);
    XDrawLine(display_, backbuffer_, gc_, p1.x, p1.y, p2.x, p2.y);
    XSetLineAttributes(display_, gc_, 1, LineSolid, CapButt, JoinMiter);
}

void X11Canvas::fillEllipse(const Rect& rect) {
    if (!display_ || !backbuffer_) return;
    XFillArc(display_, backbuffer_, gc_,
              rect.x, rect.y, rect.width, rect.height, 0, 360 * 64);
}

void X11Canvas::strokeEllipse(const Rect& rect, int lineWidth) {
    if (!display_ || !backbuffer_) return;
    XSetLineAttributes(display_, gc_, lineWidth, LineSolid, CapButt, JoinMiter);
    XDrawArc(display_, backbuffer_, gc_,
              rect.x, rect.y, rect.width, rect.height, 0, 360 * 64);
    XSetLineAttributes(display_, gc_, 1, LineSolid, CapButt, JoinMiter);
}

Size X11Canvas::measureText(const std::string& text) {
    if (!display_ || !xftFont_) {
        // Use a default font for measurement
        XftFont* f = XftFontOpenName(display_, screen_, "sans:pixelsize=12");
        if (!f) return {0, 0};

        XGlyphInfo extents;
        XftTextExtentsUtf8(display_, f,
                            reinterpret_cast<const XftChar8*>(text.c_str()),
                            text.size(), &extents);
        int w = extents.width;
        int h = extents.height;
        XftFontClose(display_, f);
        return {w, h};
    }

    XGlyphInfo extents;
    XftTextExtentsUtf8(display_, xftFont_,
                        reinterpret_cast<const XftChar8*>(text.c_str()),
                        text.size(), &extents);
    return {extents.width, extents.height};
}

} // namespace ltgui

#endif // LTGUI_PLATFORM_LINUX
