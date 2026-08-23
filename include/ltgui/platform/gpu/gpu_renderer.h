#pragma once
#include "color.h"
#include "font.h"
#include "geometry.h"
#include "platform/gpu/gpu_device.h"
#include <string>
#include <vector>

namespace ltgui {
namespace gpu {

class FontAtlas;

enum class DrawOp : uint8_t {
  FillRect,
  FillGradientRect,
  FillRoundedRect,
  FillEllipse,
  StrokeRect,
  StrokeRounded,
  StrokeEllipse,
  DrawGlyph,
  DrawImage,
  DrawLine,
};

struct DrawCmd {
  DrawOp op = DrawOp::FillRect;
  Rect rect; // target rectangle
  Color color;
  float radius = 0;    // for rounded rect / ellipse
  float lineWidth = 1; // for stroke / line
  int texId = -1;      // -1 = solid color, >=0 = texture
  Point p1, p2;        // for DrawLine
  Color color2;        // second color for FillGradientRect
  Rect gradBounds;     // full gradient extent for FillGradientRect (empty =
                       // use rect)
};

// Manages texture uploads — keeps GpuTexture alive and maps handles
class TextureManager {
public:
  TextureManager(GpuDevice *device);
  ~TextureManager();

  int upload(int w, int h, const uint8_t *rgba); // returns texId
  void release(int texId);
  void bind(int texId, int slot);
  GpuDevice *device() const { return device_; }

  // Direct access for partial texture updates (e.g. font atlas glyph uploads).
  GpuTexture *getTexture(int texId) const {
    if (texId >= 0 && texId < static_cast<int>(textures_.size()))
      return textures_[texId];
    return nullptr;
  }

private:
  GpuDevice *device_;
  std::vector<GpuTexture *> textures_;
  std::vector<int> freeSlots_;
};

class Renderer2D {
public:
  Renderer2D(GpuDevice *device);
  ~Renderer2D();

  void begin();
  void end(); // flush

  // Drawing commands (all deferred)
  void fillRect(const Rect &r, const Color &c);
  // Two-color linear gradient — interpolated per-vertex on the GPU.
  // fullBounds is the whole extent of the gradient; t is computed from the
  // vertex position against fullBounds (falls back to r when empty).
  void fillLinearGradient(const Rect &r, const Color &from, const Color &to,
                          bool vertical, const Rect &fullBounds = Rect{});
  void fillRoundedRect(const Rect &r, float radius, const Color &c);
  void fillEllipse(const Rect &r, const Color &c);
  void strokeRect(const Rect &r, float lineWidth, const Color &c);
  void strokeRoundedRect(const Rect &r, float radius, float lineWidth,
                         const Color &c);
  void drawLine(const Point &p1, const Point &p2, float lineWidth,
                const Color &c);
  void strokeEllipse(const Rect &r, float lineWidth, const Color &c);
  void drawGlyph(int texId, const Rect &dst, const Rect &src, const Color &c);
  void drawImage(int texId, const Rect &dst);

  // Texture management
  TextureManager &textures() { return texMgr_; }
  void setFontAtlas(FontAtlas *atlas) { fontAtlas_ = atlas; }
  FontAtlas *fontAtlas() const { return fontAtlas_; }

  // Measurement (CPU-side)
  Size measureText(const std::string &text, const Font &font);

  // Screen info
  int width() const { return width_; }
  int height() const { return height_; }
  void setSize(int w, int h) {
    width_ = w;
    height_ = h;
  }

  // Scissor
  void setScissor(const Rect &r);
  void clearScissor();

private:
  void flushBatch();
  void emitQuad(std::vector<Vertex2D> &out, const Rect &r, float u0, float v0,
                float u1, float v1, uint32_t color, float p0, float p1,
                float p2, float p3);
  void emitGradientQuad(std::vector<Vertex2D> &out, const Rect &r,
                        uint32_t colorA, uint32_t colorB, bool vertical,
                        const Rect &gradBounds);
  void emitStrokeRect(std::vector<Vertex2D> &out, const Rect &r, uint32_t color,
                      float lineWidth);
  void emitStrokeEllipse(std::vector<Vertex2D> &out, const Rect &r,
                         uint32_t color, float lineWidth);
  void emitLine(std::vector<Vertex2D> &out, const Point &p1, const Point &p2,
                uint32_t color);
  void emitThickLine(std::vector<Vertex2D> &out, const Point &p1,
                     const Point &p2, float lineWidth, uint32_t color);

  GpuDevice *device_;
  TextureManager texMgr_;
  FontAtlas *fontAtlas_ = nullptr;
  std::vector<DrawCmd> cmds_;

  int width_ = 0, height_ = 0;
  bool scissorActive_ = false;
  Rect scissorRect_;
};

} // namespace gpu
} // namespace ltgui
