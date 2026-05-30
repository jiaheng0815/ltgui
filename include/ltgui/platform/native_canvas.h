#pragma once
#include "geometry.h"
#include "color.h"
#include "font.h"
#include <string>

namespace ltgui {

class NativeCanvas {
public:
    virtual ~NativeCanvas() = default;

    virtual void resize(int width, int height) = 0;
    virtual void beginPaint() = 0;
    virtual void endPaint() = 0;

    virtual void setColor(const Color& color) = 0;
    virtual void setFont(const Font& font) = 0;

    virtual void fillRect(const Rect& rect) = 0;
    virtual void strokeRect(const Rect& rect, int lineWidth = 1) = 0;
    virtual void drawText(const std::string& text, const Rect& rect, int flags = 0) = 0;
    virtual void drawLine(const Point& p1, const Point& p2, int lineWidth = 1) = 0;
    virtual void fillEllipse(const Rect& rect) = 0;
    virtual void strokeEllipse(const Rect& rect, int lineWidth = 1) = 0;

    virtual Size measureText(const std::string& text) = 0;

    // Text alignment flags
    enum TextFlag {
        AlignLeft    = 0,
        AlignCenter  = 1,
        AlignRight   = 2,
        AlignTop     = 0,
        AlignVCenter = 4,
        AlignBottom  = 8,
        SingleLine   = 16,
        WordWrap     = 32
    };
};

} // namespace ltgui
