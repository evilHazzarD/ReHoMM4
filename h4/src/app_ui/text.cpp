// app_ui/text.h: the game's font, or the app's own (stb_truetype over h4/fonts).
#include "text.h"

#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <cstring>
#include <map>
#include <mutex>

#include "../random_map/font.h"
#include "../random_map/locale.h"
#include "../random_map_ui.h"

#if defined(__clang__)
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wall"
#pragma clang diagnostic ignored "-Wextra"
#pragma clang diagnostic ignored "-Wunused-function"
#elif defined(__GNUC__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wall"
#pragma GCC diagnostic ignored "-Wextra"
#pragma GCC diagnostic ignored "-Wunused-function"
#endif
#define STB_TRUETYPE_IMPLEMENTATION
#define STBTT_STATIC
#include "stb_truetype.h"
#if defined(__clang__)
#pragma clang diagnostic pop
#elif defined(__GNUC__)
#pragma GCC diagnostic pop
#endif

namespace h4::app_ui {
namespace {

struct UiFontFile {
  const char* name;
  const unsigned char* data;
  size_t size;
};
#include "ui_fonts.inc"  // kUiFontFiles (h4/cmake/ui_fonts.cmake)

// The app's fonts: the text's (regular, bold), then the symbols' (asked when the text's lack one).
enum { kRegular, kBold, kSymbols, kSymbols2, kFonts };
const char* const kNames[kFonts] = {"NotoSans-Regular", "NotoSans-Bold", "NotoSansSymbols-Subset",
                                    "NotoSansSymbols2-Subset"};

struct Glyph {
  int w = 0, h = 0, x = 0, y = 0;  // the bitmap and its offset from the pen (y: from the baseline)
  int advance = 0;                 // in 1/64 pixel
  std::vector<uint8_t> px;
};

struct Store {
  stbtt_fontinfo info[kFonts];
  bool ok[kFonts] = {};
  std::mutex mu;
  std::map<uint64_t, Glyph> glyphs;  // by font, size (1/16 pixel) and character
};

Store& Fonts() {
  static Store* s = [] {
    auto* st = new Store;
    for (int i = 0; i < kFonts; i++)
      for (const UiFontFile& f : kUiFontFiles)
        if (!std::strcmp(f.name, kNames[i]))
          st->ok[i] = stbtt_InitFont(&st->info[i], f.data, stbtt_GetFontOffsetForIndex(f.data, 0)) != 0;
    return st;
  }();
  return *s;
}

// The font that has `c`: the text's own, else a symbols one, else the text's (its missing glyph).
int FontFor(Store& s, int text_font, uint32_t c) {
  for (int f : {text_font, int(kSymbols), int(kSymbols2)})
    if (s.ok[f] && stbtt_FindGlyphIndex(&s.info[f], int(c))) return f;
  return text_font;
}

// (under s.mu)
const Glyph& GlyphOf(Store& s, int font, float em, uint32_t c) {
  const uint64_t key = uint64_t(font) << 56 | uint64_t(std::lround(em * 16)) << 32 | c;
  auto it = s.glyphs.find(key);
  if (it != s.glyphs.end()) return it->second;
  Glyph g;
  if (s.ok[font]) {
    const stbtt_fontinfo& fi = s.info[font];
    const float k = stbtt_ScaleForMappingEmToPixels(&fi, em);
    int adv = 0, lsb = 0;
    stbtt_GetCodepointHMetrics(&fi, int(c), &adv, &lsb);
    g.advance = int(std::lround(float(adv) * k * 64));
    int x0, y0, x1, y1;
    stbtt_GetCodepointBitmapBox(&fi, int(c), k, k, &x0, &y0, &x1, &y1);
    g.w = std::max(0, x1 - x0), g.h = std::max(0, y1 - y0), g.x = x0, g.y = y0;
    if (g.w && g.h) {
      g.px.resize(size_t(g.w) * size_t(g.h));
      stbtt_MakeCodepointBitmap(&fi, g.px.data(), g.w, g.h, g.w, k, k, int(c));
    }
  }
  return s.glyphs.emplace(key, std::move(g)).first->second;
}

// The next character of UTF-8 `s` from `i` (moved past it); a broken byte as itself.
uint32_t Next(const std::string& s, size_t& i) {
  uint32_t c = uint8_t(s[i++]);
  if (c < 0xC0) return c;
  const int more = c >= 0xF0 ? 3 : c >= 0xE0 ? 2 : 1;
  c &= 0x3Fu >> more;
  for (int k = 0; k < more && i < s.size() && (uint8_t(s[i]) & 0xC0) == 0x80; ++k) c = c << 6 | (uint8_t(s[i++]) & 0x3F);
  return c;
}

std::string GameLanguage() {
  const char* l = std::getenv("H4_LANG");
  return l && *l ? l : "en";
}

bool Ascii(const std::string& s) {
  for (char ch : s)
    if (uint8_t(ch) >= 0x80) return false;
  return true;
}

}  // namespace

Writer::Writer(int size, const std::string& lang, bool bold) : bold_(bold) {
  const random_map::Font* game = GameFont(size);
  game_ = game;
  const std::string game_lang = GameLanguage();
  code_page_ = random_map::CodePage(game_lang.substr(0, game_lang.find('-')));
  const std::string ui = lang.empty() ? game_lang : lang;
  // The interface's words in the game's font when they fit its code page (English: always), so
  // its lines do not change font from word to word.
  game_for_all_ = game && (ui.rfind("en", 0) == 0 || random_map::CodePage(ui.substr(0, ui.find('-'))) == code_page_);
  Store& s = Fonts();
  const stbtt_fontinfo& fi = s.info[bold ? kBold : kRegular];
  int asc = 1069, desc = -293, gap = 0;
  if (s.ok[bold ? kBold : kRegular]) stbtt_GetFontVMetrics(&fi, &asc, &desc, &gap);
  int em_units = 1000;
  if (s.ok[bold ? kBold : kRegular]) em_units = ttUSHORT(fi.data + fi.head + 18);
  const float line = float(asc - desc) / float(em_units);  // a line in ems
  // As tall as the game's font of that size (its lines and ours line up), else about its size.
  height_ = game ? game->Height() : int(std::lround(float(size) * 1.1f));
  em_ = float(height_) / line;
  ascent_ = int(std::lround(float(asc) / float(em_units) * em_));
}

bool Writer::GameFontFor(const std::string& utf8) const {
  if (!game_) return false;
  if (Ascii(utf8)) return true;
  if (!game_for_all_) return false;
  // Every character in the game's code page (the arrows of a d-pad are not: the app's font).
  const std::string bytes = random_map::Encode(utf8, code_page_);
  size_t q_in = 0, q_out = 0;
  for (char ch : utf8) q_in += ch == '?';
  for (char ch : bytes) q_out += ch == '?';
  return q_in == q_out;
}

int Writer::Width(const std::string& utf8) const {
  if (GameFontFor(utf8)) {
    return static_cast<const random_map::Font*>(game_)->Width(random_map::Encode(utf8, code_page_));
  }
  Store& s = Fonts();
  std::lock_guard<std::mutex> l(s.mu);
  int w64 = 0;
  const int text_font = bold_ ? kBold : kRegular;
  for (size_t i = 0; i < utf8.size();) {
    const uint32_t c = Next(utf8, i);
    if (c == 0xFE0F || c == 0xFE0E) continue;  // (emoji presentation selectors)
    w64 += GlyphOf(s, FontFor(s, text_font, c), em_, c).advance;
  }
  return (w64 + 63) / 64;
}

void Writer::Draw(std::vector<uint32_t>& canvas, int w, int h, int x, int y, const std::string& utf8,
                  uint32_t ink) const {
  if (GameFontFor(utf8)) {
    static_cast<const random_map::Font*>(game_)->Draw(canvas, w, h, x, y, random_map::Encode(utf8, code_page_), ink,
                                                      false);
    return;
  }
  Store& s = Fonts();
  std::lock_guard<std::mutex> l(s.mu);
  const int text_font = bold_ ? kBold : kRegular;
  int pen64 = x * 64;
  const int base = y + ascent_;
  for (size_t i = 0; i < utf8.size();) {
    const uint32_t c = Next(utf8, i);
    if (c == 0xFE0F || c == 0xFE0E) continue;
    const Glyph& g = GlyphOf(s, FontFor(s, text_font, c), em_, c);
    const int gx = (pen64 + 32) / 64 + g.x, gy = base + g.y;
    for (int yy = 0; yy < g.h; ++yy) {
      const int py = gy + yy;
      if (py < 0 || py >= h) continue;
      for (int xx = 0; xx < g.w; ++xx) {
        const int px = gx + xx;
        const uint32_t a = g.px[size_t(yy) * size_t(g.w) + size_t(xx)];
        if (!a || px < 0 || px >= w) continue;
        uint32_t& d = canvas[size_t(py) * size_t(w) + size_t(px)];
        uint32_t out = std::max(d >> 24, a) << 24;
        for (int sh = 0; sh < 24; sh += 8)
          out |= (((ink >> sh) & 255) * a + ((d >> sh) & 255) * (255 - a)) / 255 << sh;
        d = out;
      }
    }
    pen64 += g.advance;
  }
}

std::vector<std::string> Writer::Wrap(const std::string& utf8, int max_w) const {
  std::vector<std::string> out;
  size_t start = 0;
  while (start <= utf8.size()) {
    size_t end = utf8.find('\n', start);
    if (end == std::string::npos) end = utf8.size();
    std::string rest = utf8.substr(start, end - start), line;
    while (!rest.empty()) {
      // The longest start of `rest` up to a space that fits (a word alone if none does).
      size_t cut = std::string::npos, at = 0;
      while ((at = rest.find(' ', at + 1)) != std::string::npos && Width(rest.substr(0, at)) <= max_w) cut = at;
      if (Width(rest) <= max_w || (cut == std::string::npos && rest.find(' ') == std::string::npos)) {
        out.push_back(rest);
        break;
      }
      if (cut == std::string::npos) cut = rest.find(' ');
      out.push_back(rest.substr(0, cut));
      rest = rest.substr(cut + 1);
    }
    if (rest.empty() && utf8.substr(start, end - start).empty()) out.push_back("");
    start = end + 1;
  }
  return out;
}

}  // namespace h4::app_ui
