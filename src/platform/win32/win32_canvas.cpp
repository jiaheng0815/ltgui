#include "platform/win32/win32_canvas.h"

#ifdef LTGUI_PLATFORM_WINDOWS

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
} // namespace

Win32Canvas::Win32Canvas(HWND hwnd) : hwnd_(hwnd) {
  initGdiPlus();
  currentColor_ = Gdiplus::Color(0, 0, 0);
  brush_ = new Gdiplus::SolidBrush(currentColor_);
  pen_ = new Gdiplus::Pen(currentColor_);
}

bool Win32Canvas::loadFontFile(const Font &font, const char *ttfPath) {
  (void)font;
  int len = MultiByteToWideChar(CP_UTF8, 0, ttfPath, -1, nullptr, 0);
  std::wstring wpath(len, L'\0');
  MultiByteToWideChar(CP_UTF8, 0, ttfPath, -1, &wpath[0], len);
  if (len > 0)
    wpath.resize(len - 1);

  // FR_PRIVATE: register for this process only, no admin rights needed.
  // The font becomes visible to GDI+ via its family name for this session.
  int result = AddFontResourceExW(wpath.c_str(), FR_PRIVATE, nullptr);
  if (result > 0) {
    // The font tables may have changed (new family, updated metrics for
    // existing families) — drop cached GDI+ Font objects so the next
    // setFont()/drawText() rebuilds from fresh data.
    flushFontCache();
  }
  return result > 0;
}

void Win32Canvas::flushFontCache() {
  for (auto &pair : fontCache_)
    delete pair.second;
  fontCache_.clear();
  // Null out the active font so drawing falls back to lazy re-creation;
  // setFont() re-resolves even for the previously active descriptor.
  currentFont_ = nullptr;
}

Win32Canvas::~Win32Canvas() {
  // Note: currentFont_ is NOT deleted here — it is an alias into
  // fontCache_ (like the entries deleted below), which owns the objects.
  delete backbuffer_;
  delete graphics_;
  delete brush_;
  delete pen_;
  delete measureBitmap_;
  delete measureGraphics_;
  // Flush caches
  for (auto &pair : fontCache_)
    delete pair.second;
  for (auto &pair : imageCache_)
    delete pair.second;
  shutdownGdiPlus();
}

void Win32Canvas::resize(int width, int height) {
  if (width <= 0 || height <= 0)
    return;

  // Allocate new objects before deleting old ones — if construction
  // fails, the old canvas remains intact.
  Gdiplus::Bitmap *newBuf =
      new Gdiplus::Bitmap(width, height, PixelFormat32bppARGB);
  if (newBuf->GetLastStatus() != Gdiplus::Ok) {
    delete newBuf;
    return;
  }

  Gdiplus::Graphics *newGfx = nullptr;
  try {
    newGfx = new Gdiplus::Graphics(newBuf);
  } catch (...) {
    // Graphics construction failed (e.g. bad_alloc) — keep the old one.
    delete newBuf;
    return;
  }
  if (newGfx->GetLastStatus() != Gdiplus::Ok) {
    delete newGfx;
    delete newBuf;
    return;
  }

  newGfx->SetSmoothingMode(Gdiplus::SmoothingModeAntiAlias);
  newGfx->SetTextRenderingHint(Gdiplus::TextRenderingHintClearTypeGridFit);

  delete backbuffer_;
  delete graphics_;
  backbuffer_ = newBuf;
  graphics_ = newGfx;

  canvasWidth_ = width;
  canvasHeight_ = height;
}

void Win32Canvas::beginPaint() {
  // Don't clear the backbuffer — dirty-rect painting means only
  // the invalidated areas get repainted; the rest must persist.
}

void Win32Canvas::endPaint() {
  if (!hwnd_ || !backbuffer_)
    return;

  HDC hdc = GetDC(hwnd_);
  if (!hdc)
    return;

  Gdiplus::Graphics screenGraphics(hdc);
  screenGraphics.DrawImage(backbuffer_, 0, 0, canvasWidth_, canvasHeight_);

  ReleaseDC(hwnd_, hdc);
}

void Win32Canvas::setColor(const Color &color) {
  Gdiplus::Color newColor = toGdiColor(color);
  if (newColor.GetValue() == currentColor_.GetValue())
    return;
  currentColor_ = newColor;
  // Reuse brush and pen — only update their color
  brush_->SetColor(currentColor_);
  pen_->SetColor(currentColor_);
}

void Win32Canvas::setFont(const Font &font) {
  // Only trust the descriptor compare while a cached font is actually
  // active — after flushFontCache() currentFont_ is null and the font
  // must be re-created even for an identical descriptor.
  if (currentFont_ && font == currentFontDesc_)
    return;
  currentFontDesc_ = font;
  currentFont_ = getOrCreateFont(font);
}

Gdiplus::Font *Win32Canvas::getOrCreateFont(const Font &f) {
  auto it = fontCache_.find(f);
  if (it != fontCache_.end()) {
    return it->second;
  }

  int len = MultiByteToWideChar(CP_UTF8, 0, f.family.c_str(), -1, nullptr, 0);
  std::wstring wfamily(len, L'\0');
  MultiByteToWideChar(CP_UTF8, 0, f.family.c_str(), -1, &wfamily[0], len);
  if (len > 0)
    wfamily.resize(len - 1);

  HDC screenDC = GetDC(nullptr);
  LOGFONTW lf = {};
  lf.lfHeight = -f.size;
  lf.lfWeight =
      (static_cast<int>(f.weight) >= static_cast<int>(FontWeight::Bold))
          ? FW_BOLD
          : FW_NORMAL;
  lf.lfItalic = (f.style == FontStyle::Italic) ? TRUE : FALSE;
  lf.lfCharSet = DEFAULT_CHARSET;
  lf.lfQuality = CLEARTYPE_QUALITY;
  lf.lfOutPrecision = OUT_TT_PRECIS;
  wcsncpy(lf.lfFaceName, wfamily.c_str(), LF_FACESIZE - 1);
  lf.lfFaceName[LF_FACESIZE - 1] = L'\0';

  Gdiplus::Font *font = new Gdiplus::Font(screenDC, &lf);
  ReleaseDC(nullptr, screenDC);

  fontCache_[f] = font;
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

Gdiplus::Color Win32Canvas::toGdiColor(const Color &c) {
  return Gdiplus::Color(c.a, c.r, c.g, c.b);
}

void Win32Canvas::fillRect(const Rect &rect) {
  if (!graphics_ || !brush_)
    return;
  graphics_->FillRectangle(brush_, rect.x, rect.y, rect.width, rect.height);
}

void Win32Canvas::strokeRect(const Rect &rect, int lineWidth) {
  if (!graphics_ || !pen_)
    return;
  pen_->SetWidth(static_cast<Gdiplus::REAL>(lineWidth));
  graphics_->DrawRectangle(pen_, rect.x, rect.y, rect.width, rect.height);
}

void Win32Canvas::drawText(const std::string &text, const Rect &rect,
                           int flags) {
  if (!graphics_)
    return;

  int len = MultiByteToWideChar(CP_UTF8, 0, text.c_str(), -1, nullptr, 0);
  std::wstring wtext(len, L'\0');
  MultiByteToWideChar(CP_UTF8, 0, text.c_str(), -1, &wtext[0], len);
  if (len > 0)
    wtext.resize(len - 1);

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

  Gdiplus::Font *font = currentFont_;
  if (!font) {
    font = getOrCreateFont(Font::systemDefault(12));
  }

  if (brush_) {
    graphics_->DrawString(wtext.c_str(), -1, font, layoutRect, &format, brush_);
  }
}

void Win32Canvas::drawLine(const Point &p1, const Point &p2, int lineWidth) {
  if (!graphics_ || !pen_)
    return;
  pen_->SetWidth(static_cast<Gdiplus::REAL>(lineWidth));
  graphics_->DrawLine(pen_, p1.x, p1.y, p2.x, p2.y);
}

void Win32Canvas::fillEllipse(const Rect &rect) {
  if (!graphics_ || !brush_)
    return;
  graphics_->FillEllipse(brush_, rect.x, rect.y, rect.width, rect.height);
}

void Win32Canvas::strokeEllipse(const Rect &rect, int lineWidth) {
  if (!graphics_ || !pen_)
    return;
  pen_->SetWidth(static_cast<Gdiplus::REAL>(lineWidth));
  graphics_->DrawEllipse(pen_, rect.x, rect.y, rect.width, rect.height);
}

void Win32Canvas::fillRoundedRect(const Rect &rect, int radius) {
  if (!graphics_ || !brush_)
    return;
  int r = std::min(radius, std::min(rect.width, rect.height) / 2);
  if (r <= 0) {
    fillRect(rect);
    return;
  }
  int x = rect.x, y = rect.y, w = rect.width, h = rect.height;
  int d = r * 2;
  Gdiplus::GraphicsPath path;
  path.AddArc(x, y, d, d, 180, 90);
  path.AddArc(x + w - d, y, d, d, 270, 90);
  path.AddArc(x + w - d, y + h - d, d, d, 0, 90);
  path.AddArc(x, y + h - d, d, d, 90, 90);
  path.CloseFigure();
  graphics_->FillPath(brush_, &path);
}

void Win32Canvas::strokeRoundedRect(const Rect &rect, int radius,
                                    int lineWidth) {
  if (!graphics_ || !pen_)
    return;
  int r = std::min(radius, std::min(rect.width, rect.height) / 2);
  if (r <= 0) {
    strokeRect(rect, lineWidth);
    return;
  }
  int x = rect.x, y = rect.y, w = rect.width, h = rect.height;
  int d = r * 2;
  Gdiplus::GraphicsPath path;
  path.AddArc(x, y, d, d, 180, 90);
  path.AddArc(x + w - d, y, d, d, 270, 90);
  path.AddArc(x + w - d, y + h - d, d, d, 0, 90);
  path.AddArc(x, y + h - d, d, d, 90, 90);
  path.CloseFigure();
  pen_->SetWidth(static_cast<Gdiplus::REAL>(lineWidth));
  graphics_->DrawPath(pen_, &path);
}

void Win32Canvas::fillLinearGradient(const Rect &rect, const Color &from,
                                     const Color &to, bool vertical,
                                     const Rect &fullBounds) {
  if (!graphics_)
    return;
  // fullBounds is the gradient parameter space (0%→100% runs across it);
  // rect is only the area actually filled. An empty fullBounds keeps the
  // legacy behavior: parameter space == filled rect.
  Gdiplus::Rect paramSpace = fullBounds.isEmpty()
                                 ? Gdiplus::Rect(rect.x, rect.y, rect.width,
                                                 rect.height)
                                 : Gdiplus::Rect(fullBounds.x, fullBounds.y,
                                                 fullBounds.width,
                                                 fullBounds.height);
  Gdiplus::LinearGradientBrush brush(
      paramSpace, Gdiplus::Color(from.a, from.r, from.g, from.b),
      Gdiplus::Color(to.a, to.r, to.g, to.b), vertical ? 90.0f : 0.0f);
  graphics_->FillRectangle(&brush, rect.x, rect.y, rect.width, rect.height);
}

void Win32Canvas::drawImage(const std::string &path, const Rect &rect) {
  if (!graphics_)
    return;

  // Cache lookup
  auto it = imageCache_.find(path);
  if (it == imageCache_.end()) {
    int len = MultiByteToWideChar(CP_UTF8, 0, path.c_str(), -1, nullptr, 0);
    std::wstring wpath(len, L'\0');
    MultiByteToWideChar(CP_UTF8, 0, path.c_str(), -1, &wpath[0], len);
    if (len > 0)
      wpath.resize(len - 1);
    Gdiplus::Image *img = new Gdiplus::Image(wpath.c_str());
    if (img->GetLastStatus() != Gdiplus::Ok) {
      delete img;
      return;
    }
    it = imageCache_.insert({path, img}).first;
  }
  graphics_->DrawImage(it->second, rect.x, rect.y, rect.width, rect.height);
}

Size Win32Canvas::imageSize(const std::string &path) {
  auto it = imageCache_.find(path);
  if (it != imageCache_.end()) {
    return {static_cast<int>(it->second->GetWidth()),
            static_cast<int>(it->second->GetHeight())};
  }

  int len = MultiByteToWideChar(CP_UTF8, 0, path.c_str(), -1, nullptr, 0);
  std::wstring wpath(len, L'\0');
  MultiByteToWideChar(CP_UTF8, 0, path.c_str(), -1, &wpath[0], len);
  if (len > 0)
    wpath.resize(len - 1);
  Gdiplus::Image *image = new Gdiplus::Image(wpath.c_str());
  if (image->GetLastStatus() == Gdiplus::Ok) {
    int w = static_cast<int>(image->GetWidth());
    int h = static_cast<int>(image->GetHeight());
    imageCache_[path] = image;
    return {w, h};
  }
  delete image;
  return {};
}

void Win32Canvas::drawPixelBuffer(const uint8_t *rgba, int w, int h,
                                  const Rect &rect) {
  if (!graphics_ || !rgba || w <= 0 || h <= 0)
    return;

  Gdiplus::Bitmap bitmap(w, h, w * 4, PixelFormat32bppARGB,
                         const_cast<uint8_t *>(rgba));
  if (bitmap.GetLastStatus() == Gdiplus::Ok) {
    graphics_->DrawImage(&bitmap, rect.x, rect.y, rect.width, rect.height);
  }
}

Size Win32Canvas::measureText(const std::string &text) {
  int len = MultiByteToWideChar(CP_UTF8, 0, text.c_str(), -1, nullptr, 0);
  std::wstring wtext(len, L'\0');
  MultiByteToWideChar(CP_UTF8, 0, text.c_str(), -1, &wtext[0], len);
  if (len > 0)
    wtext.resize(len - 1);

  Gdiplus::Font *font = currentFont_;
  if (!font) {
    font = getOrCreateFont(Font::systemDefault(12));
  }

  // Reuse a small measurement bitmap instead of creating one per call
  if (!measureBitmap_) {
    measureBitmap_ = new Gdiplus::Bitmap(1, 1, PixelFormat32bppARGB);
    measureGraphics_ = new Gdiplus::Graphics(measureBitmap_);
  }

  Gdiplus::RectF boundRect;
  measureGraphics_->MeasureString(wtext.c_str(), -1, font,
                                  Gdiplus::PointF(0, 0), &boundRect);

  return {static_cast<int>(boundRect.Width + 0.5f),
          static_cast<int>(boundRect.Height + 0.5f)};
}

} // namespace ltgui

#endif // LTGUI_PLATFORM_WINDOWS
