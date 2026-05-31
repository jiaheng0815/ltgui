#include "platform/gpu/gpu_renderer.h"
#include "platform/gpu/gpu_font_atlas.h"
#include <algorithm>
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
    // Reuse freed slots
    if (!freeSlots_.empty()) {
        int id = freeSlots_.back();
        freeSlots_.pop_back();
        if (textures_[id]) device_->destroyTexture(textures_[id]);
        textures_[id] = device_->createTexture(w, h, rgba);
        return id;
    }
    int id = static_cast<int>(textures_.size());
    textures_.push_back(device_->createTexture(w, h, rgba));
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
    verts_.reserve(65536);
}

Renderer2D::~Renderer2D() = default;

void Renderer2D::begin() {
    cmds_.clear();
    verts_.clear();
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

void Renderer2D::drawGlyph(int texId, const Rect& dst, const Rect& src, const Color& c) {
    DrawCmd cmd;
    cmd.op = DrawOp::DrawGlyph;
    cmd.rect = dst;
    cmd.color = c;
    cmd.texId = texId;
    // Encode src rect in radius+lineWidth temporarily
    cmd.radius = (float)src.x;
    cmd.lineWidth = (float)src.y;
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
    scissorActive_ = true;
    scissorRect_ = r;
    device_->setScissor(r.x, r.y, r.width, r.height);
}

void Renderer2D::clearScissor() {
    scissorActive_ = false;
    device_->clearScissor();
}

// ---- Internal ----

void Renderer2D::flushBatch() {
    // Sort by (texId, op, color) to minimize state changes
    for (auto& cmd : cmds_) {
        uint32_t colorHash = cmd.color.toARGB();
        cmd.sortKey = (cmd.texId << 20) | ((static_cast<int>(cmd.op) & 0xF) << 16) | (colorHash & 0xFFFF);
    }
    std::sort(cmds_.begin(), cmds_.end(),
              [](const DrawCmd& a, const DrawCmd& b) { return a.sortKey < b.sortKey; });

    for (auto& cmd : cmds_) {
        switch (cmd.op) {
        case DrawOp::FillRect:
            emitSolidQuad(cmd.rect, cmd.color);
            break;
        case DrawOp::FillRoundedRect:
            emitTexturedQuad(cmd.rect, 0, 0, 1, 1, cmd.color.toARGB());
            // Filled rounded via shader with encoded params
            // TODO: use rounded shader with params
            emitSolidQuad(cmd.rect, cmd.color);
            break;
        case DrawOp::FillEllipse:
            emitSolidQuad(cmd.rect, cmd.color);
            break;
        case DrawOp::StrokeRect:
        case DrawOp::StrokeRounded: {
            // Stroke = 4 line segments
            float lw = cmd.lineWidth;
            Rect r = cmd.rect;
            Vertex2D lines[8];
            // Top
            lines[0] = {(float)r.x, (float)r.y, 0, 0, cmd.color.toARGB()};
            lines[1] = {(float)r.right(), (float)r.y, 0, 0, cmd.color.toARGB()};
            // Right
            lines[2] = {(float)r.right(), (float)r.y, 0, 0, cmd.color.toARGB()};
            lines[3] = {(float)r.right(), (float)r.bottom(), 0, 0, cmd.color.toARGB()};
            // Bottom
            lines[4] = {(float)r.right(), (float)r.bottom(), 0, 0, cmd.color.toARGB()};
            lines[5] = {(float)r.x, (float)r.bottom(), 0, 0, cmd.color.toARGB()};
            // Left
            lines[6] = {(float)r.x, (float)r.bottom(), 0, 0, cmd.color.toARGB()};
            lines[7] = {(float)r.x, (float)r.y, 0, 0, cmd.color.toARGB()};
            verts_.insert(verts_.end(), lines, lines + 8);
            break;
        }
        case DrawOp::DrawGlyph:
        case DrawOp::DrawImage: {
            Rect r = cmd.rect;
            float u0 = 0, v0 = 0, u1 = 1, v1 = 1;
            uint32_t col = cmd.color.toARGB();
            Vertex2D v[] = {
                {(float)r.x, (float)r.y, u0, v0, col},
                {(float)r.right(), (float)r.y, u1, v0, col},
                {(float)r.x, (float)r.bottom(), u0, v1, col},
                {(float)r.x, (float)r.bottom(), u0, v1, col},
                {(float)r.right(), (float)r.y, u1, v0, col},
                {(float)r.right(), (float)r.bottom(), u1, v1, col},
            };
            verts_.insert(verts_.end(), v, v + 6);
            break;
        }
        case DrawOp::DrawLine: {
            Vertex2D v[] = {
                {(float)cmd.p1.x, (float)cmd.p1.y, 0, 0, cmd.color.toARGB()},
                {(float)cmd.p2.x, (float)cmd.p2.y, 0, 0, cmd.color.toARGB()},
            };
            verts_.insert(verts_.end(), v, v + 2);
            break;
        }
        }
    }

    // Submit all accumulated vertices
    if (!verts_.empty()) {
        device_->drawTriangles(verts_.data(), static_cast<int>(verts_.size()));
        verts_.clear();
    }
}

void Renderer2D::emitSolidQuad(const Rect& r, const Color& c) {
    uint32_t col = c.toARGB();
    Vertex2D v[] = {
        {(float)r.x, (float)r.y, 0, 0, col},
        {(float)r.right(), (float)r.y, 0, 0, col},
        {(float)r.x, (float)r.bottom(), 0, 0, col},
        {(float)r.x, (float)r.bottom(), 0, 0, col},
        {(float)r.right(), (float)r.y, 0, 0, col},
        {(float)r.right(), (float)r.bottom(), 0, 0, col},
    };
    verts_.insert(verts_.end(), v, v + 6);
}

void Renderer2D::emitTexturedQuad(const Rect& r, float u0, float v0, float u1, float v1, uint32_t color) {
    Vertex2D v[] = {
        {(float)r.x, (float)r.y, u0, v0, color},
        {(float)r.right(), (float)r.y, u1, v0, color},
        {(float)r.x, (float)r.bottom(), u0, v1, color},
        {(float)r.x, (float)r.bottom(), u0, v1, color},
        {(float)r.right(), (float)r.y, u1, v0, color},
        {(float)r.right(), (float)r.bottom(), u1, v1, color},
    };
    verts_.insert(verts_.end(), v, v + 6);
}

} // namespace gpu
} // namespace ltgui
