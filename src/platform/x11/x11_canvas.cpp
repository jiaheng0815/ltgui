#include "platform/x11/x11_canvas.h"

#ifdef LTGUI_PLATFORM_LINUX

#include <cstring>
#include <cmath>
#include <algorithm>

namespace ltgui {

X11Canvas::X11Canvas(Display* display, ::Window window, int screen)
    : display_(display), window_(window), screen_(screen) {
    depth_ = DefaultDepth(display_, screen_);
    colormap_ = DefaultColormap(display_, screen_);

    gc_ = XCreateGC(display_, window_, 0, nullptr);

    // Compute DPI scale to match Windows rendering size.
    // Xft pixelsize and GDI+ lfHeight interpret the same number differently:
    //   - GDI+: lfHeight=-12 means cell height = 12px (EM ≈ 10px)
    //   - Xft:  pixelsize=12   means EM size   = 12px (cell ≈ 14.4px)
    // Xft actually renders bigger, but Windows often has DPI scaling (125%+)
    // while X11 runs at 96 DPI. Detect actual DPI and scale accordingly.
    float xdpi = 96.0f;
    int wmm = DisplayWidthMM(display_, screen_);
    if (wmm > 0) {
        xdpi = DisplayWidth(display_, screen_) * 25.4f / wmm;
    }
    float ydpi = 96.0f;
    int hmm = DisplayHeightMM(display_, screen_);
    if (hmm > 0) {
        ydpi = DisplayHeight(display_, screen_) * 25.4f / hmm;
    }
    dpiScale_ = (xdpi + ydpi) / (2.0f * 96.0f);
    // Clamp to reasonable range: 1.0x–2.0x to guard against
    // X servers that report bogus physical dimensions (e.g. WSL)
    if (dpiScale_ < 1.0f) dpiScale_ = 1.0f;
    if (dpiScale_ > 2.0f) dpiScale_ = 2.0f;

    // Default color
    // Initialize with black
    currentPixel_ = allocColor(Color::Black);
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
        XSetFillStyle(display_, gc_, FillSolid);
        unsigned long white = allocColor(Color::White);
        XSetForeground(display_, gc_, white);
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
    currentAlpha_ = color.a;
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
    // Apply DPI scale so rendered size matches GDI+ at equivalent scaling
    currentFontDesc_.size = static_cast<int>(font.size * dpiScale_);

    if (xftFont_) {
        XftFontClose(display_, xftFont_);
        xftFont_ = nullptr;
    }

    std::string pattern = font.family + ":pixelsize=" + std::to_string(currentFontDesc_.size);
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
        // Fallback: use pixelsize (not size) for consistency with measureText
        std::string fb = "sans:pixelsize=" + std::to_string(currentFontDesc_.size);
        xftFont_ = XftFontOpenName(display_, screen_, fb.c_str());
    }
}

unsigned long X11Canvas::allocColor(const Color& c) {
    Visual* visual = DefaultVisual(display_, screen_);

    // DirectColor / TrueColor: compute pixel directly from RGB masks
    if (visual->c_class == TrueColor || visual->c_class == DirectColor) {
        // Extract contiguous mask bits (e.g. 0xFF0000 → value=0xFF, shift=16)
        auto decompose = [](unsigned long mask) -> std::pair<unsigned long, int> {
            if (mask == 0) return {0, 0};
            int shift = 0;
            while ((mask & 1) == 0) { mask >>= 1; shift++; }
            return {mask, shift};
        };

        auto [rBits, rShift] = decompose(visual->red_mask);
        auto [gBits, gShift] = decompose(visual->green_mask);
        auto [bBits, bShift] = decompose(visual->blue_mask);

        unsigned long r = (static_cast<unsigned long>(c.r) * rBits / 255) << rShift;
        unsigned long g = (static_cast<unsigned long>(c.g) * gBits / 255) << gShift;
        unsigned long b = (static_cast<unsigned long>(c.b) * bBits / 255) << bShift;
        return r | g | b;
    }

    // Fallback: allocate from colormap
    XColor xc;
    xc.red   = c.r * 0x101;
    xc.green = c.g * 0x101;
    xc.blue  = c.b * 0x101;
    xc.flags = DoRed | DoGreen | DoBlue;
    if (XAllocColor(display_, colormap_, &xc)) {
        return xc.pixel;
    }
    return BlackPixel(display_, screen_);
}

void X11Canvas::fillRect(const Rect& rect) {
    if (!display_ || !backbuffer_ || currentAlpha_ == 0) return;
    XFillRectangle(display_, backbuffer_, gc_, rect.x, rect.y, rect.width, rect.height);
}

void X11Canvas::strokeRect(const Rect& rect, int lineWidth) {
    if (!display_ || !backbuffer_ || currentAlpha_ == 0) return;
    XSetLineAttributes(display_, gc_, lineWidth, LineSolid, CapButt, JoinMiter);
    XDrawRectangle(display_, backbuffer_, gc_, rect.x, rect.y, rect.width, rect.height);
    XSetLineAttributes(display_, gc_, 1, LineSolid, CapButt, JoinMiter);
}

void X11Canvas::drawText(const std::string& text, const Rect& rect, int flags) {
    if (!display_ || !backbuffer_ || !xftDraw_ || !xftFont_ || !xftColorSet_ || currentAlpha_ == 0)
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
    if (!display_ || !backbuffer_ || currentAlpha_ == 0) return;
    XSetLineAttributes(display_, gc_, lineWidth, LineSolid, CapButt, JoinMiter);
    XDrawLine(display_, backbuffer_, gc_, p1.x, p1.y, p2.x, p2.y);
    XSetLineAttributes(display_, gc_, 1, LineSolid, CapButt, JoinMiter);
}

void X11Canvas::fillEllipse(const Rect& rect) {
    if (!display_ || !backbuffer_ || currentAlpha_ == 0) return;
    XFillArc(display_, backbuffer_, gc_,
              rect.x, rect.y, rect.width, rect.height, 0, 360 * 64);
}

void X11Canvas::strokeEllipse(const Rect& rect, int lineWidth) {
    if (!display_ || !backbuffer_ || currentAlpha_ == 0) return;
    XSetLineAttributes(display_, gc_, lineWidth, LineSolid, CapButt, JoinMiter);
    XDrawArc(display_, backbuffer_, gc_,
              rect.x, rect.y, rect.width, rect.height, 0, 360 * 64);
    XSetLineAttributes(display_, gc_, 1, LineSolid, CapButt, JoinMiter);
}

void X11Canvas::fillRoundedRect(const Rect& rect, int radius) {
    if (!display_ || !backbuffer_ || currentAlpha_ == 0) return;
    int r = std::min(radius, std::min(rect.width, rect.height) / 2);
    if (r <= 0) { fillRect(rect); return; }
    int x = rect.x, y = rect.y, w = rect.width, h = rect.height;
    int d = r * 2;
    // Fill center rect
    XFillRectangle(display_, backbuffer_, gc_, x + r, y, w - d, h);
    XFillRectangle(display_, backbuffer_, gc_, x, y + r, w, h - d);
    // Fill corner arcs
    XFillArc(display_, backbuffer_, gc_, x, y, d, d, 90 * 64, 90 * 64);
    XFillArc(display_, backbuffer_, gc_, x + w - d, y, d, d, 0 * 64, 90 * 64);
    XFillArc(display_, backbuffer_, gc_, x + w - d, y + h - d, d, d, 270 * 64, 90 * 64);
    XFillArc(display_, backbuffer_, gc_, x, y + h - d, d, d, 180 * 64, 90 * 64);
}

void X11Canvas::strokeRoundedRect(const Rect& rect, int radius, int lineWidth) {
    if (!display_ || !backbuffer_ || currentAlpha_ == 0) return;
    int r = std::min(radius, std::min(rect.width, rect.height) / 2);
    if (r <= 0) { strokeRect(rect, lineWidth); return; }
    int x = rect.x, y = rect.y, w = rect.width, h = rect.height;
    int d = r * 2;
    XSetLineAttributes(display_, gc_, lineWidth, LineSolid, CapButt, JoinMiter);
    // Top edge
    XDrawLine(display_, backbuffer_, gc_, x + r, y, x + w - r, y);
    // Right edge
    XDrawLine(display_, backbuffer_, gc_, x + w, y + r, x + w, y + h - r);
    // Bottom edge
    XDrawLine(display_, backbuffer_, gc_, x + w - r, y + h, x + r, y + h);
    // Left edge
    XDrawLine(display_, backbuffer_, gc_, x, y + h - r, x, y + r);
    // Corner arcs
    XDrawArc(display_, backbuffer_, gc_, x, y, d, d, 90 * 64, 90 * 64);
    XDrawArc(display_, backbuffer_, gc_, x + w - d, y, d, d, 0 * 64, 90 * 64);
    XDrawArc(display_, backbuffer_, gc_, x + w - d, y + h - d, d, d, 270 * 64, 90 * 64);
    XDrawArc(display_, backbuffer_, gc_, x, y + h - d, d, d, 180 * 64, 90 * 64);
    XSetLineAttributes(display_, gc_, 1, LineSolid, CapButt, JoinMiter);
}

Size X11Canvas::measureText(const std::string& text) {
    if (!display_ || !xftFont_) {
        int fs = static_cast<int>(12 * dpiScale_);
        std::string fb = "sans:pixelsize=" + std::to_string(fs);
        XftFont* f = XftFontOpenName(display_, screen_, fb.c_str());
        if (!f) return {0, 0};

        XGlyphInfo extents;
        XftTextExtentsUtf8(display_, f,
                            reinterpret_cast<const XftChar8*>(text.c_str()),
                            text.size(), &extents);
        int w = extents.width;
        int h = f->ascent + f->descent;  // cell height, matches GDI+ MeasureString
        XftFontClose(display_, f);
        return {w, h};
    }

    XGlyphInfo extents;
    XftTextExtentsUtf8(display_, xftFont_,
                        reinterpret_cast<const XftChar8*>(text.c_str()),
                        text.size(), &extents);
    return {extents.width, xftFont_->ascent + xftFont_->descent};
}

} // namespace ltgui

#endif // LTGUI_PLATFORM_LINUX
