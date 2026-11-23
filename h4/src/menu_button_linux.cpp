// menu_button.h in the Linux app (H4_LINUX_APP: the Steam Deck): no native
// toolkit to put a button over the game (and Game Mode's gamescope shows one
// window only), so the button is drawn into the presented frame
// (gfx::SetPresentBadge) and its clicks (taps, the trackpad) seen by an SDL
// event watch. The app cannot draw text: its generic pictures come with it (ui/*.argb, made at
// build time by platforms/linux/render_ui.swift): letters.argb, the characters the buttons are
// put together from ("🌐 UK", a language's: any language's code), gear.argb, the settings one
// (⚙), and note.argb, the "restart to apply" note shown after a pick. A click on ⚙ opens the settings:
// the languages' buttons one under the other (save sync: a row whose help says how; Linux syncs
// by its sync.conf, save_sync.h; the Switch does not), the intro setting (a checkbox row: intro.h), the
// gamepad's panel (pad_button.argb opens it: MakeGamepad, pad_config.h) and, on Linux, the update check.
#include <SDL3/SDL.h>
#include <unistd.h>

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <mutex>
#include <string>
#include <vector>
#if defined(H4_LINUX_APP)
#include <sys/stat.h>

#include <cstdlib>
#include <fstream>
#include <thread>

#include "install/json.h"
#include "install/platform.h"
#endif

#include "language.h"
#if defined(H4_LINUX_APP)
#include <spawn.h>
#include <sys/wait.h>
extern char** environ;
#endif
#include <thread>

#include "intro.h"
#include "window_mode.h"
#include "menu_button.h"
#include "pad_config.h"
#include "runtime/gfx/device.h"
#include "runtime/hle/user32_window.h"

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
Picture g_restart;  // ui/restart.argb (the Linux app): the note with OK (restart) and Cancel
constexpr int kRestartBand = 40;  // its bottom rows: the two buttons, each half its width
Picture g_gear;                       // the settings button: ⚙
Picture g_kofi_gear;                  // Linux: ☕ (the author's Ko-fi page) and ⚙ (none: ⚙ alone)
Picture g_list;                       // the open list: every button, one under the other
std::vector<int> g_rows;              // where each one starts in it (pixels)
// What each row is: a language (its index in g_choices, 0...) or one of these.
enum { kRowVersion = -1, kRowUpd = -2, kRowFull = -3, kRowSkip = -4, kRowPad = -5, kRowLangs = -6, kRowSync = -7, kRowSides = -8 };
std::vector<int> g_row_what;
bool g_langs_open = false;  // the language dropdown: all of them shown (else the chosen one, ▾)
bool g_list_new = false;              // made again since it was last given to the frame
enum { kButton, kList, kNote, kUpdate, kGamepad, kSyncHelp } g_state = kButton;

// The update check (Linux): the list's last button; its state's message (ui/upd_*.argb) is shown
// until a click (a click on "a new version" downloads it). Its threads change it under g_mu.
enum Upd { kChecking, kLatest, kNewer, kDownloading, kDone, kFailed, kUpdCount };
Picture g_upd_button, g_upd_msg[kUpdCount];
Picture g_skip_pic[2];   // the intro setting's row: off, on (intro.h)
bool g_has_skip = false;
Picture g_sides_pic[2];
bool g_has_sides = false;
Picture g_pad_button;  // "Gamepad": opens its panel (MakeGamepad)
bool g_has_pad = false;
Picture g_version;       // "ReHoMM4 v<version>", the list's last row (none: not shown)
Picture g_full_pic[2];   // the fullscreen setting's row (Linux: window_mode.h)
bool g_has_full = false;
bool g_has_upd = false;  // its pictures are there (and this is Linux)
Picture g_sync_button, g_sync_help;  // the save sync's row ("?") and its help (Linux)
bool g_has_sync = false;
Upd g_upd = kChecking;
std::string g_upd_url, g_upd_tag;  // the newer release's .tar.gz, its version
bool g_hooked = false;
const Picture* g_shown_pic = nullptr;  // in the frame now
bool g_clicked = false;               // a click, for the guest thread
float g_click_x = 0, g_click_y = 0;   // where (window x, y)
bool g_click_in = false;              // on the picture
float g_hit[4] = {0, 0, 0, 0};        // where it is in the window: l, t, r, b
uint64_t g_note_until = 0;            // the note shown until then (ms)

// The gamepad panel (MakeGamepad): its parts' places in the picture, what a click there does.
struct Hit {
  int id, x0, y0, x1, y1;
};
enum { kHitClose = 1, kHitPointer, kHitScroll, kHitPreset, kHitReset, kHitBind = 100 };
std::vector<Hit> g_gp_hits;
Picture g_gp;              // the panel as it is now
bool g_gp_new = false;     // made again since it was last given to the frame
bool g_click_right = false;  // the click was the right button (a list's previous item)
int g_drag = 0;            // the slider the left button went down on (its moves set it)
float g_drag_px = -1;      // where it is now (the panel's x), for the guest thread
bool g_back = false;       // Esc (a gamepad's): back from the panel or the list
bool g_taken = false;      // the last button event was on the settings (not the game's)

int HitAt(float px, float py) {
  for (const Hit& h : g_gp_hits)
    if (px >= h.x0 && px < h.x1 && py >= h.y0 && py < h.y1) return h.id;
  return 0;
}

// Window (x, y) -> the shown picture's pixels.
bool ToPicture(float x, float y, int w, int h, float* px, float* py) {
  if (g_hit[2] <= g_hit[0] || g_hit[3] <= g_hit[1]) return false;
  *px = (x - g_hit[0]) * float(w) / (g_hit[2] - g_hit[0]);
  *py = (y - g_hit[1]) * float(h) / (g_hit[3] - g_hit[1]);
  return true;
}

bool OnEvent(void*, SDL_Event* e) {
  std::lock_guard<std::mutex> l(g_mu);
  if (!g_shown_pic) return true;
  if (e->type == SDL_EVENT_MOUSE_MOTION) {  // a slider dragged
    float px, py;
    if (g_drag && g_state == kGamepad && ToPicture(e->motion.x, e->motion.y, g_gp.w, g_gp.h, &px, &py)) g_drag_px = px;
    return true;
  }
  if (e->type != SDL_EVENT_MOUSE_BUTTON_DOWN && e->type != SDL_EVENT_MOUSE_BUTTON_UP) return true;
  const float x = e->button.x, y = e->button.y;
  const bool in = x >= g_hit[0] && x < g_hit[2] && y >= g_hit[1] && y < g_hit[3];
  g_taken = in || g_state != kButton;
  if (e->type == SDL_EVENT_MOUSE_BUTTON_DOWN) {
    float px, py;
    g_drag = 0;
    if (e->button.button == SDL_BUTTON_LEFT && g_state == kGamepad && ToPicture(x, y, g_gp.w, g_gp.h, &px, &py)) {
      const int id = HitAt(px, py);
      if (id == kHitPointer || id == kHitScroll) g_drag = id;
    }
    return true;
  }
  g_drag = 0;
  const bool right = e->button.button == SDL_BUTTON_RIGHT;
  if (e->button.button != SDL_BUTTON_LEFT && !(right && g_state == kGamepad)) return true;
  g_click_in = in;
  if (g_click_in || g_state != kButton) {  // (a click elsewhere closes the list or the note)
    g_clicked = true;
    g_click_right = right;
    g_click_x = x;
    g_click_y = y;
  }
  return true;
}

// The gamepad as the mouse (recomp::SetPadEventHook): its events never reach the SDL event watch,
// so here they are given to OnEvent; a click on the settings (or anywhere while they are open) is
// not the game's, nor any key while they are open (Esc: back).
float g_pad_x = -1, g_pad_y = -1;  // its pointer (its button events carry no place)
bool PadEvent(const SDL_Event& e) {
  SDL_Event c = e;
  switch (e.type) {
    case SDL_EVENT_MOUSE_MOTION:
      g_pad_x = e.motion.x;
      g_pad_y = e.motion.y;
      OnEvent(nullptr, &c);
      return false;
    case SDL_EVENT_MOUSE_BUTTON_DOWN:
    case SDL_EVENT_MOUSE_BUTTON_UP: {
      if (g_pad_x < 0) return false;
      c.button.x = g_pad_x;
      c.button.y = g_pad_y;
      OnEvent(nullptr, &c);
      std::lock_guard<std::mutex> l(g_mu);
      return g_shown_pic && g_taken;
    }
    case SDL_EVENT_KEY_DOWN:
    case SDL_EVENT_KEY_UP: {
      std::lock_guard<std::mutex> l(g_mu);
      if (!g_shown_pic || g_state == kButton) return false;
      if (e.type == SDL_EVENT_KEY_DOWN && e.key.scancode == SDL_SCANCODE_ESCAPE) g_back = true;
      return true;
    }
    default:
      return false;
  }
}

// The author's Ko-fi page in the desktop's browser (xdg-open; Linux only: the Switch has none).
void OpenKofi() {
#if defined(H4_LINUX_APP)
  pid_t pid = 0;
  const char* argv[] = {"xdg-open", "https://ko-fi.com/hazzard", nullptr};
  if (posix_spawnp(&pid, "xdg-open", nullptr, nullptr, const_cast<char* const*>(argv), environ) == 0)
    std::thread([pid] { waitpid(pid, nullptr, 0); }).detach();
  else
    std::fprintf(stderr, "h4: no xdg-open for the Ko-fi page\n");
#endif
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

// A button: the glyphs `text` (letters.argb's cells; -2 a space) in cream on the brown,
// gold-edged plate.
Picture MakePlate(const std::vector<int>& text) {
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

// The button of a language: "🌐 UK".
Picture MakeButton(const std::string& code) {
  std::vector<int> text{0, -2};  // the globe, a space (-2), then the code
  for (char ch : code) text.push_back(LetterIndex(char32_t(static_cast<unsigned char>(ch))));
  return MakePlate(text);
}

// ---- The gamepad panel (pad_config.h), put together from ui/gp_*.argb (labels, render_ui.swift)
// with the boxes, sliders and plates drawn here. Top to bottom: the title and ✕, the two speeds
// (sliders), the buttons (two columns: the name, its action in a box that takes the next action on
// a click, the previous one on a right-click), the note, and the preset (a box: the next one) at
// the left with Reset at the right.
struct GpPics {
  Picture title, pointer, scroll, buttons, preset, reset, hint, pct;
  Picture style[h4::kPadStyles];
  std::vector<Picture> act;                           // by action
  Picture names[h4::kPadStyles][h4::kPadButtons];     // by style, button
} g_gp_pics;

bool LoadGamepad() {
  GpPics& g = g_gp_pics;
  const std::string d = UiDir();
  bool ok = Load(d, "gp_title", &g.title) && Load(d, "gp_pointer", &g.pointer) &&
            Load(d, "gp_scroll", &g.scroll) && Load(d, "gp_buttons", &g.buttons) && Load(d, "gp_preset", &g.preset) &&
            Load(d, "gp_reset", &g.reset) && Load(d, "gp_hint", &g.hint) && Load(d, "gp_pct", &g.pct);
  g.act.resize(size_t(h4::PadActionCount()));
  for (int a = 0; ok && a < h4::PadActionCount(); a++)
    ok = Load(d, std::string("gp_act_") + h4::PadActionId(a), &g.act[size_t(a)]);
  for (int st = 0; ok && st < h4::kPadStyles; st++) {
    ok = Load(d, std::string("gp_style_") + h4::PadStyleId(st), &g.style[st]);
    for (int b = 0; ok && b < h4::kPadButtons; b++)
      ok = Load(d, std::string("gp_b_") + h4::PadStyleId(st) + "_" + h4::PadButtonId(b), &g.names[st][b]);
  }
  if (!ok) std::fprintf(stderr, "h4: gamepad panel: pictures missing in %s\n", d.c_str());
  return ok;
}

constexpr int kGpW = 700, kGpPad = 14;                                  // the panel's width, margin
constexpr int kSlX0 = kGpPad + 260, kSlX1 = kGpW - kGpPad - 60;          // the sliders' track
constexpr uint32_t kGold = 0xFFD6AB57u, kCream = 0xFFFAEBC7u, kBrown = 0xF51A1208u, kBox = 0xFF2A1D0Eu,
                   kDark = 0xFF100A04u;

void Fill(Picture& p, int x, int y, int w, int h, uint32_t c) {
  for (int yy = std::max(0, y); yy < std::min(p.h, y + h); yy++)
    for (int xx = std::max(0, x); xx < std::min(p.w, x + w); xx++) p.argb[size_t(yy) * p.w + xx] = c;
}

// A rounded plate: `fill` inside a 1-pixel `edge`.
void Plate(Picture& p, int x, int y, int w, int h, uint32_t fill, uint32_t edge, int rad = 6) {
  for (int yy = 0; yy < h; yy++)
    for (int xx = 0; xx < w; xx++) {
      const int cx = xx < rad ? rad - xx : xx >= w - rad ? xx - (w - rad - 1) : 0;
      const int cy = yy < rad ? rad - yy : yy >= h - rad ? yy - (h - rad - 1) : 0;
      const int d2 = cx * cx + cy * cy;
      if (d2 > rad * rad || x + xx < 0 || y + yy < 0 || x + xx >= p.w || y + yy >= p.h) continue;
      const bool e = xx == 0 || yy == 0 || xx == w - 1 || yy == h - 1 || d2 > (rad - 1) * (rad - 1);
      p.argb[size_t(y + yy) * p.w + x + xx] = e ? edge : fill;
    }
}

// `src` (premultiplied) over p at (x, y), at most `max_w` of its columns.
void Blit(Picture& p, const Picture& src, int x, int y, int max_w = 1 << 20) {
  for (int yy = 0; yy < src.h; yy++)
    for (int xx = 0; xx < std::min(src.w, max_w); xx++) {
      const int dx = x + xx, dy = y + yy;
      if (dx < 0 || dy < 0 || dx >= p.w || dy >= p.h) continue;
      const uint32_t g = src.argb[size_t(yy) * src.w + xx], a = g >> 24;
      if (!a) continue;
      uint32_t& d = p.argb[size_t(dy) * p.w + dx];
      uint32_t out = std::max(a, d >> 24) << 24;
      for (int sh = 0; sh < 24; sh += 8) out |= std::min(255u, ((g >> sh) & 255) + ((d >> sh) & 255) * (255 - a) / 255) << sh;
      d = out;
    }
}

// A number in letters.argb's digits, then "%"; its width (drawn only when `p`).
int Percent(Picture* p, int value, int x, int y) {
  const std::string t = std::to_string(value);
  int w = 0;
  for (char ch : t) {
    const int i = LetterIndex(char32_t(ch));
    int l = kCell, r = -1;
    for (int xx = 0; xx < kCell; xx++)
      for (int yy = 0; yy < g_letters.h; yy++)
        if (g_letters.argb[size_t(yy) * g_letters.w + i * kCell + xx] >> 24) {
          l = std::min(l, xx), r = std::max(r, xx);
          break;
        }
    if (r < l) continue;
    if (p) {
      Picture glyph;
      glyph.w = r - l + 1, glyph.h = g_letters.h;
      glyph.argb.resize(size_t(glyph.w) * glyph.h);
      for (int yy = 0; yy < glyph.h; yy++)
        for (int xx = 0; xx < glyph.w; xx++)
          glyph.argb[size_t(yy) * glyph.w + xx] = g_letters.argb[size_t(yy) * g_letters.w + i * kCell + l + xx];
      Blit(*p, glyph, x + w, y);
    }
    w += r - l + 2;
  }
  if (p) Blit(*p, g_gp_pics.pct, x + w, y - 1);
  return w + g_gp_pics.pct.w;
}

void MakeGamepad() {
  const GpPics& g = g_gp_pics;
  const int W = kGpW, pad = kGpPad, style = h4::PadStyle();
  g_gp_hits.clear();
  // The height first: the rows as laid out below.
  const int H = pad + 30 + 2 * 30 + 24 + 8 * 26 + 4 + g.hint.h + 8 + 30 + pad;
  Picture& p = g_gp;
  p.w = W, p.h = H;
  p.argb.assign(size_t(W) * H, 0);
  Plate(p, 0, 0, W, H, kBrown, kGold, 10);
  int y = pad;
  Blit(p, g.title, (W - g.title.w) / 2, y);
  {  // ✕
    const int s = 24, x = W - pad - s;
    Plate(p, x, y - 4, s, s, kBox, kGold);
    for (int k = 6; k < s - 6; k++)
      for (int t = 0; t < 2; t++) {
        Fill(p, x + k + t, y - 4 + k, 1, 1, kCream);
        Fill(p, x + s - 1 - k - t, y - 4 + k, 1, 1, kCream);
      }
    g_gp_hits.push_back({kHitClose, x - 4, y - 8, x + s + 4, y + s});
  }
  y += 30;
  // The sliders: the track from x0 to x1, the knob at the value, the value (%) right of it.
  const int x0 = kSlX0, x1 = kSlX1;
  auto slider = [&](const Picture& title, int id, float v, float lo, float hi) {
    Blit(p, title, pad, y + 4);
    const float f = std::clamp((v - lo) / (hi - lo), 0.0f, 1.0f);
    const int kx = x0 + int(f * float(x1 - x0));
    Fill(p, x0, y + 11, x1 - x0, 4, kBox);
    Fill(p, x0, y + 11, kx - x0, 4, kGold);
    Plate(p, kx - 7, y + 4, 14, 18, kCream, kGold, 4);
    Percent(&p, int(std::lround(v * 100)), W - pad - Percent(nullptr, int(std::lround(v * 100)), 0, 0), y + 5);
    g_gp_hits.push_back({id, x0 - 10, y, x1 + 10, y + 26});
    y += 30;
  };
  slider(g.pointer, kHitPointer, h4::PadPointerSpeed(), h4::kPointerMin, h4::kPointerMax);
  slider(g.scroll, kHitScroll, h4::PadScrollSpeed(), h4::kScrollMin, h4::kScrollMax);
  Blit(p, g.buttons, pad, y + 2);
  y += 24;
  // The buttons, two columns.
  const int col = (W - 2 * pad - 16) / 2, nw = 118, half = (h4::kPadButtons + 1) / 2;
  for (int r = 0; r < half; r++) {
    for (int c = 0; c < 2; c++) {
      const int b = r + c * half;
      if (b >= h4::kPadButtons) continue;
      const int x = pad + c * (col + 16);
      Blit(p, g.names[style][b], x, y + 4, nw - 4);
      const int bx = x + nw, bw = col - nw;
      Plate(p, bx, y, bw, 22, kBox, kGold, 5);
      Blit(p, g.act[size_t(h4::PadBinding(b))], bx + 7, y + 3, bw - 22);
      for (int k = 0; k < 4; k++) Fill(p, bx + bw - 14 + k, y + 9 + k, 8 - 2 * k, 1, kGold);  // ▾
      g_gp_hits.push_back({kHitBind + b, bx, y, bx + bw, y + 22});
    }
    y += 26;
  }
  y += 4;
  Blit(p, g.hint, pad, y);
  y += g.hint.h + 8;
  {  // the preset (a box; not on the Switch: its own pads only) and Reset
#if !defined(__SWITCH__)
    Blit(p, g.preset, pad, y + 7);
    const int bx = pad + 70, bw = 170;
    Plate(p, bx, y + 2, bw, 24, kBox, kGold, 5);
    Blit(p, g.style[style], bx + 8, y + 6);
    for (int k = 0; k < 4; k++) Fill(p, bx + bw - 14 + k, y + 12 + k, 8 - 2 * k, 1, kGold);
    g_gp_hits.push_back({kHitPreset, bx, y, bx + bw, y + 28});
#endif
    const int rw = g.reset.w + 28, rx = W - pad - rw;
    Plate(p, rx, y, rw, 28, kBox, kGold, 7);
    Blit(p, g.reset, rx + 14, y + (28 - g.reset.h) / 2);
    g_gp_hits.push_back({kHitReset, rx, y, rx + rw, y + 28});
  }
  g_gp_new = true;
}

// A click at (px, py) of the panel (right: the right button). False: beside it (closed).
bool GamepadClick(float px, float py, bool right) {
  if (px < 0 || py < 0 || px >= g_gp.w || py >= g_gp.h) return false;
  const int id = HitAt(px, py), step = right ? -1 : 1;
  auto slide = [&](float lo, float hi) { return lo + std::clamp((px - kSlX0) / float(kSlX1 - kSlX0), 0.0f, 1.0f) * (hi - lo); };
  if (id == kHitClose) return false;
  if (id == kHitPointer && !right) h4::SetPadPointerSpeed(slide(h4::kPointerMin, h4::kPointerMax));
  if (id == kHitScroll && !right) h4::SetPadScrollSpeed(slide(h4::kScrollMin, h4::kScrollMax));
  if (id == kHitPreset) h4::ApplyPadPreset((h4::PadStyle() + step + h4::kPadStyles) % h4::kPadStyles);
  if (id == kHitReset && !right) h4::ResetPadConfig();
  if (id >= kHitBind && id < kHitBind + h4::kPadButtons) {
    const int b = id - kHitBind, n = h4::PadActionCount();
    h4::SetPadBinding(b, (h4::PadBinding(b) + step + n) % n);
  }
  MakeGamepad();
  return true;
}

// The languages now (a new translations release adds some), each with its button.
bool LoadChoices() {
  if (g_letters.argb.empty() &&
      !(Load(UiDir(), "letters", &g_letters) && g_letters.w == kCell * kLetterCount && Load(UiDir(), "note", &g_note) &&
        Load(UiDir(), "gear", &g_gear))) {
    g_letters = Picture{};
    return false;
  }
  if (g_version.argb.empty() && !Load(UiDir(), "version", &g_version)) g_version = Picture{};
#if !defined(H4_WEB_APP)  // the browser: the language and the intro are the page's (index.html)
  if (!g_has_skip) g_has_skip = Load(UiDir(), "skip_off", &g_skip_pic[0]) && Load(UiDir(), "skip_on", &g_skip_pic[1]);
#endif
#if !defined(H4_WEB_APP)
  if (!g_has_sides)
    g_has_sides = Load(UiDir(), "sides_off", &g_sides_pic[0]) &&
                  Load(UiDir(), "sides_on", &g_sides_pic[1]);
#endif
  if (!g_has_pad) g_has_pad = Load(UiDir(), "pad_button", &g_pad_button) && LoadGamepad();
#if defined(H4_LINUX_APP)
  if (g_kofi_gear.argb.empty() && !Load(UiDir(), "kofi_gear", &g_kofi_gear)) g_kofi_gear = Picture{};
#if !defined(H4_WEB_APP)  // the browser: the page's full screen; the saves stay in the browser
  if (!g_has_full) g_has_full = Load(UiDir(), "full_off", &g_full_pic[0]) && Load(UiDir(), "full_on", &g_full_pic[1]);
#endif
  if (!g_has_upd) {
    static const char* const kMsg[kUpdCount] = {"upd_checking", "upd_latest", "upd_newer",
                                                "upd_downloading", "upd_done", "upd_failed"};
    g_has_upd = Load(UiDir(), "upd_button", &g_upd_button);
    for (int i = 0; i < kUpdCount && g_has_upd; i++) g_has_upd = Load(UiDir(), kMsg[i], &g_upd_msg[i]);
  }
#if !defined(H4_WEB_APP)
  if (!g_has_sync) g_has_sync = Load(UiDir(), "sync_button", &g_sync_button) && Load(UiDir(), "sync_help", &g_sync_help);
#endif
#endif
  std::vector<Choice> out;
#if !defined(H4_WEB_APP)  // the browser: chosen on the page before the game starts (index.html)
  if (!g_saved.empty())
    for (const h4::LanguageInfo& l : h4::Languages()) out.push_back({l.code, MakeButton(l.code)});
  else if (const std::string gog = h4::GogLanguage(); !gog.empty())
    out.push_back({gog, MakeButton(gog)});  // installed from GOG in it: shown, nothing to choose
#endif
  g_choices = std::move(out);
  return true;
}

// The list: the buttons one under the other, 4 pixels apart, right-aligned (under ⚙): first
// "ReHoMM4 v<version>" and "Check for updates" (Linux), then the languages (the chosen one with a
// gold bar) and the settings.
void MakeList() {
  std::vector<const Picture*> rows;
  g_row_what.clear();
  auto add = [&](const Picture* p, int what) {
    rows.push_back(p);
    g_row_what.push_back(what);
  };
  if (!g_version.argb.empty()) add(&g_version, kRowVersion);
  if (g_has_upd) add(&g_upd_button, kRowUpd);
  // The language, a dropdown: the chosen one (▾), a click shows them all; a click on one of them
  // picks it (or, the chosen one, closes the list).
  if (g_langs_open || g_choices.size() < 2) {
    for (size_t i = 0; i < g_choices.size(); i++) add(&g_choices[i].button, int(i));
  } else if (!g_choices.empty()) {
    size_t c = 0;
    for (size_t i = 0; i < g_choices.size(); i++)
      if (g_choices[i].code == g_next) c = i;
    add(&g_choices[c].button, kRowLangs);
  }
  if (g_has_full) add(&g_full_pic[h4::Fullscreen() ? 1 : 0], kRowFull);
  if (g_has_skip) add(&g_skip_pic[h4::SkipIntro() ? 1 : 0], kRowSkip);
  if (g_has_sides) add(&g_sides_pic[h4::ReadBlackSidePanels(h4::SidePanelsPath()) ? 1 : 0], kRowSides);
  if (g_has_sync) add(&g_sync_button, kRowSync);
  if (g_has_pad) add(&g_pad_button, kRowPad);
  int w = 0, h = 0;
  g_rows.clear();
  for (const Picture* b : rows) {
    g_rows.push_back(h);
    w = std::max(w, b->w + 10);
    h += b->h + 4;
  }
  g_list_new = true;
  g_list.w = w;
  g_list.h = std::max(1, h - 4);
  g_list.argb.assign(size_t(g_list.w) * g_list.h, 0);
  for (size_t i = 0; i < rows.size(); i++) {
    const Picture& b = *rows[i];
    const int x0 = g_list.w - b.w;
    for (int y = 0; y < b.h && g_rows[i] + y < g_list.h; y++) {
      for (int x = 0; x < b.w; x++) g_list.argb[size_t(g_rows[i] + y) * g_list.w + x0 + x] = b.argb[size_t(y) * b.w + x];
      const int what = g_row_what[i];
      if (what >= 0 && g_choices[size_t(what)].code == g_next && y > 3 && y < b.h - 3)
        for (int x = x0 - 8; x < x0 - 4; x++) g_list.argb[size_t(g_rows[i] + y) * g_list.w + x] = 0xFFD6AB57u;
      // ▾ left of the closed dropdown
      const int ty = y - (b.h / 2 - 2);
      if (what == kRowLangs && ty >= 0 && ty < 4)
        for (int x = x0 - 9 + ty; x < x0 - 2 - ty; x++) g_list.argb[size_t(g_rows[i] + y) * g_list.w + x] = 0xFFD6AB57u;
    }
  }
}

#if defined(H4_LINUX_APP)
// "0.10" > "0.9" ("v" in front ignored).
// This build's file in a release (its architecture's).
#if defined(__aarch64__)
const std::string kTarSuffix = "-linux-arm64.tar.gz";
#else
const std::string kTarSuffix = "-linux-x86_64.tar.gz";
#endif

bool Newer(std::string a, std::string b) {
  auto nums = [](std::string s) {
    if (!s.empty() && s[0] == 'v') s.erase(0, 1);
    std::vector<long> v;
    size_t i = 0;
    while (i <= s.size()) {
      const size_t j = std::min(s.find('.', i), s.size());
      v.push_back(std::strtol(s.substr(i, j - i).c_str(), nullptr, 10));
      i = j + 1;
    }
    return v;
  };
  std::vector<long> x = nums(a), y = nums(b);
  const size_t n = std::max(x.size(), y.size());
  x.resize(n), y.resize(n);
  return x > y;
}

void SetUpd(Upd u) {
  std::lock_guard<std::mutex> l(g_mu);
  g_upd = u;
}

// The latest release on GitHub against this app's version (a thread).
void CheckForUpdates() {
  std::thread([] {
    int status = 0;
    std::string body, error;
    h4::install::Json rel;
    if (!h4::install::HttpGet("https://api.github.com/repos/evilHazzarD/ReHoMM4/releases/latest", "", &status,
                              &body, &error) ||
        status != 200 || !h4::install::Json::Parse(body, &rel) || rel["tag_name"].text().empty()) {
      std::fprintf(stderr, "h4: update check failed: %s (HTTP %d)\n", error.c_str(), status);
      SetUpd(kFailed);
      return;
    }
    const std::string tag = rel["tag_name"].text();
    std::string url;
    for (size_t i = 0; i < rel["assets"].size(); i++)
      if (const std::string n = rel["assets"][i]["name"].text();  // reHoMM4-v<version>-linux-… (0.2: HeroesIV-<version>-…)
          (n.rfind("reHoMM4-", 0) == 0 || n.rfind("HeroesIV-", 0) == 0) && n.size() > kTarSuffix.size() &&
          n.compare(n.size() - kTarSuffix.size(), kTarSuffix.size(), kTarSuffix) == 0)
        url = rel["assets"][i]["browser_download_url"].text();
    std::fprintf(stderr, "h4: update check: %s on GitHub, this is %s\n", tag.c_str(), H4_VERSION_STRING);
    std::lock_guard<std::mutex> l(g_mu);
    g_upd_tag = tag;
    g_upd_url = url;
    g_upd = Newer(tag, H4_VERSION_STRING) && !url.empty() ? kNewer : kLatest;
  }).detach();
}

// The newer release's .tar.gz into ~/Downloads (never over a file there; a thread).
void DownloadUpdate() {
  std::string url, tag;
  {
    std::lock_guard<std::mutex> l(g_mu);
    url = g_upd_url, tag = g_upd_tag;
  }
  std::thread([url, tag] {
    int status = 0;
    std::string body, error;
    const char* home = std::getenv("HOME");
    const std::string dir = std::string(home ? home : ".") + "/Downloads";
    mkdir(dir.c_str(), 0755);
    // The release's own name for it (never over a file there: "<name> 2.tar.gz"...).
    std::string base = url.substr(url.rfind('/') + 1);
    if (base.size() > 7 && base.compare(base.size() - 7, 7, ".tar.gz") == 0) base.resize(base.size() - 7);
    if (base.empty()) base = "reHoMM4-" + tag + kTarSuffix.substr(0, kTarSuffix.size() - 7);
    std::string path = dir + "/" + base + ".tar.gz";
    for (int i = 2; access(path.c_str(), F_OK) == 0; i++) path = dir + "/" + base + " " + std::to_string(i) + ".tar.gz";
    bool ok = h4::install::HttpGet(url, "", &status, &body, &error) && status == 200 && !body.empty();
    if (ok) {
      std::ofstream f(path, std::ios::binary);
      ok = bool(f.write(body.data(), std::streamsize(body.size())));
    }
    std::fprintf(stderr, "h4: update download: %s (%s HTTP %d)\n", ok ? path.c_str() : "failed", error.c_str(), status);
    SetUpd(ok ? kDone : kFailed);
  }).detach();
}
#endif

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
  if (g_lang.empty()) return;  // (g_saved "": no languages to choose, the rest of the settings)
  if (g_letters.argb.empty()) {
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
    recomp::SetPadEventHook(PadEvent);
  }
  const uint64_t now = SDL_GetTicks();
#if defined(H4_LINUX_APP)
  static bool restart_tried = false;  // (once: none on an older package)
  if (!restart_tried && !Load(UiDir(), "restart", &g_restart)) g_restart = Picture{};
  restart_tried = true;
#endif
  if (g_state == kNote && g_note_until <= now && g_restart.argb.empty()) g_state = kButton;
  if (!shown) g_state = kButton;
  if (g_back) {  // Esc (a gamepad's): the panel back to the list, the rest closed
    g_back = false;
    if (g_state == kGamepad) {
      MakeList();
      g_state = kList;
    } else {
      g_state = kButton;
    }
  }
  if (g_state == kGamepad && g_drag_px >= 0) {  // a slider dragged
    const float px = g_drag_px;
    g_drag_px = -1;
    const float f = std::clamp((px - kSlX0) / float(kSlX1 - kSlX0), 0.0f, 1.0f);
    if (g_drag == kHitPointer) h4::SetPadPointerSpeed(h4::kPointerMin + f * (h4::kPointerMax - h4::kPointerMin));
    if (g_drag == kHitScroll) h4::SetPadScrollSpeed(h4::kScrollMin + f * (h4::kScrollMax - h4::kScrollMin));
    MakeGamepad();
  }
  if (g_clicked) {
    g_clicked = false;
    // ☕ (the left plate of kofi_gear: 44 of its pixels): the Ko-fi page in the browser.
    const bool kofi = g_state == kButton && g_click_in && !g_kofi_gear.argb.empty() && g_hit[2] > g_hit[0] &&
                      (g_click_x - g_hit[0]) * float(g_kofi_gear.w) / (g_hit[2] - g_hit[0]) < 44;
    if (kofi) {
      OpenKofi();
    } else if (g_state == kButton) {
      LoadChoices();  // a release may have added languages since
      g_langs_open = false;
      MakeList();
      g_state = kList;
    } else if (g_state == kList && g_click_in && g_hit[3] > g_hit[1]) {
      // Which row: the click's place in the picture.
      const int py = int((g_click_y - g_hit[1]) * float(g_list.h) / (g_hit[3] - g_hit[1]));
      size_t i = 0;
      while (i + 1 < g_rows.size() && py >= g_rows[i + 1]) i++;
      const int what = i < g_row_what.size() ? g_row_what[i] : kRowVersion;
      if (what == kRowLangs || (what >= 0 && g_choices[size_t(what)].code == g_next && g_choices.size() > 1)) {
        g_langs_open = what == kRowLangs;  // opened, or closed by the chosen one again
        MakeList();
      } else if (what >= 0 && g_choices[size_t(what)].code != g_next) {
        g_langs_open = false;
        g_next = g_choices[size_t(what)].code;
        Choose(g_next);  // saved
#if defined(H4_LINUX_APP)
        l.unlock();
        h4::RestartApp();  // the app again, with it (back here only when that failed)
        l.lock();
#endif
        // (the Switch, or the restart failed) the note: it applies once the game is restarted
        g_state = kNote;
        g_note_until = now + 6000;
      } else if (what == kRowFull) {
        // At once; the list stays. Not under g_mu: X11's SDL_SetWindowSize pumps the events while
        // it waits for the window, and OnEvent (an event watch) takes g_mu: the game hung.
        const bool full = !h4::Fullscreen();
        l.unlock();
        h4::SetFullscreen(full);
        l.lock();
        MakeList();
      } else if (what == kRowSkip) {
        h4::SetSkipIntro(!h4::SkipIntro());  // the box flips; the list stays
        MakeList();
      } else if (what == kRowSides) {
        h4::SaveBlackSidePanels(!h4::ReadBlackSidePanels(h4::SidePanelsPath()));
        MakeList();
      } else if (what == kRowSync) {
        g_state = kSyncHelp;  // its help; a click closes it
      } else if (what == kRowPad) {
        MakeGamepad();  // its panel
        g_state = kGamepad;
#if defined(H4_LINUX_APP)
      } else if (what == kRowUpd) {
        g_state = kUpdate;
        g_upd = kChecking;
        l.unlock();
        CheckForUpdates();
        l.lock();
#endif
      } else {
        g_state = kButton;
      }
#if defined(H4_LINUX_APP)
    } else if (g_state == kUpdate) {
      if (g_upd == kNewer && g_click_in) {
        g_upd = kDownloading;
        l.unlock();
        DownloadUpdate();
        l.lock();
      } else if (g_upd != kChecking && g_upd != kDownloading) {
        MakeList();  // read: back to the settings
        g_state = kList;
      }
#endif
#if defined(H4_LINUX_APP)
    } else if (g_state == kNote && !g_restart.argb.empty()) {
      // OK: the app starts again with the new language; Cancel, or beside: at the next start.
      if (g_click_in && g_hit[2] > g_hit[0] && g_hit[3] > g_hit[1]) {
        const int px = int((g_click_x - g_hit[0]) * float(g_restart.w) / (g_hit[2] - g_hit[0]));
        const int py = int((g_click_y - g_hit[1]) * float(g_restart.h) / (g_hit[3] - g_hit[1]));
        if (py >= g_restart.h - kRestartBand) {
          if (px < g_restart.w / 2) {
            l.unlock();
            h4::RestartApp();
            l.lock();
          }
          g_state = kButton;
        }
      } else {
        g_state = kButton;
      }
#endif
    } else if (g_state == kSyncHelp) {
      MakeList();  // read: back to the settings
      g_state = kList;
    } else if (g_state == kGamepad) {
      float px = -1, py = -1;
      ToPicture(g_click_x, g_click_y, g_gp.w, g_gp.h, &px, &py);
      if (!g_click_in || !GamepadClick(px, py, g_click_right)) {  // ✕, or beside it: back to the settings
        MakeList();
        g_state = kList;
      }
    } else {
      g_state = kButton;  // a click on the note, or beside the list
    }
  }
  const Picture* pic = !shown ? nullptr
                       : g_state == kList ? &g_list
                       : g_state == kNote ? (g_restart.argb.empty() ? &g_note : &g_restart)
                       : g_state == kUpdate ? &g_upd_msg[g_upd]
                       : g_state == kGamepad ? &g_gp
                       : g_state == kSyncHelp ? &g_sync_help
                       : !g_kofi_gear.argb.empty() ? &g_kofi_gear
                                          : &g_gear;
  int px = 0, py = y;
  if (pic) {
    // Top-right corner at (x, y); kept inside the frame.
    const recomp::gfx::Presentation p = recomp::gfx::LastPresentation();
    px = std::max(4, std::min(x - pic->w, (p.src_w > 0 ? p.src_w : x + pic->w) - pic->w - 4));
    if (pic == &g_gp && p.src_w > 0 && p.src_h > 0) {  // the gamepad panel: in the middle
      px = std::max(0, (p.src_w - pic->w) / 2);
      py = std::max(0, (p.src_h - pic->h) / 2);
    }
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
  if (pic != g_shown_pic || (pic == &g_list && g_list_new) || (pic == &g_gp && g_gp_new)) {
    if (pic == &g_list) g_list_new = false;
    if (pic == &g_gp) g_gp_new = false;
    if (pic) recomp::gfx::SetPresentBadge(pic->argb.data(), pic->w, pic->h);
    g_shown_pic = pic;
  }
  recomp::gfx::SetPresentBadgePos(px, py, pic != nullptr);
}

}  // namespace h4
