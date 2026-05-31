#pragma once
#include "geometry.h"
#include "color.h"
#include <cstdint>
#include <vector>

namespace ltgui {
namespace gpu {

enum class VertexFormat { Pos2D, Pos2D_Color, Pos2D_Tex };

struct Vertex2D {
    float x, y;
    float u, v;
    uint32_t color; // ARGB
};

enum class Primitive { Triangles, TriangleStrip, Lines };

class GpuTexture {
public:
    virtual ~GpuTexture() = default;
    virtual int width() const = 0;
    virtual int height() const = 0;
    virtual void update(const uint8_t* rgba, int x, int y, int w, int h) = 0;
    virtual void bind(int slot) = 0;
};

class GpuDevice {
public:
    virtual ~GpuDevice() = default;

    // Lifecycle: must be called before any draw
    virtual bool initialize(void* windowHandle, int width, int height) = 0;
    virtual void resize(int width, int height) = 0;
    virtual void shutdown() = 0;

    // Frame
    virtual void beginFrame() = 0;
    virtual void endFrame() = 0; // Present

    // Primitives
    virtual void drawTriangles(const Vertex2D* verts, int count) = 0;
    virtual void drawTriangleStrip(const Vertex2D* verts, int count) = 0;
    virtual void drawLines(const Vertex2D* verts, int count) = 0;

    // Textures
    virtual GpuTexture* createTexture(int w, int h, const uint8_t* rgba) = 0;
    virtual void destroyTexture(GpuTexture* tex) = 0;

    // Render state
    virtual void setBlend(bool enable) = 0;
    virtual void setScissor(int x, int y, int w, int h) = 0;
    virtual void clearScissor() = 0;

    // Info
    virtual const char* name() const = 0;
    virtual int width() const = 0;
    virtual int height() const = 0;
};

} // namespace gpu
} // namespace ltgui
