#include "platform/gpu/gpu_canvas.h"
#include "log.h"
#include <cmath>
#include <cstring>

#ifdef LTGUI_PLATFORM_WINDOWS
#ifndef NOMINMAX
#define NOMINMAX
#endif
// windows.h MUST precede gdiplus.h (GDI+ needs PROPID etc. from wtypes.h).
#include <windows.h>
#include <gdiplus.h>
#endif

#ifdef LTGUI_PLATFORM_MACOS
#include <CoreGraphics/CoreGraphics.h>
#include <ImageIO/ImageIO.h>
#endif

namespace ltgui {
namespace gpu {

// Factory functions
GpuDevice *CreateD3D11Device();
GpuDevice *CreateGLDevice();

// ---- GDI+ helper for image decoding (Windows only) ----
#ifdef LTGUI_PLATFORM_WINDOWS
namespace {
static ULONG_PTR g_gdipToken = 0;
static bool g_gdipInit = false;

void ensureGdiPlus() {
  if (!g_gdipInit) {
    Gdiplus::GdiplusStartupInput inp;
    Gdiplus::GdiplusStartup(&g_gdipToken, &inp, nullptr);
    g_gdipInit = true;
  }
}
} // namespace
#endif

GpuCanvas::GpuCanvas()
    : currentColor_(Color::Black), currentFont_(Font::systemDefault(12)) {}

GpuCanvas::~GpuCanvas() {
  if (tempTexId_ >= 0 && renderer_) {
    renderer_->textures().release(tempTexId_);
  }
  for (auto &kv : imageCache_) {
    if (kv.second.texId >= 0 && renderer_) {
      renderer_->textures().release(kv.second.texId);
    }
  }
  // renderer_, fontAtlas_, device_ destroyed automatically via unique_ptr
  // in reverse declaration order: fontAtlas_ → renderer_ → device_.
  // This ensures GL textures are deleted before EGL termination.
  // Do NOT call device_->shutdown() here — it would terminate EGL/GL
  // before texture destructors run, causing undefined behaviour.
}

bool GpuCanvas::initialize(void *windowHandle, int width, int height) {
  gpuInfo_ = selectBestGpu();

  // Even if no GPUs were detected, still attempt device creation.
  // On Windows, D3D11CreateDevice with D3D_DRIVER_TYPE_WARP provides
  // a software rasterizer. On Linux, EGL might have a software fallback.
  // This ensures GPU acceleration works on VMs and driverless systems.
  GpuBackend target = gpuInfo_.backend;
#ifdef LTGUI_PLATFORM_WINDOWS
  if (target == GpuBackend::None)
    target = GpuBackend::D3D11;
#elif defined(LTGUI_PLATFORM_LINUX)
  if (target == GpuBackend::None)
    target = GpuBackend::OpenGL;
#endif

  if (target == GpuBackend::None) {
    LOG_INFO("GPU", "No GPU back-end available on this platform.");
    return false;
  }

#ifdef LTGUI_PLATFORM_WINDOWS
  device_.reset(CreateD3D11Device());
#elif defined(LTGUI_PLATFORM_LINUX)
  device_.reset(CreateGLDevice());
#else
  (void)windowHandle;
#endif

  if (!device_ || !device_->initialize(windowHandle, width, height)) {
    LOG_INFO("GPU", "GPU device init failed.");
    device_.reset();
    return false;
  }

  renderer_ = std::make_unique<Renderer2D>(device_.get());
  renderer_->setSize(width, height);

  fontAtlas_ =
      std::make_unique<FontAtlas>(device_.get(), &renderer_->textures());
  renderer_->setFontAtlas(fontAtlas_.get());

  // Load a default system font so text rendering works out of the box.
  Font defaultFont = Font("Deng", 12);
  bool fontLoaded = false;
  for (const char *path : defaultFontSearchPaths()) {
    if (fontAtlas_->loadFontFile(defaultFont, path)) {
      LOG_INFO("GPU", "Loaded default font: %s", path);
      fontLoaded = true;
      break;
    }
  }
  if (!fontLoaded) {
    LOG_WARN("GPU", "No system font found. Text will be invisible. "
                    "Call GpuCanvas::loadFontFile() with a .ttf path.");
  }

  LOG_INFO("GPU", "Initialized: %s, %dx%d", device_->name(), width, height);
  initialized_ = true;
  return true;
}

void GpuCanvas::resize(int width, int height) {
  if (device_)
    device_->resize(width, height);
  if (renderer_)
    renderer_->setSize(width, height);
}

void GpuCanvas::beginPaint() {
  if (!initialized_ || !device_)
    return;
  device_->beginFrame();
  if (renderer_)
    renderer_->begin();
}

void GpuCanvas::endPaint() {
  if (!initialized_ || !device_)
    return;
  if (renderer_)
    renderer_->end();
  device_->endFrame();
}

void GpuCanvas::setColor(const Color &color) { currentColor_ = color; }

void GpuCanvas::setFont(const Font &font) { currentFont_ = font; }

void GpuCanvas::fillRect(const Rect &rect) {
  if (renderer_)
    renderer_->fillRect(rect, currentColor_);
}

void GpuCanvas::fillLinearGradient(const Rect &rect, const Color &from,
                                   const Color &to, bool vertical) {
  if (renderer_)
    renderer_->fillLinearGradient(rect, from, to, vertical);
}

void GpuCanvas::strokeRect(const Rect &rect, int lineWidth) {
  if (renderer_)
    renderer_->strokeRect(rect, (float)lineWidth, currentColor_);
}

void GpuCanvas::fillRoundedRect(const Rect &rect, int radius) {
  if (renderer_)
    renderer_->fillRoundedRect(rect, (float)radius, currentColor_);
}

void GpuCanvas::strokeRoundedRect(const Rect &rect, int radius, int lineWidth) {
  if (renderer_)
    renderer_->strokeRoundedRect(rect, (float)radius, (float)lineWidth,
                                 currentColor_);
}

void GpuCanvas::drawText(const std::string &text, const Rect &rect, int flags) {
  if (!renderer_ || !fontAtlas_ || text.empty())
    return;

#ifdef LTGUI_HAS_STB_TRUETYPE
  float x = (float)rect.x;
  float y = (float)rect.y;

  Size measured = measureText(text);
  if (flags & AlignCenter) {
    x += (rect.width - measured.width) / 2.0f;
  } else if (flags & AlignRight) {
    x += rect.width - measured.width;
  }
  if (flags & AlignVCenter) {
    y += (rect.height - measured.height) / 2.0f;
  } else if (flags & AlignBottom) {
    y += rect.height - measured.height;
  }

  // Y is the top of the layout box. Compute the baseline position:
  // the ascent (baseline-to-top) is font-dependent. stb_truetype glyph
  // offsetY values are relative to the baseline (negative for ascenders),
  // so each glyph's top-left = (baselineX + offsetX, baselineY + offsetY).
  float baselineY = y + fontAtlas_->getAscent(currentFont_);
  // If no ascent info (font not loaded), fall back to using the ascent-
  // proportion of the measured line height (ascent ≈ 80% of line height).
  if (baselineY <= y) {
    baselineY = y + measured.height * 0.8f;
  }

  const uint8_t *p = reinterpret_cast<const uint8_t *>(text.data());
  const uint8_t *end = p + text.size();

  while (p < end) {
    uint32_t cp;
    if ((*p & 0x80) == 0) {
      cp = *p++;
    } else if ((*p & 0xE0) == 0xC0) {
      if (p + 1 >= end) {
        p++;
        continue;
      }
      cp = (*p++ & 0x1F) << 6;
      cp |= (*p++ & 0x3F);
    } else if ((*p & 0xF0) == 0xE0) {
      if (p + 2 >= end) {
        p++;
        continue;
      }
      cp = (*p++ & 0x0F) << 12;
      cp |= (*p++ & 0x3F) << 6;
      cp |= (*p++ & 0x3F);
    } else if ((*p & 0xF8) == 0xF0) {
      if (p + 3 >= end) {
        p++;
        continue;
      }
      cp = (*p++ & 0x07) << 18;
      cp |= (*p++ & 0x3F) << 12;
      cp |= (*p++ & 0x3F) << 6;
      cp |= (*p++ & 0x3F);
    } else {
      p++;
      continue;
    }

    const GlyphEntry *g = fontAtlas_->getGlyph(cp, currentFont_);
    if (g && g->texId >= 0 && g->w > 0) {
      Rect dst((int)x + g->offsetX, (int)(baselineY + g->offsetY), g->w, g->h);
      Rect src(g->atlasX, g->atlasY, g->w, g->h);
      renderer_->drawGlyph(g->texId, dst, src, currentColor_);
    }
    if (g)
      x += g->advance;
  }
#else
  // No stb_truetype — draw a crossed-out placeholder so the user can see
  // text is missing rather than a misleading solid color block.
  renderer_->drawLine({rect.x, rect.y}, {rect.right(), rect.bottom()}, 1.0f,
                      currentColor_);
  renderer_->drawLine({rect.right(), rect.y}, {rect.x, rect.bottom()}, 1.0f,
                      currentColor_);
#endif
  (void)flags;
}

void GpuCanvas::drawLine(const Point &p1, const Point &p2, int lineWidth) {
  if (renderer_)
    renderer_->drawLine(p1, p2, (float)lineWidth, currentColor_);
}

void GpuCanvas::fillEllipse(const Rect &rect) {
  if (renderer_)
    renderer_->fillEllipse(rect, currentColor_);
}

void GpuCanvas::strokeEllipse(const Rect &rect, int lineWidth) {
  if (renderer_)
    renderer_->strokeEllipse(rect, static_cast<float>(lineWidth),
                             currentColor_);
}

void GpuCanvas::drawImage(const std::string &path, const Rect &rect) {
  if (!renderer_ || path.empty())
    return;
  auto it = imageCache_.find(path);
  if (it == imageCache_.end()) {
    loadImageTexture(path);
    it = imageCache_.find(path);
  }
  if (it != imageCache_.end() && it->second.texId >= 0) {
    renderer_->drawImage(it->second.texId, rect);
  } else if (it != imageCache_.end() && it->second.texId == -1) {
    // Load failed previously — draw a placeholder cross so the user
    // can see an image is missing, but don't retry loading every frame.
    renderer_->drawLine({rect.x, rect.y}, {rect.right(), rect.bottom()}, 1.0f,
                        Color::Red);
    renderer_->drawLine({rect.right(), rect.y}, {rect.x, rect.bottom()}, 1.0f,
                        Color::Red);
  }
}

Size GpuCanvas::imageSize(const std::string &path) {
  auto it = imageCache_.find(path);
  if (it != imageCache_.end())
    return {it->second.width, it->second.height};
  return {};
}

void GpuCanvas::drawPixelBuffer(const uint8_t *rgba, int w, int h,
                                const Rect &rect) {
  if (!renderer_ || !rgba)
    return;
  // Release previously temp-allocated texture if any
  if (tempTexId_ >= 0) {
    renderer_->textures().release(tempTexId_);
    tempTexId_ = -1;
  }
  tempTexId_ = renderer_->textures().upload(w, h, rgba);
  renderer_->drawImage(tempTexId_, rect);
}

Size GpuCanvas::measureText(const std::string &text) {
  if (renderer_)
    return renderer_->measureText(text, currentFont_);
  return {0, 0};
}

bool GpuCanvas::loadFontFile(const Font &font, const char *ttfPath) {
  if (!fontAtlas_)
    return false;
  return fontAtlas_->loadFontFile(font, ttfPath);
}

void GpuCanvas::loadImageTexture(const std::string &path) {
  if (!device_ || path.empty())
    return;

  int w = 0, h = 0;
  std::vector<uint8_t> rgba;

#ifdef LTGUI_PLATFORM_WINDOWS
  ensureGdiPlus();

  int len = MultiByteToWideChar(CP_UTF8, 0, path.c_str(), -1, nullptr, 0);
  std::wstring wpath(len, L'\0');
  MultiByteToWideChar(CP_UTF8, 0, path.c_str(), -1, &wpath[0], len);
  if (len > 0)
    wpath.resize(len - 1);

  Gdiplus::Bitmap bmp(wpath.c_str());
  if (bmp.GetLastStatus() != Gdiplus::Ok) {
    imageCache_[path] = {-1, 0, 0}; // mark as failed
    return;
  }

  w = (int)bmp.GetWidth();
  h = (int)bmp.GetHeight();
  if (w <= 0 || h <= 0) {
    imageCache_[path] = {-1, 0, 0};
    return;
  }

  rgba.resize(w * h * 4);
  Gdiplus::Rect rc(0, 0, w, h);
  Gdiplus::BitmapData bd;
  bmp.LockBits(&rc, Gdiplus::ImageLockModeRead, PixelFormat32bppARGB, &bd);
  memcpy(rgba.data(), bd.Scan0, w * h * 4);
  bmp.UnlockBits(&bd);
#elif defined(LTGUI_PLATFORM_MACOS)
  // macOS: decode via ImageIO/CoreGraphics
  {
    CFStringRef cfPath = CFStringCreateWithCString(
        kCFAllocatorDefault, path.c_str(), kCFStringEncodingUTF8);
    if (!cfPath) {
      imageCache_[path] = {-1, 0, 0};
      return;
    }

    CFURLRef url = CFURLCreateWithFileSystemPath(kCFAllocatorDefault, cfPath,
                                                 kCFURLPOSIXPathStyle, false);
    CFRelease(cfPath);
    if (!url) {
      imageCache_[path] = {-1, 0, 0};
      return;
    }

    CGImageSourceRef src = CGImageSourceCreateWithURL(url, nullptr);
    CFRelease(url);
    if (!src) {
      imageCache_[path] = {-1, 0, 0};
      return;
    }

    CGImageRef img = CGImageSourceCreateImageAtIndex(src, 0, nullptr);
    CFRelease(src);
    if (!img) {
      imageCache_[path] = {-1, 0, 0};
      return;
    }

    w = (int)CGImageGetWidth(img);
    h = (int)CGImageGetHeight(img);
    if (w <= 0 || h <= 0) {
      CGImageRelease(img);
      imageCache_[path] = {-1, 0, 0};
      return;
    }

    // Draw into a bitmap context to get RGBA pixels
    rgba.resize(w * h * 4, 0);
    CGColorSpaceRef cs = CGColorSpaceCreateDeviceRGB();
    CGContextRef ctx = CGBitmapContextCreate(rgba.data(), w, h, 8, w * 4, cs,
                                             kCGImageAlphaPremultipliedLast |
                                                 kCGBitmapByteOrder32Big);
    CGColorSpaceRelease(cs);
    if (!ctx) {
      CGImageRelease(img);
      imageCache_[path] = {-1, 0, 0};
      return;
    }

    CGContextDrawImage(ctx, CGRectMake(0, 0, (CGFloat)w, (CGFloat)h), img);
    CGContextRelease(ctx);
    CGImageRelease(img);

    // CoreGraphics produces premultiplied alpha by default, but the GPU
    // blending pipeline uses non-premultiplied alpha (GL_SRC_ALPHA blending).
    // Unpremultiply to prevent dark halos on semi-transparent pixels.
    for (int i = 0; i < w * h; i++) {
      uint8_t *p = &rgba[i * 4];
      if (p[3] > 0 && p[3] < 255) {
        p[0] = static_cast<uint8_t>((static_cast<int>(p[0]) * 255) / p[3]);
        p[1] = static_cast<uint8_t>((static_cast<int>(p[1]) * 255) / p[3]);
        p[2] = static_cast<uint8_t>((static_cast<int>(p[2]) * 255) / p[3]);
      }
    }
  }
#else
  (void)path;
  // Mark as failed so we don't retry every frame on non-Windows/non-macOS
  // platforms
  imageCache_[path] = {-1, 0, 0};
  return;
#endif

  if (w > 0 && h > 0 && !rgba.empty() && renderer_) {
    int texId = renderer_->textures().upload(w, h, rgba.data());
    imageCache_[path] = {texId, w, h};
  } else {
    imageCache_[path] = {-1, 0, 0};
  }
}

} // namespace gpu
} // namespace ltgui
