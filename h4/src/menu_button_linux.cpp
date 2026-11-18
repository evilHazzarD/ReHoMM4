// menu_button.h in the Linux app (H4_LINUX_APP: the Steam Deck): no native
// toolkit to put a button over the game (and Game Mode's gamescope shows one
// window only), so the button is drawn into the presented frame
// (gfx::SetPresentBadge) and its clicks (taps, the trackpad) seen by an SDL
// event watch. The app cannot draw text: its generic pictures come with it (ui/*.argb, made at
// build time by platforms/linux/render_ui.swift): letters.argb, the characters the button
// "🌐 UK" is put together from (any language's code), and note.argb, the "restart to apply"
// note shown after a pick. A click opens the list: those buttons one under the other.
#include <SDL3/SDL.h>
#include <unistd.h>

#include <algorithm>
#include <cstdint>
#include <cstdio>
#include <mutex>
#include <string>
#include <vector>

#include "language.h"
#include "menu_button.h"
#include "runtime/gfx/device.h"

namespace {

struct Picture {
  int w = 0, h = 0;
  std::vector<uint32_t> argb;
};

// <ui dir>/<name>.argb: width, height (little-endian 32-bit), then ARGB pixels.
bool Load(const std::string& dir, const std::string& name, Picture* p) {
  FILE* f = std::fopen((dir + "/" + name + ".argb").c_str(), "rb");
  if (!f) return false;
  uint32_t wh[2];
  bool ok = std::fread(wh, 4, 2, f) == 2 && wh[0] > 0 && wh[1] > 0 && wh[0] < 4096 && wh[1] < 4096;
  if (ok) {
    p->w = int(wh[0]);
    p->h = int(wh[1]);
    p->argb.resize(size_t(p->w) * p->h);
    ok = std::fread(p->argb.data(), 4, p->argb.size(), f) == p->argb.size();
  }
  std::fclose(f);
  return ok;
}

std::string UiDir() {
#if defined(__SWITCH__)
  return "romfs:/ui";  // in the .nro (platforms/switch/package.sh)
#endif
  char exe[4096];
  const ssize_t n = readlink("/proc/self/exe", exe, sizeof exe - 1);
  if (n <= 0) return "ui";
  std::string s(exe, size_t(n));
  return s.substr(0, s.rfind('/')) + "/ui";
}

std::mutex g_mu;
std::string g_lang, g_saved, g_next;  // running, saved file, the one picked
struct Choice {
  std::string code;
  Picture button;
};
std::vector<Choice> g_choices;        // every language, English first
Picture g_letters, g_note;            // ui/letters.argb, ui/note.argb
Picture g_list;                       // the open list: every button, one under the other
std::vector<int> g_rows;              // where each one starts in it (pixels)
bool g_list_new = false;              // made again since it was last given to the frame
enum { kButton, kList, kNote } g_state = kButton;
bool g_hooked = false;
const Picture* g_shown_pic = nullptr;  // in the frame now
bool g_clicked = false;               // a click, for the guest thread
float g_click_y = 0;                  // where (window y)
bool g_click_in = false;              // on the picture
float g_hit[4] = {0, 0, 0, 0};        // where it is in the window: l, t, r, b
uint64_t g_note_until = 0;            // the note shown until then (ms)

bool OnEvent(void*, SDL_Event* e) {
  if (e->type != SDL_EVENT_MOUSE_BUTTON_UP || e->button.button != SDL_BUTTON_LEFT) return true;
  const float x = e->button.x, y = e->button.y;
  std::lock_guard<std::mutex> l(g_mu);
  if (!g_shown_pic) return true;
  g_click_in = x >= g_hit[0] && x < g_hit[2] && y >= g_hit[1] && y < g_hit[3];
  if (g_click_in || g_state != kButton) {  // (a click elsewhere closes the list or the note)
    g_clicked = true;
    g_click_y = y;
  }
  return true;
}

void Choose(const std::string& lang) {
  if (FILE* f = std::fopen(g_saved.c_str(), "w")) {
    std::fputs(lang.c_str(), f);
    std::fclose(f);
  }
}

// letters.argb: kLetterCount cells of kCell pixels, in render_ui.swift's order.
constexpr int kCell = 20, kLetterCount = 38;
int LetterIndex(char32_t c) {
  if (c == U'\U0001F310') return 0;  // 🌐
  if (c >= 'a' && c <= 'z') c -= 'a' - 'A';
  if (c >= 'A' && c <= 'Z') return 1 + int(c - 'A');
  if (c >= '0' && c <= '9') return 27 + int(c - '0');
  if (c == '-') return 37;
  return -1;
}

// The button of a language: "🌐 UK" in cream on the brown, gold-edged plate.
Picture MakeButton(const std::string& code) {
  std::vector<int> text{0, -2};  // the globe, a space (-2), then the code
  for (char ch : code) text.push_back(LetterIndex(char32_t(static_cast<unsigned char>(ch))));
  const int lh = g_letters.h, pad_x = 12, pad_y = 5;
  // Each glyph's ink: its first and last column with any alpha.
  std::vector<std::pair<int, int>> ink;
  int w = 2 * pad_x;
  for (int i : text) {
    int l = 0, r = -1;
    if (i >= 0) {
      l = kCell, r = -1;
      for (int x = 0; x < kCell; x++)
        for (int y = 0; y < lh; y++)
          if (g_letters.argb[size_t(y) * g_letters.w + i * kCell + x] >> 24) {
            l = std::min(l, x), r = std::max(r, x);
            break;
          }
    }
    ink.emplace_back(l, r);
    w += i == -2 ? 4 : (r >= l ? r - l + 2 : 0);
  }
  Picture p;
  p.w = w, p.h = lh + 2 * pad_y;
  p.argb.assign(size_t(p.w) * p.h, 0);
  const int rad = 8;
  for (int y = 0; y < p.h; y++)
    for (int x = 0; x < p.w; x++) {
      // Rounded corners: the distance to the nearest corner circle's centre.
      const int cx = x < rad ? rad - x : x >= p.w - rad ? x - (p.w - rad - 1) : 0;
      const int cy = y < rad ? rad - y : y >= p.h - rad ? y - (p.h - rad - 1) : 0;
      const int d2 = cx * cx + cy * cy;
      if (d2 > rad * rad) continue;
      const bool edge = x == 0 || y == 0 || x == p.w - 1 || y == p.h - 1 || d2 > (rad - 1) * (rad - 1);
      p.argb[size_t(y) * p.w + x] = edge ? 0xFFD6AB57u : 0xEB1A1208u;
    }
  int x0 = pad_x;
  for (size_t k = 0; k < text.size(); k++) {
    const int i = text[k];
    if (i == -2) { x0 += 4; continue; }
    if (i < 0 || ink[k].second < ink[k].first) continue;
    for (int y = 0; y < lh; y++)
      for (int x = ink[k].first; x <= ink[k].second; x++) {
        const uint32_t g = g_letters.argb[size_t(y) * g_letters.w + i * kCell + x];
        const uint32_t a = g >> 24;
        if (!a) continue;
        uint32_t& d = p.argb[size_t(pad_y + y) * p.w + x0 + x - ink[k].first];
        uint32_t out = std::max(a, d >> 24) << 24;
        for (int sh = 0; sh < 24; sh += 8)  // premultiplied: glyph + plate * (1 - alpha)
          out |= std::min(255u, ((g >> sh) & 255) + ((d >> sh) & 255) * (255 - a) / 255) << sh;
        d = out;
      }
    x0 += ink[k].second - ink[k].first + 2;
  }
  return p;
}

// The languages now (a new translations release adds some), each with its button.
bool LoadChoices() {
  if (g_letters.argb.empty() &&
      !(Load(UiDir(), "letters", &g_letters) && g_letters.w == kCell * kLetterCount && Load(UiDir(), "note", &g_note))) {
    g_letters = Picture{};
    return false;
  }
  std::vector<Choice> out;
  for (const h4::LanguageInfo& l : h4::Languages()) out.push_back({l.code, MakeButton(l.code)});
  g_choices = std::move(out);
  return true;
}

const Choice* Current() {
  for (const Choice& c : g_choices)
    if (c.code == g_next) return &c;
  return g_choices.empty() ? nullptr : &g_choices[0];
}

// The list: the buttons one under the other, 4 pixels apart, the chosen one with a gold bar.
void MakeList() {
  int w = 0, h = 0;
  g_rows.clear();
  for (const Choice& c : g_choices) {
    g_rows.push_back(h);
    w = std::max(w, c.button.w + 10);
    h += c.button.h + 4;
  }
  g_list_new = true;
  g_list.w = w;
  g_list.h = std::max(1, h - 4);
  g_list.argb.assign(size_t(g_list.w) * g_list.h, 0);
  for (size_t i = 0; i < g_choices.size(); i++) {
    const Picture& b = g_choices[i].button;
    for (int y = 0; y < b.h && g_rows[i] + y < g_list.h; y++) {
      for (int x = 0; x < b.w; x++) g_list.argb[size_t(g_rows[i] + y) * g_list.w + 10 + x] = b.argb[size_t(y) * b.w + x];
      if (g_choices[i].code == g_next && y > 3 && y < b.h - 3)
        for (int x = 2; x < 6; x++) g_list.argb[size_t(g_rows[i] + y) * g_list.w + x] = 0xFFD6AB57u;
    }
  }
}

}  // namespace

namespace h4 {

void SetLanguageChoice(const std::string& lang, const std::string& saved_path) {
  std::lock_guard<std::mutex> l(g_mu);
  g_lang = lang;
  g_saved = saved_path;
  g_next = lang;
}

void ShowLanguageButton(bool shown, int x, int y, int /*left: iOS only*/) {
  std::unique_lock<std::mutex> l(g_mu);
  if (g_saved.empty()) return;
  if (g_choices.empty()) {
    if (!LoadChoices()) {
      static bool said = false;
      if (!said) std::fprintf(stderr, "h4: language button: no pictures in %s\n", UiDir().c_str());
      said = true;
      return;
    }
  }
  if (!g_hooked) {
    g_hooked = true;
    SDL_AddEventWatch(OnEvent, nullptr);
  }
  const uint64_t now = SDL_GetTicks();
  if (g_state == kNote && g_note_until <= now) g_state = kButton;
  if (!shown) g_state = kButton;
  if (g_clicked) {
    g_clicked = false;
    if (g_state == kButton) {
      LoadChoices();  // a release may have added languages since
      MakeList();
      g_state = kList;
    } else if (g_state == kList && g_click_in && g_hit[3] > g_hit[1]) {
      // Which row: the click's place in the picture.
      const int py = int((g_click_y - g_hit[1]) * float(g_list.h) / (g_hit[3] - g_hit[1]));
      size_t i = 0;
      while (i + 1 < g_rows.size() && py >= g_rows[i + 1]) i++;
      if (i < g_choices.size() && g_choices[i].code != g_next) {
        g_next = g_choices[i].code;
        Choose(g_next);  // saved; the note says it applies once the game is restarted
        g_state = kNote;
        g_note_until = now + 6000;
      } else {
        g_state = kButton;
      }
    } else {
      g_state = kButton;  // a click on the note, or beside the list
    }
  }
  const Choice* c = Current();
  const Picture* pic = !shown || !c ? nullptr
                       : g_state == kList ? &g_list
                       : g_state == kNote ? &g_note
                                          : &c->button;
  int px = 0, py = y;
  if (pic) {
    // Top centre at (x, y); kept inside the frame.
    const recomp::gfx::Presentation p = recomp::gfx::LastPresentation();
    px = std::max(4, std::min(x - pic->w / 2, (p.src_w > 0 ? p.src_w : x + pic->w) - pic->w - 4));
    if (SDL_Window** wins = SDL_GetWindows(nullptr)) {
      int ww = 0, wh = 0;
      if (wins[0]) SDL_GetWindowSize(wins[0], &ww, &wh);
      SDL_free(wins);
      float l0, t0, r0, b0;
      FrameToView(px, py, float(ww), float(wh), &l0, &t0);
      FrameToView(px + pic->w, py + pic->h, float(ww), float(wh), &r0, &b0);
      g_hit[0] = l0, g_hit[1] = t0, g_hit[2] = r0, g_hit[3] = b0;
    }
  }
  if (pic != g_shown_pic || (pic == &g_list && g_list_new)) {
    g_list_new = false;
    if (pic) recomp::gfx::SetPresentBadge(pic->argb.data(), pic->w, pic->h);
    g_shown_pic = pic;
  }
  recomp::gfx::SetPresentBadgePos(px, py, pic != nullptr);
}

}  // namespace h4
