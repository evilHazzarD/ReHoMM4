#include "font.h"

#include <algorithm>

#include "resources.h"

namespace h4::random_map {

bool Font::Load(const Resources& res, std::string_view name) {
  std::vector<uint8_t> d;
  if (!res.Read(name, d) || d.size() < 7 || d[0] != 1 || d[1] != 0) return false;
  auto u32 = [&](size_t p) { return uint32_t(d[p] | d[p + 1] << 8 | d[p + 2] << 16 | uint32_t(d[p + 3]) << 24); };
  const int first = d[2], count = d[6];
  line_ = std::max<int>(d[3], d[4]);
  glyphs_.assign(256, {});
  size_t p = 7;
  for (int i = 0; i < count; ++i) {
    if (p + 16 > d.size()) return false;
    Glyph g;
    g.w = int(u32(p));
    g.h = int(u32(p + 4));
    g.x = int(int32_t(u32(p + 8)));
    g.y = int(int32_t(u32(p + 12)));
    p += 16;
    const size_t n = size_t(g.w) * size_t(g.h);
    if (g.w > 256 || g.h > 256 || p + n > d.size()) return false;
    g.px.assign(d.begin() + long(p), d.begin() + long(p + n));
    p += n;
    if (first + i < 256) glyphs_[size_t(first + i)] = std::move(g);
  }
  return true;
}

int Font::Width(std::string_view text) const {
  if (glyphs_.empty()) return 0;
  int w = 0;
  for (unsigned char ch : text) w += glyphs_[ch].x + glyphs_[ch].w + glyphs_[ch].y;
  return w;
}

void Font::Draw(std::vector<uint32_t>& canvas, int w, int h, int x, int y, std::string_view text,
                uint32_t ink) const {
  if (glyphs_.empty()) return;
  auto blend = [&](int px, int py, uint32_t colour, int alpha) {  // alpha 0..15
    if (px < 0 || py < 0 || px >= w || py >= h || alpha <= 0) return;
    uint32_t& dst = canvas[size_t(py) * w + px];
    const int a = alpha * 17, ia = 255 - a;
    const uint32_t r = (((colour >> 16) & 255) * a + ((dst >> 16) & 255) * ia) / 255;
    const uint32_t g = (((colour >> 8) & 255) * a + ((dst >> 8) & 255) * ia) / 255;
    const uint32_t b = ((colour & 255) * a + (dst & 255) * ia) / 255;
    const uint32_t da = std::max<uint32_t>(dst >> 24, uint32_t(a));
    dst = da << 24 | r << 16 | g << 8 | b;
  };
  for (unsigned char ch : text) {
    const Glyph& g = glyphs_[ch];
    const int gx = x + g.x, gy = y;
    for (int pass = 0; pass < 2; ++pass)  // the shadow, then the ink over it
      for (int row = 0; row < g.h; ++row)
        for (int col = 0; col < g.w; ++col) {
          const uint8_t v = g.px[size_t(row) * g.w + col];
          if (pass == 0) blend(gx + col, gy + row, 0xFF000000, v & 15);
          else blend(gx + col, gy + row, ink, v >> 4);
        }
    x += g.x + g.w + g.y;
  }
}

}  // namespace h4::random_map
