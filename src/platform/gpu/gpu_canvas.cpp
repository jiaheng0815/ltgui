#include "platform/gpu/gpu_canvas.h"
#include <cstdio>
#include <cstring>
#include <cmath>

#ifdef LTGUI_PLATFORM_WINDOWS
#include <gdiplus.h>
#endif

namespace ltgui {
namespace gpu {

// Factory functions
GpuDevice* CreateD3D11Device();
GpuDevice* CreateGLDevice();

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
}
#endif

GpuCanvas::GpuCanvas()
    : currentColor_(Color::Black)
    , currentFont_("Segoe UI", 12) {}

GpuCanvas::~GpuCanvas() {
    for (auto& kv : imageCache_) {
        if (kv.second.texId >= 0 && renderer_) {
            renderer_->textures().release(kv.second.texId);
        }
    }
    if (renderer_) delete renderer_;
    if (fontAtlas_) delete fontAtlas_;
    if (device_) {
        device_->shutdown();
        delete device_;
    }
}

bool GpuCanvas::initialize(void* windowHandle, int width, int height) {
    gpuInfo_ = selectBestGpu();
    if (gpuInfo_.backend == GpuBackend::None) {
        printf("[GpuCanvas] No GPU found, falling back to CPU rendering.\n");
        return false;
    }

#ifdef LTGUI_PLATFORM_WINDOWS
    device_ = CreateD3D11Device();
#elif defined(LTGUI_PLATFORM_LINUX)
    device_ = CreateGLDevice();
#else
    (void)windowHandle;
    device_ = nullptr;
#endif

    if (!device_ || !device_->initialize(windowHandle, width, height)) {
        printf("[GpuCanvas] GPU device init failed.\n");
        delete device_; device_ = nullptr;
        return false;
    }

    renderer_ = new Renderer2D(device_);
    renderer_->setSize(width, height);

    fontAtlas_ = new FontAtlas(device_);
    renderer_->setFontAtlas(fontAtlas_);

    printf("[GpuCanvas] Initialized: %s, %dx%d\n", device_->name(), width, height);
    initialized_ = true;
    return true;
}

void GpuCanvas::resize(int width, int height) {
    if (device_) device_->resize(width, height);
    if (renderer_) renderer_->setSize(width, height);
}

void GpuCanvas::beginPaint() {
    if (!initialized_ || !device_) return;
    device_->beginFrame();
    if (renderer_) renderer_->begin();
}

void GpuCanvas::endPaint() {
    if (!initialized_ || !device_) return;
    if (renderer_) renderer_->end();
    device_->endFrame();
}

void GpuCanvas::setColor(const Color& color) {
    currentColor_ = color;
}

void GpuCanvas::setFont(const Font& font) {
    currentFont_ = font;
}

void GpuCanvas::fillRect(const Rect& rect) {
    if (renderer_) renderer_->fillRect(rect, currentColor_);
}

void GpuCanvas::strokeRect(const Rect& rect, int lineWidth) {
    if (renderer_) renderer_->strokeRect(rect, (float)lineWidth, currentColor_);
}

void GpuCanvas::fillRoundedRect(const Rect& rect, int radius) {
    if (renderer_) renderer_->fillRoundedRect(rect, (float)radius, currentColor_);
}

void GpuCanvas::strokeRoundedRect(const Rect& rect, int radius, int lineWidth) {
    if (renderer_) renderer_->strokeRoundedRect(rect, (float)radius, (float)lineWidth, currentColor_);
}

void GpuCanvas::drawText(const std::string& text, const Rect& rect, int flags) {
    if (!renderer_ || !fontAtlas_ || text.empty()) return;

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

    const uint8_t* p = reinterpret_cast<const uint8_t*>(text.data());
    const uint8_t* end = p + text.size();

    while (p < end) {
        uint32_t cp;
        if ((*p & 0x80) == 0)      { cp = *p++; }
        else if ((*p & 0xE0) == 0xC0) { cp = (*p++ & 0x1F) << 6;  cp |= (*p++ & 0x3F); }
        else if ((*p & 0xF0) == 0xE0) { cp = (*p++ & 0x0F) << 12; cp |= (*p++ & 0x3F) << 6; cp |= (*p++ & 0x3F); }
        else if ((*p & 0xF8) == 0xF0) { cp = (*p++ & 0x07) << 18; cp |= (*p++ & 0x3F) << 12; cp |= (*p++ & 0x3F) << 6; cp |= (*p++ & 0x3F); }
        else { p++; continue; }

        const GlyphEntry* g = fontAtlas_->getGlyph(cp, currentFont_);
        if (g && g->texId >= 0 && g->w > 0) {
            Rect dst((int)x + g->offsetX, (int)y + g->offsetY, g->w, g->h);
            renderer_->drawGlyph(g->texId, dst, {}, currentColor_);
        }
        if (g) x += g->advance;
    }
#else
    renderer_->fillRect(rect, currentColor_);
#endif
    (void)flags;
}

void GpuCanvas::drawLine(const Point& p1, const Point& p2, int lineWidth) {
    if (renderer_) renderer_->drawLine(p1, p2, (float)lineWidth, currentColor_);
}

void GpuCanvas::fillEllipse(const Rect& rect) {
    if (renderer_) renderer_->fillEllipse(rect, currentColor_);
}

void GpuCanvas::strokeEllipse(const Rect& rect, int lineWidth) {
    if (renderer_) renderer_->fillEllipse(rect, currentColor_);
    (void)lineWidth;
}

void GpuCanvas::drawImage(const std::string& path, const Rect& rect) {
    if (!renderer_ || path.empty()) return;
    auto it = imageCache_.find(path);
    if (it == imageCache_.end()) {
        loadImageTexture(path);
        it = imageCache_.find(path);
    }
    if (it != imageCache_.end() && it->second.texId >= 0) {
        renderer_->drawImage(it->second.texId, rect);
    }
}

Size GpuCanvas::imageSize(const std::string& path) {
    auto it = imageCache_.find(path);
    if (it != imageCache_.end()) return {it->second.width, it->second.height};
    return {};
}

void GpuCanvas::drawPixelBuffer(const uint8_t* rgba, int w, int h, const Rect& rect) {
    if (!renderer_ || !rgba) return;
    int texId = renderer_->textures().upload(w, h, rgba);
    renderer_->drawImage(texId, rect);
}

Size GpuCanvas::measureText(const std::string& text) {
    if (renderer_) return renderer_->measureText(text, currentFont_);
    return {0, 0};
}

void GpuCanvas::loadImageTexture(const std::string& path) {
    if (!device_ || path.empty()) return;

#ifdef LTGUI_PLATFORM_WINDOWS
    ensureGdiPlus();

    int len = MultiByteToWideChar(CP_UTF8, 0, path.c_str(), -1, nullptr, 0);
    std::wstring wpath(len, L'\0');
    MultiByteToWideChar(CP_UTF8, 0, path.c_str(), -1, &wpath[0], len);
    if (len > 0) wpath.resize(len - 1);

    Gdiplus::Bitmap bmp(wpath.c_str());
    if (bmp.GetLastStatus() != Gdiplus::Ok) return;

    int w = (int)bmp.GetWidth(), h = (int)bmp.GetHeight();
    if (w <= 0 || h <= 0) return;

    std::vector<uint8_t> rgba(w * h * 4);
    Gdiplus::Rect rc(0, 0, w, h);
    Gdiplus::BitmapData bd;
    bmp.LockBits(&rc, Gdiplus::ImageLockModeRead, PixelFormat32bppARGB, &bd);
    memcpy(rgba.data(), bd.Scan0, w * h * 4);
    bmp.UnlockBits(&bd);

    int texId = renderer_->textures().upload(w, h, rgba.data());
    imageCache_[path] = {texId, w, h};
#else
    (void)path;
#endif
}

} // namespace gpu
} // namespace ltgui
