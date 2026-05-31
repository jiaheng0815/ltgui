#include "platform/gpu/gpu_font_atlas.h"
#include <cstring>
#include <cstdio>

namespace ltgui {
namespace gpu {

FontAtlas::FontAtlas(GpuDevice* device, int atlasW, int atlasH)
    : device_(device), atlasW_(atlasW), atlasH_(atlasH) {}

FontAtlas::~FontAtlas() {
    for (auto& page : pages_) {
        if (page.texture) device_->destroyTexture(page.texture);
    }
}

bool FontAtlas::loadFont(const Font& fontDesc, const uint8_t* ttfData, int ttfSize) {
#ifdef LTGUI_HAS_STB_TRUETYPE
    if (loadedFonts_.count(fontDesc)) return true;

    FontCache cache;
    cache.ttfData.assign(ttfData, ttfData + ttfSize);

    int offset = stbtt_GetFontOffsetForIndex(cache.ttfData.data(), 0);
    if (offset < 0) return false;

    if (!stbtt_InitFont(&cache.info, cache.ttfData.data(), offset)) return false;

    cache.scale = stbtt_ScaleForPixelHeight(&cache.info, (float)fontDesc.size);
    stbtt_GetFontVMetrics(&cache.info, &cache.ascent, &cache.descent, &cache.lineGap);
    loadedFonts_[fontDesc] = cache;
    return true;
#else
    (void)fontDesc; (void)ttfData; (void)ttfSize;
    return false;
#endif
}

const GlyphEntry* FontAtlas::getGlyph(uint32_t codepoint, const Font& fontDesc) {
#ifdef LTGUI_HAS_STB_TRUETYPE
    auto fit = loadedFonts_.find(fontDesc);
    if (fit == loadedFonts_.end()) return nullptr;

    uint64_t key = (fontKey(fontDesc) << 32) | codepoint;
    auto git = glyphCache_.find(key);
    if (git != glyphCache_.end()) return &git->second;

    auto& cache = fit->second;
    int glyphIdx = stbtt_FindGlyphIndex(&cache.info, (int)codepoint);

    int advanceW, x0, y0, x1, y1;
    stbtt_GetGlyphHMetrics(&cache.info, glyphIdx, &advanceW, nullptr);
    stbtt_GetGlyphBitmapBox(&cache.info, glyphIdx, cache.scale, cache.scale, &x0, &y0, &x1, &y1);

    int gw = x1 - x0;
    int gh = y1 - y0;
    if (gw <= 0 || gh <= 0) {
        GlyphEntry empty;
        empty.advance = static_cast<int>(advanceW * cache.scale);
        glyphCache_[key] = empty;
        return &glyphCache_[key];
    }

    int ax, ay, texId;
    if (!allocGlyphRect(gw, gh, ax, ay, texId)) {
        createAtlasPage();
        if (!allocGlyphRect(gw, gh, ax, ay, texId)) return nullptr;
    }

    std::vector<uint8_t> bitmap(gw * gh);
    stbtt_MakeGlyphBitmap(&cache.info, bitmap.data(), gw, gh, gw,
                          cache.scale, cache.scale, (float)glyphIdx);

    // Upload to atlas texture
    auto& page = pages_[texId];
    page.texture->update(bitmap.data(), ax, ay, gw, gh);

    GlyphEntry entry;
    entry.atlasX = ax;
    entry.atlasY = ay;
    entry.w = gw;
    entry.h = gh;
    entry.offsetX = x0;
    entry.offsetY = y0;
    entry.advance = static_cast<int>(advanceW * cache.scale);
    entry.texId = texId;

    glyphCache_[key] = entry;
    return &glyphCache_[key];
#else
    (void)codepoint; (void)fontDesc;
    return nullptr;
#endif
}

Size FontAtlas::measureText(const std::string& text, const Font& fontDesc) {
#ifdef LTGUI_HAS_STB_TRUETYPE
    auto fit = loadedFonts_.find(fontDesc);
    if (fit == loadedFonts_.end()) return {0, 0};

    auto& cache = fit->second;
    float ascent = cache.ascent * cache.scale;
    float descent = cache.descent * cache.scale;
    float lineGap = cache.lineGap * cache.scale;
    float lineHeight = ascent - descent + lineGap;

    float width = 0;
    uint32_t cp = 0;
    const uint8_t* p = reinterpret_cast<const uint8_t*>(text.data());
    const uint8_t* end = p + text.size();

    while (p < end) {
        // UTF-8 decode
        if ((*p & 0x80) == 0)      { cp = *p++; }
        else if ((*p & 0xE0) == 0xC0) { cp = (*p++ & 0x1F) << 6;  cp |= (*p++ & 0x3F); }
        else if ((*p & 0xF0) == 0xE0) { cp = (*p++ & 0x0F) << 12; cp |= (*p++ & 0x3F) << 6; cp |= (*p++ & 0x3F); }
        else if ((*p & 0xF8) == 0xF0) { cp = (*p++ & 0x07) << 18; cp |= (*p++ & 0x3F) << 12; cp |= (*p++ & 0x3F) << 6; cp |= (*p++ & 0x3F); }
        else { p++; continue; }

        int glyphIdx = stbtt_FindGlyphIndex(&cache.info, (int)cp);
        int advanceW = 0;
        stbtt_GetGlyphHMetrics(&cache.info, glyphIdx, &advanceW, nullptr);
        width += advanceW * cache.scale;
    }

    return {static_cast<int>(width + 0.5f), static_cast<int>(lineHeight + 0.5f)};
#else
    (void)text; (void)fontDesc;
    return {static_cast<int>(text.size()) * fontDesc.size * 2 / 3, fontDesc.size * 4 / 3};
#endif
}

int FontAtlas::atlasTexId(const Font& fontDesc) const {
    (void)fontDesc;
    return pages_.empty() ? -1 : 0;
}

void FontAtlas::flush() {
    // Atlas texture updates are immediate via update() — nothing to flush
}

int FontAtlas::createAtlasPage() {
    // Create empty RGBA atlas texture
    std::vector<uint8_t> empty(atlasW_ * atlasH_ * 4, 0);
    GpuTexture* tex = device_->createTexture(atlasW_, atlasH_, empty.data());
    int texId = static_cast<int>(pages_.size());
    pages_.push_back({tex, 1, 1, 0, texId});
    return texId;
}

bool FontAtlas::allocGlyphRect(int w, int h, int& outX, int& outY, int& outTexId) {
    if (pages_.empty()) {
        if (createAtlasPage() < 0) return false;
    }

    for (auto& page : pages_) {
        if (page.cursorX + w + 1 >= atlasW_) {
            page.cursorX = 1;
            page.cursorY += page.rowHeight + 1;
            page.rowHeight = 0;
        }
        if (page.cursorY + h + 1 >= atlasH_) continue;

        outX = page.cursorX;
        outY = page.cursorY;
        outTexId = page.currentTexId;

        page.cursorX += w + 1;
        page.rowHeight = std::max(page.rowHeight, h);
        return true;
    }
    return false;
}

uint64_t FontAtlas::fontKey(const Font& f) const {
    auto h = std::hash<std::string>{}(f.family);
    h ^= static_cast<uint64_t>(f.size) << 32;
    h ^= static_cast<uint64_t>(static_cast<int>(f.weight)) << 40;
    h ^= static_cast<uint64_t>(static_cast<int>(f.style)) << 48;
    return h;
}

} // namespace gpu
} // namespace ltgui
