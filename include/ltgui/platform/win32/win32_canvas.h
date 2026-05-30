#pragma once
#include "platform/platform.h"
#include <windows.h>
#include <gdiplus.h>
#include "platform/native_canvas.h"

namespace ltgui {

class Win32Canvas : public NativeCanvas {
public:
    explicit Win32Canvas(HWND hwnd);
    ~Win32Canvas() override;

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
    Gdiplus::Color toGdiColor(const Color& c);
    Gdiplus::Font* getOrCreateFont(const Font& f);
    int toGdiFontStyle(FontWeight w, FontStyle s);

    HWND hwnd_ = nullptr;
    Gdiplus::Bitmap* backbuffer_ = nullptr;
    Gdiplus::Graphics* graphics_ = nullptr;

    Gdiplus::Color currentColor_;
    Gdiplus::Font* currentFont_ = nullptr;
    Font currentFontDesc_;
    Gdiplus::SolidBrush* brush_ = nullptr;
    Gdiplus::Pen* pen_ = nullptr;

    int canvasWidth_ = 0;
    int canvasHeight_ = 0;
};

} // namespace ltgui
