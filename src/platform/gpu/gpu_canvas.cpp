#include "platform/gpu/gpu_canvas.h"
#include "log.h"
#include <cstring>
#include <cmath>

#ifdef LTGUI_PLATFORM_WINDOWS
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
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
    , currentFont_(Font::systemDefault(12)) {}

GpuCanvas::~GpuCanvas() {
    if (tempTexId_ >= 0 && renderer_) {
        renderer_->textures().release(tempTexId_);
    }
    for (auto& kv : imageCache_) {
        if (kv.second.texId >= 0 && renderer_) {
            renderer_->textures().release(kv.second.texId);
        }
    }
    // renderer_, fontAtlas_, device_ destroyed automatically via unique_ptr
    if (device_) {
        device_->shutdown();
    }
}

bool GpuCanvas::initialize(void* windowHandle, int width, int height) {
    gpuInfo_ = selectBestGpu();
    if (gpuInfo_.backend == GpuBackend::None) {
        LOG_INFO("GPU", "No GPU found, falling back to CPU rendering.");
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

    fontAtlas_ = std::make_unique<FontAtlas>(device_.get());
    renderer_->setFontAtlas(fontAtlas_.get());

    // Load a default system font so text rendering works out of the box.
    // Try platform-specific system font paths; if none work, text will be
    // invisible (caller should use loadFontFile to provide a font).
    Font defaultFont = Font::systemDefault(12);
    const char* fontPaths[] = {
#ifdef LTGUI_PLATFORM_WINDOWS
        "C:\\Windows\\Fonts\\segoeui.ttf",
        "C:\\Windows\\Fonts\\arial.ttf",
        "C:\\Windows\\Fonts\\calibri.ttf",
#elif defined(LTGUI_PLATFORM_LINUX)
        "/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf",
        "/usr/share/fonts/TTF/DejaVuSans.ttf",
        "/usr/share/fonts/truetype/liberation/LiberationSans-Regular.ttf",
        "/usr/share/fonts/truetype/noto/NotoSans-Regular.ttf",
#elif defined(LTGUI_PLATFORM_MACOS)
        "/System/Library/Fonts/Helvetica.ttc",
        "/System/Library/Fonts/SFNSText.ttf",
        "/Library/Fonts/Arial.ttf",
#endif
    };
    for (const char* path : fontPaths) {
        if (fontAtlas_->loadFontFile(defaultFont, path)) {
            LOG_INFO("GPU", "Loaded default font: %s", path);
            break;
        }
    }

    LOG_INFO("GPU", "Initialized: %s, %dx%d", device_->name(), width, height);
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
            Rect src(g->atlasX, g->atlasY, g->w, g->h);
            renderer_->drawGlyph(g->texId, dst, src, currentColor_);
        }
        if (g) x += g->advance;
    }
#else
    // No stb_truetype — draw a crossed-out placeholder so the user can see
    // text is missing rather than a misleading solid color block.
    renderer_->drawLine({rect.x, rect.y}, {rect.right(), rect.bottom()}, 1.0f, currentColor_);
    renderer_->drawLine({rect.right(), rect.y}, {rect.x, rect.bottom()}, 1.0f, currentColor_);
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
    if (renderer_) renderer_->strokeEllipse(rect, static_cast<float>(lineWidth), currentColor_);
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
    // Release previously temp-allocated texture if any
    if (tempTexId_ >= 0) {
        renderer_->textures().release(tempTexId_);
        tempTexId_ = -1;
    }
    tempTexId_ = renderer_->textures().upload(w, h, rgba);
    renderer_->drawImage(tempTexId_, rect);
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
