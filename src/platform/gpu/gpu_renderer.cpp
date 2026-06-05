#include "platform/gpu/gpu_renderer.h"
#include "platform/gpu/gpu_font_atlas.h"
#include <algorithm>
#include <cmath>
#include <cstring>

namespace ltgui {
namespace gpu {

// ---- TextureManager ----

TextureManager::TextureManager(GpuDevice* device) : device_(device) {}

TextureManager::~TextureManager() {
    for (auto* tex : textures_) {
        if (tex) device_->destroyTexture(tex);
    }
}

int TextureManager::upload(int w, int h, const uint8_t* rgba) {
    if (!rgba || w <= 0 || h <= 0) return -1;

    GpuTexture* tex = device_->createTexture(w, h, rgba);
    if (!tex) return -1; // GPU texture creation failed — don't store nullptr

    // Reuse freed slots
    if (!freeSlots_.empty()) {
        int id = freeSlots_.back();
        freeSlots_.pop_back();
        if (textures_[id]) device_->destroyTexture(textures_[id]);
        textures_[id] = tex;
        return id;
    }
    int id = static_cast<int>(textures_.size());
    textures_.push_back(tex);
    return id;
}

void TextureManager::release(int texId) {
    if (texId >= 0 && texId < static_cast<int>(textures_.size()) && textures_[texId]) {
        device_->destroyTexture(textures_[texId]);
        textures_[texId] = nullptr;
        freeSlots_.push_back(texId);
    }
}

void TextureManager::bind(int texId, int slot) {
    if (texId >= 0 && texId < static_cast<int>(textures_.size()) && textures_[texId]) {
        textures_[texId]->bind(slot);
    }
}

// ---- Renderer2D ----

Renderer2D::Renderer2D(GpuDevice* device)
    : device_(device), texMgr_(device) {
    cmds_.reserve(4096);
}

Renderer2D::~Renderer2D() = default;

void Renderer2D::begin() {
    cmds_.clear();
    scissorActive_ = false;
}

void Renderer2D::end() {
    if (cmds_.empty()) return;
    flushBatch();
}

void Renderer2D::fillRect(const Rect& r, const Color& c) {
    if (c.a == 0) return;
    cmds_.push_back({DrawOp::FillRect, r, c});
}

void Renderer2D::fillRoundedRect(const Rect& r, float radius, const Color& c) {
    if (c.a == 0) return;
    cmds_.push_back({DrawOp::FillRoundedRect, r, c, radius});
}

void Renderer2D::fillEllipse(const Rect& r, const Color& c) {
    if (c.a == 0) return;
    cmds_.push_back({DrawOp::FillEllipse, r, c});
}

void Renderer2D::strokeRect(const Rect& r, float lineWidth, const Color& c) {
    if (c.a == 0) return;
    cmds_.push_back({DrawOp::StrokeRect, r, c, 0, lineWidth});
}

void Renderer2D::strokeRoundedRect(const Rect& r, float radius, float lineWidth, const Color& c) {
    if (c.a == 0) return;
    cmds_.push_back({DrawOp::StrokeRounded, r, c, radius, lineWidth});
}

void Renderer2D::drawLine(const Point& p1, const Point& p2, float lineWidth, const Color& c) {
    if (c.a == 0) return;
    cmds_.push_back({DrawOp::DrawLine, {}, c, 0, lineWidth, -1, 0, p1, p2});
}

void Renderer2D::strokeEllipse(const Rect& r, float lineWidth, const Color& c) {
    if (c.a == 0) return;
    cmds_.push_back({DrawOp::StrokeEllipse, r, c, 0, lineWidth});
}

void Renderer2D::drawGlyph(int texId, const Rect& dst, const Rect& src, const Color& c) {
    DrawCmd cmd;
    cmd.op = DrawOp::DrawGlyph;
    cmd.rect = dst;
    cmd.color = c;
    cmd.texId = texId;
    // Store src rect (atlas coords) in p1/p2
    cmd.p1 = {src.x, src.y};
    cmd.p2 = {src.width, src.height};
    cmds_.push_back(cmd);
}

void Renderer2D::drawImage(int texId, const Rect& dst) {
    cmds_.push_back({DrawOp::DrawImage, dst, Color::White, 0, 0, texId});
}

Size Renderer2D::measureText(const std::string& text, const Font& font) {
    if (fontAtlas_ && fontAtlas_->hasFont(font)) {
        return fontAtlas_->measureText(text, font);
    }
    // Fallback: estimate
    return {static_cast<int>(text.size()) * font.size * 2 / 3, font.size * 4 / 3};
}

void Renderer2D::setScissor(const Rect& r) {
    // Flush any pending batch with the current scissor state before changing.
    if (!cmds_.empty()) flushBatch();
    scissorActive_ = true;
    scissorRect_ = r;
    device_->setScissor(r.x, r.y, r.width, r.height);
}

void Renderer2D::clearScissor() {
    // Flush any pending batch with the current scissor state before changing.
    if (!cmds_.empty()) flushBatch();
    scissorActive_ = false;
    device_->clearScissor();
}

// ---- Internal ----

void Renderer2D::flushBatch() {
    if (cmds_.empty()) return;

    auto shaderFor = [](DrawOp op) -> int {
        switch (op) {
        case DrawOp::FillRect:          return 0; // Solid
        case DrawOp::FillRoundedRect:   return 1; // Rounded
        case DrawOp::StrokeRect:        return 1;
        case DrawOp::StrokeRounded:     return 1;
        case DrawOp::FillEllipse:       return 2; // Ellipse
        case DrawOp::StrokeEllipse:     return 2;
        case DrawOp::DrawGlyph:         return 3; // Texture
        case DrawOp::DrawImage:         return 3;
        case DrawOp::DrawLine:          return 0; // Solid (lines)
        default: return 0;
        }
    };

    auto isLineOp = [](DrawOp op) -> bool {
        return op == DrawOp::StrokeRect ||
               op == DrawOp::StrokeEllipse;
    };

    // Preserve original draw order (painter's algorithm) while batching
    // ADJACENT commands that share the same (shader, isLine, texId).
    size_t idx = 0;
    while (idx < cmds_.size()) {
        int batchShader = shaderFor(cmds_[idx].op);
        bool batchIsLines = isLineOp(cmds_[idx].op);
        int batchTexId = (batchShader == 3) ? cmds_[idx].texId : -1;

        // Expand batch to include all following commands with the same config
        size_t batchEnd = idx + 1;
        while (batchEnd < cmds_.size()) {
            auto& next = cmds_[batchEnd];
            int ns = shaderFor(next.op);
            bool nl = isLineOp(next.op);
            int nt = (ns == 3) ? next.texId : -1;
            if (ns == batchShader && nl == batchIsLines && nt == batchTexId) {
                batchEnd++;
            } else {
                break;
            }
        }

        // Emit vertices for this batch
        device_->selectShader(batchShader);
        if (batchTexId >= 0) {
            texMgr_.bind(batchTexId, 0);
        }

        std::vector<Vertex2D> verts;
        verts.reserve((batchEnd - idx) * 6);

        for (size_t i = idx; i < batchEnd; i++) {
            auto& cmd = cmds_[i];
            uint32_t color = cmd.color.toABGR();

            switch (cmd.op) {
            case DrawOp::FillRect:
                emitQuad(verts, cmd.rect, 0, 0, 1, 1, color, 0, 0, 0, 0);
                break;
            case DrawOp::FillRoundedRect:
                emitQuad(verts, cmd.rect, 0, 0, 1, 1, color,
                         static_cast<float>(cmd.rect.width), static_cast<float>(cmd.rect.height),
                         cmd.radius, -1.0f);
                break;
            case DrawOp::FillEllipse:
                emitQuad(verts, cmd.rect, 0, 0, 1, 1, color,
                         static_cast<float>(cmd.rect.width), static_cast<float>(cmd.rect.height),
                         0, -1.0f);
                break;
            case DrawOp::StrokeRect:
                emitStrokeRect(verts, cmd.rect, color);
                break;
            case DrawOp::StrokeRounded:
                emitQuad(verts, cmd.rect, 0, 0, 1, 1, color,
                         static_cast<float>(cmd.rect.width), static_cast<float>(cmd.rect.height),
                         cmd.radius, cmd.lineWidth);
                break;
            case DrawOp::StrokeEllipse:
                emitStrokeEllipse(verts, cmd.rect, color);
                break;
            case DrawOp::DrawGlyph: {
                float aw = fontAtlas_ ? (float)fontAtlas_->atlasW() : 2048.0f;
                float ah = fontAtlas_ ? (float)fontAtlas_->atlasH() : 2048.0f;
                float u0 = (float)cmd.p1.x / aw;
                float v0 = (float)cmd.p1.y / ah;
                float u1 = (float)(cmd.p1.x + cmd.p2.x) / aw;
                float v1 = (float)(cmd.p1.y + cmd.p2.y) / ah;
                emitQuad(verts, cmd.rect, u0, v0, u1, v1, color, 0, 0, 0, 0);
                break;
            }
            case DrawOp::DrawImage:
                emitQuad(verts, cmd.rect, 0, 0, 1, 1, color, 0, 0, 0, 0);
                break;
            case DrawOp::DrawLine:
                emitThickLine(verts, cmd.p1, cmd.p2, cmd.lineWidth, color);
                break;
            }
        }

        if (!verts.empty()) {
            if (batchIsLines)
                device_->drawLines(verts.data(), static_cast<int>(verts.size()));
            else
                device_->drawTriangles(verts.data(), static_cast<int>(verts.size()));
        }

        idx = batchEnd;
    }
}

// Convert pixel coordinate to NDC [-1, 1] for GPU vertex shader
static inline float toNdcX(float px, int screenW) { return (px / (float)screenW) * 2.0f - 1.0f; }
static inline float toNdcY(float py, int screenH) { return 1.0f - (py / (float)screenH) * 2.0f; }

void Renderer2D::emitQuad(std::vector<Vertex2D>& out, const Rect& r,
                           float u0, float v0, float u1, float v1, uint32_t color,
                           float p0, float p1, float p2, float p3) {
    float x0 = toNdcX(static_cast<float>(r.x), width_);
    float y0 = toNdcY(static_cast<float>(r.y), height_);
    float x1 = toNdcX(static_cast<float>(r.right()), width_);
    float y1 = toNdcY(static_cast<float>(r.bottom()), height_);
    Vertex2D v[] = {
        {x0, y0, u0, v0, color, p0, p1, p2, p3},
        {x1, y0, u1, v0, color, p0, p1, p2, p3},
        {x0, y1, u0, v1, color, p0, p1, p2, p3},
        {x0, y1, u0, v1, color, p0, p1, p2, p3},
        {x1, y0, u1, v0, color, p0, p1, p2, p3},
        {x1, y1, u1, v1, color, p0, p1, p2, p3},
    };
    out.insert(out.end(), v, v + 6);
}

void Renderer2D::emitStrokeRect(std::vector<Vertex2D>& out, const Rect& r, uint32_t color) {
    float x0 = toNdcX(static_cast<float>(r.x), width_);
    float y0 = toNdcY(static_cast<float>(r.y), height_);
    float x1 = toNdcX(static_cast<float>(r.right()), width_);
    float y1 = toNdcY(static_cast<float>(r.bottom()), height_);
    Vertex2D lines[8] = {
        {x0, y0, 0,0,color, 0,0,0,0}, {x1, y0, 0,0,color, 0,0,0,0},
        {x1, y0, 0,0,color, 0,0,0,0}, {x1, y1, 0,0,color, 0,0,0,0},
        {x1, y1, 0,0,color, 0,0,0,0}, {x0, y1, 0,0,color, 0,0,0,0},
        {x0, y1, 0,0,color, 0,0,0,0}, {x0, y0, 0,0,color, 0,0,0,0},
    };
    out.insert(out.end(), lines, lines + 8);
}

void Renderer2D::emitStrokeEllipse(std::vector<Vertex2D>& out, const Rect& r, uint32_t color) {
    float rx = r.width * 0.5f, ry = r.height * 0.5f;
    float cx = static_cast<float>(r.x) + rx, cy = static_cast<float>(r.y) + ry;
    const int kSegments = 16;
    for (int i = 0; i < kSegments; i++) {
        float a0 = 2.0f * 3.14159265f * i / kSegments;
        float a1 = 2.0f * 3.14159265f * (i + 1) / kSegments;
        Vertex2D v[] = {
            {toNdcX(cx + rx * cosf(a0), width_),
             toNdcY(cy + ry * sinf(a0), height_), 0,0,color, 0,0,0,0},
            {toNdcX(cx + rx * cosf(a1), width_),
             toNdcY(cy + ry * sinf(a1), height_), 0,0,color, 0,0,0,0},
        };
        out.insert(out.end(), v, v + 2);
    }
}

void Renderer2D::emitLine(std::vector<Vertex2D>& out, const Point& p1, const Point& p2, uint32_t color) {
    Vertex2D v[] = {
        {toNdcX(static_cast<float>(p1.x), width_),
         toNdcY(static_cast<float>(p1.y), height_), 0,0,color, 0,0,0,0},
        {toNdcX(static_cast<float>(p2.x), width_),
         toNdcY(static_cast<float>(p2.y), height_), 0,0,color, 0,0,0,0},
    };
    out.insert(out.end(), v, v + 2);
}

void Renderer2D::emitThickLine(std::vector<Vertex2D>& out, const Point& p1, const Point& p2,
                                float lineWidth, uint32_t color) {
    if (lineWidth <= 1.0f) {
        emitLine(out, p1, p2, color);
        return;
    }
    // Draw a rectangle (2 tris) oriented along the line segment.
    float dx = static_cast<float>(p2.x - p1.x);
    float dy = static_cast<float>(p2.y - p1.y);
    float len = std::sqrt(dx * dx + dy * dy);
    if (len < 0.5f) return;
    float nx = -dy / len * lineWidth * 0.5f;
    float ny =  dx / len * lineWidth * 0.5f;

    float x0 = toNdcX(static_cast<float>(p1.x) + nx, width_);
    float y0 = toNdcY(static_cast<float>(p1.y) + ny, height_);
    float x1 = toNdcX(static_cast<float>(p1.x) - nx, width_);
    float y1 = toNdcY(static_cast<float>(p1.y) - ny, height_);
    float x2 = toNdcX(static_cast<float>(p2.x) - nx, width_);
    float y2 = toNdcY(static_cast<float>(p2.y) - ny, height_);
    float x3 = toNdcX(static_cast<float>(p2.x) + nx, width_);
    float y3 = toNdcY(static_cast<float>(p2.y) + ny, height_);

    Vertex2D verts[6] = {
        {x0, y0, 0, 0, color, 0, 0, 0, 0},
        {x3, y3, 0, 0, color, 0, 0, 0, 0},
        {x1, y1, 0, 0, color, 0, 0, 0, 0},
        {x1, y1, 0, 0, color, 0, 0, 0, 0},
        {x3, y3, 0, 0, color, 0, 0, 0, 0},
        {x2, y2, 0, 0, color, 0, 0, 0, 0},
    };
    out.insert(out.end(), verts, verts + 6);
}

} // namespace gpu
} // namespace ltgui
