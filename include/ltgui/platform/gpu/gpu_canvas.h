#pragma once
#include "platform/native_canvas.h"
#include "platform/gpu/gpu_device.h"
#include "platform/gpu/gpu_renderer.h"
#include "platform/gpu/gpu_font_atlas.h"
#include "platform/gpu/gpu_detect.h"
#include "color.h"
#include "font.h"
#include <string>
#include <memory>
#include <unordered_map>

namespace ltgui {
namespace gpu {

class GpuCanvas : public NativeCanvas {
public:
    GpuCanvas();
    ~GpuCanvas() override;

    // Initialize GPU backend. Returns true if GPU was successfully set up.
    // Falls back to nullptr — caller should use GDI+/X11 in that case.
    bool initialize(void* windowHandle, int width, int height);

    // NativeCanvas implementation
    void resize(int width, int height) override;
    void beginPaint() override;
    void endPaint() override;

    void setColor(const Color& color) override;
    void setFont(const Font& font) override;

    void fillRect(const Rect& rect) override;
    void strokeRect(const Rect& rect, int lineWidth = 1) override;
    void fillRoundedRect(const Rect& rect, int radius) override;
    void strokeRoundedRect(const Rect& rect, int radius, int lineWidth = 1) override;
    void drawText(const std::string& text, const Rect& rect, int flags = 0) override;
    void drawLine(const Point& p1, const Point& p2, int lineWidth = 1) override;
    void fillEllipse(const Rect& rect) override;
    void strokeEllipse(const Rect& rect, int lineWidth = 1) override;
    void drawImage(const std::string& path, const Rect& rect) override;
    Size imageSize(const std::string& path) override;
    void drawPixelBuffer(const uint8_t* rgba, int w, int h, const Rect& rect) override;

    Size measureText(const std::string& text) override;

    // Explicitly load a .ttf font file for GPU text rendering.
    // Returns false if the file can't be read or parsed.
    bool loadFontFile(const Font& font, const char* ttfPath);

    // GPU info
    const GpuInfo& gpuInfo() const { return gpuInfo_; }
    bool isGpuAccelerated() const { return device_ != nullptr; }

private:
    void loadImageTexture(const std::string& path);

    std::unique_ptr<GpuDevice> device_;
    std::unique_ptr<Renderer2D> renderer_;
    std::unique_ptr<FontAtlas> fontAtlas_;

    Color currentColor_;
    Font currentFont_;
    uint8_t defaultFontData_[256]; // placeholder for default font

    struct CachedImage {
        int texId = -1;
        int width = 0;
        int height = 0;
    };
    std::unordered_map<std::string, CachedImage> imageCache_;
    GpuInfo gpuInfo_;
    int tempTexId_ = -1; // transient texture from drawPixelBuffer, released on next call

    bool initialized_ = false;
};

} // namespace gpu
} // namespace ltgui
