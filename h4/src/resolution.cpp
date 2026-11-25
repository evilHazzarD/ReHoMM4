// Contains code from HoMM4Deadware (https://github.com/LeoDeadware/HoMM4Deadware): Copyright (c) 2026 LeoDeadware, MIT License.
// The game's screen size and wide screens. Replacements of lifted functions
// named with `replace` in lifter.manifest.
//
// Every display mode change goes through set_mode(width, height, repaint,
// full_screen) at 0x8D3750 (cdecl): the startup mode from the "resolution"
// setting (0x73D9BF), the options dialog's 800x600 / 1024x768 / 1280x1024
// (0x773EB6..), and the full-screen toggle (0x8D3710). It records the size
// (0xAD2508 / 0xAD250C), recreates the DirectDraw surfaces and lays the
// interface out for it.
//
// The interface exists for 800, 1024 and 1280 wide screens only: every
// screen (main menu 0x76D9B0, adventure map, combat, towns, dialogs) picks
// its art and layout by the exact width of its window, and finds nothing
// for another. Its windows are positioned from their parents; a screen
// without one takes the "screen rectangle" (0x8D2E20, the client area) and
// the client area again on WM_SIZE (0x8D22D0). Wide (H4_RESOLUTION=WxH,
// W > 1280, H >= 1024, or "wide": 1024 high at the host display's aspect):
// the display mode is W x H and the screen rectangle the 1280x1024 one in
// its middle, so every screen lays out as at 1280x1024, centred.
//
// The adventure map then takes the whole width (WideAdventure below): the
// top window and the adventure frame span the screen, the right-hand panel
// moves to the right edge, the frame's left edge to the left one, and the
// map window grows in between. Screens built while it is shown (towns,
// combat, dialogs) read their parent's width in their constructors, so
// those run with the top window back at the layout rectangle.

#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <chrono>
#include <map>
#include <string>
#include <vector>
#include <random>
#include <set>

#include <SDL3/SDL.h>
#if defined(__APPLE__)
#include <TargetConditionals.h>
#endif
#if defined(__EMSCRIPTEN__)
#include <emscripten.h>
#endif

#include "intro.h"
#include "window_mode.h"
#include "renderer.h"
#include "menu_button.h"
#include "random_map/hero_card.h"
#include "random_map_ui.h"
#include "runtime/ctx.h"
#include "runtime/gfx/device.h"
#include "runtime/display.h"
#include "runtime/guest_alloc.h"
#include "runtime/hle/ddraw_impl.h"
#include "runtime/hle/present.h"
#include "runtime/log.h"
#include "runtime/runtime.h"

// The game's Options dialog chooses the layout (800x600, 1024x768 or 1280x1024; renderer.h's
// LoadLayout keeps it) in the apps (not the plain recompiled build, which has no player folder).
#if defined(__ANDROID__) || (defined(__APPLE__) && TARGET_OS_IOS) || \
    defined(H4_SWITCH_APP) || defined(H4_DESKTOP_APP) || defined(H4_WINDOWS_APP)
#define H4_LAYOUT_CHOICE 1
#endif

namespace recomp::gen {

void f_008D3750_lifted(::recomp::Ctx& c);
void f_005A29A0_lifted(::recomp::Ctx& c);
void f_006ECB80_lifted(::recomp::Ctx& c);
void f_008D1360_lifted(::recomp::Ctx& c);
void f_00598170_lifted(::recomp::Ctx& c);
void f_004ACBB0_lifted(::recomp::Ctx& c);
void f_004A3450_lifted(::recomp::Ctx& c);
void f_004A37C0_lifted(::recomp::Ctx& c);
void f_0062B6A0_lifted(::recomp::Ctx& c);
void f_0069FB50_lifted(::recomp::Ctx& c);
void f_0058D970_lifted(::recomp::Ctx& c);
void f_008AC0C0_lifted(::recomp::Ctx& c);
void f_0076D9B0_lifted(::recomp::Ctx& c);
void f_0076DEB0_lifted(::recomp::Ctx& c);
void f_008D2E20_lifted(::recomp::Ctx& c);
void f_008D22D0_lifted(::recomp::Ctx& c);
void f_007CA0D0_lifted(::recomp::Ctx& c);
void f_008D0C70_lifted(::recomp::Ctx& c);
void f_00720530_lifted(::recomp::Ctx& c);
void f_0071E6D0_lifted(::recomp::Ctx& c);
void f_0063B840_lifted(::recomp::Ctx& c);

namespace {

// The layout every screen is laid out in, centred (H4_LAYOUT: 1280, 1280x1024, the default, or
// 1024, the game's 1024x768: everything a quarter larger on the same screen).
int kLayoutW = 1280, kLayoutH = 1024;

// The adventure frame's parts in that layout (layers.adventure.<width>), in the frame's coordinates:
// where the right-hand panel (the right dragon column) starts, where the left edge ends, the
// horizontal strips (left, top, bottom: their right ends vary) stretched over the widened width, and
// how many rows at the top and the bottom are its thin gold lines (PaintDirty).
struct FrameParts {
  int panel, left;
  struct Strip { int l, t, b; } strips[4];
  int top_rows, bottom_rows;
  // The widened map window's top and bottom (0: its own): clear of those gold lines, which are
  // painted over the stretched part after the frame. 1024's map runs 9 rows under the top one and 5
  // under the bottom one: redrawn, it painted over them, and they flickered.
  int map_top, map_bottom;
  // What else is over the map: 1280's and 1024's are the left dragons' ornaments (kept by the left
  // edge); 800's (no dragons) its resource bar (whole, or only the gold), docked at the map's
  // bottom right corner.
  bool inner_left;
};
constexpr FrameParts kFrame1280{880, 58, {{0, 16, 80}, {58, 964, 1016}, {0, 0, 16}, {58, 1016, 1024}}, 17, 15, 0, 0, true};
constexpr FrameParts kFrame1024{631, 21, {{20, 9, 75}, {0, 683, 756}, {21, 0, 9}, {0, 756, 768}}, 18, 17, 18, 751, true};
// (800's frame has no dragons: plain gold lines all round)
constexpr FrameParts kFrame800{480, 21, {{20, 11, 20}, {0, 577, 594}, {21, 0, 11}, {0, 594, 600}}, 20, 19, 20, 581, false};
const FrameParts& Frame() { return kLayoutW == 1024 ? kFrame1024 : kLayoutW == 800 ? kFrame800 : kFrame1280; }
constexpr uint32_t kSetRect = 0x71EEE0;  // window::move(const RECT*, bool repaint), thiscall
constexpr uint32_t kTopWindow = 0xABACF4;
constexpr uint32_t kScreenW = 0xAD2508;  // the size set_mode last set
constexpr uint32_t kInvalidate = 0x71FD10;  // window::invalidate(const RECT* local), thiscall
constexpr uint32_t kButtonUpdate = 0x5A1200;  // button::update_state(), thiscall: its look after a change
constexpr uint32_t kMapResize = 0x4F5480;   // adventure_map_window::on_size(const RECT*), thiscall

struct Screen {
  int w = 0, h = 0;  // 0: the game's own sizes
  bool wide = false;
};

// "wide": the layout's height at the host display's aspect.
void WideSize(Screen* r) {
  int dw = 0, dh = 0;
  DesktopSize(&dw, &dh);
  r->h = kLayoutH;
  r->w = dh > 0 ? int(int64_t(dw) * kLayoutH / dh) & ~1 : kLayoutW;
}

Screen& MutableScreen() {
  static Screen s = [] {
    Screen r;
    if (const char* l = Env("LAYOUT"); l && !std::strncmp(l, "1024", 4)) {
      kLayoutW = 1024, kLayoutH = 768;
    } else if (l && !std::strncmp(l, "800", 3)) {
      kLayoutW = 800, kLayoutH = 600;
    } else if (l && !std::strcmp(l, "desktop")) {
      // A desktop's (and a computer's browser's) default: 1280x1024 where the screen has the
      // pixels for it (its usable height in its own pixels, not points: a Retina or 150% screen
      // shows 1280x1024 sharper than 1024x768 stretched; a window's title and the menu bar or task
      // bar left out), else 1024x768.
      int dw = 0, dh = 0;
      DesktopSize(&dw, &dh);  // (SDL's video up)
      int pixels = 0;
#if defined(__EMSCRIPTEN__)
      // (SDL gives the browser's screen in CSS pixels and no scale)
      pixels = MAIN_THREAD_EM_ASM_INT({ return Math.round(screen.availHeight * (devicePixelRatio || 1)); });
#else
      const SDL_DisplayID display = SDL_GetPrimaryDisplay();
      SDL_Rect u{};
      if (SDL_GetDisplayUsableBounds(display, &u)) {
        // (Windows and Linux say the desktop's zoom as the content scale, a Mac its Retina as the
        // mode's pixel density)
        float scale = std::max(1.0f, SDL_GetDisplayContentScale(display));
        if (const SDL_DisplayMode* m = SDL_GetDesktopDisplayMode(display)) scale = std::max(scale, m->pixel_density);
        pixels = int(float(u.h) * scale + 0.5f);
      }
#endif
      const bool room = pixels <= 0 || pixels >= 1024;
      if (!room) kLayoutW = 1024, kLayoutH = 768;
      WarnF("h4: layout %dx%d (the screen's usable height %d pixels)\n", kLayoutW, kLayoutH, pixels);
    }
    const char* e = Env("RESOLUTION");
    if (!e) return r;
    if (!std::strcmp(e, "wide")) {
      WideSize(&r);
    } else if (std::sscanf(e, "%dx%d", &r.w, &r.h) != 2) {
      r.w = r.h = 0;
    }
    r.wide = r.w > kLayoutW && r.h >= kLayoutH;
    WarnF("h4: screen %dx%d%s%dx%d%s\n", r.w, r.h, r.wide ? " (wide: " : "", r.wide ? kLayoutW : 0,
          r.wide ? kLayoutH : 0, r.wide ? " interface centred)" : "");
    return r;
  }();
  return s;
}
const Screen& TheScreen() { return MutableScreen(); }

// "wide" only: the layout switched to `width`'s (800, 1024 or 1280, the Options dialog's choice).
bool SwitchLayout(int width) {
  const char* e = Env("RESOLUTION");
  if (!e || std::strcmp(e, "wide") || (width != 800 && width != 1024 && width != 1280) || width == kLayoutW)
    return false;
  kLayoutW = width;
  kLayoutH = width == 800 ? 600 : width == 1024 ? 768 : 1024;
  Screen& s = MutableScreen();
  WideSize(&s);
  s.wide = s.w > kLayoutW && s.h >= kLayoutH;
  WarnF("h4: layout %dx%d, screen %dx%d\n", kLayoutW, kLayoutH, s.w, s.h);
#if defined(H4_LAYOUT_CHOICE)
  h4::SaveLayout(width);
#endif
  return true;
}

// Wide layout in effect: a wide size set and the game in it (a mode change
// may still be under way, or the game in another size).
bool Wide() {
  const Screen& s = TheScreen();
  return s.wide && int32_t(Rd32(kScreenW)) == s.w;
}

// The layout rectangle in client coordinates.
void LayoutRect(int* l, int* t) {
  const Screen& s = TheScreen();
  *l = (s.w - kLayoutW) / 2;
  *t = (s.h - kLayoutH) / 2;
}

// A guest RECT for calls into the game.
gaddr ScratchRect() {
  static gaddr rect = 0;
  if (!rect) {
    GuestAllocator& vm = g_proc->vm;
    rect = vm.Reserve(0, 0x1000);
    vm.Commit(rect, 0x1000, Prot::RW);
  }
  return rect;
}

// Window fields: rectangle in screen coordinates, children (circular list
// of {next, prev, window}), visibility.
constexpr uint32_t kRect = 0x90, kChildren = 0x34, kVisible = 0xA0;

struct Rect {
  int l, t, r, b;
};
Rect RectOf(uint32_t w) {
  return {int32_t(Rd32(w + kRect)), int32_t(Rd32(w + kRect + 4)), int32_t(Rd32(w + kRect + 8)),
          int32_t(Rd32(w + kRect + 12))};
}
void SetRectRaw(uint32_t w, const Rect& r) {
  Wr32(w + kRect, uint32_t(r.l));
  Wr32(w + kRect + 4, uint32_t(r.t));
  Wr32(w + kRect + 8, uint32_t(r.r));
  Wr32(w + kRect + 12, uint32_t(r.b));
}

template <typename F>
void ForChildren(uint32_t w, F f) {
  const uint32_t head = Rd32(w + kChildren);
  if (!head) return;
  int i = 0;
  for (uint32_t node = Rd32(head); node != head && i < 1000; node = Rd32(node), i++)
    f(Rd32(node + 8));
}

// The windows' vtables (their RTTI: t_adventure_frame, t_adventure_map_window).
// Compared on every main loop pass, so by address.
constexpr uint32_t kAdventureFrameVtable = 0x979720, kAdventureMapVtable = 0x979BC0;
bool IsAdventureFrame(uint32_t w) { return Rd32(w) == kAdventureFrameVtable; }
bool IsAdventureMap(uint32_t w) { return Rd32(w) == kAdventureMapVtable; }

// The layout rectangle in screen coordinates (the client origin is 0,0 in
// full screen).
Rect Layout() {
  int l, t;
  LayoutRect(&l, &t);
  return {l, t, l + kLayoutW, t + kLayoutH};
}

// Moves a child of `parent` to `r` (screen coordinates), its own children
// along, through the game's window::move (which also tells a window that
// changed size).
void MoveWindow(::recomp::Ctx& c, uint32_t w, const Rect& r) {
  const uint32_t parent = Rd32(w + 0x7C);
  const Rect p = parent ? RectOf(parent) : Rect{0, 0, 0, 0};
  const gaddr rect = ScratchRect();
  Wr32(rect, uint32_t(r.l - p.l));
  Wr32(rect + 4, uint32_t(r.t - p.t));
  Wr32(rect + 8, uint32_t(r.r - p.l));
  Wr32(rect + 12, uint32_t(r.b - p.t));
  const uint32_t ecx = c.ecx, eax = c.eax;
  c.ecx = w;
  CallGuest(c, kSetRect, {rect, 1});
  c.ecx = ecx;
  c.eax = eax;
}

// The adventure frame shown in the top window, or 0.
uint32_t AdventureFrame(uint32_t top) {
  uint32_t frame = 0;
  ForChildren(top, [&](uint32_t w) {
    if (!frame && GPtr(w + kVisible)[0] && IsAdventureFrame(w)) frame = w;
  });
  return frame;
}

// Repaints a window's w x h area.
void InvalidateWhole(::recomp::Ctx& c, uint32_t window, int w, int h) {
  const gaddr rect = ScratchRect();
  Wr32(rect, 0);
  Wr32(rect + 4, 0);
  Wr32(rect + 8, uint32_t(w));
  Wr32(rect + 12, uint32_t(h));
  const uint32_t ecx = c.ecx, eax = c.eax;
  c.ecx = window;
  CallGuest(c, kInvalidate, {rect});
  c.ecx = ecx;
  c.eax = eax;
}

uint32_t g_widened = 0;  // the adventure frame laid out across the screen
std::set<uint32_t> g_tiled;  // horizontal frame layers extended through their middle
std::map<uint32_t, Rect> g_layout_rects;  // the widened frame's children, as laid out
int g_refresh_map = 0;       // main loop passes until the map is repainted whole

// Moves the adventure map window. Its own resize (vtable +0x44) only acts
// on a size that differs from its rectangle, and window::move calls it after
// changing that, so it is called first; it also renders only the area it did
// not have before, so an empty size first (which drops the view) makes all
// of it new.
void MoveMapWindow(::recomp::Ctx& c, uint32_t w, const Rect& r) {
  const gaddr size = ScratchRect() + 0x20;
  const uint32_t ecx = c.ecx, eax = c.eax;
  for (int pass = 0; pass < 2; pass++) {
    Wr32(size, 0);
    Wr32(size + 4, 0);
    Wr32(size + 8, pass ? uint32_t(r.r - r.l) : 0);
    Wr32(size + 12, pass ? uint32_t(r.b - r.t) : 0);
    c.ecx = w;
    CallGuest(c, kMapResize, {size});
  }
  c.ecx = ecx;
  c.eax = eax;
  MoveWindow(c, w, r);
}

// Puts the widened frame's children back where its 1280 layout had them
// (before the game lays the frame out again), the frame and the top window
// at the layout rectangle.
void Unwiden(::recomp::Ctx& c, uint32_t frame) {
  const uint32_t top = Rd32(kTopWindow);
  const Rect lay = Layout();
  if (top) SetRectRaw(top, lay);
  SetRectRaw(frame, lay);
  ForChildren(frame, [&](uint32_t w) {
    auto it = g_layout_rects.find(w);
    if (it == g_layout_rects.end()) return;
    if (IsAdventureMap(w))
      MoveMapWindow(c, w, it->second);
    else
      MoveWindow(c, w, it->second);
  });
  g_layout_rects.clear();
  g_tiled.clear();
  g_widened = 0;
}

void WideAdventure(::recomp::Ctx& c) {
  const Screen& s = TheScreen();
  const uint32_t top = Rd32(kTopWindow);
  if (!Wide() || !top) return;
  const Rect lay = Layout();
  const uint32_t frame = AdventureFrame(top);
  if (!frame) {
    // Centred screens (main menu, or combat over the hidden map, which
    // stays widened for when it is shown again): the top window spans the
    // screen all the same, so that the game's cursor (its child) shows
    // over the wings. A move of its origin shifts what the offscreen holds:
    // all of it is painted again.
    const Rect full{0, lay.t, s.w, lay.b};
    if (RectOf(top).l != full.l || RectOf(top).r != full.r) {
      SetRectRaw(top, full);
      InvalidateWhole(c, top, s.w, lay.b - lay.t);
    }
    return;
  }
  const Rect full{0, lay.t, s.w, lay.b};
  if (frame == g_widened) {
    // Laid out wide already. The top window may have gone back to the
    // layout rectangle meanwhile (a screen over the map, combat, WM_SIZE
    // when the game is reactivated): it would clip the frame's sides.
    if (RectOf(top).l != full.l || RectOf(top).r != full.r) {
      SetRectRaw(top, full);
      g_refresh_map = 1;
      InvalidateWhole(c, frame, s.w, lay.b - lay.t);
    }
    g_widened = frame;
    // The map window paints from a cache built for its old size: once it
    // has settled, have it repaint whole.
    if (g_refresh_map > 0 && --g_refresh_map == 0) {
      ForChildren(frame, [&](uint32_t w) {
        if (!IsAdventureMap(w)) return;
        const Rect r = RectOf(w);
        const gaddr rect = ScratchRect();
        Wr32(rect, 0);
        Wr32(rect + 4, 0);
        Wr32(rect + 8, uint32_t(r.r - r.l));
        Wr32(rect + 12, uint32_t(r.b - r.t));
        const uint32_t ecx = c.ecx, eax = c.eax;
        c.ecx = w;
        CallGuest(c, kInvalidate, {rect});
        c.ecx = ecx;
        c.eax = eax;
      });
    }
    return;
  }
  g_tiled.clear();
  const Rect f = RectOf(frame);
  if (f.r - f.l != kLayoutW) return;  // not laid out yet, or ours already
  const int dx = lay.l;               // how far each side grows
  SetRectRaw(top, full);
  SetRectRaw(frame, full);
  g_layout_rects.clear();
  ForChildren(frame, [&](uint32_t w) {
    const Rect r = RectOf(w);
    g_layout_rects[w] = r;
    const int rl = r.l - f.l, rr = r.r - f.l;  // in the layout
    const int rt = r.t - f.t, rb = r.b - f.t;
    // The transparent dragon borders are separate from the thin Top/Bottom
    // strips in layers.adventure.<width>. Both must reach the shifted right edge.
    const FrameParts& fp = Frame();
    bool strip = false;
    for (const auto& st : fp.strips) strip |= rl == st.l && rt == st.t && rb == st.b && rr <= fp.panel + 60;
    static const bool dump = Env("DUMP_WINDOWS") != nullptr;
    if (dump)
      WarnF("h4: widen %08X vt %08X %d,%d-%d,%d vis %u\n", w, Rd32(w), rl, r.t, rr, r.b,
            GPtr(w + kVisible)[0]);
    if (IsAdventureMap(w)) {
      // Its own resize (vtable +0x44) only acts on a size that differs from
      // its rectangle, and window::move calls it after changing that: call
      // it first.
      MoveMapWindow(c, w, {r.l - dx, fp.map_top ? std::max(r.t, f.t + fp.map_top) : r.t, r.r + dx,
                           fp.map_bottom ? std::min(r.b, f.t + fp.map_bottom) : r.b});
    } else if (rl >= fp.panel) {  // the map's right edge, the panel
      MoveWindow(c, w, {r.l + dx, r.t, r.r + dx, r.b});
    } else if (rr <= fp.left) {  // the left edge
      MoveWindow(c, w, {r.l - dx, r.t, r.r - dx, r.b});
    } else if (strip) {
      g_tiled.insert(w);
      MoveWindow(c, w, {r.l - dx, r.t, r.r + dx, r.b});
    } else if (fp.inner_left) {  // the ornaments by the left corners
      MoveWindow(c, w, {r.l - dx, r.t, r.r - dx, r.b});
    } else {  // 800's resource bar, by the map's right edge
      MoveWindow(c, w, {r.l + dx, r.t, r.r + dx, r.b});
    }
  });
  g_refresh_map = 3;
  g_widened = frame;
  InvalidateWhole(c, frame, s.w, lay.b - lay.t);
}

// Runs a screen constructor with the top window, and its children wider
// than the layout (the widened adventure frame), at the layout rectangle:
// the constructors pick their art by their parent's width (800, 1024 or
// 1280) and use an unset layout for any other (the town screen crashed).
int g_narrowed = 0;  // screen constructors running narrowed (KeepWide waits)
int g_presents = 0;  // the game's presents so far
uint32_t g_building_town = 0, g_building_town_window = 0;  // a town screen being made
bool g_wings_primary = false;  // the wings must go onto the primary surface again

// A constructor that shows something meanwhile (an event's message as a
// loaded game opens a town) has it drawn with the top window narrowed, its
// origin the layout's: the offscreen then holds it shifted by the layout's
// left edge, and only changes are repainted after. Then the whole screen is
// painted again (the wings too) once the windows are wide again.
void Narrowed(::recomp::Ctx& c, void (*lifted)(::recomp::Ctx&)) {
  const uint32_t top = Rd32(kTopWindow);
  std::vector<std::pair<uint32_t, Rect>> saved;
  if (Wide() && top) {
    auto narrow = [&](uint32_t w) {
      const Rect r = RectOf(w);
      if (r.r - r.l <= kLayoutW) return;
      saved.push_back({w, r});
      SetRectRaw(w, Layout());
    };
    narrow(top);
    ForChildren(top, narrow);
  }
  const int presents = g_presents;
  g_narrowed++;
  lifted(c);
  g_narrowed--;
  for (const auto& [w, r] : saved) SetRectRaw(w, r);
  if (!saved.empty() && g_presents != presents && !g_narrowed) {
    const Rect t = RectOf(top);
    const uint32_t eax = c.eax;
    InvalidateWhole(c, top, t.r - t.l, t.b - t.t);
    c.eax = eax;
    g_wings_primary = true;
    WarnF("h4: shown while narrowed: the screen painted again\n");
  }
}

// ---- wings: the side areas left by the centred 1280 screens (main menu,
// towns, combat) show two panels (H4_WINGS_DIR/paradise.jpg left, hell.jpg
// right: any size, scaled to the side's width and the layout's height), drawn
// straight onto the primary surface and the game's offscreen, which it never paints
// outside its windows.

constexpr uint32_t kCombatWindowVtable = 0x984388;  // t_combat_window
constexpr uint32_t kTownWindowVtable = 0x9966E4;    // t_town_window
constexpr uint32_t kCursorWindowVtable = 0x98D174;  // the game's cursor, a window under the top one
constexpr uint32_t kPrimarySurface = 0xAD2548;      // IDirectDrawSurface7*
constexpr uint32_t kBackSurface = 0xAD254C;         // the offscreen the game composes in
constexpr uint32_t kLockSlot = 0x64, kUnlockSlot = 0x80;  // its vtable

struct Image {
  int w = 0, h = 0;
  std::vector<uint8_t> rgb;  // top-down
};

// A JPEG, decoded by SDL's own stb_image through its MJPG pixel format, which
// wants the size first: from the frame header.
bool LoadJpg(const std::string& path, Image* out) {
  size_t n = 0;
  auto* d = static_cast<uint8_t*>(SDL_LoadFile(path.c_str(), &n));
  if (!d) {
    WarnF("h4: wings: %s cannot be read: %s\n", path.c_str(), SDL_GetError());
    return false;
  }
  int w = 0, h = 0;
  if (n > 4 && d[0] == 0xFF && d[1] == 0xD8) {
    for (size_t at = 2; at + 9 < n && d[at] == 0xFF;) {
      const uint8_t m = d[at + 1];
      if (m >= 0xC0 && m <= 0xCF && m != 0xC4 && m != 0xC8 && m != 0xCC) {
        h = d[at + 5] << 8 | d[at + 6];
        w = d[at + 7] << 8 | d[at + 8];
        break;
      }
      at += 2 + size_t(d[at + 2] << 8 | d[at + 3]);
    }
  }
  bool ok = false;
  if (w > 0 && h > 0) {
    out->w = w;
    out->h = h;
    out->rgb.resize(size_t(w) * h * 3);
    ok = SDL_ConvertPixels(w, h, SDL_PIXELFORMAT_MJPG, d, int(n), SDL_PIXELFORMAT_RGB24,
                           out->rgb.data(), w * 3);
  }
  if (!ok)
    WarnF("h4: wings: %s (%zu bytes, %dx%d) not decoded: %s\n", path.c_str(), n, w, h,
          w > 0 ? SDL_GetError() : "no JPEG frame header");
  SDL_free(d);
  return ok;
}

// `src` scaled to w x h (box filter over each output pixel's source area),
// as RGB.
std::vector<uint8_t> Scale(const Image& src, int w, int h) {
  std::vector<uint8_t> out(size_t(w) * h * 3);
  for (int y = 0; y < h; y++) {
    const int y0 = y * src.h / h, y1 = std::max(y0 + 1, (y + 1) * src.h / h);
    for (int x = 0; x < w; x++) {
      const int x0 = x * src.w / w, x1 = std::max(x0 + 1, (x + 1) * src.w / w);
      uint32_t sum[3] = {0, 0, 0}, cnt = 0;
      for (int sy = y0; sy < y1; sy++)
        for (int sx = x0; sx < x1; sx++) {
          const uint8_t* p = &src.rgb[(size_t(sy) * src.w + sx) * 3];
          sum[0] += p[0];
          sum[1] += p[1];
          sum[2] += p[2];
          cnt++;
        }
      uint8_t* o = &out[(size_t(y) * w + x) * 3];
      for (int k = 0; k < 3; k++) o[k] = uint8_t(sum[k] / cnt);
    }
  }
  return out;
}

struct Wings {
  bool tried = false, ok = false;
  int w = 0, h = 0;
  std::vector<uint8_t> left, right;  // RGB, w x h
};

// The wings at their own resolution, drawn over the frame at the window's
// wherever it still shows them as PaintWingsOn draws them (RGB565).
void ShowWingsSharp(const Wings& wings, const Image& l, const Image& r) {
  const Screen& s = TheScreen();
  const Rect lay = Layout();
  std::vector<::recomp::gfx::PresentOverlay> ov(2);
  for (int side = 0; side < 2; side++) {
    const Image& img = side ? r : l;
    const std::vector<uint8_t>& small = side ? wings.right : wings.left;
    ::recomp::gfx::PresentOverlay& o = ov[size_t(side)];
    o.x = side ? s.w - wings.w : 0;
    o.y = lay.t;
    o.w = wings.w;
    o.h = wings.h;
    o.iw = img.w;
    o.ih = img.h;
    o.image.resize(size_t(img.w) * img.h);
    for (size_t i = 0; i < o.image.size(); i++)
      o.image[i] = 0xFF000000u | uint32_t(img.rgb[i * 3]) << 16 |
                   uint32_t(img.rgb[i * 3 + 1]) << 8 | img.rgb[i * 3 + 2];
    o.ref.resize(size_t(wings.w) * wings.h);
    for (size_t i = 0; i < o.ref.size(); i++) {
      const uint8_t* p = &small[i * 3];
      const uint16_t v = uint16_t((p[0] >> 3) << 11 | (p[1] >> 2) << 5 | (p[2] >> 3));
      o.ref[i] = ::recomp::gfx::DecodePixel(reinterpret_cast<const uint8_t*>(&v),
                                            ::recomp::gfx::Format::R5G6B5);
    }
  }
  ::recomp::gfx::SetPresentOverlays(std::move(ov));
}

// The wings shown: H4_WINGS_DIR/<name>.jpg, the right one mirrored when
// `mirror_right` (faction wings face the screen). The main menu and other
// centred screens: paradise and hell.
std::string g_wing_left = "paradise", g_wing_right = "hell";
bool g_mirror_right = false;

void Mirror(Image* im) {
  for (int y = 0; y < im->h; y++)
    for (int x = 0; x < im->w / 2; x++)
      for (int k = 0; k < 3; k++)
        std::swap(im->rgb[(size_t(y) * im->w + x) * 3 + k],
                  im->rgb[(size_t(y) * im->w + (im->w - 1 - x)) * 3 + k]);
}

bool BlackSidePanels() {
  static const bool enabled = [] {
    const char* mode = Env("SIDE_PANELS");
    if (mode && *mode) return std::strcmp(mode, "black") == 0;
    const char* legacy = Env("BLACK_SIDE_PANELS");
    if (legacy && std::strcmp(legacy, "1") == 0) return true;
    const char* config = Env("SIDE_PANELS_FILE");
    const std::string path = config ? config : ::recomp::DataDir() + "/side-panels.txt";
    return h4::ReadBlackSidePanels(path.c_str());
  }();
  return enabled;
}

const Wings& TheWings(int w, int h) {
  static Wings wings;
  static std::string shown;  // the pair wings holds
  const std::string want = g_wing_left + "|" + g_wing_right + (g_mirror_right ? "|m" : "");
  if (shown == want && wings.tried && wings.w == w && wings.h == h) return wings;
  shown = want;
  wings = {};
  wings.tried = true;
  wings.w = w;
  wings.h = h;
  const char* dir = Env("WINGS_DIR");
  // Decoded once each: a town or battle switching collages would otherwise
  // decode two JPEGs as it opens.
  static std::map<std::string, Image> decoded;
  auto load = [&](const std::string& name, Image* out) {
    auto it = decoded.find(name);
    if (it == decoded.end()) {
      Image im;
      if (!LoadJpg(std::string(dir) + "/" + name + ".jpg", &im)) return false;
      it = decoded.emplace(name, std::move(im)).first;
    }
    *out = it->second;
    return true;
  };
  Image l, r;
  if (dir && load(g_wing_left, &l) && load(g_wing_right, &r) && w > 0 && h > 0) {
    if (g_mirror_right) Mirror(&r);
    wings.w = w;
    wings.h = h;
    wings.left = Scale(l, w, h);
    wings.right = Scale(r, w, h);
    wings.ok = true;
    ShowWingsSharp(wings, l, r);
  } else if (dir) {
    WarnF("h4: no wings %s, %s in %s\n", g_wing_left.c_str(), g_wing_right.c_str(), dir);
  }
  return wings;
}

// Draws the side panels onto a surface (locked through its own vtable).
// (ox, oy): where the surface's origin is on the screen; `hole`: a screen
// area left alone, or where `black_in_hole`, painted only where black.
bool PaintWingsOn(::recomp::Ctx& c, uint32_t prim, int ox, int oy, const Rect& hole,
                  bool black_in_hole = false);

// A surface the game has locked: a Lock and Unlock of ours in between would
// end its lock too, and what it then draws there would never be shown.
bool SurfaceBusy(uint32_t surf) {
  const auto* s = ::recomp::com::As<::recomp::ddraw::Surface>(surf);
  return !s || s->locked;
}

// The wings go into the game's offscreen, which its present and its cursor
// restore copy to the screen (its origin is the top window's), and with
// `primary` straight onto the primary surface: the sides of a centred
// screen lie outside the top window, which the game never presents.
// False when the primary surface could not be painted (locked by the game).
bool PaintWings(::recomp::Ctx& c, bool primary) {
  const uint32_t eax = c.eax, ecx = c.ecx, edx = c.edx;
  const uint32_t top = Rd32(kTopWindow);
  bool done = true;
  if (top) PaintWingsOn(c, Rd32(kBackSurface), RectOf(top).l, RectOf(top).t, Rect{});
  if (primary && top) {
    // All of it (a hole round the cursor kept whatever was there, the
    // adventure panel say), then the cursor drawn again over it.
    done = PaintWingsOn(c, Rd32(kPrimarySurface), 0, 0, Rect{});
    ForChildren(top, [&](uint32_t w) {
      if (Rd32(w) != kCursorWindowVtable) return;
      const Rect r = RectOf(w);
      InvalidateWhole(c, w, r.r - r.l, r.b - r.t);
    });
  }
  c.eax = eax;
  c.ecx = ecx;
  c.edx = edx;
  return done;
}

bool PaintWingsOn(::recomp::Ctx& c, uint32_t prim, int ox, int oy, const Rect& hole,
                  bool black_in_hole) {
  const Screen& s = TheScreen();
  const Rect lay = Layout();
  if (!prim) return true;
  const Wings* wings = BlackSidePanels() ? nullptr : &TheWings(lay.l, lay.b - lay.t);
  if (wings && !wings->ok) return true;
  const int width = wings ? wings->w : lay.l;
  const int height = wings ? wings->h : lay.b - lay.t;
  if (SurfaceBusy(prim)) return false;
  const uint32_t vt = Rd32(prim);
  const gaddr desc = ScratchRect() + 0x100;
  std::memset(GPtr(desc), 0, 124);
  Wr32(desc, 124);
  const uint32_t ecx = c.ecx, eax = c.eax;
  const uint32_t hr = uint32_t(CallGuest(c, Rd32(vt + kLockSlot), {prim, 0, desc, 1, 0}));
  if (hr == 0) {
    uint8_t* bits = GPtr(Rd32(desc + 36));
    const int32_t pitch = int32_t(Rd32(desc + 16));
    const int bpp = int(Rd32(desc + 72 + 12));
    const int sw = int(Rd32(desc + 12)), sh = int(Rd32(desc + 8));
    constexpr uint8_t black[3] = {};
    for (int side = 0; side < 2; side++) {
      const uint8_t* img = wings ? (side ? wings->right : wings->left).data() : nullptr;
      const int x0 = side ? s.w - width : 0;
      for (int y = 0; y < height; y++) {
        const int sy = lay.t + y - oy;
        if (sy < 0 || sy >= sh) continue;
        uint8_t* row = bits + int64_t(sy) * pitch;
        const bool in_hole_rows = lay.t + y >= hole.t && lay.t + y < hole.b;
        for (int x = 0; x < width; x++) {
          const int sx = x0 + x - ox;
          if (sx < 0 || sx >= sw) continue;
          const uint8_t* p = img ? img + (size_t(y) * width + x) * 3 : black;
          if (in_hole_rows && x0 + x >= hole.l && x0 + x < hole.r) {
            if (!black_in_hole) continue;
            uint32_t v = 0;
            std::memcpy(&v, row + sx * (bpp / 8), size_t(bpp / 8));
            if (v) continue;
          }
          if (bpp == 16) {
            const uint16_t v = uint16_t((p[0] >> 3) << 11 | (p[1] >> 2) << 5 | (p[2] >> 3));
            std::memcpy(row + sx * 2, &v, 2);
          } else if (bpp == 32) {
            const uint32_t v = uint32_t(p[0]) << 16 | uint32_t(p[1]) << 8 | p[2];
            std::memcpy(row + sx * 4, &v, 4);
          }
        }
      }
    }
    CallGuest(c, Rd32(vt + kUnlockSlot), {prim, 0});
  }
  c.ecx = ecx;
  c.eax = eax;
  return true;
}

// Dev aid: H4_DUMP_WINDOWS=1 logs the window tree (vtable, rectangle,
// visibility) whenever the wings come or go.
void DumpTree(uint32_t w, int depth) {
  WarnF("h4: %*s%08X vt %08X rect %d,%d-%d,%d vis %u\n", depth * 2, "", w, Rd32(w),
        int(Rd32(w + 0x90)), int(Rd32(w + 0x94)), int(Rd32(w + 0x98)), int(Rd32(w + 0x9C)),
        GPtr(w + kVisible)[0]);
  if (depth >= 1) return;
  ForChildren(w, [&](uint32_t ch) { DumpTree(ch, depth + 1); });
}

bool g_wings = false;          // the wings are wanted

// A centred full screen shown over the adventure map: a battle or a town
// (by class), or any visible 1280 x 1024 window, right under the top window
// or inside the adventure frame (the town opens there). 0 if none.
uint32_t CentredScreen(uint32_t top) {
  uint32_t found = 0;
  auto check = [&](uint32_t w) {
    if (found || !GPtr(w + kVisible)[0] || IsAdventureFrame(w)) return;
    const uint32_t vt = Rd32(w);
    const Rect r = RectOf(w);
    if (vt == kCombatWindowVtable || vt == kTownWindowVtable ||
        (r.r - r.l == kLayoutW && r.b - r.t == kLayoutH))
      found = w;
  };
  ForChildren(top, check);
  if (const uint32_t frame = AdventureFrame(top)) ForChildren(frame, check);
  return found;
}

// ---- faction wings: a battle shows its armies' collages (tools/re/collage),
// the attacker's on the left and the defender's on the right (mirrored to face
// the field); a town its faction's on both sides. A light screen gets the dark
// collage, a dark one the light collage (life has only a light one, death only
// a dark one). A side with no faction (neutral creatures) gets a random one.
constexpr uint32_t kCombatBattlefield = 0x104;  // t_combat_window -> t_battlefield
constexpr uint32_t kCombatContext = 0x140;      // t_combat_window -> the battle's setup
constexpr uint32_t kContextArmies = 0x08;       // setup -> attacker, defender t_creature_array*
constexpr uint32_t kArraySlots = 0x10;          // t_creature_array: 7 t_creature_stack*
constexpr uint32_t kStackSlots = 7;
// A stack's alignment (its interface at +8, virtual +4): a creature's from
// its type's traits, a hero's own; anything else (an empty slot) has none.
constexpr uint32_t kCreatureStackVtable = 0x984624;  // t_creature, at +8
constexpr uint32_t kHeroStackVtable = 0x989298;      // t_hero, at +8
constexpr uint32_t kCreatureType = 0x9C;             // t_creature -> t_creature_type
constexpr uint32_t kHeroAlignment = 0xA8;            // t_hero -> t_town_type
constexpr uint32_t kCreatureTraits = 0xAB1F30, kTraitsSize = 0xE0, kTraitsAlignment = 0x44;
constexpr uint32_t kTownOfWindow = 0x6D0;  // t_town_window -> t_town
constexpr uint32_t kTownVtable = 0x995F4C;
constexpr uint32_t kTownType = 0x118;      // t_town -> t_town_type
// t_town_type: the alignments.
constexpr const char* kFactions[] = {"life", "order", "death", "chaos", "nature", "might"};
constexpr int kFactionCount = 6;

bool Mapped(uint32_t p, uint32_t n = 4) {
  return p >= 0x10000 && g_proc->mem.IsCommitted(p) && g_proc->mem.IsCommitted(p + n - 1);
}

// The faction of a stack, -1 if none.
int StackFaction(uint32_t stack) {
  if (!Mapped(stack, 0x100)) return -1;
  const uint32_t vt = Rd32(stack + 8);
  int a = -1;
  if (vt == kHeroStackVtable) {
    a = int(Rd32(stack + kHeroAlignment));
  } else if (vt == kCreatureStackVtable) {
    const uint32_t type = Rd32(stack + kCreatureType);
    const uint32_t traits = kCreatureTraits + type * kTraitsSize;
    if (type < 512 && Mapped(traits + kTraitsAlignment)) a = int(Rd32(traits + kTraitsAlignment));
  }
  return a >= 0 && a < kFactionCount ? a : -1;
}

// An army's faction: the one most of its stacks (the hero too) have, on a tie
// the first in it; -1 if none has one.
int ArmyFaction(uint32_t army) {
  if (!Mapped(army + kArraySlots, kStackSlots * 4)) return -1;
  int count[kFactionCount] = {}, first[kFactionCount];
  std::fill(first, first + kFactionCount, int(kStackSlots));
  for (uint32_t i = 0; i < kStackSlots; i++) {
    const int f = StackFaction(Rd32(army + kArraySlots + i * 4));
    if (f < 0) continue;
    count[f]++;
    first[f] = std::min(first[f], int(i));
  }
  int best = -1;
  for (int f = 0; f < kFactionCount; f++)
    if (count[f] && (best < 0 || count[f] > count[best] ||
                     (count[f] == count[best] && first[f] < first[best])))
      best = f;
  return best;
}

// A town's faction, -1 if it cannot be read.
int FactionOfTown(uint32_t town) {
  if (!Mapped(town, kTownType + 4) || Rd32(town) != kTownVtable) return -1;
  const int t = int(Rd32(town + kTownType));
  return t >= 0 && t < kFactionCount ? t : -1;
}
int TownFaction(uint32_t town_window) { return FactionOfTown(Rd32(town_window + kTownOfWindow)); }

bool WingFile(const std::string& name) {
  const char* dir = Env("WINGS_DIR");
  if (!dir) return false;
  FILE* f = std::fopen((std::string(dir) + "/" + name + ".jpg").c_str(), "rb");
  if (f) std::fclose(f);
  return f != nullptr;
}

// The collage for a faction on a dark (light collage) or light screen; a
// random faction for -1.
std::string FactionWing(int faction, bool dark_screen) {
  static std::mt19937 rng(std::random_device{}());
  if (faction < 0) faction = int(rng() % kFactionCount);
  const std::string base = kFactions[faction];
  const std::string want = base + (dark_screen ? "_light" : "_dark");
  const std::string other = base + (dark_screen ? "_dark" : "_light");
  return WingFile(want) || !WingFile(other) ? want : other;
}

// The screen's mean luma (0..255) on the primary surface, sampled over the
// layout's left four fifths (in combat the right one is the panel); -1
// when it cannot be read.
int FieldLuma(::recomp::Ctx& c) {
  const uint32_t prim = Rd32(kPrimarySurface);
  if (!prim || SurfaceBusy(prim)) return -1;
  const Rect lay = Layout();
  const uint32_t vt = Rd32(prim);
  const gaddr desc = ScratchRect() + 0x100;
  std::memset(GPtr(desc), 0, 124);
  Wr32(desc, 124);
  const uint32_t ecx = c.ecx, eax = c.eax, edx = c.edx;
  int luma = -1;
  if (uint32_t(CallGuest(c, Rd32(vt + kLockSlot), {prim, 0, desc, 1, 0})) == 0) {
    const uint8_t* bits = GPtr(Rd32(desc + 36));
    const int32_t pitch = int32_t(Rd32(desc + 16));
    const int bpp = int(Rd32(desc + 72 + 12));
    const int sw = int(Rd32(desc + 12)), sh = int(Rd32(desc + 8));
    uint64_t sum = 0, n = 0;
    const int x1 = std::min(sw, lay.l + (lay.r - lay.l) * 4 / 5);
    for (int y = std::max(0, lay.t); y < std::min(sh, lay.b); y += 7)
      for (int x = std::max(0, lay.l); x < x1; x += 7) {
        int r, g, b;
        if (bpp == 16) {
          uint16_t v;
          std::memcpy(&v, bits + int64_t(y) * pitch + x * 2, 2);
          r = (v >> 11) << 3, g = ((v >> 5) & 63) << 2, b = (v & 31) << 3;
        } else {
          const uint8_t* q = bits + int64_t(y) * pitch + x * (bpp / 8);
          b = q[0], g = q[1], r = q[2];
        }
        sum += uint64_t(r * 77 + g * 150 + b * 29) >> 8;
        n++;
      }
    luma = n ? int(sum / n) : -1;
    CallGuest(c, Rd32(vt + kUnlockSlot), {prim, 0});
  }
  c.ecx = ecx;
  c.eax = eax;
  c.edx = edx;
  return luma;
}

uint32_t g_wings_for = 0;  // the combat or town window the wings were chosen for

// Once per battle or town, when it is on screen: the wings for its armies or
// its faction. Back to the default pair elsewhere.
void ChooseWings(::recomp::Ctx& c) {
  const uint32_t top = Rd32(kTopWindow);
  uint32_t combat = 0, town = 0;
  if (Wide() && top) {
    ForChildren(top, [&](uint32_t w) {
      if (Rd32(w) == kCombatWindowVtable && GPtr(w + kVisible)[0]) combat = w;
    });
    if (!combat)
      if (const uint32_t w = CentredScreen(top); w && Rd32(w) == kTownWindowVtable) town = w;
  }
  const uint32_t screen = combat ? combat : town;
  std::string left = "paradise", right = "hell";
  bool mirror = false;
  if (!screen) {
    g_wings_for = 0;
  } else if (screen == g_wings_for) {
    return;
  } else {
    if (combat && !Rd32(combat + kCombatBattlefield)) return;
    const int luma = FieldLuma(c);
    if (luma < 12) return;  // not drawn yet, or the screen locked: next pass
    const bool dark = luma < 110;
    int lf, rf;
    if (combat) {
      const uint32_t setup = Rd32(combat + kCombatContext);
      const bool ok = Mapped(setup + kContextArmies, 8);
      lf = ok ? ArmyFaction(Rd32(setup + kContextArmies)) : -1;
      rf = ok ? ArmyFaction(Rd32(setup + kContextArmies + 4)) : -1;
    } else {
      lf = rf = TownFaction(town);
    }
    left = FactionWing(lf, dark);
    right = lf == rf && lf >= 0 ? left : FactionWing(rf, dark);
    mirror = true;
    g_wings_for = screen;
    WarnF("h4: %s wings: luma %d, factions %d, %d: %s, %s\n", combat ? "battle" : "town", luma,
          lf, rf, left.c_str(), right.c_str());
  }
  if (left == g_wing_left && right == g_wing_right && mirror == g_mirror_right) return;
  g_wing_left = left;
  g_wing_right = right;
  g_mirror_right = mirror;
  g_wings_primary = true;  // painted again over the old ones
}

// On the centred screens (the main menu, combat: no adventure frame shown;
// a town or combat window over it), wide: the wings. The paint pass keeps
// them in the offscreen; onto the primary surface (and the offscreen) only
// when they appear or the screen was redone: painting them there over and
// over made the cursor over them blink.
void SideWings(::recomp::Ctx& c) {
  if (!BlackSidePanels()) ChooseWings(c);
  static uint32_t last_primary = 0;
  const uint32_t top = Rd32(kTopWindow);
  const bool on = Wide() && top && (!AdventureFrame(top) || CentredScreen(top));
  const bool was = g_wings;
  g_wings = on;
  if (was != on) WarnF("h4: side wings %s\n", on ? "on" : "off");
  // Clicks on the wings go nowhere (the pointer moves over them freely).
  if (was != on) {
    const Rect lay = Layout();
    SetPointerBounds(on, lay.l, lay.t, lay.r, lay.b);
  }
  static const bool dump = Env("DUMP_WINDOWS") != nullptr;
  if (was != on && dump && top) DumpTree(top, 0);
  if (!on) {
    // Whatever replaces the wings (the adventure frame back under a closed
    // town or battle) repaints only what the game thinks changed, which is
    // not the sides it had there all along: repaint the whole screen.
    if (was && top) {
      const Rect r = RectOf(top);
      InvalidateWhole(c, top, r.r - r.l, r.b - r.t);
    }
    return;
  }
  const uint32_t primary = Rd32(kPrimarySurface);
  if (!was || primary != last_primary) g_wings_primary = true;
  last_primary = primary;
  if (!g_wings_primary) return;
  if (PaintWings(c, true)) g_wings_primary = false;  // else again next pass
}

// Screens and dialogs right under the top window, placed for a top window
// at the layout rectangle but put there while it spanned the screen, sit
// exactly the layout's offset left of centre (combat: built without a
// parent and attached later, at x 0): they go back to the middle. Other
// windows off centre are logged once (H4_DUMP_WINDOWS), to find more.
void CentreScreens(::recomp::Ctx& c) {
  static std::set<uint32_t> logged;
  static const bool dump = Env("DUMP_WINDOWS") != nullptr;
  const uint32_t top = Rd32(kTopWindow);
  if (!Wide() || !top) return;
  const Rect lay = Layout();
  const int mid = TheScreen().w / 2;
  ForChildren(top, [&](uint32_t w) {
    const uint32_t vt = Rd32(w);
    if (vt == kCursorWindowVtable || IsAdventureFrame(w) || !GPtr(w + kVisible)[0]) return;
    const Rect r = RectOf(w);
    const int width = r.r - r.l, centre = (r.l + r.r) / 2;
    if (width >= 300 && std::abs(centre - (mid - lay.l)) <= 1) {
      WarnF("h4: window %08X (vt %08X) at x %d, %d wide: moved right by %d\n", w, vt, r.l, width,
            lay.l);
      MoveWindow(c, w, {r.l + lay.l, r.t, r.r + lay.l, r.b});
      InvalidateWhole(c, w, width, r.b - r.t);
    } else if (dump && width >= 300 && std::abs(centre - mid) > 1 && logged.insert(w).second) {
      WarnF("h4: off-centre window %08X (vt %08X) %d,%d-%d,%d\n", w, vt, r.l, r.t, r.r, r.b);
    }
  });
  // With the wings shown, popups (right-click help, creature info) kept on
  // the screen by the game would run under them: they go inside the
  // layout rectangle.
  if (!g_wings) return;
  const uint32_t screen = CentredScreen(top);
  ForChildren(top, [&](uint32_t w) {
    if (w == screen || Rd32(w) == kCursorWindowVtable || IsAdventureFrame(w) ||
        !GPtr(w + kVisible)[0])
      return;
    const Rect r = RectOf(w);
    if (r.r - r.l > kLayoutW) return;
    const int dx = r.l < lay.l ? lay.l - r.l : r.r > lay.r ? lay.r - r.r : 0;
    if (!dx) return;
    MoveWindow(c, w, {r.l + dx, r.t, r.r + dx, r.b});
    InvalidateWhole(c, w, r.r - r.l, r.b - r.t);
  });
}

// The wide layout kept up and the wings drawn, from every loop the game
// runs (after a battle it runs one that never calls the main loop pass).
// ---- the main menu on its own (the language button over it: menu_button.h)

constexpr uint32_t kMainMenuVtable = 0x98BABC;  // t_main_menu (its window base)

// The RTTI class name of a guest object (diagnostics).
std::string ClassOf(uint32_t obj) {
  const uint32_t vt = Rd32(obj);
  if (vt < 0x401000 || vt > 0xAD6000) return "?";
  const uint32_t col = Rd32(vt - 4);
  if (col < 0x401000 || col > 0xAD6000) return "?";
  const uint32_t td = Rd32(col + 12);
  return td ? GStr(td + 8) : "?";
}

// ---- the on-screen keyboard (iOS, Android): up while one of the game's text
// fields has the keyboard focus (a save's name, a hero's, the chat...), down
// when none has. t_text_edit_window (vtable 0x9959AC; the chat's
// t_custom_edit_window derives from it): +0x137 focused (set by its
// gain-focus 0x8854B0, which shows the caret, cleared by 0x885490), +0x138
// editable. Only the changes are acted on, so the three-finger tap
// (x86recomp's touch input) still shows or hides it by hand.

constexpr uint32_t kTextEditVtable = 0x9959AC;

bool IsTextEdit(uint32_t w) {
  static std::map<uint32_t, bool> known;  // by vtable
  const uint32_t vt = Rd32(w);
  const auto it = known.find(vt);
  if (it != known.end()) return it->second;
  return known[vt] = vt == kTextEditVtable || ClassOf(w).find("edit_window@") != std::string::npos;
}

// The focused text field under w (visible), or 0.
uint32_t FocusedTextEdit(uint32_t w, int depth = 0) {
  if (!GPtr(w + kVisible)[0] || depth > 12) return 0;
  if (IsTextEdit(w) && GPtr(w + 0x137)[0] && GPtr(w + 0x138)[0]) return w;
  uint32_t found = 0;
  ForChildren(w, [&](uint32_t ch) {
    if (!found) found = FocusedTextEdit(ch, depth + 1);
  });
  return found;
}

// Tells SDL where the field is (window coordinates), so the keyboard does not
// cover it: iOS moves the game's view up by what the keyboard would hide of
// it, Android pans the window to SDL's edit box placed there (adjustPan in
// the manifest). Phones and tablets only (FrameToView is the apps').
void PlaceTextInputArea(SDL_Window* win, uint32_t field) {
#if defined(__ANDROID__) || (defined(__APPLE__) && TARGET_OS_IOS)
  const Rect r = RectOf(field);
  int ww = 0, wh = 0;
  SDL_GetWindowSize(win, &ww, &wh);
  float l, t, rr, b;
  h4::FrameToView(r.l, r.t, float(ww), float(wh), &l, &t);
  h4::FrameToView(r.r, r.b, float(ww), float(wh), &rr, &b);
  if (l < 0 || rr <= l || b <= t) return;
  const SDL_Rect area{int(l), int(t), int(rr - l), int(b - t) + 8};  // a little room under it
  static SDL_Rect last{};
  if (area.x == last.x && area.y == last.y && area.w == last.w && area.h == last.h) return;
  last = area;
  SDL_SetTextInputArea(win, &area, 0);
#else
  (void)win;
  (void)field;
#endif
}

void KeepScreenKeyboard() {
  // (H4_SCREEN_KEYBOARD=1: the same where there is none, to test it on a desktop)
  static const bool screen = SDL_HasScreenKeyboardSupport() || Env("SCREEN_KEYBOARD");
  if (!screen) return;
  static uint64_t next = 0;
  const uint64_t now = SDL_GetTicks();
  if (now < next) return;
  next = now + 100;
  const uint32_t top = Rd32(kTopWindow);
  const uint32_t field = top ? FocusedTextEdit(top) : 0;
  const bool want = field != 0 || h4::RandomMapTyping();  // a game's field, or the generator's seed
  static bool was = false;
  SDL_Window* win = SDL_GetKeyboardFocus();
  if (!win) {
    int n = 0;
    SDL_Window** all = SDL_GetWindows(&n);
    if (all && n > 0) win = all[0];
    SDL_free(all);
  }
  if (!win) return;
  if (field) PlaceTextInputArea(win, field);  // again while it is up: the zoom may move it
  if (want == was) return;
  was = want;
  WarnF("h4: text field %s: keyboard %s\n", want ? "focused" : "left", want ? "up" : "down");
  if (want) SDL_StartTextInput(win);
  else SDL_StopTextInput(win);
}

// The main menu shown with nothing over it: a visible t_main_menu right under
// the top window and no other visible window there but the cursor's. The
// visible ones are logged when they change.
bool MainMenuAlone() {
  const uint32_t top = Rd32(kTopWindow);
  if (!top) return false;
  bool menu = false, other = false;
  std::string seen;
  ForChildren(top, [&](uint32_t w) {
    if (!GPtr(w + kVisible)[0]) return;
    const uint32_t vt = Rd32(w);
    if (vt == kMainMenuVtable) menu = true;
    else if (vt != kCursorWindowVtable) other = true;
    seen += ClassOf(w) + " ";
  });
  static std::string last;
  if (seen != last) {
    WarnF("h4: screens: %s\n", seen.c_str());
    last = seen;
  }
  return menu && !other;
}

// ---- the random map generator in the scenario list (random_map_ui.h)
//
// t_new_map_dialog (New Game > Scenario): +0x110 the folder it lists, an MSVC string ({allocator,
// char* +0x114, size +0x118, capacity}; its full path, ending in '\\'). 0x7B58A0 (thiscall, a
// const string&) reads a folder into the list (opening a subfolder calls it too): it changes to
// the folder, keeps its path at +0x110, lists its .h4c files and subfolders, selects the first map
// and changes back. 0x401D30 assigns (const char*, size) to such a string, 0x401CE0(1) frees it.
constexpr uint32_t kReadMapFolder = 0x7B58A0, kStringAssign = 0x401D30, kStringTidy = 0x401CE0;

// The dialog, when it is up with nothing over it. The top window's children are in their
// creation order: what is behind the dialog (the menu; from a game going on, the adventure screen
// and whatever the game keeps up under it) is on one side of it in the list, a window over it (a
// message) would be on the other. So: the dialog at either end of the visible windows' list (the
// tooltip and the cursor's window left out).
// The scenario dialog (t_new_map_dialog) when it is shown and nothing is over it: 0 otherwise.
// From the main menu it is a child of the top window; from a game (the system menu's New Game)
// one of the adventure frame's, so it is looked for under the top window's children too. Each
// window on the way must be the front (or back) one of its parent's shown children, as the
// dialog itself. (Windows' rectangles are all in the same coordinates, a child's as its parent's.)
uint32_t NewMapDialog() {
  const uint32_t top = Rd32(kTopWindow);
  if (!top) return 0;
  static uint32_t vtable = 0;   // t_new_map_dialog's, once seen
  static uint32_t balloon = 0;  // t_help_balloon's: the tooltip shown on hover, not a window over it
  auto shown_children = [&](uint32_t parent) {
    std::vector<uint32_t> shown;
    ForChildren(parent, [&](uint32_t w) {
      if (!GPtr(w + kVisible)[0]) return;
      const uint32_t vt = Rd32(w);
      if (vt == kCursorWindowVtable || (balloon && vt == balloon)) return;
      if (!balloon && ClassOf(w) == ".?AVt_help_balloon@@") {
        balloon = vt;
        return;
      }
      if (!vtable && ClassOf(w) == ".?AVt_new_map_dialog@@") vtable = vt;
      shown.push_back(w);
    });
    return shown;
  };
  auto front = [](const std::vector<uint32_t>& shown, size_t at) { return at == 0 || at + 1 == shown.size(); };
  const std::vector<uint32_t> level0 = shown_children(top);
  uint32_t found = 0;
  bool on_top = false;
  std::string now;
  for (size_t i = 0; i < level0.size() && !found; i++) {
    if (vtable && Rd32(level0[i]) == vtable) {
      found = level0[i];
      on_top = front(level0, i);
      for (uint32_t w : level0) now += ClassOf(w) + " ";
      break;
    }
    if (i + 1 != level0.size()) continue;  // (a game's dialog: under the front screen only)
    const std::vector<uint32_t> level1 = shown_children(level0[i]);
    for (size_t j = 0; j < level1.size(); j++)
      if (vtable && Rd32(level1[j]) == vtable) {
        found = level1[j];
        on_top = front(level1, j);
        now = ClassOf(level0[i]) + " > ";
        for (uint32_t w : level1) now += ClassOf(w) + " ";
        break;
      }
  }
  if (!found) return 0;
  static std::string last;  // what is up with it, when that changes (for the log)
  if (now != last) {
    std::fprintf(stderr, "h4: new map dialog %s: %s\n", on_top ? "on top" : "covered", now.c_str());
    last = now;
  }
  return on_top ? found : 0;
}

std::string MapFolder(uint32_t dialog) {
  const uint32_t text = Rd32(dialog + 0x114), size = Rd32(dialog + 0x118);
  if (!text || !size || size > 1024) return "C:\\Heroes4\\maps\\";
  std::string folder = GStr(text);
  if (folder.empty() || folder.back() != '\\') folder += '\\';
  return folder;
}

// The file name of the map selected in the dialog's list ("" none, or a folder). Its items
// (+0x124, a vector of pointers; the one selected: +0x17c) are a map's (FUN_007afa00: vtable
// 0x98D4D4, its file name a string at +8) or a folder's.
std::string SelectedMap(uint32_t dialog) {
  constexpr uint32_t kMapItemVtable = 0x98D4D4;
  const uint32_t begin = Rd32(dialog + 0x124), end = Rd32(dialog + 0x128);
  const int32_t index = int32_t(Rd32(dialog + 0x17C));
  if (!begin || end < begin || index < 0 || uint32_t(index) >= (end - begin) / 4) return {};
  const uint32_t item = Rd32(begin + uint32_t(index) * 4);
  if (!item || Rd32(item) != kMapItemVtable) return {};
  const uint32_t text = Rd32(item + 0xC), size = Rd32(item + 0x10);
  if (!text || !size || size > 260) return {};
  return std::string(reinterpret_cast<const char*>(GPtr(text)), size);
}

// The list read again from its folder, so a map just written there shows (and is selected).
void ReadMapFolder(::recomp::Ctx& c, uint32_t dialog) {
  const std::string folder = MapFolder(dialog);
  const gaddr str = ScratchRect() + 0x100, text = ScratchRect() + 0x200;
  if (folder.size() >= 0x400) return;
  std::memset(GPtr(str), 0, 16);
  std::memcpy(GPtr(text), folder.c_str(), folder.size() + 1);
  const uint32_t eax = c.eax, ecx = c.ecx, edx = c.edx, esp = c.esp;
  c.ecx = str;
  CallGuest(c, kStringAssign, {text, folder.size()});
  c.ecx = dialog;
  CallGuest(c, kReadMapFolder, {str});
  c.ecx = str;
  CallGuest(c, kStringTidy, {1});
  c.eax = eax;
  c.ecx = ecx;
  c.edx = edx;
  c.esp = esp;
  WarnF("h4: random map: the list read again (%s)\n", folder.c_str());
}

// Points the dialog's list at `folder` (its folder string, +0x110: what MapFolder reads), to be
// read with ReadMapFolder.
void SetMapFolder(::recomp::Ctx& c, uint32_t dialog, const std::string& folder) {
  const gaddr text = ScratchRect() + 0x200;
  if (folder.size() >= 0x400) return;
  std::memcpy(GPtr(text), folder.c_str(), folder.size() + 1);
  const uint32_t eax = c.eax, ecx = c.ecx, edx = c.edx, esp = c.esp;
  c.ecx = dialog + 0x110;
  CallGuest(c, kStringAssign, {text, folder.size()});
  c.eax = eax;
  c.ecx = ecx;
  c.edx = edx;
  c.esp = esp;
}

// The generated maps' folder lists them newest first (their names, rm_<date>_<time>_<seed>, sort
// oldest first): once for each list read (its items another set), the maps' items put in
// descending name order after the folders', the newest selected, and the list shown again by the
// dialog's sort (FUN_007b71a0) asked for a column it has not (7): no sorting, only the selection
// found again, scrolled to and drawn. A column header clicked after sorts as the game does.
void NewestFirst(::recomp::Ctx& c, uint32_t dialog) {
  constexpr uint32_t kMapItemVtable = 0x98D4D4, kSortList = 0x7B71A0;
  static uint64_t done = 0;
  const uint32_t begin = Rd32(dialog + 0x124), end = Rd32(dialog + 0x128);
  if (!begin || end <= begin || (end - begin) / 4 > 8192) return;
  std::vector<uint32_t> items((end - begin) / 4);
  for (size_t i = 0; i < items.size(); ++i) items[i] = Rd32(begin + uint32_t(i) * 4);
  std::vector<uint32_t> set = items;
  std::sort(set.begin(), set.end());
  uint64_t sig = 1469598103934665603ull ^ begin;
  for (uint32_t it : set) sig = (sig ^ it) * 1099511628211ull;
  if (sig == done) return;
  done = sig;
  auto name = [&](uint32_t item) {
    const uint32_t text = Rd32(item + 0xC), size = Rd32(item + 0x10);
    std::string s = text && size <= 260 ? std::string(reinterpret_cast<const char*>(GPtr(text)), size) : std::string();
    for (char& ch : s) ch = char(std::tolower(static_cast<unsigned char>(ch)));
    return s;
  };
  auto is_map = [&](uint32_t item) { return item && Rd32(item) == kMapItemVtable; };
  std::vector<uint32_t> order = items;
  const auto maps = std::stable_partition(order.begin(), order.end(), [&](uint32_t it) { return !is_map(it); });
  const size_t first_map = size_t(maps - order.begin());
  if (first_map == order.size()) return;
  std::stable_sort(maps, order.end(), [&](uint32_t a, uint32_t b) { return name(a) > name(b); });
  for (size_t i = 0; i < order.size(); ++i) Wr32(begin + uint32_t(i) * 4, order[i]);
  Wr32(dialog + 0x17C, uint32_t(first_map));
  Wr32(dialog + 0x1B4, 0);
  Wr32(dialog + 400, 0);
  const uint32_t eax = c.eax, ecx = c.ecx, edx = c.edx, esp = c.esp;
  c.ecx = dialog;
  CallGuest(c, kSortList, {0, 7});
  c.eax = eax;
  c.ecx = ecx;
  c.edx = edx;
  c.esp = esp;
}

void KeepWide(::recomp::Ctx& c) {
  // The settings button (⚙︎) in the menu's top right corner, next to the wings; the ☁️ one (when the
  // settings show it) and "ReHoMM4 <version>" left of it.
  {
    const Rect lay = Layout();
    const bool menu = MainMenuAlone();
    if (menu && h4::SkipIntroMoviesNow()) h4::SkipIntroMovies(false);  // the intro is over
  h4::TestWindowMode();
    h4::ShowLanguageButton(menu, Wide() ? lay.l + kLayoutW - 14 : int(Rd32(kScreenW)) - 14,
                           Wide() ? lay.t + 14 : 14, Wide() ? lay.l + 14 : 14);
  }
  {
    const uint32_t dialog = NewMapDialog();
    const Rect r = dialog ? RectOf(dialog) : Rect{0, 0, 0, 0};
    std::string open;
    if (h4::RandomMapPass(dialog != 0, r.l, r.t, r.r, r.b, dialog ? MapFolder(dialog) : std::string(),
                          dialog ? SelectedMap(dialog) : std::string(), &open) &&
        dialog) {
      if (!open.empty() && open != MapFolder(dialog)) SetMapFolder(c, dialog, open);
      ReadMapFolder(c, dialog);
    }
    if (dialog && h4::IsRandomMapsFolder(MapFolder(dialog))) NewestFirst(c, dialog);
  }
  // A constructor presenting (a loading screen) while the top window is
  // narrowed for it: leave it so until it returns.
  if (g_narrowed) return;
  const uint32_t eax = c.eax, ecx = c.ecx, edx = c.edx, esp = c.esp;
  WideAdventure(c);
  CentreScreens(c);
  SideWings(c);
  c.eax = eax;
  c.ecx = ecx;
  c.edx = edx;
  c.esp = esp;
}

}  // namespace

void PaintDirty(::recomp::Ctx& c);

namespace {

// ---- the pointer drawn over the frame (H4_GPU_CURSOR=0: the game's own)
//
// The game's cursor is a window (t_mouse_window, an animated window in it)
// at kCursorWindowVtable. Every move of it ran a paint pass, and every pass
// recomputes the visible region of every window (0x71FA90): 15-30 ms in a
// town or under a combat dialog, so the pointer lagged wherever windows
// were open. Here the window stays invisible to the game (moving it paints
// nothing) and its picture goes over the frame (gfx::SetPresentCursor),
// where the game keeps the window. The picture is taken from the game when
// its sprite changes, once per sprite: one paint pass of the cursor's area
// without it and one with it, the pixels that differ being the cursor.
// (Animated cursors keep their first frame.)
bool GpuCursorOn() {
  static const bool on = [] {
    const char* e = Env("GPU_CURSOR");
    return !e || *e != '0';
  }();
  return on;
}

// The cursor window: the newest, should there be several.
uint32_t CursorWindow(uint32_t top) {
  uint32_t cw = 0;
  ForChildren(top, [&](uint32_t w) {
    if (Rd32(w) == kCursorWindowVtable) cw = w;
  });
  return cw;
}

// Every cursor window invisible to the game.
void HideCursors(uint32_t top) {
  ForChildren(top, [](uint32_t w) {
    if (Rd32(w) == kCursorWindowVtable) GPtr(w + kVisible)[0] = 0;
  });
}

// The offscreen's pixels in r (16-bit), or none.
std::vector<uint16_t> ReadOffscreen(::recomp::Ctx& c, const Rect& r) {
  std::vector<uint16_t> out;
  const uint32_t surf = Rd32(kBackSurface);
  if (!surf || r.r <= r.l || r.b <= r.t) return out;
  const uint32_t vt = Rd32(surf);
  const gaddr desc = ScratchRect() + 0x300;
  std::memset(GPtr(desc), 0, 124);
  Wr32(desc, 124);
  const uint32_t eax = c.eax, ecx = c.ecx, edx = c.edx, esp = c.esp;
  if (uint32_t(CallGuest(c, Rd32(vt + kLockSlot), {surf, 0, desc, 1, 0})) == 0) {
    const uint8_t* bits = GPtr(Rd32(desc + 36));
    const int32_t pitch = int32_t(Rd32(desc + 16));
    const int sw = int(Rd32(desc + 12)), sh = int(Rd32(desc + 8));
    if (int(Rd32(desc + 72 + 12)) == 16 && r.l >= 0 && r.t >= 0 && r.r <= sw && r.b <= sh) {
      out.resize(size_t(r.r - r.l) * (r.b - r.t));
      for (int y = r.t; y < r.b; y++)
        std::memcpy(&out[size_t(y - r.t) * (r.r - r.l)], bits + int64_t(y) * pitch + r.l * 2,
                    size_t(r.r - r.l) * 2);
    }
    CallGuest(c, Rd32(vt + kUnlockSlot), {surf, 0});
  }
  c.eax = eax;
  c.ecx = ecx;
  c.edx = edx;
  c.esp = esp;
  return out;
}

// Marks r (screen coordinates) for the next paint pass.
void InvalidateArea(::recomp::Ctx& c, uint32_t top, const Rect& r) {
  const Rect t = RectOf(top);
  const gaddr rect = ScratchRect() + 0x280;
  Wr32(rect, uint32_t(r.l - t.l));
  Wr32(rect + 4, uint32_t(r.t - t.t));
  Wr32(rect + 8, uint32_t(r.r - t.l));
  Wr32(rect + 12, uint32_t(r.b - t.t));
  const uint32_t ecx = c.ecx, eax = c.eax;
  c.ecx = top;
  CallGuest(c, kInvalidate, {rect});
  c.ecx = ecx;
  c.eax = eax;
}

// The paint pass once more (for the cursor's area), the stack as on entry.
void PaintAgain(::recomp::Ctx& c) {
  const uint32_t esp = c.esp, eax = c.eax;
  PaintDirty(c);
  c.esp = esp;
  c.eax = eax;
}

// Before a paint pass: every cursor window invisible to the game; the
// picture of the newest taken again when its sprite or size changed.
void GpuCursorBeforePaint(::recomp::Ctx& c) {
  const uint32_t top = Rd32(kTopWindow);
  const uint32_t cw = top ? CursorWindow(top) : 0;
  if (!cw) return;
  uint32_t aw = 0;
  ForChildren(cw, [&](uint32_t w) {
    if (!aw) aw = w;
  });
  const Rect r = RectOf(cw);
  const int w = r.r - r.l, h = r.b - r.t;
  const uint32_t sprite = aw ? Rd32(aw + 0xD8) : 0;
  static uint32_t last_sprite = 0;
  static int last_w = -1, last_h = -1;
  static std::map<uint64_t, std::vector<uint32_t>> taken;  // by sprite and size
  if (sprite != last_sprite || w != last_w || h != last_h) {
    last_sprite = sprite;
    last_w = w;
    last_h = h;
    const uint64_t key = uint64_t(sprite) << 32 | uint32_t(w) << 16 | uint32_t(h);
    auto it = taken.find(key);
    if (it == taken.end()) {
      // Without the cursor, then with it, over its area; then without again.
      HideCursors(top);
      InvalidateArea(c, top, r);
      PaintAgain(c);
      const std::vector<uint16_t> bg = ReadOffscreen(c, r);
      GPtr(cw + kVisible)[0] = 1;
      InvalidateArea(c, top, r);
      PaintAgain(c);
      const std::vector<uint16_t> fg = ReadOffscreen(c, r);
      HideCursors(top);
      InvalidateArea(c, top, r);
      PaintAgain(c);
      if (fg.empty() || fg.size() != bg.size()) return;
      std::vector<uint32_t> argb(fg.size());
      for (size_t i = 0; i < fg.size(); i++) {
        if (fg[i] == bg[i]) continue;
        const uint32_t v = fg[i];
        argb[i] = 0xFF000000u | ((v >> 11) & 31) * 255 / 31 << 16 |
                  ((v >> 5) & 63) * 255 / 63 << 8 | (v & 31) * 255 / 31;
      }
      it = taken.emplace(key, std::move(argb)).first;
    }
    ::recomp::gfx::SetPresentCursor(it->second.data(), w, h);
  }
  HideCursors(top);
}

// From the main loop pass and the present: the pointer where the game keeps
// the cursor window; none while there is none (an army being dragged: the
// game takes the window away and moves the army's picture instead).
void GpuCursorPlace() {
  const uint32_t top = Rd32(kTopWindow);
  const uint32_t cw = top ? CursorWindow(top) : 0;
  const Rect r = cw ? RectOf(cw) : Rect{};
  static int last_x = -1, last_y = -1;
  static bool last_shown = false;
  if (!!cw == last_shown && (!cw || (r.l == last_x && r.t == last_y))) return;
  last_x = r.l;
  last_y = r.t;
  last_shown = cw != 0;
  ::recomp::gfx::SetPresentCursorPos(r.l, r.t, cw != 0);
  ::recomp::hle::PresentAgain();  // the game drew nothing
}

// A town whose screen shows something while it is built (an event's message
// as a loaded game opens it): its wings at once, beside the message, onto
// the primary surface (the narrowed top window leaves the sides alone). The
// main loop pass keeps them for the finished screen.
void WingsWhileBuilding(::recomp::Ctx& c) {
  if (!g_narrowed || !g_building_town || !Wide() || g_wings_for == g_building_town_window) return;
  const uint32_t eax = c.eax, ecx = c.ecx, edx = c.edx, esp = c.esp;
  const int luma = FieldLuma(c);
  if (luma >= 12) {
    const int f = FactionOfTown(g_building_town);
    g_wing_left = g_wing_right = FactionWing(f, luma < 110);
    g_mirror_right = true;
    if (PaintWingsOn(c, Rd32(kPrimarySurface), 0, 0, Rect{})) {
      g_wings_for = g_building_town_window;
      WarnF("h4: town wings (while built): luma %d, faction %d: %s\n", luma, f,
            g_wing_left.c_str());
    }
  }
  c.eax = eax;
  c.ecx = ecx;
  c.edx = edx;
  c.esp = esp;
}

}  // namespace

// Paints the windows' dirty region into the offscreen and returns it
// (RECT*), the cursor drawn by the GPU (above).
void f_00720530(::recomp::Ctx& c) {
  if (GpuCursorOn()) GpuCursorBeforePaint(c);
  PaintDirty(c);
}

// The paint pass. With the wings shown, the adventure frame under a town or
// battle sits the pass out (the game clears what it repaints to black and
// draws the windows there), and the wings go back over the side columns
// after it: under the game's cursor (a window of its own) only onto the
// black round its shape.
void PaintDirty(::recomp::Ctx& c) {
  const uint32_t top = Rd32(kTopWindow);
  if (!g_wings || !top) {
    f_00720530_lifted(c);
    if (top && Wide() && AdventureFrame(top) == g_widened && g_widened) {
      const auto* surface = ::recomp::com::As<::recomp::ddraw::Surface>(Rd32(kBackSurface));
      if (!surface || !surface->mem || surface->fmt != gfx::Format::R5G6B5 ||
          surface->kind != ::recomp::ddraw::Surface::Kind::Offscreen) return;
      const Rect origin = RectOf(top);
      const Rect frame = RectOf(g_widened);
      if (frame.r - frame.l <= kLayoutW) return;
      const int left = std::max(0, frame.l + 336 - origin.l);
      const int right = std::min(surface->width, frame.r - (kLayoutW - Frame().panel) - origin.l);
      if (left >= right) return;
      std::vector<Rect> overlays;
      auto collect = [&](uint32_t w) {
        if (w != g_widened && GPtr(w + kVisible)[0] && !g_layout_rects.count(w))
          overlays.push_back(RectOf(w));
      };
      ForChildren(top, collect);
      ForChildren(g_widened, collect);
      // Sample the intact frame immediately before the join, past the
      // left dragon. Keep the original profile and texture for each row.
      constexpr int sampleWidth = 8;
      if (left < sampleWidth) return;
      auto band = [&](int edge, int direction, int height) {
        for (int i = 0; i < height; ++i) {
          const int y = edge + direction * i;
          if (y < 0 || y >= surface->height) continue;
          auto* row = reinterpret_cast<uint16_t*>(GPtr(surface->mem + y * surface->pitch));
          const bool sampleCovered = std::any_of(overlays.begin(), overlays.end(), [&](const Rect& r) {
            return y + origin.t >= r.t && y + origin.t < r.b &&
                   left - sampleWidth + origin.l < r.r && left + origin.l > r.l;
          });
          if (sampleCovered) continue;
          int x = left;
          while (x < right) {
            int next = right, skip = x;
            for (const Rect& r : overlays) {
              if (y + origin.t < r.t || y + origin.t >= r.b || x + origin.l >= r.r) continue;
              if (x + origin.l >= r.l) skip = std::max(skip, r.r - origin.l);
              else next = std::min(next, r.l - origin.l);
            }
            if (skip > x) x = skip;
            else {
              for (; x < next; ++x)
                row[x] = row[left - sampleWidth + (x - left) % sampleWidth];
            }
          }
        }
      };
      band(frame.t - origin.t, 1, Frame().top_rows);
      band(frame.b - origin.t - 1, -1, Frame().bottom_rows);
      // The top strip goes on under the right dragon column (to 934 of the 1280 layout, the
      // column from 880), where the column's top is see-through; its art is cut at the join
      // above, so nothing painted there and the map's old pixels showed. The strip's own end
      // there (the dragon's head is on it), with the column laid over it by its alpha.
      const h4::random_map::Image* strip = h4::AdventureTopStrip();
      if (const h4::random_map::Image* dragon = h4::AdventureRightBorder();
          kLayoutW == 1280 && dragon && strip && strip->w == 934 && strip->h >= 16) {
        const int x0 = right, x1 = std::min(surface->width, frame.r - 346 - origin.l);
        auto unpack = [](uint32_t v, int* r, int* g, int* b) { *r = (v >> 11) & 31, *g = (v >> 5) & 63, *b = v & 31; };
        for (int i = 0; i < 16 && i < dragon->h; ++i) {
          const int y = frame.t - origin.t + i;
          if (y < 0 || y >= surface->height) continue;
          auto* row = reinterpret_cast<uint16_t*>(GPtr(surface->mem + y * surface->pitch));
          for (int x = x0; x < x1 && x - x0 < dragon->w; ++x) {
            const bool covered = std::any_of(overlays.begin(), overlays.end(), [&](const Rect& r) {
              return y + origin.t >= r.t && y + origin.t < r.b && x + origin.l >= r.l && x + origin.l < r.r;
            });
            if (covered) continue;
            const uint32_t d = dragon->argb[size_t(i) * size_t(dragon->w) + size_t(x - x0)];
            const uint32_t a = d >> 24;
            if (a == 255) continue;
            const uint32_t s = strip->argb[size_t(i) * size_t(strip->w) + size_t(880 + x - x0)];
            const uint16_t under = uint16_t(((s >> 19) & 31) << 11 | ((s >> 10) & 63) << 5 | ((s >> 3) & 31));
            if (a == 0) {
              row[x] = under;
              continue;
            }
            int ur, ug, ub;
            unpack(under, &ur, &ug, &ub);
            const int dr = int((d >> 19) & 31), dg = int((d >> 10) & 63), db = int((d >> 3) & 31);
            auto mix = [&](int top, int bottom) { return (top * int(a) + bottom * int(255 - a) + 127) / 255; };
            row[x] = uint16_t(mix(dr, ur) << 11 | mix(dg, ug) << 5 | mix(db, ub));
          }
        }
      }
    }
    return;
  }
  const uint32_t out = Rd32(c.esp + 4);
  // Under the screen: the adventure frame, or (a screen inside it) the
  // frame's other children.
  // A screen inside the frame: the frame's other children skip the pass,
  // and the frame itself is cut to the layout rectangle for it, so that
  // nothing of it lands in the side columns (under the cursor there, where
  // the wings go only onto black).
  std::vector<uint32_t> hidden;
  uint32_t narrowed = 0;
  Rect frame_rect{};
  if (const uint32_t frame = AdventureFrame(top)) {
    const uint32_t screen = CentredScreen(top);
    if (screen && Rd32(screen + 0x7C) == frame) {
      // Only the frame's windows that were there when the screen came: the
      // ones under it. A window made later is over it, a dialog opened from
      // the town (creature info on a double click), which hidden was never
      // drawn: the town froze, the dialog waiting for a click nobody saw.
      static uint32_t under_screen = 0;
      static std::set<uint32_t> under;
      if (screen != under_screen) {
        under_screen = screen;
        under.clear();
        ForChildren(frame, [&](uint32_t w) {
          if (w != screen) under.insert(w);
        });
      }
      ForChildren(frame, [&](uint32_t w) {
        if (w != screen && GPtr(w + kVisible)[0] && under.count(w)) hidden.push_back(w);
      });
      narrowed = frame;
      frame_rect = RectOf(frame);
    } else {
      hidden.push_back(frame);
    }
  }
  for (uint32_t w : hidden) GPtr(w + kVisible)[0] = 0;
  if (narrowed) SetRectRaw(narrowed, Layout());
  f_00720530_lifted(c);
  if (narrowed) SetRectRaw(narrowed, frame_rect);
  for (uint32_t w : hidden) GPtr(w + kVisible)[0] = 1;
  const Rect lay = Layout();
  const int l = out ? int32_t(Rd32(out)) : 0, r = out ? int32_t(Rd32(out + 8)) : 0;
  if (r <= l || (l >= lay.l && r <= lay.r)) return;
  uint32_t cursor = 0;
  ForChildren(top, [&](uint32_t w) {
    if (Rd32(w) == kCursorWindowVtable) cursor = w;
  });
  const uint32_t eax = c.eax, ecx = c.ecx, edx = c.edx, esp = c.esp;
  const Rect t = RectOf(top);
  PaintWingsOn(c, Rd32(kBackSurface), t.l, t.t, cursor ? RectOf(cursor) : Rect{}, true);
  c.eax = eax;
  c.ecx = ecx;
  c.edx = edx;
  c.esp = esp;
}

// The cinematic window's tick (thiscall, no arguments): draws the movie's frame and, once its
// last one is shown, sets its "ended" flag (+0x44; +0x54 is "a frame is due"), which closes the
// window as a click does. Skipping the intro movies (intro.h: the 3DO and NWC logos, the intro
// the first time; until the main menu first shows), the flag is set at the first tick instead.
// The menu's "replay intro" and the campaigns' movies come after the menu and play whole.
void f_0058D970(::recomp::Ctx& c) {
  if (h4::SkipIntroMoviesNow() && Rd32(c.ecx + 0x04)) {
    static uint32_t said = 0;
    if (said != Rd32(c.ecx + 0x04)) WarnF("h4: intro movie skipped\n");
    said = Rd32(c.ecx + 0x04);
    GPtr(c.ecx + 0x44)[0] = 1;
    GPtr(c.ecx + 0x54)[0] = 0;
    c.esp += 4;  // its `ret`
    return;
  }
  f_0058D970_lifted(c);
}

// H4_FPS_REPORT=s: every s seconds, the game's frame rate (its presents) and the spread of the
// intervals between them, in the log ("fps: ..."). The runtime's own report (PresentFrame) is
// not reached: the game's present is this one.
static void ReportFrameRate() {
  static const double every = [] {
    const char* e = std::getenv("H4_FPS_REPORT");
    return e ? std::atof(e) : 0.0;
  }();
  if (!(every > 0)) return;
  using Clock = std::chrono::steady_clock;
  static Clock::time_point first, last;
  static std::vector<double> gaps;
  const Clock::time_point now = Clock::now();
  if (first == Clock::time_point()) first = now;
  else gaps.push_back(std::chrono::duration<double, std::milli>(now - last).count());
  last = now;
  const double span = std::chrono::duration<double>(now - first).count();
  if (span < every || gaps.empty()) return;
  std::sort(gaps.begin(), gaps.end());
  auto at = [&](double q) { return gaps[size_t(q * double(gaps.size() - 1) + 0.5)]; };
  std::fprintf(stderr, "fps: %.1f (frame ms: median %.1f p90 %.1f p99 %.1f max %.1f)\n",
               double(gaps.size()) / span, at(0.5), at(0.9), at(0.99), gaps.back());
  gaps.clear();
  first = now;
}

// The game's present (dirty region to the screen).
void f_008D0C70(::recomp::Ctx& c) {
  g_presents++;
  ReportFrameRate();
  KeepWide(c);
  KeepScreenKeyboard();
#if defined(__ANDROID__) || (defined(__APPLE__) && TARGET_OS_IOS)
  const auto t0 = std::chrono::steady_clock::now();
  f_008D0C70_lifted(c);
  h4::NotePresent(std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - t0).count());
#else
  f_008D0C70_lifted(c);
#endif
  WingsWhileBuilding(c);
  if (GpuCursorOn()) GpuCursorPlace();  // its own loops (a drag) present here
}

#if defined(H4_SWITCH_APP)
}  // namespace recomp::gen
namespace h4 {
bool SwitchHandheld();  // main.cpp
}
namespace recomp::gen {
namespace {
// The Switch put in its dock or taken out (checked about once a second): the layout kept for that
// mode (or its default: 800x600 in the hands, 1024x768 on the TV), set as the Options dialog sets it.
void FollowDock(::recomp::Ctx& c) {
  static int last = -1;
  static uint64_t next = 0;
  const uint64_t now = SDL_GetTicks();
  if (now < next || g_narrowed) return;
  next = now + 1000;
  const int handheld = h4::SwitchHandheld() ? 1 : 0;
  if (last < 0 || handheld == last) {
    last = handheld;
    return;
  }
  last = handheld;
  const int w = h4::UseLayoutFile(handheld ? "layout_handheld" : "layout_docked", handheld ? 800 : 1024);
  WarnF("h4: the Switch %s: layout %d\n", handheld ? "in the hands" : "in its dock", w);
  if (w == kLayoutW || !TheScreen().wide) return;
  const uint32_t eax = c.eax, ecx = c.ecx, edx = c.edx;
  CallGuest(c, 0x8D3750, {uint32_t(w), uint32_t(w == 800 ? 600 : w == 1024 ? 768 : 1024), 1, 1});
  c.eax = eax;
  c.ecx = ecx;
  c.edx = edx;
}
}  // namespace
#endif

void f_008D1360(::recomp::Ctx& c) {
#if defined(H4_SWITCH_APP)
  FollowDock(c);
#endif
  KeepWide(c);
  KeepScreenKeyboard();
  if (GpuCursorOn()) GpuCursorPlace();
  f_008D1360_lifted(c);
}

// Clip the original horizontal artwork at the start of the plain band.
void f_00598170(::recomp::Ctx& c) {
  if (!g_tiled.count(c.ecx)) return f_00598170_lifted(c);
  const uint32_t ctx = Rd32(c.esp + 4);
  const uint32_t top = Rd32(kTopWindow);
  if (!top || !g_widened || RectOf(g_widened).r - RectOf(g_widened).l <= kLayoutW)
    return f_00598170_lifted(c);
  const int join = RectOf(g_widened).l - RectOf(top).l + 336;
  const int right = int32_t(Rd32(ctx + 36));
  if (int32_t(Rd32(ctx + 28)) >= join) {
    c.esp += 8;  // ret 4
    return;
  }
  Wr32(ctx + 36, uint32_t(std::min(right, join)));
  f_00598170_lifted(c);
  Wr32(ctx + 36, uint32_t(right));
}

// adventure_frame::layout(width), thiscall: its window's resize (vtable
// +0x44) lays the frame out anew for an 800, 1024 or 1280 wide screen,
// e.g. when the game is reactivated and sets its mode again. Wide, it lays
// out the centred 1280 layout, and the next main loop pass widens it again.
void f_004ACBB0(::recomp::Ctx& c) {
  const uint32_t frame = c.ecx;
  const uint32_t top = Rd32(kTopWindow);
  if (!Wide() || !top) return f_004ACBB0_lifted(c);
  Wr32(c.esp + 4, uint32_t(kLayoutW));
  if (frame == g_widened) {
    Unwiden(c, frame);
  } else {
    SetRectRaw(top, Layout());
    SetRectRaw(frame, Layout());
  }
  f_004ACBB0_lifted(c);
  // The next main loop pass widens it again.
}

void f_004A3450(::recomp::Ctx& c) { Narrowed(c, f_004A3450_lifted); }
void f_004A37C0(::recomp::Ctx& c) { Narrowed(c, f_004A37C0_lifted); }
void f_0062B6A0(::recomp::Ctx& c) { Narrowed(c, f_0062B6A0_lifted); }
void f_0069FB50(::recomp::Ctx& c) { Narrowed(c, f_0069FB50_lifted); }
// town_window::town_window(town*, ..., window* parent), thiscall.
void f_008AC0C0(::recomp::Ctx& c) {
  const uint32_t window = c.ecx, town = Rd32(c.esp + 4);
  const uint32_t building = g_building_town, building_window = g_building_town_window;
  g_building_town = town;
  g_building_town_window = window;
  Narrowed(c, f_008AC0C0_lifted);
  g_building_town = building;
  g_building_town_window = building_window;
}
// main_menu::layout(int width): picks its art by the width it is given (a
// rectangle's, the screen's when the top window spans it).
void f_0076D9B0(::recomp::Ctx& c) {
  TheScreen();  // (the layout read)
  if (int32_t(Rd32(c.esp + 4)) > kLayoutW) Wr32(c.esp + 4, uint32_t(kLayoutW));
  Narrowed(c, f_0076D9B0_lifted);
}
void f_0076DEB0(::recomp::Ctx& c) { Narrowed(c, f_0076DEB0_lifted); }
// t_options_dialog's constructor (thiscall). The app sets the screen's size (wide, or the window's
// on the desktop), so the dialog's 800x600 / 1024x768 / 1280x1024 choice is shown disabled: the
// buttons its resolution group (+0x104) gets while it is built (button_group_add below) are marked
// as the game marks a disabled button (+0xE4) and redrawn (0x5A1200).
uint32_t g_options_dialog = 0;
std::vector<uint32_t> g_resolution_buttons;
uint32_t g_fullscreen_box = 0;  // the box made right after the resolution group's three buttons

void f_007CA0D0(::recomp::Ctx& c) {
  const bool app_size = TheScreen().w != 0;
  if (app_size) {
    g_options_dialog = c.ecx;
    g_resolution_buttons.clear();
    g_fullscreen_box = 0;
  }
  Narrowed(c, f_007CA0D0_lifted);
  if (!app_size) return;
  g_options_dialog = 0;
  const uint32_t eax = c.eax, ecx = c.ecx;
  std::vector<uint32_t> off = g_resolution_buttons;
#if defined(H4_LAYOUT_CHOICE)
  // The apps: the three switch the layout (set_mode below).
  if (TheScreen().wide && off.size() == 3) off.clear();
#endif
#if !defined(H4_DESKTOP_APP)
  if (g_fullscreen_box) off.push_back(g_fullscreen_box);  // ticked (read_setting), not to be changed
#endif
  for (const uint32_t b : off) {
    GPtr(b + 0xE4)[0] = 1;
    c.ecx = b;
    CallGuest(c, kButtonUpdate, {});
  }
  c.eax = eax;
  c.ecx = ecx;
}

// The dialog's add_button(layout), thiscall -> the button.
void f_006ECB80(::recomp::Ctx& c) {
  const uint32_t dialog = c.ecx;
  f_006ECB80_lifted(c);
  if (g_options_dialog && dialog == g_options_dialog && g_resolution_buttons.size() == 3 && !g_fullscreen_box)
    g_fullscreen_box = c.eax;
}

// button_group::add(button*), thiscall.
void f_005A29A0(::recomp::Ctx& c) {
  if (g_options_dialog && c.ecx == g_options_dialog + 0x104) g_resolution_buttons.push_back(Rd32(c.esp + 4));
  f_005A29A0_lifted(c);
}

// window::window(const RECT* rect, window* parent), thiscall: no rectangle
// means the parent's whole area. Screens made that way right under the top
// window (combat, the adventure frame) take the layout rectangle, not the
// screen: combat built while the top window spanned it came out at x 0.
void f_0071E6D0(::recomp::Ctx& c) {
  const uint32_t top = Rd32(kTopWindow);
  if (Rd32(c.esp + 4) == 0 && top && Rd32(c.esp + 8) == top)
    Narrowed(c, f_0071E6D0_lifted);
  else
    f_0071E6D0_lifted(c);
}

// The game's "full screen": the page's in the browser, the desktop apps' setting, always on
// elsewhere.
bool AppFullscreen() {
#if defined(H4_WEB_APP)
  return MAIN_THREAD_EM_ASM_INT({ return document.fullscreenElement ? 1 : 0; }) != 0;
#elif defined(H4_DESKTOP_APP)
  return h4::Fullscreen();
#else
  return true;
#endif
}

// The player's choice in the Options dialog.
void SetAppFullscreen(bool on) {
#if defined(H4_WEB_APP)
  // The page's enterFullscreen / leaveFullscreen (web/shell/index.html), in answer to the click
  // on OK (the browser's user activation still holds).
  MAIN_THREAD_ASYNC_EM_ASM({ if (Module.h4Status) Module.h4Status('fullscreen', $0); }, on ? 1 : 0);
#elif defined(H4_DESKTOP_APP)
  h4::SetFullscreen(on);
#else
  (void)on;
#endif
}

// read_setting(const char* name, uint32_t* out, bool user) -> found. Wide,
// "resolution" (the screen width the game picks its layout and buffers
// by) reads 1024 whatever was saved: the Options dialog, which shows the
// 1280 layout, saves 1280, and with that the adventure map keeps the width
// it is made with and never renders the widened part (black up to the
// panel). set_mode above decides the real size anyway.
void f_0063B840(::recomp::Ctx& c) {
  const uint32_t name = Rd32(c.esp + 4), out = Rd32(c.esp + 8);
  f_0063B840_lifted(c);
  const Screen& s = TheScreen();
  if (!out || !name) return;
  // The Options dialog's "full screen" box shows the app's own setting (⚙, window_mode.h), so a
  // set_mode with another value (below) is the player's change; elsewhere the game is always
  // full screen (the box is shown ticked and disabled, f_007CA0D0).
  if (s.w && GStr(name) == "full_screen") {
    Wr32(out, AppFullscreen() ? 1 : 0);
    c.eax = (c.eax & ~0xFFu) | 1;
    return;
  }
  if (!s.wide || !(c.eax & 0xFF) || GStr(name) != "resolution") return;
  // (the Options dialog, while it is made, its choice marked: the layout in use)
  Wr32(out, g_options_dialog ? uint32_t(kLayoutW) : 1024);
}

void f_008D3750(::recomp::Ctx& c) {
  const uint32_t args = c.esp + 4;  // width, height, repaint, full_screen
  const Screen& s = TheScreen();
#if defined(H4_DESKTOP_APP)
  // The Options dialog's "full screen" box (its OK and the toggle pass repaint 1; the start's call,
  // repaint 0, has the game's own guess, which is not the player's): the app's window and setting
  // (the page's full screen in the browser) follow it, as ⚙ does.
  // Not the window procedure's on WM_ACTIVATE (0x8D169F: being activated while full screen, the
  // game sets a window and full screen again, DirectDraw's way back to its surfaces): the first of
  // those took the app to a window and saved that, so the next start was in a window.
  const uint32_t ret = Rd32(c.esp);
  const bool activation = ret == 0x8D16CE || ret == 0x8D1705 || ret == 0x8D170F;
  bool full_now = AppFullscreen();
  if (const bool full = (Rd32(args + 12) & 0xFF) != 0;
      !activation && (Rd32(args + 8) & 0xFF) && full != full_now) {
    SetAppFullscreen(full);
    full_now = full;
  }
  if (s.w) Wr32(args + 12, full_now ? 1 : 0);
#else
  if (s.w) Wr32(args + 12, 1);
#endif
  // The Options dialog's 800x600 / 1024x768 / 1280x1024 (phones, tablets, the Switch; wide): the
  // layout switched, the adventure frame put back in the old one first (the game lays it out anew
  // for the new one).
  if (const int w = int(Rd32(args)); (Rd32(args + 8) & 0xFF) && s.wide && w != kLayoutW &&
      (w == 800 || w == 1024 || w == 1280)) {
    if (g_widened) Unwiden(c, g_widened);
    SwitchLayout(int(Rd32(args)));
  }
  // Windowed too (the game's full_screen setting off): the host shows the
  // window like a full screen, fitted to the display.
  if (s.w) {
    Wr32(args, uint32_t(s.w));
    Wr32(args + 4, uint32_t(s.h));
  }
  f_008D3750_lifted(c);
  // Applying the options sets the mode again: the new surfaces are blank,
  // and the game repaints only what changes next (the main menu stayed
  // black but under the cursor).
  if (const uint32_t top = Rd32(kTopWindow); top && s.w) {
    const uint32_t eax = c.eax;
    const Rect r = RectOf(top);
    InvalidateWhole(c, top, r.r - r.l, r.b - r.t);
    c.eax = eax;
  }
}

// screen_rect(RECT* out) -> out: the client area in screen coordinates.
void f_008D2E20(::recomp::Ctx& c) {
  const uint32_t out = Rd32(c.esp + 4);
  f_008D2E20_lifted(c);
  if (!Wide()) return;
  int l, t;
  LayoutRect(&l, &t);
  const int x = int32_t(Rd32(out)), y = int32_t(Rd32(out + 4));
  Wr32(out, uint32_t(x + l));
  Wr32(out + 4, uint32_t(y + t));
  Wr32(out + 8, uint32_t(x + l + kLayoutW));
  Wr32(out + 12, uint32_t(y + t + kLayoutH));
}

// on_size(type, width, height): grows the back buffer and fits the top
// window to the client area; wide, the top window goes back to the layout
// rectangle.
void f_008D22D0(::recomp::Ctx& c) {
  f_008D22D0_lifted(c);
  const uint32_t top = Rd32(kTopWindow);
  if (!Wide() || !top) return;
  // The top window goes where it was: across the screen, as the main loop
  // keeps it (moved with its children, which undoes the move to the client
  // area above).
  const uint32_t eax = c.eax;
  const gaddr rect = ScratchRect();
  CallGuest(c, 0x8D2E20, {rect});  // the layout rectangle, via the replacement above
  const int dx = Layout().l;
  Wr32(rect, Rd32(rect) - uint32_t(dx));
  Wr32(rect + 8, Rd32(rect + 8) + uint32_t(dx));
  c.ecx = top;
  CallGuest(c, kSetRect, {rect, 1});
  c.eax = eax;
  g_wings_primary = true;  // reactivated: the screen was redone without them
}

}  // namespace recomp::gen
