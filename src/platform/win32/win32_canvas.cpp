#include "platform/win32/win32_canvas.h"

namespace ltgui {

namespace {
    ULONG_PTR gdiplusToken = 0;
    int gdiplusRefCount = 0;

    void initGdiPlus() {
        if (gdiplusRefCount++ == 0) {
            Gdiplus::GdiplusStartupInput input;
            Gdiplus::GdiplusStartup(&gdiplusToken, &input, nullptr);
        }
    }

    void shutdownGdiPlus() {
        if (--gdiplusRefCount == 0) {
            Gdiplus::GdiplusShutdown(gdiplusToken);
        }
    }
}

Win32Canvas::Win32Canvas(HWND hwnd) : hwnd_(hwnd) {
    initGdiPlus();
    currentColor_ = Gdiplus::Color(0, 0, 0);
    brush_ = new Gdiplus::SolidBrush(currentColor_);
    pen_ = new Gdiplus::Pen(currentColor_);
}

Win32Canvas::~Win32Canvas() {
    delete backbuffer_;
    delete graphics_;
    delete currentFont_;
    delete brush_;
    delete pen_;
    shutdownGdiPlus();
}

void Win32Canvas::resize(int width, int height) {
    if (width <= 0 || height <= 0) return;

    delete backbuffer_;
    delete graphics_;

    canvasWidth_ = width;
    canvasHeight_ = height;

    backbuffer_ = new Gdiplus::Bitmap(width, height, PixelFormat32bppARGB);
    graphics_ = new Gdiplus::Graphics(backbuffer_);
    graphics_->SetSmoothingMode(Gdiplus::SmoothingModeAntiAlias);
    graphics_->SetTextRenderingHint(Gdiplus::TextRenderingHintClearTypeGridFit);
}

void Win32Canvas::beginPaint() {
    if (graphics_) {
        graphics_->Clear(Gdiplus::Color(255, 255, 255, 255));
    }
}

void Win32Canvas::endPaint() {
    if (!hwnd_ || !backbuffer_) return;

    HDC hdc = GetDC(hwnd_);
    if (!hdc) return;

    Gdiplus::Graphics screenGraphics(hdc);
    screenGraphics.DrawImage(backbuffer_, 0, 0, canvasWidth_, canvasHeight_);

    ReleaseDC(hwnd_, hdc);
}

void Win32Canvas::setColor(const Color& color) {
    currentColor_ = toGdiColor(color);
    if (brush_) {
        delete brush_;
        brush_ = new Gdiplus::SolidBrush(currentColor_);
    }
    if (pen_) {
        pen_->SetColor(currentColor_);
    }
}

void Win32Canvas::setFont(const Font& font) {
    currentFontDesc_ = font;
    if (currentFont_) {
        delete currentFont_;
        currentFont_ = nullptr;
    }
    currentFont_ = getOrCreateFont(font);
}

Gdiplus::Font* Win32Canvas::getOrCreateFont(const Font& f) {
    if (currentFont_) {
        delete currentFont_;
    }

    int len = MultiByteToWideChar(CP_UTF8, 0, f.family.c_str(), -1, nullptr, 0);
    std::wstring wfamily(len, L'\0');
    MultiByteToWideChar(CP_UTF8, 0, f.family.c_str(), -1, &wfamily[0], len);
    if (len > 0) wfamily.resize(len - 1);  // Remove null terminator

    // Use LOGFONTW for better CJK/charset support
    HDC screenDC = GetDC(nullptr);
    LOGFONTW lf = {};
    lf.lfHeight = -f.size;
    lf.lfWeight = (static_cast<int>(f.weight) >= static_cast<int>(FontWeight::Bold)) ? FW_BOLD : FW_NORMAL;
    lf.lfItalic = (f.style == FontStyle::Italic) ? TRUE : FALSE;
    lf.lfCharSet = DEFAULT_CHARSET;
    lf.lfQuality = CLEARTYPE_QUALITY;
    lf.lfOutPrecision = OUT_TT_PRECIS;
    wcsncpy(lf.lfFaceName, wfamily.c_str(), LF_FACESIZE - 1);
    lf.lfFaceName[LF_FACESIZE - 1] = L'\0';

    Gdiplus::Font* font = new Gdiplus::Font(screenDC, &lf);
    ReleaseDC(nullptr, screenDC);
    return font;
}

int Win32Canvas::toGdiFontStyle(FontWeight w, FontStyle s) {
    int style = Gdiplus::FontStyleRegular;
    if (static_cast<int>(w) >= static_cast<int>(FontWeight::Bold)) {
        style |= Gdiplus::FontStyleBold;
    }
    if (s == FontStyle::Italic) {
        style |= Gdiplus::FontStyleItalic;
    }
    return style;
}

Gdiplus::Color Win32Canvas::toGdiColor(const Color& c) {
    return Gdiplus::Color(c.a, c.r, c.g, c.b);
}

void Win32Canvas::fillRect(const Rect& rect) {
    if (!graphics_ || !brush_) return;
    graphics_->FillRectangle(brush_, rect.x, rect.y, rect.width, rect.height);
}

void Win32Canvas::strokeRect(const Rect& rect, int lineWidth) {
    if (!graphics_ || !pen_) return;
    pen_->SetWidth(static_cast<Gdiplus::REAL>(lineWidth));
    graphics_->DrawRectangle(pen_, rect.x, rect.y, rect.width, rect.height);
}

void Win32Canvas::drawText(const std::string& text, const Rect& rect, int flags) {
    if (!graphics_) return;

    int len = MultiByteToWideChar(CP_UTF8, 0, text.c_str(), -1, nullptr, 0);
    std::wstring wtext(len, L'\0');
    MultiByteToWideChar(CP_UTF8, 0, text.c_str(), -1, &wtext[0], len);
    if (len > 0) wtext.resize(len - 1);

    Gdiplus::StringFormat format;

    if (flags & AlignRight) {
        format.SetAlignment(Gdiplus::StringAlignmentFar);
    } else if (flags & AlignCenter) {
        format.SetAlignment(Gdiplus::StringAlignmentCenter);
    } else {
        format.SetAlignment(Gdiplus::StringAlignmentNear);
    }

    if (flags & AlignBottom) {
        format.SetLineAlignment(Gdiplus::StringAlignmentFar);
    } else if (flags & AlignVCenter) {
        format.SetLineAlignment(Gdiplus::StringAlignmentCenter);
    } else {
        format.SetLineAlignment(Gdiplus::StringAlignmentNear);
    }

    if (flags & SingleLine) {
        format.SetFormatFlags(Gdiplus::StringFormatFlagsNoWrap);
    }

    Gdiplus::RectF layoutRect(static_cast<Gdiplus::REAL>(rect.x),
                               static_cast<Gdiplus::REAL>(rect.y),
                               static_cast<Gdiplus::REAL>(rect.width),
                               static_cast<Gdiplus::REAL>(rect.height));

    Gdiplus::Font* font = currentFont_;
    if (!font) {
        font = getOrCreateFont(Font("Segoe UI", 12));
    }

    if (brush_) {
        graphics_->DrawString(wtext.c_str(), -1, font, layoutRect, &format, brush_);
    }
}

void Win32Canvas::drawLine(const Point& p1, const Point& p2, int lineWidth) {
    if (!graphics_ || !pen_) return;
    pen_->SetWidth(static_cast<Gdiplus::REAL>(lineWidth));
    graphics_->DrawLine(pen_, p1.x, p1.y, p2.x, p2.y);
}

void Win32Canvas::fillEllipse(const Rect& rect) {
    if (!graphics_ || !brush_) return;
    graphics_->FillEllipse(brush_, rect.x, rect.y, rect.width, rect.height);
}

void Win32Canvas::strokeEllipse(const Rect& rect, int lineWidth) {
    if (!graphics_ || !pen_) return;
    pen_->SetWidth(static_cast<Gdiplus::REAL>(lineWidth));
    graphics_->DrawEllipse(pen_, rect.x, rect.y, rect.width, rect.height);
}

Size Win32Canvas::measureText(const std::string& text) {
    int len = MultiByteToWideChar(CP_UTF8, 0, text.c_str(), -1, nullptr, 0);
    std::wstring wtext(len, L'\0');
    MultiByteToWideChar(CP_UTF8, 0, text.c_str(), -1, &wtext[0], len);
    if (len > 0) wtext.resize(len - 1);

    Gdiplus::Font* font = currentFont_;
    if (!font) {
        font = getOrCreateFont(Font("Segoe UI", 12));
    }

    // Create a temporary bitmap + graphics for measurement
    Gdiplus::Bitmap temp(1, 1, PixelFormat32bppARGB);
    Gdiplus::Graphics tempGraphics(&temp);
    Gdiplus::RectF boundRect;
    tempGraphics.MeasureString(wtext.c_str(), -1, font, Gdiplus::PointF(0, 0), &boundRect);

    return {static_cast<int>(boundRect.Width + 0.5f),
            static_cast<int>(boundRect.Height + 0.5f)};
}

} // namespace ltgui
