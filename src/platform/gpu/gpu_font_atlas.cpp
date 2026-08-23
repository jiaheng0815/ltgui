#define STB_TRUETYPE_IMPLEMENTATION
#include "platform/gpu/gpu_font_atlas.h"
#include "log.h"
#include "platform/gpu/gpu_renderer.h"
#include <cstdio>
#include <cstring>

namespace ltgui {
namespace gpu {

FontAtlas::FontAtlas(GpuDevice *device, TextureManager *texMgr, int atlasW,
                     int atlasH)
    : device_(device), texMgr_(texMgr), atlasW_(atlasW), atlasH_(atlasH) {}

FontAtlas::~FontAtlas() {
  // Atlas textures are owned by TextureManager; no manual cleanup needed.
}

bool FontAtlas::loadFont(const Font &fontDesc, const uint8_t *ttfData,
                         int ttfSize) {
#ifdef LTGUI_HAS_STB_TRUETYPE
  // Store the raw TTF blob keyed by (family, weight, style) so we can
  // rasterise at any requested size later.  Must store BEFORE using the
  // data pointer, since stbtt_fontinfo references it and it must outlive
  // the font cache.
  Font ttfKey(fontDesc.family, 0, fontDesc.weight, fontDesc.style);
  auto &stored = ttfData_[ttfKey];
  stored.assign(ttfData, ttfData + ttfSize);

  if (loadedFonts_.count(fontDesc))
    return true;

  FontCache cache;
  int offset = stbtt_GetFontOffsetForIndex(stored.data(), 0);
  if (offset < 0)
    return false;

  if (!stbtt_InitFont(&cache.info, stored.data(), offset))
    return false;

  cache.scale = stbtt_ScaleForPixelHeight(&cache.info, (float)fontDesc.size);
  stbtt_GetFontVMetrics(&cache.info, &cache.ascent, &cache.descent,
                        &cache.lineGap);
  loadedFonts_[fontDesc] = std::move(cache);
  return true;
#else
  (void)fontDesc;
  (void)ttfData;
  (void)ttfSize;
  return false;
#endif
}

bool FontAtlas::loadFontFile(const Font &fontDesc, const char *ttfPath) {
#ifdef LTGUI_HAS_STB_TRUETYPE
  FILE *f = fopen(ttfPath, "rb");
  if (!f)
    return false;
  fseek(f, 0, SEEK_END);
  long size = ftell(f);
  fseek(f, 0, SEEK_SET);
  if (size <= 0) {
    fclose(f);
    return false;
  }
  std::vector<uint8_t> data(size);
  size_t read = fread(data.data(), 1, size, f);
  fclose(f);
  if (read != (size_t)size)
    return false;
  return loadFont(fontDesc, data.data(), (int)size);
#else
  (void)fontDesc;
  (void)ttfPath;
  return false;
#endif
}

const GlyphEntry *FontAtlas::getGlyph(uint32_t codepoint,
                                      const Font &fontDesc) {
#ifdef LTGUI_HAS_STB_TRUETYPE
  // Ensure the font (or its system-default fallback) is loaded at this size.
  // TTF data is stored once per (family,weight,style) and size-specific
  // caches are created on demand by ensureFontLoaded.
  ensureFontLoaded(fontDesc);
  auto fit = loadedFonts_.find(fontDesc);
  if (fit == loadedFonts_.end()) {
    Font fallback = Font::systemDefault(fontDesc.size);
    if (!(fallback == fontDesc)) {
      ensureFontLoaded(fallback);
      fit = loadedFonts_.find(fallback);
    }
    if (fit == loadedFonts_.end())
      return nullptr;
  }

  // Mix the full 64-bit font key (which includes size/weight/style in the
  // high bits) with the codepoint via XOR + golden-ratio multiplication.
  // The old `(fontKey << 32) | codepoint` shifted the font's size/weight/
  // style bits out of the 64-bit range, so glyphs of the same codepoint at
  // different sizes/styles collided in the cache.
  uint64_t key =
      fontKey(fit->first) ^
      (static_cast<uint64_t>(codepoint) * 0x9E3779B97F4A7C15ULL);
  auto git = glyphCache_.find(key);
  if (git != glyphCache_.end()) {
    touchGlyph(key);
    return &git->second;
  }

  auto &cache = fit->second;
  int glyphIdx = stbtt_FindGlyphIndex(&cache.info, (int)codepoint);

  int advanceW, x0, y0, x1, y1;
  stbtt_GetGlyphHMetrics(&cache.info, glyphIdx, &advanceW, nullptr);
  stbtt_GetGlyphBitmapBox(&cache.info, glyphIdx, cache.scale, cache.scale, &x0,
                          &y0, &x1, &y1);

  int gw = x1 - x0;
  int gh = y1 - y0;
  if (gw <= 0 || gh <= 0) {
    GlyphEntry empty;
    empty.advance = static_cast<int>(advanceW * cache.scale);
    addGlyph(key, empty);
    return &glyphCache_[key];
  }

  int ax, ay, texId;
  // allocGlyphRect now handles page creation internally (up to kMaxAtlasPages),
  // so the old manual createAtlasPage() fallback here is no longer needed.
  if (!allocGlyphRect(gw, gh, ax, ay, texId))
    return nullptr;

  std::vector<uint8_t> gray(gw * gh);
  stbtt_MakeGlyphBitmap(&cache.info, gray.data(), gw, gh, gw, cache.scale,
                        cache.scale, (float)glyphIdx);

  // stb_truetype outputs 1-byte-per-pixel grayscale, but the GPU texture
  // expects RGBA (4 bytes per pixel). Expand: R=G=B=255, A=gray value.
  // The vertex color in the texture shader (tex.Sample * input.col)
  // will tint the white glyph to the desired text color.
  std::vector<uint8_t> rgba(gw * gh * 4);
  for (int i = 0; i < gw * gh; i++) {
    rgba[i * 4 + 0] = 255;
    rgba[i * 4 + 1] = 255;
    rgba[i * 4 + 2] = 255;
    rgba[i * 4 + 3] = gray[i];
  }

  // Upload to atlas texture via TextureManager
  GpuTexture *tex = texMgr_->getTexture(texId);
  if (tex)
    tex->update(rgba.data(), ax, ay, gw, gh);

  GlyphEntry entry;
  entry.atlasX = ax;
  entry.atlasY = ay;
  entry.w = gw;
  entry.h = gh;
  entry.offsetX = x0;
  entry.offsetY = y0;
  entry.advance = static_cast<int>(advanceW * cache.scale);
  entry.texId = texId;

  addGlyph(key, entry);
  return &glyphCache_[key];
#else
  (void)codepoint;
  (void)fontDesc;
  return nullptr;
#endif
}

void FontAtlas::touchGlyph(uint64_t key) {
  auto it = glyphCache_.find(key);
  if (it == glyphCache_.end())
    return;
  glyphLRU_.splice(glyphLRU_.begin(), glyphLRU_, it->second.lruIt);
}

void FontAtlas::addGlyph(uint64_t key, GlyphEntry entry) {
  auto it = glyphCache_.find(key);
  if (it != glyphCache_.end()) {
    // Replacing an existing entry: refresh its LRU position.
    glyphLRU_.splice(glyphLRU_.begin(), glyphLRU_, it->second.lruIt);
    it->second = entry;
    return;
  }
  glyphCache_[key] = entry;
  glyphLRU_.push_front(key);
  glyphCache_[key].lruIt = glyphLRU_.begin();
  while (glyphLRU_.size() > kMaxGlyphs) {
    uint64_t victim = glyphLRU_.back();
    glyphLRU_.pop_back();
    glyphCache_.erase(victim);
  }
}

Size FontAtlas::measureText(const std::string &text, const Font &fontDesc) {
#ifdef LTGUI_HAS_STB_TRUETYPE
  ensureFontLoaded(fontDesc);
  auto fit = loadedFonts_.find(fontDesc);
  if (fit == loadedFonts_.end()) {
    Font fallback = Font::systemDefault(fontDesc.size);
    if (!(fallback == fontDesc)) {
      ensureFontLoaded(fallback);
      fit = loadedFonts_.find(fallback);
    }
    if (fit == loadedFonts_.end())
      return {0, 0};
  }

  auto &cache = fit->second;
  float ascent = cache.ascent * cache.scale;
  float descent = cache.descent * cache.scale;
  // Exclude lineGap from the measured height.  lineGap is inter-line
  // spacing and does not contribute to the visual extent of a single
  // line.  Including it makes AlignVCenter text appear high by lineGap/2
  // because drawText() positions glyphs relative to the baseline (ascent
  // only), not the bottom of the line-gap box.
  float lineHeight = ascent - descent;

  float width = 0;
  uint32_t cp = 0;
  const uint8_t *p = reinterpret_cast<const uint8_t *>(text.data());
  const uint8_t *end = p + text.size();

  while (p < end) {
    // UTF-8 decode
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

    int glyphIdx = stbtt_FindGlyphIndex(&cache.info, (int)cp);
    int advanceW = 0;
    stbtt_GetGlyphHMetrics(&cache.info, glyphIdx, &advanceW, nullptr);
    width += advanceW * cache.scale;
  }

  return {static_cast<int>(width + 0.5f), static_cast<int>(lineHeight + 0.5f)};
#else
  (void)text;
  (void)fontDesc;
  return {static_cast<int>(text.size()) * fontDesc.size * 2 / 3,
          fontDesc.size * 4 / 3};
#endif
}

void FontAtlas::ensureFontLoaded(const Font &fontDesc) {
#ifdef LTGUI_HAS_STB_TRUETYPE
  if (loadedFonts_.count(fontDesc))
    return;

  // Look up raw TTF data by (family, weight, style) — ignore size in the key
  Font ttfKey(fontDesc.family, 0, fontDesc.weight, fontDesc.style);
  auto ttfIt = ttfData_.find(ttfKey);
  if (ttfIt == ttfData_.end()) {
    // Try system default family as a last resort
    Font sysKey = Font::systemDefault(0);
    sysKey.weight = fontDesc.weight;
    sysKey.style = fontDesc.style;
    ttfIt = ttfData_.find(sysKey);
    if (ttfIt == ttfData_.end()) {
      // Try any loaded TTF data
      if (!ttfData_.empty())
        ttfIt = ttfData_.begin();
      else
        return;
    }
  }

  const uint8_t *data = ttfIt->second.data();

  FontCache cache;
  int offset = stbtt_GetFontOffsetForIndex(data, 0);
  if (offset < 0)
    return;

  if (!stbtt_InitFont(&cache.info, data, offset))
    return;

  cache.scale = stbtt_ScaleForPixelHeight(&cache.info, (float)fontDesc.size);
  stbtt_GetFontVMetrics(&cache.info, &cache.ascent, &cache.descent,
                        &cache.lineGap);
  loadedFonts_[fontDesc] = std::move(cache);
#else
  (void)fontDesc;
#endif
}

float FontAtlas::getAscent(const Font &fontDesc) {
#ifdef LTGUI_HAS_STB_TRUETYPE
  ensureFontLoaded(fontDesc);
  auto fit = loadedFonts_.find(fontDesc);
  if (fit == loadedFonts_.end()) {
    Font fallback = Font::systemDefault(fontDesc.size);
    if (!(fallback == fontDesc)) {
      ensureFontLoaded(fallback);
      fit = loadedFonts_.find(fallback);
    }
  }
  if (fit != loadedFonts_.end()) {
    return fit->second.ascent * fit->second.scale;
  }
#endif
  (void)fontDesc;
  return 0.0f;
}

int FontAtlas::createAtlasPage() {
  // Create empty RGBA atlas texture via TextureManager so its texId
  // is valid for binding in flushBatch (unified ID space).
  std::vector<uint8_t> empty(atlasW_ * atlasH_ * 4, 0);
  int texId = texMgr_->upload(atlasW_, atlasH_, empty.data());
  if (texId < 0) {
    LOG_ERROR("GPU", "failed to create atlas page (GPU texture upload failed)");
    return -1;
  }
  pages_.push_back({texId, 1, 1, 0});
  return texId;
}

bool FontAtlas::allocGlyphRect(int w, int h, int &outX, int &outY,
                               int &outTexId) {
  // Reject glyphs larger than the atlas itself — they can never fit.
  if (w > atlasW_ || h > atlasH_) {
    LOG_WARN("GPU", "glyph (%dx%d) exceeds atlas (%dx%d), skipped", w, h,
             atlasW_, atlasH_);
    return false;
  }

  if (pages_.empty()) {
    if (createAtlasPage() < 0)
      return false;
  }

  for (auto &page : pages_) {
    if (page.cursorX + w + 1 >= atlasW_) {
      page.cursorX = 1;
      page.cursorY += page.rowHeight + 1;
      page.rowHeight = 0;
    }
    if (page.cursorY + h + 1 >= atlasH_)
      continue;

    outX = page.cursorX;
    outY = page.cursorY;
    outTexId = page.texId;

    page.cursorX += w + 1;
    page.rowHeight = std::max(page.rowHeight, h);
    return true;
  }

  // No room on any existing page — create a new one.
  // Guard against infinite page creation for oddly shaped glyphs.
  if (static_cast<int>(pages_.size()) >= kMaxAtlasPages) {
    LOG_ERROR("GPU", "max atlas pages (%d) reached, glyph (%dx%d) skipped",
              kMaxAtlasPages, w, h);
    return false;
  }
  if (createAtlasPage() < 0)
    return false;

  // Place glyph directly on the newly-created page (always pages_.back())
  // instead of recursing, which would re-scan all full pages unnecessarily.
  auto &page = pages_.back();
  outX = page.cursorX;
  outY = page.cursorY;
  outTexId = page.texId;
  page.cursorX += w + 1;
  page.rowHeight = std::max(page.rowHeight, h);
  return true;
}

uint64_t FontAtlas::fontKey(const Font &f) const {
  auto h = std::hash<std::string>{}(f.family);
  h ^= static_cast<uint64_t>(f.size) << 32;
  h ^= static_cast<uint64_t>(static_cast<int>(f.weight)) << 40;
  h ^= static_cast<uint64_t>(static_cast<int>(f.style)) << 48;
  return h;
}

} // namespace gpu
} // namespace ltgui
