#pragma once
#include "color.h"
#include "font.h"
#include "geometry.h"
#include "platform/gpu/gpu_device.h"
#include <list>
#include <string>
#include <unordered_map>
#include <vector>

// stb_truetype is a single-header public-domain library by Sean Barrett.
// Download from: https://github.com/nothings/stb/blob/master/stb_truetype.h
// Place in vendor/stb_truetype.h before building with GPU backend.
//
// If stb_truetype.h is not available, FontAtlas falls back to empty glyphs
// and returns zero from measureText().

#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable : 4244 4456 4701)
#endif

#ifdef __has_include
#if __has_include("stb_truetype.h") && !defined(LTGUI_NO_STB_TRUETYPE)
#define LTGUI_HAS_STB_TRUETYPE
#include "stb_truetype.h"
#endif
#elif __has_include(<stb_truetype.h>) && !defined(LTGUI_NO_STB_TRUETYPE)
#define LTGUI_HAS_STB_TRUETYPE
#include <stb_truetype.h>
#endif

#ifdef _MSC_VER
#pragma warning(pop)
#endif

namespace ltgui {
namespace gpu {

class TextureManager;

struct GlyphEntry {
  int atlasX = 0, atlasY = 0;   // position in atlas texture
  int w = 0, h = 0;             // size in pixels
  int offsetX = 0, offsetY = 0; // offset from pen position
  int advance = 0;              // horizontal advance
  int texId = -1;
  // Position inside the access-order list (LRU eviction); see glyphLRU_.
  std::list<uint64_t>::iterator lruIt;
};

class FontAtlas {
public:
  FontAtlas(GpuDevice *device, TextureManager *texMgr, int atlasW = 2048,
            int atlasH = 2048);
  ~FontAtlas();

  // Load a font from memory (TTF/OTF data)
  bool loadFont(const Font &fontDesc, const uint8_t *ttfData, int ttfSize);

  // Load a font from a file path (returns false if file cannot be read)
  bool loadFontFile(const Font &fontDesc, const char *ttfPath);

  // Get or rasterize a glyph
  const GlyphEntry *getGlyph(uint32_t codepoint, const Font &fontDesc);

  // Measure text (in pixels)
  Size measureText(const std::string &text, const Font &fontDesc);

  bool hasFont(const Font &f) const { return loadedFonts_.count(f) > 0; }
  int atlasW() const { return atlasW_; }
  int atlasH() const { return atlasH_; }

  // Distance from baseline to top of the line (in pixels).
  // Returns 0 if the font is not loaded.
  float getAscent(const Font &fontDesc);

  // Ensure the given font (or its system-default fallback) is loaded
  // at the requested size.  Creates a size-specific cache from stored
  // TTF data if necessary.
  void ensureFontLoaded(const Font &fontDesc);

private:
#ifdef LTGUI_HAS_STB_TRUETYPE
  struct FontCache {
    stbtt_fontinfo info;
    std::vector<uint8_t> ttfData;
    float scale = 0;
    int ascent = 0, descent = 0, lineGap = 0;
  };
#else
  struct FontCache {
    std::vector<uint8_t> ttfData;
    float scale = 0;
    int ascent = 0, descent = 0, lineGap = 0;
  };
#endif

  struct AtlasPage {
    int texId = -1;
    int cursorX = 1, cursorY = 1;
    int rowHeight = 0;
  };

  int createAtlasPage();
  bool allocGlyphRect(int w, int h, int &outX, int &outY, int &outTexId);
  uint64_t fontKey(const Font &f) const;
  void touchGlyph(uint64_t key);
  void addGlyph(uint64_t key, GlyphEntry entry);

  static constexpr int kMaxAtlasPages = 16; // prevent unbounded page growth

  GpuDevice *device_;
  TextureManager *texMgr_;
  int atlasW_, atlasH_;

  // Raw TTF data keyed by (family, weight, style) — size is ignored
  // so we can rasterise at any requested size from the same blob.
  std::unordered_map<Font, std::vector<uint8_t>> ttfData_;
  // Size-specific font caches created on demand from ttfData_
  std::unordered_map<Font, FontCache> loadedFonts_;
  // Glyph cache with a bound on the entry count. Atlas memory is bounded by
  // kMaxAtlasPages already (glyphs never leave a page once rasterized); the
  // LRU here bounds the bookkeeping (and lets an N-glyph face cycle through
  // long sessions without growing forever).
  std::unordered_map<uint64_t, GlyphEntry> glyphCache_;
  std::list<uint64_t> glyphLRU_;  // front = most recently used
  static constexpr size_t kMaxGlyphs = 4096;
  std::vector<AtlasPage> pages_;
};

} // namespace gpu
} // namespace ltgui
