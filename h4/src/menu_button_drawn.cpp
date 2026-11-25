// menu_button.h in every app: the ⚙ button over the main menu (☁️ the save sync and ☕ the author's
// Ko-fi page left of it, where the app has them) and the settings it opens, drawn into the
// presented frame (gfx::SetPresentBadge), their clicks (the mouse, taps, the trackpad, the
// gamepad's pointer) taken before the game's (SetMenuInputHook). One panel for all of them, in the
// game's dialogs' look (app_ui/ui.h): the game's and the interface's languages, the scaling, the
// renderer (the Mac, iOS, Android), the intro movies, the black side panels, the save sync (its
// help on Linux and Windows: save_sync.h; iOS, Android and the Mac keep it themselves:
// app_ui/native.h), the folders, the gamepad's panel (pad_config.h) and the update check. What
// only the system can do (a browser, its files app, its folder picker) is app_ui/native.h's.
#include <SDL3/SDL.h>
#if defined(__APPLE__)
#include <TargetConditionals.h>
#endif
#if defined(_WIN32)
#include <windows.h>
#include <io.h>
#include <shellapi.h>
#ifndef F_OK
#define F_OK 0
#endif
#else
#include <unistd.h>
#endif
#if defined(H4_WEB_APP)
#include <emscripten.h>
#endif
#if defined(H4_LINUX_APP) && !defined(H4_WEB_APP)
#include <spawn.h>
#include <sys/wait.h>
extern char** environ;
#endif

#include <algorithm>
#include <atomic>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

#include "app_ui/native.h"
#include "app_ui/ui.h"
#include "filter_notice.h"
#include "intro.h"
#include "language.h"
#include "menu_button.h"
#include "pad_config.h"
#include "random_map_ui.h"
#include "renderer.h"
#include "side_panels.h"
#include "ui_text.h"
#include "runtime/gfx/device.h"
#include "runtime/hle/present.h"
#include "runtime/hle/user32_window.h"

// Which app this is, and what it has.
#if defined(__APPLE__) && TARGET_OS_IOS
#define H4_DRAWN_IOS 1
#elif defined(__APPLE__)
#define H4_DRAWN_MAC 1
#endif
#if defined(H4_DRAWN_IOS) || defined(H4_DRAWN_MAC) || defined(__ANDROID__)
#define H4_NATIVE_SYNC 1  // the save sync set here (app_ui/native.h), the ☁️ button; the renderer; the folders
#endif
// (the browser defines H4_LINUX_APP too, web/CMakeLists.txt)
#if (defined(H4_LINUX_APP) && !defined(H4_WEB_APP)) || defined(H4_NATIVE_SYNC) || defined(H4_WINDOWS_APP)
#define H4_UPDATES 1  // the update check
#endif
#if (defined(H4_LINUX_APP) && !defined(H4_WEB_APP)) || defined(H4_DRAWN_MAC) || defined(__ANDROID__)
#define H4_RESTART 1  // a new game language: the app starts again (RestartApp)
#endif
#if (defined(H4_LINUX_APP) || defined(H4_WINDOWS_APP)) && !defined(H4_WEB_APP)
#define H4_SYNC_HELP 1  // the save sync is sync.conf's: its help
#endif
#if !defined(H4_SWITCH_APP)
#define H4_KOFI 1
#endif

#include "install/json.h"
#include "install/platform.h"

namespace {

using h4::app_ui::Ui;

std::mutex g_mu;
std::string g_lang, g_saved, g_next;  // the game's language running, its saved file, the one picked
std::string g_game_lang;              // the game's ("en", "uk"...)

h4::app_ui::Picture g_gear;   // ⚙, with ☁️ and ☕ left of it (g_icons)
std::vector<std::string> g_icons;  // what g_gear shows, left to right: "sync", "kofi", "settings"
constexpr int kGearSide = 40, kGearGap = 8;
int g_icons_x = 0;                 // the row's x in g_gear (a tooltip longer than it widens it)
std::string g_tip;                 // the tooltip shown under ("" none): the icon's name
float g_point_x = -1, g_point_y = -1;  // the pointer (window x, y), for the tooltips
uint64_t g_point_ms = 0;               // when it last moved
h4::app_ui::Panel g_panel;    // the settings, or what they opened
bool g_open = false;          // the panel is shown (else ⚙)
bool g_restart_now = false;   // a language picked (H4_RESTART): the app starts again, outside g_mu

// The update check: its state, shown on its panel; its threads change it under g_mu.
enum Upd { kChecking, kLatest, kNewer, kDownloading, kDone, kFailed };
Upd g_upd = kChecking;
Upd g_upd_shown = kChecking;       // what the panel was drawn with
std::string g_upd_url, g_upd_tag, g_upd_page;  // the newer release's file, its version, its page
std::string g_upd_file;                        // where it was downloaded to

// Input, from the SDL event watch (any thread) for the guest thread's pass (under g_mu).
bool g_hooked = false;
const h4::app_ui::Picture* g_shown_pic = nullptr;  // in the frame now
bool g_clicked = false;               // a click (the button released)
bool g_click_right = false;
float g_click_x = 0, g_click_y = 0;   // where (window x, y)
bool g_pressed = false;               // the left button went down (a slider held)
float g_press_x = 0, g_press_y = 0;
bool g_released = false;
float g_drag_x = -1;                  // the pointer's x while the left button is down
bool g_back = false;                  // Esc (a gamepad's)
bool g_taken = false;                 // the last button event was on the settings (not the game's)
float g_hit[4] = {0, 0, 0, 0};        // where the picture is in the window: l, t, r, b

std::string UiLang() {
  const std::string c = h4::UiLanguageChoice();
  return c.empty() ? g_game_lang : c;
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
  if (e->type == SDL_EVENT_MOUSE_MOTION) {
    if (g_open) g_drag_x = e->motion.x;
    g_point_x = e->motion.x, g_point_y = e->motion.y;
    g_point_ms = SDL_GetTicks();
    return true;
  }
  if (e->type != SDL_EVENT_MOUSE_BUTTON_DOWN && e->type != SDL_EVENT_MOUSE_BUTTON_UP) return true;
  const float x = e->button.x, y = e->button.y;
  const bool in = x >= g_hit[0] && x < g_hit[2] && y >= g_hit[1] && y < g_hit[3];
  g_taken = in || g_open;
  const bool right = e->button.button == SDL_BUTTON_RIGHT;
  if (e->button.button != SDL_BUTTON_LEFT && !(right && g_open)) return true;
  if (e->type == SDL_EVENT_MOUSE_BUTTON_DOWN) {
    if (g_open && !right) {
      g_pressed = true;
      g_press_x = x, g_press_y = y;
    }
    return true;
  }
  g_released = true;
  g_point_ms = 0;  // (no tooltip after a click, until the pointer moves)
  if (in || g_open) {  // (a click elsewhere closes the panel)
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
      if (!g_shown_pic || !g_open) return false;
      if (e.type == SDL_EVENT_KEY_DOWN && e.key.scancode == SDL_SCANCODE_ESCAPE) g_back = true;
      return true;
    }
    default:
      return false;
  }
}

// The window resized (the browser's full screen, a desktop window dragged): the game, idle in a
// menu, shows nothing new, so the frame stays as it was until the next input. Seen by an SDL event
// watch (any thread): a pointer move where it is has the game run its loop, whose pass
// (ShowLanguageButton) shows the frame again.
std::atomic<bool> g_resized{false};
bool OnWindowEvent(void*, SDL_Event* e) {
  if (e->type != SDL_EVENT_WINDOW_RESIZED && e->type != SDL_EVENT_WINDOW_PIXEL_SIZE_CHANGED &&
      e->type != SDL_EVENT_WINDOW_ENTER_FULLSCREEN && e->type != SDL_EVENT_WINDOW_LEAVE_FULLSCREEN)
    return true;
  if (g_resized.exchange(true)) return true;  // (one wake for a burst of them)
  SDL_Event m{};
  m.type = SDL_EVENT_MOUSE_MOTION;
  m.motion.windowID = e->window.windowID;
  float x = 0, y = 0;
  SDL_GetMouseState(&x, &y);
  m.motion.x = x;
  m.motion.y = y;
  SDL_PushEvent(&m);
  return true;
}

// Every mouse and key event before the game (SetMenuInputHook, the gamepad's too): a click on the
// settings (or anywhere while they are open), and the wheel and keys while they are open, are not
// the game's (Esc: back).
bool OnInput(const SDL_Event& e) {
  SDL_Event c = e;
  OnEvent(nullptr, &c);
  std::lock_guard<std::mutex> l(g_mu);
  if (!g_shown_pic) return false;
  switch (e.type) {
    case SDL_EVENT_MOUSE_BUTTON_DOWN:
    case SDL_EVENT_MOUSE_BUTTON_UP:
      return g_taken;
    case SDL_EVENT_MOUSE_WHEEL:
    case SDL_EVENT_TEXT_INPUT:
      return g_open;
    case SDL_EVENT_KEY_DOWN:
    case SDL_EVENT_KEY_UP:
      if (g_open && e.type == SDL_EVENT_KEY_DOWN && e.key.scancode == SDL_SCANCODE_ESCAPE) g_back = true;
      return g_open;
    default:
      return false;
  }
}

}  // namespace

// ---- app_ui/native.h on Linux and Windows (a browser, the files app; the sync is sync.conf's),
// the Switch and the browser (none)
#if !defined(H4_NATIVE_SYNC)
namespace h4::native {
bool OpenUrl(const std::string& url) {
#if defined(_WIN32)
  const int n = MultiByteToWideChar(CP_UTF8, 0, url.c_str(), -1, nullptr, 0);
  std::wstring w(size_t(n > 0 ? n : 1), L'\0');
  MultiByteToWideChar(CP_UTF8, 0, url.c_str(), -1, w.data(), n);
  return INT_PTR(ShellExecuteW(nullptr, L"open", w.c_str(), nullptr, nullptr, SW_SHOWNORMAL)) > 32;
#elif defined(H4_LINUX_APP) && !defined(H4_WEB_APP)
  pid_t pid = 0;
  const char* argv[] = {"xdg-open", url.c_str(), nullptr};
  if (posix_spawnp(&pid, "xdg-open", nullptr, nullptr, const_cast<char* const*>(argv), environ) != 0) {
    std::fprintf(stderr, "h4: no xdg-open for %s\n", url.c_str());
    return false;
  }
  std::thread([pid] { waitpid(pid, nullptr, 0); }).detach();
  return true;
#elif defined(H4_WEB_APP)
  // A new tab (from the page's thread, not waited for: it may be waiting for g_mu); a browser that
  // blocks it (the click was a moment ago, on another thread) gets the address said instead.
  static char at[512];
  std::snprintf(at, sizeof at, "%s", url.c_str());
  MAIN_THREAD_ASYNC_EM_ASM({
    const u = UTF8ToString($0);
    let w = null;
    try { w = window.open(u, '_blank'); } catch (e) { /* blocked */ }
    if (w) w.opener = null;
    else if (typeof toast === 'function') toast(u, 10000);
  }, at);
  return true;
#else
  (void)url;
  return false;
#endif
}
bool RevealFolder(const std::string& path) {
#if defined(H4_WEB_APP)
  (void)path;
  return false;
#else
  return OpenUrl(path);
#endif
}
bool HasSyncChoice() { return false; }
bool SyncOn() { return false; }
void SetSync(bool) {}
SyncPanel SyncPanelFor(const std::string&) { return {}; }
std::string SavesFolder() { return ""; }
std::string SyncedFolder() { return ""; }
void DownloadApk(const std::string&, const std::string&) {}
}  // namespace h4::native
#endif

namespace {

void Choose(const std::string& lang) {
  if (FILE* f = std::fopen(g_saved.c_str(), "w")) {
    std::fputs(lang.c_str(), f);
    std::fclose(f);
  }
}

// "0.10" > "0.9" ("v" in front ignored).
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

#if defined(H4_UPDATES)
// This build's file in a release (reHoMM4-v<version><suffix>; 0.2: HeroesIV-<version>…).
#if defined(H4_DRAWN_MAC) && defined(__aarch64__)
const std::string kSuffix = "-macos-arm64.dmg";
#elif defined(H4_DRAWN_MAC)
const std::string kSuffix = "-macos-x86_64.dmg";
#elif defined(H4_DRAWN_IOS)
const std::string kSuffix = "-ios.ipa";
#elif defined(__ANDROID__)
const std::string kSuffix = "-android_arm64.apk";
#elif defined(H4_WINDOWS_APP)
const std::string kSuffix = "-windows-x86_64.zip";
#elif defined(__aarch64__)
const std::string kSuffix = "-linux-arm64.tar.gz";
#else
const std::string kSuffix = "-linux-x86_64.tar.gz";
#endif

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
      if (const std::string n = rel["assets"][i]["name"].text();
          (n.rfind("reHoMM4-", 0) == 0 || n.rfind("HeroesIV-", 0) == 0) && n.size() > kSuffix.size() &&
          n.compare(n.size() - kSuffix.size(), kSuffix.size(), kSuffix) == 0)
        url = rel["assets"][i]["browser_download_url"].text();
    std::string page = rel["html_url"].text();
    if (page.empty()) page = "https://github.com/evilHazzarD/ReHoMM4/releases";
    std::fprintf(stderr, "h4: update check: %s on GitHub, this is %s\n", tag.c_str(), H4_VERSION_STRING);
    std::lock_guard<std::mutex> l(g_mu);
    g_upd_tag = tag;
    g_upd_url = url;
    g_upd_page = page;
    g_upd = Newer(tag, H4_VERSION_STRING) ? kNewer : kLatest;
  }).detach();
}

// The newer release's file: into Downloads (never over a file there; a thread; the Mac: its .dmg
// opened, which mounts it), or Android's DownloadManager's (its notification installs it).
void DownloadUpdate() {
  const std::string url = g_upd_url, tag = g_upd_tag;  // (under g_mu)
#if defined(__ANDROID__)
  h4::native::DownloadApk(url, tag);
  g_upd = kDone;
#else
  std::thread([url, tag] {
    int status = 0;
    std::string body, error;
#if defined(_WIN32)
    const char* home = std::getenv("USERPROFILE");
#else
    const char* home = std::getenv("HOME");
#endif
    const std::string dir = std::string(home ? home : ".") + "/Downloads";
    const std::string ext = kSuffix.substr(kSuffix.find('.'));  // ".tar.gz", ".dmg"
    // The release's own name for it ("<name> 2.tar.gz"... when there is one).
    std::string base = url.substr(url.rfind('/') + 1);
    if (base.size() > ext.size() && base.compare(base.size() - ext.size(), ext.size(), ext) == 0)
      base.resize(base.size() - ext.size());
    if (base.empty()) base = "reHoMM4-" + tag + kSuffix.substr(0, kSuffix.size() - ext.size());
    std::string path = dir + "/" + base + ext;
    for (int i = 2; access(path.c_str(), F_OK) == 0; i++) path = dir + "/" + base + " " + std::to_string(i) + ext;
    bool ok = h4::install::HttpGet(url, "", &status, &body, &error) && status == 200 && !body.empty();
    if (ok) {
      FILE* f = std::fopen(path.c_str(), "wb");
      ok = f && std::fwrite(body.data(), 1, body.size(), f) == body.size();
      if (f) ok = std::fclose(f) == 0 && ok;
    }
    std::fprintf(stderr, "h4: update download: %s (%s HTTP %d)\n", ok ? path.c_str() : "failed", error.c_str(), status);
#if defined(H4_DRAWN_MAC)
    if (ok) h4::native::OpenUrl(path);  // mounted: Finder shows the app
#endif
    std::lock_guard<std::mutex> l(g_mu);
    g_upd_file = path;
    g_upd = ok ? kDone : kFailed;
  }).detach();
#endif
}
#endif

// ---- What's new: the releases on GitHub newer than this version, newest first (their notes as
// plain text); fetched on a thread when the settings first open, which also tells the update
// button that there is one.
enum Wn { kWnNone, kWnLoading, kWnReady, kWnFailed };
Wn g_wn = kWnNone, g_wn_shown = kWnNone;
struct Release {
  std::string tag, text;
};
std::vector<Release> g_newer;  // newer than this version, newest first
Release g_current;              // this version's
int g_wn_page = 0;
const char* const kReleasesPage = "https://github.com/evilHazzarD/ReHoMM4/releases";

// The notes' Markdown as text: the downloads' table left out, **bold** and `code` plain, lists
// with bullets, one empty line at most.
std::string PlainNotes(const std::string& md) {
  std::string out;
  size_t i = 0;
  bool blank = true;
  while (i <= md.size()) {
    size_t j = md.find('\n', i);
    if (j == std::string::npos) j = md.size();
    std::string line = md.substr(i, j - i);
    i = j + 1;
    if (!line.empty() && line.back() == '\r') line.pop_back();
    if (!line.empty() && line[0] == '|') continue;
    for (const char* mark : {"**", "`"})
      for (size_t at; (at = line.find(mark)) != std::string::npos;) line.erase(at, std::strlen(mark));
    size_t indent = line.find_first_not_of(' ');
    if (indent == std::string::npos) {
      if (!blank) out += "\n";
      blank = true;
      continue;
    }
    if (line.compare(indent, 2, "- ") == 0) line = std::string(indent * 2, ' ') + (indent ? "– " : "• ") + line.substr(indent + 2);
    out += line + "\n";
    blank = false;
  }
  while (!out.empty() && out.back() == '\n') out.pop_back();
  return out;
}

void FetchReleases() {
  if (g_wn == kWnLoading || g_wn == kWnReady) return;  // (under g_mu)
  g_wn = kWnLoading;
  std::thread([] {
    int status = 0;
    std::string body, error;
    h4::install::Json list;
    const bool ok = h4::install::HttpGet("https://api.github.com/repos/evilHazzarD/ReHoMM4/releases?per_page=30", "",
                                         &status, &body, &error) &&
                    status == 200 && h4::install::Json::Parse(body, &list) && list.type == h4::install::Json::Array;
    std::vector<Release> newer;
    Release current;  // this version's (shown when nothing is newer)
    const char* test = std::getenv("H4_TEST_VERSION");  // (testing: as if this were that version)
    const std::string have = test && *test ? test : H4_VERSION_STRING;
    for (size_t i = 0; ok && i < list.size(); i++) {
      const h4::install::Json& r = list[i];
      if (r["draft"].b || r["prerelease"].b) continue;
      if (Newer(r["tag_name"].text(), have)) newer.push_back({r["tag_name"].text(), PlainNotes(r["body"].text())});
      else if (!Newer(have, r["tag_name"].text())) current = {r["tag_name"].text(), PlainNotes(r["body"].text())};
    }
    std::sort(newer.begin(), newer.end(), [](const Release& a, const Release& b) { return Newer(a.tag, b.tag); });
    if (!ok) std::fprintf(stderr, "h4: releases: %s (HTTP %d)\n", error.c_str(), status);
    std::lock_guard<std::mutex> l(g_mu);
    g_newer = newer;
    g_current = current;
    g_wn = ok ? kWnReady : kWnFailed;
  }).detach();
}

// ---- the panels (under g_mu)

constexpr int kSettingsW = 480, kGamepadW = 780;

void ShowSettings();

void Close() { g_open = false; }

// A message with buttons (none: OK, back to the settings).
void ShowMessage(const std::string& title, const std::string& text,
                 std::vector<std::pair<std::string, std::function<void()>>> buttons = {}) {
  g_panel.Set(kSettingsW, UiLang(), [title, text, buttons](Ui& ui) {
    if (!title.empty()) ui.Title(title);
    ui.Paragraph(text, true);
    ui.Gap(4);
    if (buttons.empty()) ui.Buttons({{"OK", [] { ShowSettings(); }}});
    else ui.Buttons(buttons);
  });
  g_open = true;
}

// The game's language `code` chosen: saved; the app starts again with it (H4_RESTART: its pack, if
// it has to be downloaded, then), else it applies at the next start.
void PickLanguage(const std::string& code) {
  auto apply = [code] {
    g_next = code;
    Choose(code);
#if defined(H4_RESTART)
    g_restart_now = true;  // (after the click, outside g_mu)
#endif
    ShowMessage("", h4::TrIn(UiLang(), "Restart the game to apply the new language."));
  };
  const std::string note = h4::PackDownloadNote(code, UiLang());
  if (note.empty()) return apply();
  // A pack to download first: said, and chosen only with Download.
  const std::string lang = UiLang();
  ShowMessage(h4::TrIn(lang, "Language pack"), note,
              {{h4::TrIn(lang, "Download"), apply}, {h4::TrIn(lang, "Cancel"), [] { ShowSettings(); }}});
}

#if defined(H4_UPDATES)
void ShowUpdate() {
  g_panel.Set(kSettingsW, UiLang(), [](Ui& ui) {
    g_upd_shown = g_upd;
    std::string v = g_upd_tag;
    if (!v.empty() && v[0] == 'v') v.erase(0, 1);
    const std::string have = H4_VERSION_STRING;
    const std::string name = g_upd_file.substr(g_upd_file.rfind('/') + 1);
    switch (g_upd) {
      case kChecking:
        ui.Title(ui.T("Updates"));
        ui.Paragraph(ui.T("Checking GitHub for a new version…"), true);
        ui.Buttons({{ui.T("Cancel"), [] { ShowSettings(); }}});
        break;
      case kLatest:
        ui.Title(ui.T("Up to date"));
        ui.Paragraph(h4::TrIn(ui.lang(), "You have the latest version: {1}.", {have}), true);
        ui.Buttons({{"OK", [] { ShowSettings(); }}});
        break;
      case kNewer: {
        ui.Title(ui.T("A new version"));
        std::vector<std::pair<std::string, std::function<void()>>> buttons;
#if defined(H4_DRAWN_IOS)  // (installed as the first one was: AltStore, Sideloadly)
        ui.Paragraph(h4::TrIn(ui.lang(), "ReHoMM4 {1} (you have {2}). Download reHoMM4-{1}-ios.ipa and install it with "
                                         "AltStore or Sideloadly, as the first one.", {g_upd_tag, have}), true);
        buttons.push_back({ui.T("Open the release"), [] { h4::native::OpenUrl(g_upd_page); }});
#else
        ui.Paragraph(h4::TrIn(ui.lang(), "ReHoMM4 {1} (you have {2})", {g_upd_tag, have}), true);
        if (!g_upd_url.empty())
          buttons.push_back({ui.T("Download"), [] {
                               g_upd = kDownloading;
                               DownloadUpdate();
                             }});
        buttons.push_back({ui.T("What's new"), [] { h4::native::OpenUrl(g_upd_page); }});
#endif
        buttons.push_back({ui.T("Later"), [] { ShowSettings(); }});
        ui.Buttons(buttons);
        break;
      }
      case kDownloading:
        ui.Title(ui.T("Updates"));
        ui.Paragraph(ui.T("Downloading the new version…"), true);
        break;
      case kDone:
        ui.Title(ui.T("Updates"));
#if defined(__ANDROID__)
        ui.Paragraph(ui.T("The APK is downloading into Downloads. When it is done, tap its notification to install "
                          "it (Android asks to allow installing from that source)."), true);
#elif defined(H4_DRAWN_MAC)
        ui.Paragraph(h4::TrIn(ui.lang(), "ReHoMM4 {1} is in Downloads ({2}). Quit the game and drag ReHoMM4 into "
                                         "Applications, replacing the old one (an old Heroes IV, if there, to the Bin).",
                              {v, name}), true);
#else
        {
          std::string done = ui.T("Downloaded into Downloads (reHoMM4-v<version>-linux-x86_64.tar.gz).\nQuit the game and unpack it over the old one.");
          if (const size_t at = done.find("reHoMM4-v<version>-linux-x86_64.tar.gz"); at != std::string::npos)
            done.replace(at, 38, name);
          ui.Paragraph(done, true);
        }
#endif
        ui.Buttons({{"OK", [] { ShowSettings(); }}});
        break;
      case kFailed:
        ui.Title(ui.T("Updates"));
        ui.Paragraph(ui.T("Could not reach GitHub. Try again later."), true);
        ui.Buttons({{"OK", [] { ShowSettings(); }}});
        break;
    }
  });
  g_open = true;
}
#endif

#if defined(H4_SYNC_HELP)
void ShowSyncHelp() {
  g_panel.Set(560, UiLang(), [](Ui& ui) {
    ui.Anywhere([] { ShowSettings(); });
    ui.Paragraph(ui.T(
        "Saved games sync\n\nYour saves are kept in step with the folder ~/ReHOMM4 (made at the first start): file by "
        "file, the newer copy wins, nothing is deleted. Carry that folder to your other devices with a sync tool:\n\n"
        "• Syncthing: share ~/ReHOMM4 with your other devices. On Android or an iPhone, turn the sync on in "
        "ReHoMM4's settings and pick the shared folder; the Mac app syncs iCloud Drive/ReHOMM4.\n\n• A cloud (Google "
        "Drive, Dropbox, OneDrive…) through rclone: set the remote up once with `rclone config`, then put the line  "
        "rclone=<remote>:ReHOMM4  into ~/.local/share/ReHoMM4/sync.conf. The game brings newer saves down and sends "
        "its own up.\n\nIn sync.conf, folder=<path> syncs another folder instead; a line  off  stops the sync.\n\n"
        "Click to close.\n"));
  });
  g_open = true;
}
#endif

#if defined(H4_NATIVE_SYNC)
// The ☁️ button's panel (app_ui/native.h): what the sync is doing, what can be done; a button done,
// the panel shut (`back`: to the settings).
void ShowSyncPanel(bool back) {
  h4::native::SyncPanel p = h4::native::SyncPanelFor(UiLang());
  for (auto& b : p.buttons) {
    auto act = b.second;
    b.second = [act, back] {
      if (act) act();
      if (back) ShowSettings();
      else g_open = false;
    };
  }
  p.buttons.push_back({h4::TrIn(UiLang(), "Cancel"), [back] {
                         if (back) ShowSettings();
                         else g_open = false;
                       }});
  ShowMessage(p.title, p.text, p.buttons);
}

#if defined(__ANDROID__)
void ShowAndroidSyncHelp() {
  g_panel.Set(560, UiLang(), [](Ui& ui) {
    ui.Anywhere([] { ShowSettings(); });
    ui.Title(ui.T("Setting the sync up"));
    ui.Paragraph(ui.T(
        "The saves are kept in step with a folder you pick once (the game makes ReHOMM4 in it): file by file, "
        "the newer copy wins, nothing is deleted.\n\nTo have them on your other devices too, sync that folder "
        "with any app:\n\n• Syncthing (Syncthing-Fork from Google Play or F-Droid): add a shared folder, "
        "say Documents/ReHOMM4, and share it with your other devices. ReHoMM4 on Linux and the Steam Deck "
        "syncs ~/ReHOMM4, on the Mac iCloud Drive/ReHOMM4: share it into that folder there.\n\n• A cloud "
        "(Google Drive, Dropbox, OneDrive…): FolderSync, Round Sync (rclone) or Autosync keep the folder "
        "in two-way sync with it. ReHoMM4 on Linux and the Mac takes the same cloud through rclone (the line "
        "rclone=<remote>:ReHOMM4 in its sync.conf).\n\n• Nextcloud: turn the folder's sync on in its app.\n\nThen "
        "tick \"Sync saved games\" and pick that folder. The ☁️ button in the menu shows the state and syncs "
        "at once."));
  });
  g_open = true;
}
#endif

// A path in lines that fit the panel: broken after its slashes (it has no spaces to wrap at).
std::string PathLines(const Ui& ui, const std::string& path) {
  std::string out, line;
  size_t i = 0;
  while (i < path.size()) {
    size_t j = path.find('/', i + 1);
    j = j == std::string::npos ? path.size() : j;
    const std::string part = path.substr(i, j - i);
    if (!line.empty() && ui.TextWidth(line + part) > ui.right() - ui.left()) {
      out += line + "\n";
      line.clear();
    }
    line += part;
    i = j;
  }
  return out + line;
}

// The folders: the saves (the synced one while the sync is on: the one the player keeps) and the
// game's, each with its path and (not on iOS: the Files app shows the app's own) Open.
void ShowFolders() {
  g_panel.Set(kSettingsW, UiLang(), [](Ui& ui) {
    ui.Close([] { ShowSettings(); });
    ui.Title(ui.T("Folders"));
    auto folder = [&ui](const char* title, const std::string& path) {
      if (path.empty()) return;
#if defined(H4_DRAWN_IOS)
      ui.Text(ui.left(), ui.y, ui.T(title), 0xFF3C2410);
      ui.Gap(ui.LineHeight() + 4);
#else
      ui.LabelButton(ui.T(title), ui.T("Open"), [path] { h4::native::RevealFolder(path); });
#endif
      ui.Paragraph(PathLines(ui, path), false, 0xFF6A5434);
      ui.Gap(6);
    };
    const std::string synced = h4::native::SyncedFolder();
    if (!synced.empty()) folder("Saves (synced)", synced);
    else folder("Saves", h4::native::SavesFolder());
    folder("Game folder", h4::GameFolder());
    ui.Buttons({{ui.T("Close"), [] { ShowSettings(); }}});
  });
  g_open = true;
}
#endif

// What's new: the newer releases' notes, newest first (none: this version's), in pages as tall as the frame lets them
// be (← →); Go to releases (the browser), Close.
void ShowWhatsNew() {
  FetchReleases();
  g_wn_page = 0;
  const recomp::gfx::Presentation fr = recomp::gfx::LastPresentation();
  const int fw = fr.src_w > 0 ? fr.src_w : 800, fh = fr.src_h > 0 ? fr.src_h : 600;
  g_panel.Set(std::min(720, fw - 40), UiLang(), [fh](Ui& ui) {
    g_wn_shown = g_wn;
    ui.Close([] { ShowSettings(); });
    ui.Title(ui.T("What's new?"));
    std::vector<std::pair<std::string, std::function<void()>>> bottom;
#if !defined(H4_SWITCH_APP)  // (no browser there)
    bottom.push_back({ui.T("Go to releases"), [] { h4::native::OpenUrl(kReleasesPage); }});
#endif
    bottom.push_back({ui.T("Close"), [] { ShowSettings(); }});
    // The newer releases', or (the latest here) this one's.
    const std::vector<Release> shown = !g_newer.empty() ? g_newer
                                       : g_current.tag.empty() ? std::vector<Release>{} : std::vector<Release>{g_current};
    if (g_wn != kWnReady || shown.empty()) {
      ui.Paragraph(g_wn == kWnFailed   ? ui.T("Could not reach GitHub. Try again later.")
                   : g_wn == kWnReady  ? h4::TrIn(ui.lang(), "You have the latest version: {1}.", {H4_VERSION_STRING})
                                       : std::string(ui.T("Checking GitHub…")),
                   true);
      ui.Buttons(bottom);
      return;
    }
    // The lines: each release's heading ("\x01" in front), then its notes wrapped at spaces (a
    // list item's next lines under its text).
    std::vector<std::string> lines;
    const int width = ui.right() - ui.left();
    for (const Release& r : shown) {
      if (!lines.empty()) lines.push_back("");
      lines.push_back("\x01ReHoMM4 " + r.tag);
      size_t i = 0;
      while (i <= r.text.size()) {
        size_t j = r.text.find('\n', i);
        if (j == std::string::npos) j = r.text.size();
        std::string rest = r.text.substr(i, j - i);
        i = j + 1;
        const size_t lead = std::min(rest.find_first_not_of(' '), rest.size());
        const size_t mark = rest.find(' ', lead);  // after "•" or "–"
        const bool item = rest.compare(lead, 3, "•") == 0 || rest.compare(lead, 3, "–") == 0;
        const std::string pad(item && mark != std::string::npos ? mark + 1 + lead : 0, ' ');
        if (rest.empty()) lines.push_back("");
        while (!rest.empty()) {
          size_t cut = rest.size();
          if (ui.TextWidth(rest) > width) {
            cut = std::string::npos;
            for (size_t at = rest.find(' ', lead + 1); at != std::string::npos; at = rest.find(' ', at + 1)) {
              if (ui.TextWidth(rest.substr(0, at)) > width) break;
              cut = at;
            }
            if (cut == std::string::npos) cut = rest.find(' ', lead + 1);
            if (cut == std::string::npos) cut = rest.size();
          }
          lines.push_back(rest.substr(0, cut));
          rest = cut < rest.size() ? pad + rest.substr(cut + 1) : "";
        }
      }
    }
    const int lh = ui.LineHeight() + 2;
    const int per = std::max(6, (fh - 40 - ui.y - 70) / lh);
    const int pages = std::max(1, int((lines.size() + size_t(per) - 1) / size_t(per)));
    g_wn_page = std::clamp(g_wn_page, 0, pages - 1);
    for (int k = 0; k < per; ++k) {
      const size_t n = size_t(g_wn_page * per + k);
      if (n < lines.size()) {
        const bool head = !lines[n].empty() && lines[n][0] == '\x01';
        ui.Text(ui.left(), ui.y, head ? lines[n].substr(1) : lines[n], head ? 0xFF7A2E0A : 0xFF3C2410);
      }
      ui.Gap(lh);
    }
    ui.Gap(4);
    std::vector<std::pair<std::string, std::function<void()>>> buttons;
    if (pages > 1) {
      buttons.push_back({"←", [] {
                           if (g_wn_page > 0) --g_wn_page;
                           g_panel.Invalidate();
                         }});
      buttons.push_back({std::to_string(g_wn_page + 1) + " / " + std::to_string(pages), [] {}});
      buttons.push_back({"→", [pages] {
                           if (g_wn_page < pages - 1) ++g_wn_page;
                           g_panel.Invalidate();
                         }});
    }
    buttons.insert(buttons.end(), bottom.begin(), bottom.end());
    ui.Buttons(buttons);
  });
  g_open = true;
}

// The gamepad's panel: the speeds, each button's action (a click: the next one, a right-click the
// previous), the preset (not on the Switch: its own pads only) and Reset.
void ShowGamepad() {
  g_panel.Set(kGamepadW, UiLang(), [](Ui& ui) {
    ui.Close([] { ShowSettings(); });
    ui.Title(std::string(ui.T("Gamepad")) + " — " + h4::PadStyleName(h4::PadStyle()));
    ui.Slider(ui.T("Pointer speed (left stick)"), h4::PadPointerSpeed(), h4::kPointerMin, h4::kPointerMax,
              [](float v) { h4::SetPadPointerSpeed(v); });
    ui.Slider(ui.T("Map scrolling speed (right stick)"), h4::PadScrollSpeed(), h4::kScrollMin, h4::kScrollMax,
              [](float v) { h4::SetPadScrollSpeed(v); });
    ui.Text(ui.left(), ui.y, ui.T("Buttons"), 0xFF7A2E0A, 20);
    ui.Gap(ui.LineHeight(20) + 6);
    // Two columns: the button's name, its action in a box.
    const int lh = ui.LineHeight(), rh = lh + 8, half = (h4::kPadButtons + 1) / 2;
    const int col = (ui.right() - ui.left() - 16) / 2;
    int nw = 40;  // the names' column: as wide as the longest
    for (int b = 0; b < h4::kPadButtons; ++b) nw = std::max(nw, ui.TextWidth(h4::PadButtonName(b, ui.lang())) + 12);
    for (int r = 0; r < half; ++r) {
      for (int c = 0; c < 2; ++c) {
        const int b = r + c * half;
        if (b >= h4::kPadButtons) continue;
        const int x = ui.left() + c * (col + 16);
        ui.Text(x, ui.y + (rh - lh) / 2, h4::PadButtonName(b, ui.lang()), 0xFF3C2410);
        ui.Box(x + nw, ui.y, x + col, ui.y + rh, h4::PadActionName(h4::PadBinding(b), ui.lang()), true, [b](bool right) {
          const int n = h4::PadActionCount();
          h4::SetPadBinding(b, (h4::PadBinding(b) + (right ? -1 : 1) + n) % n);
        });
      }
      ui.Gap(rh + 4);
    }
    ui.Gap(4);
    ui.Paragraph(ui.T("With a gamepad: click presses or takes a list's next item, right-click its previous one, Esc closes."));
#if !defined(__SWITCH__)
    {
      std::vector<std::string> styles;
      for (int s = 0; s < h4::kPadStyles; ++s) styles.push_back(h4::PadStyleName(s));
      ui.Dropdown("preset", ui.T("Preset"), styles, h4::PadStyle(), [](int s) { h4::ApplyPadPreset(s); });
    }
#endif
    ui.Buttons({{ui.T("Reset to the defaults"), [] { h4::ResetPadConfig(); }}});
  });
  g_open = true;
}

void ShowSettings() {
  g_panel.Set(kSettingsW, UiLang(), [](Ui& ui) {
    ui.Close(Close);
    ui.Title(std::string(ui.T("Settings")) + " ReHoMM4 v" H4_VERSION_STRING);
    FetchReleases();  // (once: tells whether there is a newer version)
    {  // What's new?; the update check ("Update available" in another colour when there is one)
      const bool newer = g_wn == kWnReady && !g_newer.empty();
      std::vector<std::pair<std::string, std::function<void()>>> row = {{ui.T("What's new?"), ShowWhatsNew}};
#if defined(H4_UPDATES)
      row.push_back({ui.T(newer ? "Update available" : "Check for updates"), [] {
                       g_upd = kChecking;
                       ShowUpdate();
                       CheckForUpdates();
                     }});
      ui.InfoButtons("", row, newer ? 1 : -1);
#else
      ui.InfoButtons("", row);
#endif
      g_wn_shown = g_wn;
    }
#if !defined(H4_WEB_APP)  // (the browser: the game's language is the page's, index.html)
    if (!g_saved.empty()) {
      const std::vector<h4::LanguageInfo> all = h4::Languages();
      std::vector<std::string> names;
      int selected = 0;
      for (size_t i = 0; i < all.size(); ++i) {
        names.push_back(all[i].name);
        if (all[i].code == g_next) selected = int(i);
      }
      ui.Dropdown("lang", ui.T("Game language"), names, selected, [all](int i) {
        if (all[size_t(i)].code != g_next) PickLanguage(all[size_t(i)].code);
      });
    } else if (const std::string gog = h4::GogLanguage(); !gog.empty()) {
      ui.Value(ui.T("Game language"), h4::GogLanguageName(gog) + " (GOG)");  // nothing to choose
    }
#endif
    {  // the interface's language: as the game's (the default) or another (ui_text.h); at once
      const std::vector<std::string> choices = h4::UiLanguageChoices();
      std::vector<std::string> names;
      int selected = 0;
      for (size_t i = 0; i < choices.size(); ++i) {
        names.push_back(h4::UiLanguageChoiceName(choices[i], g_game_lang));
        if (choices[i] == h4::UiLanguageChoice()) selected = int(i);
      }
      ui.Dropdown("ui", ui.T("Interface language"), names, selected, [choices](int i) {
        h4::SetUiLanguageChoice(choices[size_t(i)]);
        ShowSettings();  // again, in it
      });
    }
    {  // the scaling filter (F10): at once
      const std::vector<std::string> ids = h4::FilterIds();
      std::vector<std::string> names;
      int selected = 0;
      for (size_t i = 0; i < ids.size(); ++i) {
        names.push_back(h4::FilterName(ids[i], ui.lang()));
        if (ids[i] == h4::CurrentFilter()) selected = int(i);
      }
      ui.Dropdown("filter", ui.T("Scaling"), names, selected, [ids](int i) { h4::SetFilter(ids[size_t(i)]); });
    }
#if defined(H4_NATIVE_SYNC)
    {  // who draws the frame (Auto, the GPU, the CPU): from the next start
      const std::vector<std::string> choices = h4::RendererChoices();
      std::vector<std::string> names;
      int selected = 0;
      for (size_t i = 0; i < choices.size(); ++i) {
        names.push_back(h4::RendererChoiceName(choices[i], ui.lang()));
        if (choices[i] == h4::RendererChoice()) selected = int(i);
      }
      ui.Dropdown("renderer", ui.T("Renderer"), names, selected, [choices](int i) {
        h4::SetRenderer(choices[size_t(i)]);
        ShowSettings();  // again: "(from the next start)" on it
      });
    }
#endif
    ui.Check(ui.T("Skip the intro movies (from the next start)"), h4::SkipIntro(),
             [] { h4::SetSkipIntro(!h4::SkipIntro()); });
    ui.Check(ui.T("Black side panels (from the next start)"), h4::ReadBlackSidePanels(h4::SidePanelsPath()),
             [] { h4::SaveBlackSidePanels(!h4::ReadBlackSidePanels(h4::SidePanelsPath())); });
#if defined(H4_SYNC_HELP)
    ui.LabelButton(ui.T("Saved games sync"), "?", ShowSyncHelp);
#endif
#if defined(H4_NATIVE_SYNC)
    ui.Check(ui.T("Sync saved games (the ☁️ button in the menu)"), h4::native::SyncOn(), [] {
      const bool on = !h4::native::SyncOn();
      h4::native::SetSync(on);  // (on: may open the system's folder picker; on once one is picked)
#if defined(H4_DRAWN_MAC)
      if (on && !h4::native::SyncOn()) ShowSyncPanel(true);  // (no iCloud Drive: said, another folder offered)
#endif
    });
#if defined(__ANDROID__)
    ui.LabelButton(ui.T("Setting the sync up"), "?", ShowAndroidSyncHelp);
#endif
#endif
    ui.LabelButton(ui.T("Gamepad"), ui.T("Set up"), ShowGamepad);
#if defined(H4_NATIVE_SYNC)
    ui.LabelButton(ui.T("Folders"), ui.T("Show"), ShowFolders);
#endif
  });
  g_open = true;
}

}  // namespace

namespace h4 {

void SetLanguageChoice(const std::string& lang, const std::string& saved_path) {
  std::lock_guard<std::mutex> l(g_mu);
  g_lang = lang;
  g_saved = saved_path;
  g_next = lang;
  g_game_lang = lang.empty() ? "en" : lang.substr(0, lang.find('-'));
}

void ShowLanguageButton(bool shown, int x, int y, int /*left: iOS only*/) {
  // The window's size, before g_mu: SDL's video calls go to the main thread in the browser, which
  // may be waiting in OnInput for g_mu.
  int ww = 0, wh = 0;
  if (SDL_Window** wins = SDL_GetWindows(nullptr)) {
    if (wins[0]) SDL_GetWindowSize(wins[0], &ww, &wh);
    SDL_free(wins);
  }
  std::unique_lock<std::mutex> l(g_mu);
  if (g_lang.empty()) return;
  if (!DialogArt()) return;  // (the game's dialog art and font: read on a thread, soon after the start)
  if (!g_hooked) {
    g_hooked = true;
    h4::SetMenuInputHook(OnInput);  // (before the game: what the settings take, it does not get)
    SDL_AddEventWatch(OnWindowEvent, nullptr);
    recomp::SetPadEventHook(PadEvent);
  }
  {  // the buttons: ☁️ while the sync is on, ☕, ⚙
    std::vector<std::string> icons;
#if defined(H4_NATIVE_SYNC)
    if (h4::native::SyncOn()) icons.push_back("sync");
#endif
#if defined(H4_KOFI)
    icons.push_back("kofi");
#endif
    icons.push_back("settings");
    // The tooltip: the icon the pointer has rested on for 0.4 s (not right after a click).
    std::string tip;
    int under = -1;
    float tx = -1, ty = -1;
    if (g_shown_pic == &g_gear && g_point_ms && SDL_GetTicks() - g_point_ms >= 400 &&
        ToPicture(g_point_x, g_point_y, g_gear.w, g_gear.h, &tx, &ty) && ty >= 0 && ty < kGearSide) {
      const int rx = int(tx) - g_icons_x;
      const int i = rx >= 0 ? rx / (kGearSide + kGearGap) : -1;
      if (i >= 0 && size_t(i) < g_icons.size() && rx - i * (kGearSide + kGearGap) < kGearSide) {
        under = i;
        tip = g_icons[size_t(i)];
      }
    }
    if (icons != g_icons || g_gear.argb.empty() || tip != g_tip) {
      g_icons = icons;
      g_tip = tip;
      std::vector<std::string> glyphs;
      for (const std::string& i : icons) glyphs.push_back(i == "sync" ? "☁" : i == "kofi" ? "☕" : "⚙");
      const std::string lang = UiLang();
      const std::string text = tip.empty()       ? ""
                               : tip == "sync"   ? h4::TrIn(lang, "Save sync")
                               : tip == "kofi"   ? h4::TrIn(lang, "Support the author on Ko-fi")
                                                 : std::string(h4::TrIn(lang, "Settings")) + " ReHoMM4";
      g_gear = app_ui::IconRow(glyphs, kGearSide, kGearGap, text, under, lang, &g_icons_x);
      if (g_shown_pic == &g_gear) g_shown_pic = nullptr;  // (given to the frame again)
    }
  }
#if defined(H4_NATIVE_SYNC)
  {  // the sync turned on or off (the system's folder picker answered): the settings show it
    static bool sync_was = h4::native::SyncOn();
    if (h4::native::SyncOn() != sync_was) {
      sync_was = !sync_was;
      if (g_open) g_panel.Invalidate();
    }
  }
#endif
#if defined(H4_DRAWN_IOS)
  if (shown && !g_open && h4::AskSaveSyncFolder()) ShowSyncPanel(false);  // the first menu: where to sync
#endif
  if (!shown) g_open = false;
  if (static bool tested = false; shown && !tested) {  // H4_TEST_SETTINGS=<panel>: open at the first main menu
    tested = true;
    if (const char* t = std::getenv("H4_TEST_SETTINGS")) {
      const std::string p = t;
      if (p == "gamepad") ShowGamepad();
      else if (p == "whatsnew") ShowWhatsNew();
#if defined(H4_NATIVE_SYNC)
      else if (p == "folders") ShowFolders();
      else if (p == "sync") ShowSyncPanel(false);
#endif
#if defined(H4_UPDATES)
      else if (p == "update") {
        g_upd = kChecking;
        ShowUpdate();
        CheckForUpdates();
      }
#endif
      else ShowSettings();
    }
  }
  if (g_back) {  // Esc (a gamepad's): an open list closed, else the panel back to the settings or shut
    g_back = false;
    if (g_open && !g_panel.Back()) {
      g_open = false;
    }
  }
  const h4::app_ui::Picture* pic_now = g_open ? &g_panel.Draw() : &g_gear;
  float px = -1, py = -1;
  if (g_pressed) {
    g_pressed = false;
    if (g_open && ToPicture(g_press_x, g_press_y, pic_now->w, pic_now->h, &px, &py)) g_panel.Press(int(px), int(py));
  }
  if (g_drag_x >= 0) {
    if (g_open && g_panel.holding() && ToPicture(g_drag_x, 0, pic_now->w, pic_now->h, &px, &py)) g_panel.Drag(int(px));
    g_drag_x = -1;
  }
  if (g_released) {
    g_released = false;
    g_panel.Release();
  }
  if (g_clicked) {
    g_clicked = false;
    const bool in = ToPicture(g_click_x, g_click_y, pic_now->w, pic_now->h, &px, &py) && px >= 0 && py >= 0 &&
                    px < pic_now->w && py < pic_now->h;
    if (!g_open) {
      const int rx = int(px) - g_icons_x;
      const size_t i = in && rx >= 0 && py < kGearSide && rx % (kGearSide + kGearGap) < kGearSide
                           ? size_t(rx / (kGearSide + kGearGap))
                           : g_icons.size();
      const std::string icon = i < g_icons.size() ? g_icons[i] : "";
      if (icon == "kofi") h4::native::OpenUrl("https://ko-fi.com/hazzard");  // in the browser
#if defined(H4_NATIVE_SYNC)
      else if (icon == "sync") ShowSyncPanel(false);
#endif
      else if (icon == "settings") ShowSettings();
    } else if (!g_panel.Click(in ? int(px) : -1, in ? int(py) : -1, g_click_right)) {
      g_open = false;  // beside the panel: shut
    }
  }
#if defined(H4_UPDATES)
  if (g_open && g_upd != g_upd_shown) g_panel.Invalidate();  // the update check's thread got further
#endif
  if (g_open && g_wn != g_wn_shown) g_panel.Invalidate();  // the releases came

#if defined(H4_RESTART)
  if (g_restart_now) {
    g_restart_now = false;
    l.unlock();
    h4::RestartApp();  // the app again, with it (back here only when that failed: the note stays)
    l.lock();
  }
#endif
  const h4::app_ui::Picture* pic = !shown ? nullptr : g_open ? &g_panel.Draw() : &g_gear;
  int fx = 0, fy = y;
  if (pic) {
    const recomp::gfx::Presentation p = recomp::gfx::LastPresentation();
    if (pic == &g_gear) {  // its top-right corner at (x, y), kept inside the frame
      fx = std::max(4, std::min(x - pic->w, (p.src_w > 0 ? p.src_w : x + pic->w) - pic->w - 4));
    } else if (p.src_w > 0 && p.src_h > 0) {  // a panel: in the middle
      fx = std::max(0, (p.src_w - pic->w) / 2);
      fy = std::max(0, std::min((p.src_h - g_panel.base_height()) / 2, p.src_h - pic->h));
    }
    if (ww > 0 && wh > 0) {
      float l0, t0, r0, b0;
      FrameToView(fx, fy, float(ww), float(wh), &l0, &t0);
      FrameToView(fx + pic->w, fy + pic->h, float(ww), float(wh), &r0, &b0);
      g_hit[0] = l0, g_hit[1] = t0, g_hit[2] = r0, g_hit[3] = b0;
    }
  }
  static uint64_t given = 0;  // the panel's drawing last given to the frame
  const uint64_t now = pic == &g_gear ? 0 : g_panel.version();
  bool wake = pic != g_shown_pic || g_resized.exchange(false);
  if (pic && (pic != g_shown_pic || now != given)) {
    recomp::gfx::SetPresentBadge(pic->argb.data(), pic->w, pic->h);
    given = now;
    wake = true;
  }
  recomp::gfx::SetPresentBadgePos(fx, fy, pic != nullptr);
  g_shown_pic = pic;
  if (wake) {  // shown now: the menu, idle, draws nothing new
    l.unlock();  // (showing may wait for the main thread, which may be waiting in OnInput for g_mu)
    recomp::hle::PresentAgain();
  }
}

}  // namespace h4
