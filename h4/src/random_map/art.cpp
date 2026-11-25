// art.h: the game's dialogs' look for the app's own panels.
#include "art.h"

#include <algorithm>

namespace h4::random_map {

void DrawParchment(std::vector<uint32_t>& px, int w, int h, const Image* back) {
  px.assign(size_t(w) * size_t(h), art::kBack);
  if (!back || back->w <= 0 || back->h <= 0) return;
  for (int y = 0; y < h; ++y) {
    const int py = y / back->h % 2 ? back->h - 1 - y % back->h : y % back->h;
    for (int x = 0; x < w; ++x) {
      const int bx = x / back->w % 2 ? back->w - 1 - x % back->w : x % back->w;
      const uint32_t p = back->argb[size_t(py) * size_t(back->w) + size_t(bx)];
      auto lift = [&](int s, uint32_t to) { return ((((p >> s) & 255u) * 9 + to * 11) / 20) << s; };  // paler: text reads
      px[size_t(y) * size_t(w) + size_t(x)] = 0xFF000000u | lift(16, 0xF4) | lift(8, 0xE8) | lift(0, 0xC8);
    }
  }
}

void DrawDialogFrame(std::vector<uint32_t>& px, int w, int h, const Image* dialog, int border) {
  auto put = [&](int x, int y, uint32_t c) { px[size_t(y) * size_t(w) + size_t(x)] = c; };
  if (dialog && dialog->w == 800 && dialog->h == 600) {
    auto sx = [&](int x) { return x < border ? x : x >= w - border ? 800 - (w - x) : 30 + (x - border) % 380; };
    auto sy = [&](int y) { return y < border ? y : y >= h - border ? 600 - (h - y) : 100 + (y - border) % 380; };
    for (int y = 0; y < h; ++y)
      for (int x = 0; x < w; ++x)
        if (x < border || x >= w - border || y < border || y >= h - border)
          put(x, y, 0xFF000000u | dialog->argb[size_t(sy(y)) * 800 + size_t(sx(x))]);
    return;
  }
  // a drawn one: gold between dark lines
  for (int i = 0; i < 6; ++i) {
    const uint32_t c = i == 0 || i == 5 ? 0xFF3A2208 : i == 2 || i == 3 ? 0xFFD8A840 : 0xFFA07020;
    for (int x = i; x < w - i; ++x) put(x, i, c), put(x, h - 1 - i, c);
    for (int y = i; y < h - i; ++y) put(i, y, c), put(w - 1 - i, y, c);
  }
}

// `art` stretched to w x h's width into `out` (its height kept) at x0: its ends as they are, its
// middle repeated (the frame's top and bottom and the parchment have no seams to speak of).
// (y0: its top in `out`; the art's transparent pixels leave what is there.)
void DrawButtonArt(std::vector<uint32_t>& out, int ow, int x0, int w, const Image& art, int y0) {
  const int cap = std::min(16 * art.h / 40, art.w / 3), mid = art.w - 2 * cap;
  for (int y = 0; y < art.h; ++y)
    for (int x = 0; x < w; ++x) {
      const int sx = x < cap ? x : x >= w - cap ? art.w - (w - x) : cap + (x - cap) % mid;
      const uint32_t p = art.argb[size_t(y) * size_t(art.w) + size_t(sx)];
      uint32_t& d = out[size_t(y0 + y) * size_t(ow) + size_t(x0 + x)];
      if (y0 == 0 || (p >> 24) >= 0x80) d = y0 == 0 ? p : p | 0xFF000000u;
    }
}

// `src` scaled by `k` (bilinear, the colours weighted by alpha so the edges keep theirs).
Image ScaleImage(const Image& src, float k) {
  Image out;
  out.w = std::max(1, int(float(src.w) * k + 0.5f));
  out.h = std::max(1, int(float(src.h) * k + 0.5f));
  out.argb.assign(size_t(out.w) * size_t(out.h), 0);
  for (int y = 0; y < out.h; ++y)
    for (int x = 0; x < out.w; ++x) {
      const float fx = std::clamp((float(x) + 0.5f) / k - 0.5f, 0.0f, float(src.w - 1));
      const float fy = std::clamp((float(y) + 0.5f) / k - 0.5f, 0.0f, float(src.h - 1));
      const int x0 = int(fx), y0 = int(fy), x1 = std::min(x0 + 1, src.w - 1), y1 = std::min(y0 + 1, src.h - 1);
      const float ax = fx - float(x0), ay = fy - float(y0);
      float acc[4] = {};
      const int xs[2] = {x0, x1}, ys[2] = {y0, y1};
      for (int j = 0; j < 2; ++j)
        for (int i = 0; i < 2; ++i) {
          const float wgt = (i ? ax : 1 - ax) * (j ? ay : 1 - ay);
          const uint32_t p = src.argb[size_t(ys[j]) * size_t(src.w) + size_t(xs[i])];
          const float a = float(p >> 24) * wgt;
          acc[0] += a;
          acc[1] += float((p >> 16) & 0xFF) * a;
          acc[2] += float((p >> 8) & 0xFF) * a;
          acc[3] += float(p & 0xFF) * a;
        }
      if (acc[0] <= 0) continue;
      auto ch = [](float v) { return uint32_t(std::clamp(v + 0.5f, 0.0f, 255.0f)); };
      out.argb[size_t(y) * size_t(out.w) + size_t(x)] = ch(acc[0]) << 24 | ch(acc[1] / acc[0]) << 16 |
                                                        ch(acc[2] / acc[0]) << 8 | ch(acc[3] / acc[0]);
    }
  return out;
}
}  // namespace h4::random_map
