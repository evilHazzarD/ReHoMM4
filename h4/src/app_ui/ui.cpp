// app_ui/ui.h: the app's panels in the game's dialogs' look.
#include "ui.h"

#include <algorithm>
#include <cmath>
#include <cstdio>

#include "../random_map/art.h"
#include "../random_map_ui.h"
#include "../ui_text.h"

namespace h4::app_ui {
namespace art = random_map::art;

namespace {

const GameDialogArt* Art() { return DialogArt(); }

// A line `t` pixels thick from (x0, y0) to (x1, y1), smooth edges.
void Line(std::vector<uint32_t>& px, int w, int h, float x0, float y0, float x1, float y1, float t, uint32_t c) {
  const int bx0 = int(std::floor(std::min(x0, x1) - t)), bx1 = int(std::ceil(std::max(x0, x1) + t));
  const int by0 = int(std::floor(std::min(y0, y1) - t)), by1 = int(std::ceil(std::max(y0, y1) + t));
  const float dx = x1 - x0, dy = y1 - y0, len2 = std::max(dx * dx + dy * dy, 1e-6f);
  for (int y = std::max(0, by0); y <= std::min(h - 1, by1); ++y)
    for (int x = std::max(0, bx0); x <= std::min(w - 1, bx1); ++x) {
      const float px0 = float(x) + 0.5f, py0 = float(y) + 0.5f;
      const float k = std::clamp(((px0 - x0) * dx + (py0 - y0) * dy) / len2, 0.0f, 1.0f);
      const float ex = x0 + k * dx - px0, ey = y0 + k * dy - py0;
      const float cover = std::clamp(t / 2 - std::sqrt(ex * ex + ey * ey) + 0.5f, 0.0f, 1.0f);
      if (cover <= 0) continue;
      const uint32_t a = uint32_t(cover * 255 + 0.5f);
      uint32_t& d = px[size_t(y) * size_t(w) + size_t(x)];
      uint32_t out = 0xFF000000u;
      for (int sh = 0; sh < 24; sh += 8) out |= ((((c >> sh) & 255) * a + ((d >> sh) & 255) * (255 - a)) / 255) << sh;
      d = out;
    }
}

}  // namespace

Ui::Ui(int w, int h, std::vector<uint32_t>* px, const std::string& lang, std::string* open)
    : w_(w), h_(h), px_(px), lang_(lang), open_(open) {}

const char* Ui::T(const char* english) const { return TrIn(lang_, english); }

const Writer& Ui::W(int size) const {
  for (auto& [s, w] : writers_)
    if (s == size) return *w;
  writers_.emplace_back(size, std::make_unique<Writer>(size, lang_, size >= 20));
  return *writers_.back().second;
}

int Ui::LineHeight(int size) const { return W(size).Height(); }
int Ui::TextWidth(const std::string& t, int size) const { return W(size).Width(t); }
void Ui::Text(int x, int y0, const std::string& t, uint32_t ink, int size) const {
  if (px_) W(size).Draw(*px_, w_, h_, x, y0, t, ink);
}

void Ui::Fill(int x0, int y0, int x1, int y1, uint32_t c) const {
  if (!px_) return;
  for (int yy = std::max(0, y0); yy < std::min(h_, y1); ++yy)
    for (int xx = std::max(0, x0); xx < std::min(w_, x1); ++xx) (*px_)[size_t(yy) * size_t(w_) + size_t(xx)] = c;
}

void Ui::Frame(int x0, int y0, int x1, int y1, uint32_t c) const {
  Fill(x0, y0, x1, y0 + 1, c);
  Fill(x0, y1 - 1, x1, y1, c);
  Fill(x0, y0, x0 + 1, y1, c);
  Fill(x1 - 1, y0, x1, y1, c);
}

void Ui::Plate(int x0, int y0, int x1, int y1, bool lit) const {
  Fill(x0, y0, x1, y1, lit ? art::kLit : art::kPlate);
  Frame(x0, y0, x1, y1, lit ? art::kGold : art::kEdge);
}

void Ui::Arrow(int cx, int cy, uint32_t c) const {
  for (int k = 0; k < 5; ++k) Fill(cx - 5 + k, cy - 2 + k, cx + 6 - k, cy - 1 + k, c);
}

// ---- parts

void Ui::Title(const std::string& t) {
  Text((w_ - TextWidth(t, 24)) / 2, y, t, art::kGold, 24);
  y += LineHeight(24) + 8;
  bottom_ = std::max(bottom_, y);
}

void Ui::Paragraph(const std::string& t, bool centred, uint32_t ink) {
  const int lh = LineHeight();
  for (const std::string& line : W(16).Wrap(t, right() - left())) {
    Text(centred ? (w_ - TextWidth(line)) / 2 : left(), y, line, ink ? ink : art::kInk);
    y += lh + 2;
  }
  y += 4;
  bottom_ = std::max(bottom_, y);
}

void Ui::Box(int x0, int y0, int x1, int y1, const std::string& t, bool arrow, std::function<void(bool)> click) {
  const bool lit = false;
  Plate(x0, y0, x1, y1, lit);
  const int a = arrow ? 22 : 0;
  if (arrow) Frame(x1 - a, y0, x1, y1, art::kEdge), Arrow(x1 - a / 2, (y0 + y1) / 2, art::kInk);
  std::string v = t;
  while (v.size() > 1 && TextWidth(v) > x1 - x0 - a - 14) {  // (cut at a whole UTF-8 character)
    v.pop_back();
    while (!v.empty() && (uint8_t(v.back()) & 0xC0) == 0x80) v.pop_back();
  }
  Text(x0 + 8, y0 + (y1 - y0 - LineHeight()) / 2, v, art::kInk);
  hits_.push_back({x0, y0, x1, y1, [click](int, bool right) { click(right); }});
}

void Ui::Dropdown(const std::string& id, const std::string& label, const std::vector<std::string>& names, int selected,
                  std::function<void(int)> pick) {
  const int lh = LineHeight(), rh = lh + 10;
  int bw = 160;
  for (const std::string& n : names) bw = std::max(bw, TextWidth(n) + 44);
  bw = std::min(bw, (right() - left()) * 3 / 5);
  const int x1 = right(), x0 = x1 - bw, y0 = y, y1 = y + rh;
  const bool open = open_ && *open_ == id;
  Text(left(), y0 + (rh - lh) / 2, label, art::kInk);
  Plate(x0, y0, x1, y1, open);
  constexpr int a = 22;
  Frame(x1 - a, y0, x1, y1, open ? art::kGold : art::kEdge);
  Arrow(x1 - a / 2, (y0 + y1) / 2, open ? art::kGold : art::kInk);
  if (selected >= 0 && size_t(selected) < names.size()) {
    std::string v = names[size_t(selected)];
    while (v.size() > 1 && TextWidth(v) > bw - a - 14) {
      v.pop_back();
      while (!v.empty() && (uint8_t(v.back()) & 0xC0) == 0x80) v.pop_back();
    }
    Text(x0 + 8, y0 + (rh - lh) / 2, v, open ? art::kGold : art::kInk);
  }
  std::string* open_id = open_;
  hits_.push_back({x0, y0, x1, y1, [open_id, id](int, bool) {
                     if (open_id) *open_id = *open_id == id ? "" : id;
                   }});
  if (open) {  // its list under it, over what follows
    const int ih = lh + 6, ly0 = y1, ly1 = y1 + int(names.size()) * ih + 6;
    bottom_ = std::max(bottom_, ly1);
    for (size_t i = 0; i < names.size(); ++i) {
      const int iy = ly0 + 3 + int(i) * ih;
      hits_.push_back({x0, iy, x1, iy + ih, [open_id, pick, i](int, bool) {
                         if (open_id) open_id->clear();
                         pick(int(i));
                       },
                       false, true});
    }
    popups_.push_back([this, names, selected, x0, x1, ly0, ly1, ih, lh] {
      Fill(x0, ly0, x1, ly1, art::kList);
      Frame(x0, ly0, x1, ly1, art::kEdge);
      for (size_t i = 0; i < names.size(); ++i) {
        const int iy = ly0 + 3 + int(i) * ih;
        const bool chosen = int(i) == selected;
        if (chosen) Fill(x0 + 3, iy, x1 - 3, iy + ih - 1, art::kLit), Frame(x0 + 3, iy, x1 - 3, iy + ih - 1, art::kGold);
        Text(x0 + 10, iy + (ih - lh) / 2, names[i], chosen ? art::kGold : art::kInk);
      }
    });
  }
  y = y1 + 6;
  bottom_ = std::max(bottom_, y);
}

void Ui::Value(const std::string& label, const std::string& value) {
  const int lh = LineHeight(), rh = lh + 10;
  Text(left(), y + (rh - lh) / 2, label, art::kInk);
  Text(right() - TextWidth(value), y + (rh - lh) / 2, value, art::kInk);
  y += rh + 6;
  bottom_ = std::max(bottom_, y);
}

void Ui::Check(const std::string& label, bool on, std::function<void()> toggle, std::function<void()> help) {
  const int lh = LineHeight(), qs = lh + 6, lines_w = right() - left() - 26 - (help ? qs + 8 : 0);
  const std::vector<std::string> lines = W(16).Wrap(label, lines_w);
  const int rh = std::max(lh + 6, int(lines.size()) * (lh + 2) + 4);
  const int bs = 15, bx = left(), by = y + (lh + 6 - bs) / 2;
  Fill(bx, by, bx + bs, by + bs, art::kField);
  Frame(bx, by, bx + bs, by + bs, art::kEdge);
  if (on && px_) {
    Line(*px_, w_, h_, float(bx) + 3.5f, float(by) + 7.5f, float(bx) + 6.5f, float(by) + 11.0f, 2.2f, art::kInk);
    Line(*px_, w_, h_, float(bx) + 6.5f, float(by) + 11.0f, float(bx) + 12.0f, float(by) + 3.5f, 2.2f, art::kInk);
  }
  int ly = y + 3;
  for (const std::string& l : lines) {
    Text(bx + bs + 10, ly, l, art::kInk);
    ly += lh + 2;
  }
  hits_.push_back({left(), y, right(), y + rh, [toggle](int, bool right) {
                     if (!right) toggle();
                   }});
  if (help) {  // after the last line's text (taken before the row: the later hit wins)
    const int qx = bx + bs + 10 + (lines.empty() ? 0 : TextWidth(lines.back())) + 8;
    const int qy = ly - (lh + 2) - 3;
    Plate(qx, qy, qx + qs, qy + qs, false);
    Text(qx + (qs - TextWidth("?")) / 2, qy + (qs - lh) / 2, "?", art::kInk);
    hits_.push_back({qx, qy, qx + qs, qy + qs, [help](int, bool right) {
                       if (!right) help();
                     }});
  }
  y += rh + 4;
  bottom_ = std::max(bottom_, y);
}

void Ui::LabelButton(const std::string& label, const std::string& button, std::function<void()> click) {
  const int lh = LineHeight(), rh = lh + 10, bw = std::max(90, TextWidth(button) + 28);
  Text(left(), y + (rh - lh) / 2, label, art::kInk);
  const int x1 = right(), x0 = x1 - bw;
  Plate(x0, y, x1, y + rh, false);
  Text(x0 + (bw - TextWidth(button)) / 2, y + (rh - lh) / 2, button, art::kInk);
  hits_.push_back({x0, y, x1, y + rh, [click](int, bool right) {
                     if (!right) click();
                   }});
  y += rh + 6;
  bottom_ = std::max(bottom_, y);
}

void Ui::InfoButton(const std::string& info, const std::string& button, std::function<void()> click) {
  InfoButtons(info, {{button, std::move(click)}});
}

void Ui::InfoButtons(const std::string& info, const std::vector<std::pair<std::string, std::function<void()>>>& buttons,
                     int marked) {
  const int lh = LineHeight(), rh = lh + 10;
  constexpr int gap = 8;
  std::vector<int> widths;
  int total = 0;
  for (const auto& b : buttons) {
    widths.push_back(std::max(90, TextWidth(b.first) + 28));
    total += widths.back() + (widths.size() > 1 ? gap : 0);
  }
  if (!info.empty()) Text(left(), y + (rh - lh) / 2, info, art::kDim);
  int x0 = info.empty() ? (w_ - total) / 2 : right() - total;
  for (size_t i = 0; i < buttons.size(); ++i) {
    const int bw = widths[i], x1 = x0 + bw;
    const bool mark = int(i) == marked;
    Plate(x0, y, x1, y + rh, mark);
    Text(x0 + (bw - TextWidth(buttons[i].first)) / 2, y + (rh - lh) / 2, buttons[i].first, mark ? art::kWarn : art::kInk);
    auto click = buttons[i].second;
    hits_.push_back({x0, y, x1, y + rh, [click](int, bool right) {
                       if (!right) click();
                     }});
    x0 = x1 + gap;
  }
  y += rh + 6;
  bottom_ = std::max(bottom_, y);
}

void Ui::Buttons(const std::vector<std::pair<std::string, std::function<void()>>>& buttons) {
  const GameDialogArt* a = Art();
  const random_map::Image* up = a ? a->button_up : nullptr;
  const bool game = up && up->w > 0 && up->h > 0;
  const int bh = game ? up->h : LineHeight(20) + 14;
  constexpr int gap = 10;
  std::vector<int> widths;
  for (const auto& b : buttons) widths.push_back(std::max(game ? up->w : 110, TextWidth(b.first, 20) + 44));
  // In rows as wide as the panel lets them (each row centred).
  const int room = right() - left() + 2 * kPad;
  for (size_t first = 0; first < buttons.size();) {
    size_t end = first;
    int total = 0;
    while (end < buttons.size() && (end == first || total + gap + widths[end] <= room))
      total += widths[end] + (end > first ? gap : 0), ++end;
    int x = (w_ - total) / 2;
    if (first > 0) y += bh + 6;
    for (size_t i = first; i < end; ++i) {
      const int bw = widths[i];
      if (px_) {
        if (game) random_map::DrawButtonArt(*px_, w_, x, bw, *up, std::max(1, y));
        else Plate(x, y, x + bw, y + bh, false);
      }
      const std::string& label = buttons[i].first;
      Text(x + (bw - TextWidth(label, 20)) / 2, y + (bh - LineHeight(20)) / 2, label, art::kInk, 20);
      auto fn = buttons[i].second;
      hits_.push_back({x, y, x + bw, y + bh, [fn](int, bool right) {
                         if (!right) fn();
                       }});
      x += bw + gap;
    }
    first = end;
  }
  y += bh + 8;
  bottom_ = std::max(bottom_, y);
}

void Ui::Slider(const std::string& label, float value, float lo, float hi, std::function<void(float)> set) {
  const int lh = LineHeight(), rh = lh + 10;
  const int lw = std::min(260, (right() - left()) / 2);
  const int x0 = left() + lw + 10, x1 = right() - 56;
  Text(left(), y + (rh - lh) / 2, label, art::kInk);
  const float f = std::clamp((value - lo) / (hi - lo), 0.0f, 1.0f);
  const int kx = x0 + int(f * float(x1 - x0)), cy = y + rh / 2;
  Fill(x0, cy - 2, x1, cy + 2, art::kPlateDim);
  Frame(x0, cy - 2, x1, cy + 2, art::kEdge);
  Fill(x0 + 1, cy - 1, kx, cy + 1, art::kLit);
  Plate(kx - 6, y + 2, kx + 6, y + rh - 2, false);
  const std::string pct = std::to_string(int(std::lround(value * 100))) + "%";
  Text(right() - TextWidth(pct), y + (rh - lh) / 2, pct, art::kInk);
  hits_.push_back({x0 - 10, y, x1 + 10, y + rh, [set, x0, x1, lo, hi](int x, bool right) {
                     if (!right) set(lo + std::clamp(float(x - x0) / float(x1 - x0), 0.0f, 1.0f) * (hi - lo));
                   },
                   true});
  y += rh + 6;
  bottom_ = std::max(bottom_, y);
}

void Ui::Anywhere(std::function<void()> click) {
  hits_.insert(hits_.begin(), Hit{0, 0, w_, 1 << 20, [click](int, bool right) {
                                    if (!right) click();
                                  }});
}

void Ui::Close(std::function<void()> close) {
  constexpr int s = 22;
  const int x0 = w_ - kBorder - s - 4, y0 = kBorder + 4;
  Plate(x0, y0, x0 + s, y0 + s, false);
  if (px_) {
    Line(*px_, w_, h_, float(x0) + 6.5f, float(y0) + 6.5f, float(x0 + s) - 6.5f, float(y0 + s) - 6.5f, 2.2f, art::kInk);
    Line(*px_, w_, h_, float(x0 + s) - 6.5f, float(y0) + 6.5f, float(x0) + 6.5f, float(y0 + s) - 6.5f, 2.2f, art::kInk);
  }
  hits_.push_back({x0 - 4, y0 - 4, x0 + s + 4, y0 + s + 4, [close](int, bool right) {
                     if (!right) close();
                   }});
}

// ---- the panel

void Panel::Set(int width, const std::string& lang, Build build) {
  w_ = width;
  lang_ = lang;
  build_ = std::move(build);
  open_.clear();
  held_ = -1;
  dirty_ = true;
}

const Picture& Panel::Draw() {
  if (!dirty_ || !build_) return pic_;
  // Measured first (its height), then drawn on the game's parchment in its frame.
  int h;
  {
    Ui m(w_, 0, nullptr, lang_, &open_);
    m.y = Ui::kBorder + 10;
    build_(m);
    h = std::max(m.y, m.bottom_) + Ui::kBorder + 4;
    base_h_ = m.y + Ui::kBorder + 4;
  }
  const GameDialogArt* a = Art();
  pic_.w = w_;
  pic_.h = h;
  random_map::DrawParchment(pic_.argb, w_, h, a ? a->parchment : nullptr);
  random_map::DrawDialogFrame(pic_.argb, w_, h, a ? a->dialog : nullptr, Ui::kBorder);
  Ui d(w_, h, &pic_.argb, lang_, &open_);
  d.y = Ui::kBorder + 10;
  build_(d);
  for (auto& p : d.popups_) p();
  hits_ = std::move(d.hits_);
  dirty_ = false;
  ++version_;
  return pic_;
}

bool Panel::Click(int x, int y, bool right) {
  const bool popup = std::any_of(hits_.begin(), hits_.end(), [](const Ui::Hit& h) { return h.popup; });
  if (x < 0 || y < 0 || x >= pic_.w || y >= pic_.h) {
    if (!popup) return false;
    open_.clear();  // beside an open list: it closes
    dirty_ = true;
    return true;
  }
  for (size_t i = hits_.size(); i-- > 0;) {
    const Ui::Hit& h = hits_[i];
    if ((popup && !h.popup) || x < h.x0 || x >= h.x1 || y < h.y0 || y >= h.y1) continue;
    dirty_ = true;
    const auto fn = h.fn;  // (it may set another panel up: the hits go)
    fn(x, right);
    return true;
  }
  if (popup) {
    open_.clear();
    dirty_ = true;
  }
  return true;
}

void Panel::Press(int x, int y) {
  held_ = -1;
  for (size_t i = 0; i < hits_.size(); ++i) {
    const Ui::Hit& h = hits_[i];
    if (h.drag && x >= h.x0 && x < h.x1 && y >= h.y0 && y < h.y1) held_ = int(i);
  }
  if (held_ >= 0) Drag(x);
}

void Panel::Drag(int x) {
  if (held_ < 0 || size_t(held_) >= hits_.size()) return;
  const auto fn = hits_[size_t(held_)].fn;
  fn(x, false);
  dirty_ = true;
}

bool Panel::Back() {
  if (open_.empty()) return false;
  open_.clear();
  dirty_ = true;
  return true;
}

Picture IconRow(const std::vector<std::string>& glyphs, int side, int gap, const std::string& tip, int under,
                const std::string& lang, int* row_x) {
  Picture p;
  const int n = int(glyphs.size());
  const int row_w = n > 0 ? n * side + (n - 1) * gap : 0;
  // The tooltip: its box, its right edge under icon `under`'s.
  const Writer tw(16, lang);
  const bool has_tip = !tip.empty() && under >= 0 && under < n;
  const int pad = 6, box_w = has_tip ? tw.Width(tip) + 2 * pad : 0, box_h = has_tip ? tw.Height() + 2 * pad : 0;
  const int tip_r = has_tip ? under * (side + gap) + side : 0;
  const int ox = has_tip ? std::max(0, box_w - tip_r) : 0;  // the row's x in the picture
  if (row_x) *row_x = ox;
  p.w = ox + row_w;
  p.h = side + (has_tip ? 4 + box_h : 0);
  p.argb.assign(size_t(p.w) * size_t(p.h), 0);
  if (has_tip) {
    const int x0 = ox + tip_r - box_w, y0 = side + 4;
    for (int y = 0; y < box_h; ++y)
      for (int x = 0; x < box_w; ++x) {
        const bool edge = x == 0 || y == 0 || x == box_w - 1 || y == box_h - 1;
        p.argb[size_t(y0 + y) * size_t(p.w) + size_t(x0 + x)] = edge ? art::kEdge : 0xF0FFF3D6;
      }
    tw.Draw(p.argb, p.w, p.h, x0 + pad, y0 + pad, tip, art::kInk);
  }
  const GameDialogArt* a = Art();
  const Writer writer(28, "en");
  for (int i = 0; i < n; ++i) {
    std::vector<uint32_t> one;
    random_map::DrawParchment(one, side, side, a ? a->parchment : nullptr);
    for (int r = 0; r < 3; ++r) {  // the game's gold-brown edge between dark lines
      const uint32_t c = r == 1 ? art::kEdge : 0xFF3A2208;
      for (int k = r; k < side - r; ++k) {
        one[size_t(r) * size_t(side) + size_t(k)] = one[size_t(side - 1 - r) * size_t(side) + size_t(k)] = c;
        one[size_t(k) * size_t(side) + size_t(r)] = one[size_t(k) * size_t(side) + size_t(side - 1 - r)] = c;
      }
    }
    const std::string& g = glyphs[size_t(i)];
    writer.Draw(one, side, side, (side - writer.Width(g)) / 2, (side - writer.Height()) / 2, g, art::kInk);
    const int x0 = ox + i * (side + gap);
    for (int y = 0; y < side; ++y)
      for (int x = 0; x < side; ++x) p.argb[size_t(y) * size_t(p.w) + size_t(x0 + x)] = one[size_t(y) * size_t(side) + size_t(x)];
  }
  return p;
}

}  // namespace h4::app_ui
