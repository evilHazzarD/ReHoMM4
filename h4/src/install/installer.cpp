#include "install/installer.h"

#include <SDL3/SDL.h>
#if defined(__EMSCRIPTEN__)
#include <emscripten.h>
#endif
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#if defined(_WIN32)
#ifndef O_CLOEXEC
#define O_CLOEXEC 0
#endif
// POSIX's: an existing variable kept unless `overwrite`.
static int setenv(const char* name, const char* value, int overwrite) {
  if (!overwrite && std::getenv(name)) return 0;
  return _putenv_s(name, value);
}
#endif
#if defined(__APPLE__)
#include <copyfile.h>
#endif

#include <algorithm>
#include <atomic>
#include <cmath>
#include <cctype>
#include <filesystem>
#include <functional>
#include <map>
#include <memory>
#include <set>
#include <string>
#include <thread>
#include <utility>
#include <vector>

#include "install/gog.h"
#include "install/hash.h"
#include "install/inno.h"
#include "install/manifest.h"
#include "install/platform.h"
#include "install/task.h"

namespace h4::install {

namespace {

constexpr uint64_t kMB = 1000 * 1000;
// In a game folder installed from GOG in another language than English: that language's code.
constexpr const char* kGameLanguage = "rehomm4-language";
constexpr uint64_t kSlack = 64 * kMB;  // free space kept beyond the game

enum class Screen { Menu, Picking, Busy, Error, Done, Language };

struct Button {
  SDL_FRect r{};
  std::string label;
  int id = 0;
};

struct Ui {
  SDL_Window* win = nullptr;
  SDL_Renderer* ren = nullptr;
  float w = 0, h = 0, s = 1;           // output pixels, text scale
  SDL_FRect safe{};                     // usable area (notch, rounded corners)
  std::vector<Button> buttons;

  float Ch() const { return 8 * s; }    // character cell
  void Measure() {
    int iw = 0, ih = 0;
    SDL_GetRenderOutputSize(ren, &iw, &ih);
    w = float(iw), h = float(ih);
    static bool logged = false;
    if (!logged) fprintf(stderr, "installer: drawing on %dx%d\n", iw, ih), fflush(stderr), logged = true;
    SDL_Rect sa{};
    const float d = SDL_GetWindowPixelDensity(win) > 0 ? SDL_GetWindowPixelDensity(win) : 1.0f;
    if (SDL_GetWindowSafeArea(win, &sa) && sa.w > 0 && sa.h > 0)
      safe = {sa.x * d, sa.y * d, sa.w * d, sa.h * d};
    else
      safe = {0, 0, w, h};
    // Text about 10 points high whatever the screen's density, but at least 40 columns across
    // (a phone held upright).
    const float scale = SDL_GetWindowDisplayScale(win) > 0 ? SDL_GetWindowDisplayScale(win) : d;
#if defined(__SWITCH__)
    // A TV or the handheld screen: text by the height (3 at 1080p, 2 at 720p), lines at most
    // 64 columns (the whole width of a TV is too long to read).
    (void)scale;
    s = std::max(2.0f, std::round(h / 360.0f));
    if (safe.w > 64 * Ch()) safe = {safe.x + (safe.w - 64 * Ch()) / 2, safe.y, 64 * Ch(), safe.h};
#else
    s = std::max(2.0f, std::min(std::round(10.0f * scale / 8.0f), std::floor(safe.w / (42 * 8.0f))));
#endif
    const float m = 1.5f * Ch();
    safe = {safe.x + m, safe.y + m, safe.w - 2 * m, safe.h - 2 * m};
  }
  void Color(uint8_t r, uint8_t g, uint8_t b) { SDL_SetRenderDrawColor(ren, r, g, b, 255); }
  // Text at (x, y) in pixels, scale `k` times the base scale.
  void Text(float x, float y, const std::string& t, float k = 1) {
    SDL_SetRenderScale(ren, s * k, s * k);
    SDL_RenderDebugText(ren, x / (s * k), y / (s * k), t.c_str());
    SDL_SetRenderScale(ren, 1, 1);
  }
  void Centered(float y, const std::string& t, float k = 1) {
    Text(safe.x + (safe.w - float(t.size()) * Ch() * k) / 2, y, t, k);
  }
  int Columns(float k = 1) const { return std::max(10, int(safe.w / (Ch() * k))); }
  // Lines of `t` wrapped at `cols` (on spaces; "\n" breaks).
  static std::vector<std::string> Wrap(const std::string& t, int cols) {
    std::vector<std::string> out;
    std::string line, word;
    auto flush_word = [&] {
      if (word.empty()) return;
      if (!line.empty() && int(line.size() + 1 + word.size()) > cols) out.push_back(line), line.clear();
      while (int(word.size()) > cols) out.push_back(word.substr(0, cols)), word.erase(0, cols);
      line += (line.empty() ? "" : " ") + word;
      word.clear();
    };
    for (char c : t) {
      if (c == '\n') { flush_word(); out.push_back(line); line.clear(); }
      else if (c == ' ') flush_word();
      else word.push_back((unsigned char)c < 0x80 ? c : '?');  // the debug font is ASCII
    }
    flush_word();
    if (!line.empty()) out.push_back(line);
    return out;
  }
  // Wrapped, centred paragraph from y; returns the y below it.
  float Paragraph(float y, const std::string& t, float k = 1) {
    for (const std::string& l : Wrap(t, Columns(k))) {
      Centered(y, l, k);
      y += Ch() * k * 1.5f;
    }
    return y;
  }
  void AddButton(int id, float y, const std::string& label, float width_chars = 0) {
    const float bw = std::min(safe.w, (width_chars ? width_chars : float(label.size()) + 6) * Ch());
    const float bh = Ch() * 3.2f;
    Button b{{safe.x + (safe.w - bw) / 2, y, bw, bh}, label, id};
    buttons.push_back(b);
  }
  void DrawButtons() {
    for (const Button& b : buttons) {
      Color(70, 52, 22);
      SDL_RenderFillRect(ren, &b.r);
      Color(214, 172, 86);
      SDL_RenderRect(ren, &b.r);
      SDL_FRect in{b.r.x + 2, b.r.y + 2, b.r.w - 4, b.r.h - 4};
      SDL_RenderRect(ren, &in);
      Color(250, 236, 200);
      Text(b.r.x + (b.r.w - float(b.label.size()) * Ch()) / 2, b.r.y + (b.r.h - Ch()) / 2, b.label);
    }
  }
  int Hit(float x, float y) const {
    for (const Button& b : buttons)
      if (x >= b.r.x && x < b.r.x + b.r.w && y >= b.r.y && y < b.r.y + b.r.h) return b.id;
    return 0;
  }
};

// The Switch's SDL loads EGL only for an OpenGL window (its only renderer is OpenGL ES): without
// the flag the window fails ("EGL not initialized").
#if defined(__SWITCH__)
// It also wants the window's size (a fullscreen one of 0x0 gets no drawable area).
constexpr SDL_WindowFlags kGlWindow = SDL_WINDOW_OPENGL;
#else
constexpr SDL_WindowFlags kGlWindow = 0;
#endif

// The window's size: on the Switch the screen's (1920x1080 docked, 1280x720 handheld); elsewhere
// a fullscreen window takes it by itself.
SDL_Window* CreateUiWindow() {
  int w = 0, h = 0;
#if defined(__SWITCH__)
  w = 1280, h = 720;
  if (const SDL_DisplayMode* m = SDL_GetDesktopDisplayMode(SDL_GetPrimaryDisplay())) w = m->w, h = m->h;
#endif
  return SDL_CreateWindow("ReHoMM4", w, h,
                          SDL_WINDOW_FULLSCREEN | SDL_WINDOW_HIGH_PIXEL_DENSITY | SDL_WINDOW_RESIZABLE | kGlWindow);
}

// "sdmc:/switch/..." as the player sees it on the card: "/switch/...".
[[maybe_unused]] std::string SdPath(const std::string& p) {
  return p.rfind("sdmc:", 0) == 0 ? p.substr(5) : p;
}

std::string Megabytes(uint64_t b) { return std::to_string((b + kMB / 2) / kMB) + " MB"; }

enum { kGog = 1, kFiles, kFolder, kCancel, kBack, kSignOut, kQuit, kLanguage0 = 100 };

class Installer {
 public:
  Installer(const std::string& files, const std::string& autostart)
      : files_(files), autostart_(autostart) {}

  std::string Run() {
    PlatformInit();
    if (!SDL_InitSubSystem(SDL_INIT_VIDEO)) return "";
    // This screen turns with the device (the game's window is landscape only: main.cpp).
    SDL_SetHint(SDL_HINT_ORIENTATIONS, "LandscapeLeft LandscapeRight Portrait PortraitUpsideDown");
    ui_.win = CreateUiWindow();
    if (!ui_.win) fprintf(stderr, "installer: SDL_CreateWindow: %s\n", SDL_GetError());
    ui_.ren = ui_.win ? SDL_CreateRenderer(ui_.win, nullptr) : nullptr;
    if (!ui_.ren) {
      // What SDL had to draw with (a report from a device: the video driver, its renderers).
      std::string drivers;
      for (int i = 0; i < SDL_GetNumRenderDrivers(); i++) drivers += std::string(" ") + SDL_GetRenderDriver(i);
      const char* video = SDL_GetCurrentVideoDriver();
      fprintf(stderr, "installer: video driver %s, render drivers:%s\n", video ? video : "(none)",
              drivers.empty() ? " (none)" : drivers.c_str());
      fprintf(stderr, "installer: no window: %s\n", SDL_GetError());
      if (ui_.win) SDL_DestroyWindow(ui_.win);
      SDL_QuitSubSystem(SDL_INIT_VIDEO);
      return "";
    }
    SDL_SetRenderVSync(ui_.ren, 1);
    SDL_DisableScreenSaver();  // a download takes minutes
    std::string result;
    if (autostart_ == "files-fd") setenv("H4_TEST_FDS", "1", 1), autostart_ = "files";
    if (autostart_ == "files") OnPress(kFiles);
    if (autostart_ == "gog") OnPress(kGog);
    if (autostart_.rfind("folder:", 0) == 0) {  // test: that folder as if picked (desktop)
      const std::string root = autostart_.substr(7);
      std::vector<PickedFile> files;
      std::error_code ec;
      for (std::filesystem::recursive_directory_iterator it(root, ec), end; !ec && it != end; it.increment(ec))
        if (it->is_regular_file())
          files.push_back({it->path().string(), it->path().lexically_relative(root).generic_string()});
      StartCopy(files);
    }
    while (!quit_ && result.empty()) {
      SDL_Event e;
      while (SDL_PollEvent(&e)) {
        if (e.type == SDL_EVENT_QUIT) quit_ = true;
        if (Scroll(e)) continue;
        if (e.type == SDL_EVENT_MOUSE_BUTTON_UP) {
          SDL_ConvertEventToRenderCoordinates(ui_.ren, &e);
          OnPress(ui_.Hit(e.button.x, e.button.y));
        }
        if (e.type == SDL_EVENT_KEY_DOWN && e.key.key == SDLK_AC_BACK && screen_ == Screen::Busy)
          OnPress(kCancel);
      }
      if (screen_ == Screen::Picking) PollPicked();
      if (screen_ == Screen::Busy && finished_) Finish();
      if (screen_ == Screen::Done) result = files_ + "/game";
      Draw();
      SDL_Delay(16);
    }
    if (worker_.joinable()) {
      task_->cancel = true;
      worker_.join();
    }
    SDL_EnableScreenSaver();
    SDL_DestroyRenderer(ui_.ren);
    SDL_DestroyWindow(ui_.win);
    SDL_QuitSubSystem(SDL_INIT_VIDEO);
    return result;
  }

 private:
  std::string Download() const { return files_ + "/game.download"; }
  std::string Unpack() const { return files_ + "/game.unpack"; }

  void OnPress(int id) {
    if (!id) return;
    if (id == kQuit) {  // an unfinished job stops (Run: cancelled, joined); a download continues next time
      fprintf(stderr, "installer: closed (%s)\n",
              screen_ == Screen::Menu ? "at the menu" : screen_ == Screen::Picking ? "waiting for picked files"
              : screen_ == Screen::Busy ? "while busy" : screen_ == Screen::Error ? "at an error" : "elsewhere");
      quit_ = true;
      return;
    }
    if (screen_ == Screen::Menu && id == kGog) StartGog();
    else if (screen_ == Screen::Menu && id == kFiles) StartFiles();
    else if (screen_ == Screen::Menu && id == kFolder) StartFolder();
    else if (screen_ == Screen::Menu && id == kSignOut) { session_.SignOut(); RemoveTree(Download()); }
    else if (screen_ == Screen::Busy && id == kCancel) task_->cancel = true;
    else if (screen_ == Screen::Error && id == kBack) screen_ = Screen::Menu;
    else if (screen_ == Screen::Language && id == kBack) screen_ = Screen::Menu;
    else if (screen_ == Screen::Language && id >= kLanguage0 && size_t(id - kLanguage0) < gog_langs_.size())
      StartGogDownload(gog_langs_[size_t(id - kLanguage0)]);
  }

  void StartGog() {
    std::string err, code;
    if (!session_.Resume(&err)) {
      code = GogLoginCode(gog::LoginUrl());
      FreshRenderer();
      if (code.empty()) return;  // closed the login page
    }
    Start([this, code](Task* t, std::string* error) -> bool {
      if (!code.empty()) {
        t->SetPhase("Signing in to GOG");
        if (!session_.SignIn(code, error)) return false;
      }
      t->SetPhase("Checking your GOG library");
      bool owns = false;
      if (!session_.OwnsGame(&owns, error)) return false;
      if (!owns) {
        *error = "This GOG account does not own Heroes of Might and Magic IV: Complete. "
                 "Sign in with the account you bought it on (\"Forget GOG sign-in\" below).";
        return false;
      }
      t->SetPhase("Getting the game's languages from GOG");
      std::vector<std::string> langs;
      if (!session_.Languages(&langs, error)) return false;
      gog_langs_ = langs;
      return true;
    }, "");
    listing_languages_ = true;
  }

  // After the login page (another activity, the main thread waiting in Java meanwhile): Android
  // took the window's surface away, and the renderer drew into the dead one (a black screen) while
  // the download went on. The events that came meanwhile handled, then a renderer made again
  // (nothing of it kept: no textures).
  void FreshRenderer() {
#if defined(__ANDROID__)
    SDL_PumpEvents();
    SDL_FlushEvents(SDL_EVENT_FIRST, SDL_EVENT_LAST);
    SDL_DestroyRenderer(ui_.ren);
    ui_.ren = SDL_CreateRenderer(ui_.win, nullptr);
    if (!ui_.ren) {
      fprintf(stderr, "installer: no renderer after the login page: %s\n", SDL_GetError());
      quit_ = true;
      return;
    }
    SDL_SetRenderVSync(ui_.ren, 1);
#endif
  }

  // The game in a language GOG has (gog::Session::Languages): English as it is, another one as its
  // data with the English heroes4.exe; that language is marked in the game folder (kGameLanguage).
  void StartGogDownload(const std::string& lang) {
    fprintf(stderr, "installer: downloading the game from GOG in %s\n", lang.c_str());
    Start([this, lang](Task* t, std::string* error) -> bool {
      if (!EnoughSpace(Download(), error)) return false;
      if (!session_.Download(Download(), lang, t, error)) return false;
      const std::string marker = Download() + "/" + kGameLanguage;
      std::remove(marker.c_str());
      // Every chunk was checked by its MD5 as it came (gog.cpp): here only heroes4.exe.
      if (!CheckExe(Download(), t, error)) return false;
      if (lang != "en-US") {
        if (FILE* f = fopen(marker.c_str(), "w")) {
          fputs(lang.substr(0, 2).c_str(), f);
          fclose(f);
        }
      }
      session_.SignOut();  // not needed any more
      return true;
    }, Download());
  }

  // GOG's languages by name (the installer's font is ASCII only).
  static std::string LanguageName(const std::string& code) {
    static const std::pair<const char*, const char*> kNames[] = {
        {"en-US", "English"},          {"fr-FR", "French (Francais)"}, {"de-DE", "German (Deutsch)"},
        {"es-ES", "Spanish (Espanol)"}, {"it-IT", "Italian (Italiano)"}, {"pl-PL", "Polish (Polski)"},
        {"ru-RU", "Russian"},          {"cs-CZ", "Czech (Cestina)"}};
    for (const auto& [c, n] : kNames)
      if (code == c) return n;
    return code;
  }

  void StartFiles() {
    // The installer in the app's folder (copied with Finder or adb), else
    // the player picks it (the picker is polled from the main loop: PollPicked).
    std::vector<PickedFile> files = inno::FilesIn(files_);
    PickedFile exe, bin;
    std::string err;
#if defined(H4_DESKTOP_APP)
    // Else where a browser saves them (both files from one folder: the app's own comes first).
    const char* home = getenv("HOME");
    if (home && !inno::FindInstaller(files, &exe, &bin, &err))
      files = inno::FilesIn(std::string(home) + "/Downloads");
#endif
    picked_ = false;
    own_installer_.clear();
    if (inno::FindInstaller(files, &exe, &bin, &err)) {
      // Copied into the app's own folder (Finder/Files on iOS, adb on Android, the SD card on the
      // Switch): removed once the game is installed (about 1 GB). Never those elsewhere (~/Downloads).
      for (const PickedFile* f : {&exe, &bin})
        if (f->path.rfind(files_ + "/", 0) == 0) own_installer_.push_back(f->path);
      if (getenv("H4_TEST_FDS")) {  // test: read them as picked files are (/proc/self/fd/N)
        for (PickedFile* f : {&exe, &bin}) {
          const int fd = open(f->path.c_str(), O_RDONLY | O_CLOEXEC);
          if (fd >= 0) f->path = "/proc/self/fd/" + std::to_string(fd);
        }
      }
      StartUnpack(exe, bin, EnglishExe(files));
      return;
    }
    if (BeginPickFiles()) screen_ = Screen::Picking;
    else ShowError("The file picker could not be opened.");
  }

  // An installed game: the player picks its folder (PollPicked), it is copied in.
  void StartFolder() {
    picking_folder_ = true;
    if (BeginPickFiles(true)) screen_ = Screen::Picking;
    else ShowError("The folder picker could not be opened.");
  }

  void PollPicked() {
    std::vector<PickedFile> files;
    if (!PickedFiles(&files)) return;
    if (files.empty()) {  // cancelled
      picking_folder_ = false;
      screen_ = Screen::Menu;
      return;
    }
    if (picking_folder_) {
      picking_folder_ = false;
      picked_ = true;
      own_installer_.clear();
      StartCopy(files);
      return;
    }
    PickedFile exe, bin;
    std::string err;
    picked_ = true;
    own_installer_.clear();  // picked: the player's files, where they keep them
    if (!inno::FindInstaller(files, &exe, &bin, &err)) {
      EndPickedFiles();
      picked_ = false;
      ShowError(err);
      return;
    }
    StartUnpack(exe, bin, EnglishExe(files));
  }

  // heroes4.exe among the installer's files (put beside it, or picked with it): the English one,
  // for an installer in another language (StartUnpack). None: an empty path.
  static PickedFile EnglishExe(const std::vector<PickedFile>& files) {
    for (const PickedFile& f : files) {
      std::string n = f.name;
      for (char& c : n) c = char(std::tolower(static_cast<unsigned char>(c)));
      if (n == "heroes4.exe") return f;
    }
    return {};
  }

  // The language of an installer in another language than English, by its file's name (GOG's
  // names say it: "..._(french)_..."); "xx" when it does not (GogLanguageName: "Own").
  static std::string InstallerLanguage(const std::string& name) {
    std::string n = name;
    for (char& c : n) c = char(std::tolower(static_cast<unsigned char>(c)));
    static const std::pair<const char*, const char*> kWords[] = {
        {"french", "fr"}, {"german", "de"},  {"spanish", "es"}, {"italian", "it"},
        {"polish", "pl"}, {"russian", "ru"}, {"czech", "cs"}};
    for (const auto& [w, code] : kWords)
      if (n.find(w) != std::string::npos) return code;
    return "xx";
  }

  // The picked folder's files into Unpack(), under the English install's spelling (the folder may
  // be the game's or one above it: where heroes4.exe is). Its archives may hold a translation (this
  // app's, applied in place), so only heroes4.exe is checked by its SHA-256 (the app's code is made
  // from it) and every other file of the install by being there.
  void StartCopy(const std::vector<PickedFile>& files) {
    fprintf(stderr, "installer: copying an installed game (%zu files)\n", files.size());
    Start([this, files](Task* t, std::string* error) -> bool {
      namespace fs = std::filesystem;
      auto lower = [](std::string s) {
        for (char& c : s) c = char(std::tolower(static_cast<unsigned char>(c)));
        return s;
      };
      t->SetPhase("Looking at the folder");
      std::string root;  // the folder of heroes4.exe, inside the picked one ("" or "a/b/")
      bool found = false;
      for (const PickedFile& f : files) {
        const std::string n = lower(f.name);
        if (n == "heroes4.exe" || (n.size() > 12 && n.compare(n.size() - 12, 12, "/heroes4.exe") == 0))
          if (!found || f.name.size() < root.size() + 11) root = f.name.substr(0, f.name.size() - 11), found = true;
      }
      if (!found) {
        *error = "There is no heroes4.exe in this folder. Pick the folder Heroes IV is installed in.";
        return false;
      }
      // The install's spelling of every file and folder, by its lower case.
      std::map<std::string, std::string> spelled;
      for (const ManifestFile& m : Manifest())
        for (size_t at = 0; at != std::string::npos;) {
          at = std::string(m.path).find('/', at + 1);
          const std::string part = std::string(m.path).substr(0, at);
          spelled[lower(part)] = part;
        }
      auto respell = [&](const std::string& rel) {
        std::string out;
        for (size_t at = 0; at <= rel.size();) {
          size_t slash = rel.find('/', at);
          if (slash == std::string::npos) slash = rel.size();
          const std::string whole = out + rel.substr(at, slash - at);
          auto it = spelled.find(lower(whole));
          out = it != spelled.end() ? it->second : whole;
          if (slash < rel.size()) out += "/";
          at = slash + 1;
        }
        return out;
      };
      std::vector<std::pair<std::string, std::string>> todo;  // source, path inside the game
      uint64_t total = 0;
      for (const PickedFile& f : files) {
        if (f.name.size() <= root.size() || f.name.compare(0, root.size(), root) != 0) continue;
        const std::string rel = f.name.substr(root.size());
        if (rel.find("..") != std::string::npos) continue;
        struct stat st;
        if (stat(f.path.c_str(), &st) == 0) total += uint64_t(st.st_size);
        todo.push_back({f.path, respell(rel)});
      }
      std::set<std::string> have;
      for (const auto& f : todo) have.insert(lower(f.second));
      for (const ManifestFile& m : Manifest())
        if (!have.count(lower(m.path))) {
          *error = std::string("This is not a whole Heroes IV: Complete install: ") + m.path + " is missing.";
          return false;
        }
      RemoveTree(Unpack());
      if (!EnoughSpace(Unpack(), error)) return false;
      t->SetPhase("Copying the game");
      t->total = total;
      t->done = 0;
      std::vector<char> buf(1 << 20);
      for (const auto& [from, rel] : todo) {
        if (t->cancel) return *error = "cancelled", false;
        const std::string to = Unpack() + "/" + rel;
        std::error_code ec;
        fs::create_directories(fs::path(to).parent_path(), ec);
#if defined(__APPLE__)
        // A clone on the same APFS disk: no second copy of the data.
        if (copyfile(from.c_str(), to.c_str(), nullptr, COPYFILE_DATA | COPYFILE_CLONE) == 0) {
          struct stat st;
          if (stat(to.c_str(), &st) == 0) t->done += uint64_t(st.st_size);
          continue;
        }
#endif
        FILE* in = fopen(from.c_str(), "rb");
        FILE* out = in ? fopen(to.c_str(), "wb") : nullptr;
        bool ok = in && out;
        for (size_t n; ok && (n = fread(buf.data(), 1, buf.size(), in)) > 0;) {
          ok = fwrite(buf.data(), 1, n, out) == n;
          t->done += n;
          if (t->cancel) break;
        }
        if (in) fclose(in);
        if (out && fclose(out) != 0) ok = false;
        if (!ok) return *error = "Could not copy " + rel + " (is there space left?).", false;
      }
      // A German, French, Italian or Spanish install (by its heroes4.exe): played in its language.
      std::string lang;
      if (!CheckExe(Unpack(), t, error, &lang)) return false;
      if (!lang.empty()) {
        fprintf(stderr, "installer: the game in %s\n", lang.c_str());
        if (FILE* f = fopen((Unpack() + "/" + kGameLanguage).c_str(), "w")) {
          fputs(lang.c_str(), f);
          fclose(f);
        }
      }
      return true;
    }, Unpack());
  }

  // GOG's German, French, Italian and Spanish heroes4.exe of the same build (3.0 win11 GOG 0.2):
  // the English one but for the PE header and bytes after a ret that no code reaches (none in the
  // lifted code; their .rdata and .data the same), so they run the app's code as it is. The Polish
  // one is another build (other addresses) and is not among them.
  static constexpr std::pair<const char*, const char*> kExeAliases[] = {
      {"b171788633a63c38e28016b379ae97e033c880d259a5103fd006adcfcd4e4c57", "de"},
      {"a6f8d14af6add9e630addd37df29a7803f074f88fbc58a744817ae860cfa5701", "fr"},
      {"6a3c699d2d3d6e96d8f6fdc9c17fde6e03126a3073d1b7a754cb51bd5b32a023", "it"},
      {"bfc9eb079fbc3c74c879a9e90486479970f2a7cd450a6adad31ca12372809e93", "es"},
  };

  // heroes4.exe in `dir` is the English GOG 3.0 one or one of its aliases (kExeAliases): the app's
  // code is made from it, and the game loads its data sections from the file (the data of another
  // language works with it). `lang`: "" for the English one, else the alias's language.
  // GOG's German installer (and an installed game folder) can name it "Heroes4.exe": on a file system
  // that tells case apart (the browser's, Linux, Android) the app then finds no heroes4.exe. That name
  // in any case becomes "heroes4.exe", the one the app and the game open.
  static void LowercaseExeName(const std::string& dir) {
    namespace fs = std::filesystem;
    std::error_code ec;
    if (fs::exists(dir + "/heroes4.exe", ec)) return;
    for (const auto& e : fs::directory_iterator(dir, ec)) {
      std::string name = e.path().filename().string();
      std::transform(name.begin(), name.end(), name.begin(), [](unsigned char c) { return std::tolower(c); });
      if (name != "heroes4.exe") continue;
      fs::rename(e.path(), dir + "/heroes4.exe", ec);
      fprintf(stderr, "installer: %s renamed to heroes4.exe%s\n", e.path().filename().string().c_str(),
              ec ? " (failed)" : "");
      return;
    }
  }

  static bool CheckExe(const std::string& dir, Task* t, std::string* error, std::string* lang = nullptr) {
    t->SetPhase("Checking heroes4.exe");
    LowercaseExeName(dir);
    const ManifestFile* exe = FindInManifest("heroes4.exe");
    Sha256 h;
    std::vector<char> buf(1 << 20);
    if (FILE* f = fopen((dir + "/heroes4.exe").c_str(), "rb")) {
      for (size_t n; (n = fread(buf.data(), 1, buf.size(), f)) > 0;) h.Update(buf.data(), n);
      fclose(f);
    }
    const std::string got = h.HexDigest();
    if (lang) lang->clear();
    if (exe && got == exe->sha256) return true;
    for (const auto& [sha, code] : kExeAliases)
      if (got == sha) {
        if (lang) *lang = code;
        return true;
      }
    *error = "This heroes4.exe is not one of GOG's Heroes IV: Complete 3.0 (win11 GOG 0.2) in English, "
             "German, French, Italian or Spanish, which the app is made for. Other versions do not "
             "work (another language's data does: then only heroes4.exe must be one of those).";
    return false;
  }

  // A GOG installer in any language: its data with the English heroes4.exe (the one it has, if it is
  // that, else `english` put beside it); another language is marked in the game folder
  // (kGameLanguage), as from GOG.
  void StartUnpack(const PickedFile& exe, const PickedFile& bin, const PickedFile& english) {
    fprintf(stderr, "installer: unpacking %s%s\n", exe.name.c_str(),
            english.path.empty() ? "" : " (an English heroes4.exe beside it)");
    Start([this, exe, bin, english](Task* t, std::string* error) -> bool {
      t->SetPhase("Preparing");
      RemoveTree(Unpack());  // an earlier, unfinished unpack (can take a while)
      if (!EnoughSpace(Unpack(), error)) return false;
      if (!inno::Extract(exe, bin, ScratchDir() + "/installer-links", Unpack(), t, error)) return false;
      std::string exe_error, exe_lang;
      if (!CheckExe(Unpack(), t, &exe_error, &exe_lang)) {
        if (english.path.empty() || !CopyFile(english.path, Unpack() + "/heroes4.exe")) {
          *error = "This installer's heroes4.exe is not one the app is made for (GOG's English, German, "
                   "French, Italian or Spanish one). Put heroes4.exe from GOG's English version beside the "
                   "installer files (or pick it with them), and the game plays in the "
                   "installer's language.";
          return false;
        }
        if (!CheckExe(Unpack(), t, error)) {
          *error = "The heroes4.exe beside the installer is not one of those either.";
          return false;
        }
      }
      // Only heroes4.exe is checked: the installer checks its own files as it unpacks, and another
      // language's has a set of its own (GOG's Polish one: fewer contest maps, no ddraw.dll...).
      // English, or the installer's language (by its name; when the name does not say, by its own
      // heroes4.exe if that was the English one or an alias, else "xx").
      std::string lang = InstallerLanguage(exe.name);
      if (lang == "xx" && exe_error.empty()) lang = exe_lang;
      if (!lang.empty()) {
        fprintf(stderr, "installer: the game in %s\n", lang.c_str());
        if (FILE* f = fopen((Unpack() + "/" + kGameLanguage).c_str(), "w")) {
          fputs(lang.c_str(), f);
          fclose(f);
        }
      }
      return true;
    }, Unpack());
  }

  static bool CopyFile(const std::string& from, const std::string& to) {
    FILE* in = fopen(from.c_str(), "rb");
    FILE* out = in ? fopen(to.c_str(), "wb") : nullptr;
    bool ok = in && out;
    std::vector<char> buf(1 << 20);
    for (size_t n; ok && (n = fread(buf.data(), 1, buf.size(), in)) > 0;) ok = fwrite(buf.data(), 1, n, out) == n;
    if (in) fclose(in);
    if (out && fclose(out) != 0) ok = false;
    return ok;
  }

  bool EnoughSpace(const std::string& dir, std::string* error) {
    uint64_t have = 0;
    for (const ManifestFile& f : Manifest()) {
      FILE* fp = fopen((dir + "/" + f.path).c_str(), "rb");
      if (!fp) continue;
      fseek(fp, 0, SEEK_END);
      have += uint64_t(std::max(0L, ftell(fp)));
      fclose(fp);
    }
    const uint64_t need = ManifestBytes() - std::min(have, ManifestBytes()) + kSlack;
    const long long free = FreeBytes(files_);
    if (free >= 0 && uint64_t(free) < need) {
      *error = "Not enough free space: the game needs " + Megabytes(need) + ", there is " +
               Megabytes(uint64_t(free)) + ".";
      return false;
    }
    return true;
  }

  void Start(std::function<bool(Task*, std::string*)> job, const std::string& out) {
    task_ = std::make_unique<Task>();
    finished_ = false;
    out_ = out;
    screen_ = Screen::Busy;
    worker_ = std::thread([this, job] {
      ok_ = job(task_.get(), &error_);
      finished_ = true;
    });
  }

  void Finish() {
    worker_.join();
    if (picked_) EndPickedFiles(), picked_ = false;
    if (listing_languages_) {  // signed in: which language (only English: at once)
      listing_languages_ = false;
      if (ok_ && gog_langs_.size() > 1) screen_ = Screen::Language;
      else if (ok_) StartGogDownload("en-US");
      else if (task_->cancel) screen_ = Screen::Menu;
      else ShowError(error_);
      return;
    }
    if (ok_ && rename(out_.c_str(), (files_ + "/game").c_str()) == 0) {
      fprintf(stderr, "installer: game installed into %s/game\n", files_.c_str());
      for (const std::string& p : own_installer_) {
        if (std::remove(p.c_str()) == 0) fprintf(stderr, "installer: removed %s\n", p.c_str());
        // And macOS's "._" metadata beside it (copied to a FAT/exFAT card from a Mac).
        const size_t slash = p.rfind('/');
        if (slash != std::string::npos) std::remove((p.substr(0, slash + 1) + "._" + p.substr(slash + 1)).c_str());
      }
      own_installer_.clear();
      screen_ = Screen::Done;
#if defined(__EMSCRIPTEN__)
      // The browser: the page takes over from here (index.html: it loads again, at the next
      // step), rather than this process going on from its installer into the game.
      MAIN_THREAD_ASYNC_EM_ASM({ if (Module.h4Status) Module.h4Status('installed'); });
#endif
      return;
    }
    if (task_->cancel) {
      screen_ = Screen::Menu;
      return;
    }
    ShowError(ok_ ? "Could not move the game into place." : error_);
  }

  void ShowError(const std::string& e) {
    fprintf(stderr, "installer: %s\n", e.c_str());
    error_ = e;
    screen_ = Screen::Error;
  }

  void Draw() {
    ui_.Measure();
    ui_.buttons.clear();
    ui_.Color(18, 14, 10);
    SDL_RenderClear(ui_.ren);
    const float ch = ui_.Ch();
    // Centred vertically (the content's height from the frame before; it turns with the device);
    // taller than the screen (a phone on its side): from the top, scrolled.
    if (screen_ != scrolled_screen_) scroll_ = 0, scrolled_screen_ = screen_;  // a new screen: its top
    const float room = content_h_ + 2 * ch - ui_.safe.h;  // what does not fit
    scroll_ = room > 0 ? std::clamp(scroll_, 0.0f, room) : 0.0f;
    const float top = room > 0 ? ui_.safe.y + ch - scroll_ : ui_.safe.y + std::max(ch, (ui_.safe.h - content_h_) / 2);
    float y = top;
    ui_.Color(232, 190, 96);
    ui_.Centered(y, "HEROES OF MIGHT AND MAGIC IV", 1.5f);
    y += ch * 3.5f;
    ui_.Color(200, 190, 170);
    if (screen_ == Screen::Menu) {
      const bool resume = IsDirectory(Download());
      y = ui_.Paragraph(y, "The game files are needed once (" + Megabytes(ManifestBytes()) +
                               "). Use your GOG copy of Heroes of Might and Magic IV: Complete. "
                               "GOG's English, German, French, Italian and Spanish installers work "
                               "as they are; another language's needs the English heroes4.exe "
                               "beside it.");
      y += ch;
      if (CanSignInToGog()) {
        ui_.AddButton(kGog, y, resume ? "Continue the GOG.com download"
                                      : "Sign in to GOG.com and download", 38);
        y += ch * 4.4f;
      }
#if !defined(H4_SWITCH_APP)
      ui_.AddButton(kFiles, y, "Install from GOG installer files", 38);
      y += ch * 4.4f;
#endif
#if !defined(H4_SWITCH_APP)
      ui_.AddButton(kFolder, y, "Use an installed game (its folder)", 38);
      y += ch * 4.6f;
#else
      y += ch * 0.2f;
#endif
      ui_.Color(150, 140, 120);
      y = ui_.Paragraph(y,
#if defined(__ANDROID__)
          "Installer files: setup_heroes_of_might_and_magic_4_complete_...exe and its -1.bin "
          "(pick both, or adb push them to " + files_ + "). Or adb push a ready \"game\" folder there."
#elif defined(H4_SWITCH_APP)
          "Install the game on a computer from your GOG library (gog.com > Games > Heroes of "
          "Might and Magic IV: Complete), then copy its folder (the one with heroes4.exe) to the "
          "SD card as\n" + SdPath(files_) + "/game\nand start the game again."
#elif defined(H4_WEB_APP)
          "Download the Windows installer from your GOG library (gog.com > Games > Heroes of Might "
          "and Magic IV: Complete): setup_heroes_of_might_and_magic_4_complete_...exe and its "
          "-1.bin, then pick both. Or pick the folder of a game installed on this computer. The "
          "game files are kept in this browser's storage; nothing is uploaded."
#elif defined(H4_DESKTOP_APP)
          "Download the Windows installer from your GOG library (gog.com > Games > Heroes of Might "
          "and Magic IV: Complete): setup_heroes_of_might_and_magic_4_complete_...exe and its "
          "-1.bin. In ~/Downloads they are found by themselves; or pick both. Or put a ready "
          "\"game\" folder in " + files_ + "."
#else
          "Installer files: setup_heroes_of_might_and_magic_4_complete_...exe and its -1.bin "
          "(pick both, or copy them to the app in Finder or Files: On My iPhone > Heroes IV). "
          "Or copy a ready \"game\" folder there."
#endif
      );
      if (!LoadToken().empty() || resume) {
        y += ch * 0.5f;
        ui_.AddButton(kSignOut, y, "Forget GOG sign-in", 30);
      }
    } else if (screen_ == Screen::Language) {
      ui_.Color(232, 220, 190);
      y = ui_.Paragraph(y, "The game's language:");
      y += ch * 0.5f;
      ui_.Color(200, 190, 170);
      y = ui_.Paragraph(y, "GOG's own versions. Translations from ReHoMM4-translations (Ukrainian and "
                           "others) work with English only.");
      y += ch;
      for (size_t i = 0; i < gog_langs_.size(); i++) {
        ui_.AddButton(kLanguage0 + int(i), y, LanguageName(gog_langs_[i]), 26);
        y += ch * 3.7f;
      }
      ui_.AddButton(kBack, y + ch * 0.5f, "Back", 24);
    } else if (screen_ == Screen::Picking && picking_folder_) {
      ui_.Color(232, 220, 190);
      y = ui_.Paragraph(y, "Pick the folder Heroes IV is installed in (with heroes4.exe).");
      y += ch;
      y = Indeterminate(y);
    } else if (screen_ == Screen::Picking) {
      ui_.Color(232, 220, 190);
      y = ui_.Paragraph(y, "Pick both installer files:");
      y += ch * 0.5f;
      ui_.Color(200, 190, 170);
      y = ui_.Paragraph(y, "setup_heroes_of_might_and_magic_4_complete_...exe\nand its ...-1.bin");
      y += ch;
      y = Indeterminate(y);
    } else if (screen_ == Screen::Busy) {
      const uint64_t total = task_->total, done = task_->done;
      ui_.Color(232, 220, 190);
      y = ui_.Paragraph(y, task_->Phase());
      y += ch;
      const float bw = ui_.safe.w * 0.8f, bx = ui_.safe.x + (ui_.safe.w - bw) / 2;
      SDL_FRect bar{bx, y, bw, ch * 2};
      ui_.Color(60, 48, 30);
      SDL_RenderFillRect(ui_.ren, &bar);
      if (total) {
        SDL_FRect fill{bx, y, bw * float(std::min(done, total)) / float(total), ch * 2};
        ui_.Color(214, 172, 86);
        SDL_RenderFillRect(ui_.ren, &fill);
        y += ch * 3.5f;
      } else {
        y = Indeterminate(y);  // no size yet: a block sliding to and fro
      }
      ui_.Color(200, 190, 170);
      if (total) y = ui_.Paragraph(y, Megabytes(done) + " of " + Megabytes(total));
      y += ch;
      ui_.Color(150, 140, 120);
      y = ui_.Paragraph(y, "Keep the app open until it finishes.");
      ui_.AddButton(kCancel, y + ch, task_->cancel ? "Stopping..." : "Cancel", 24);
    } else if (screen_ == Screen::Error) {
      ui_.Color(240, 150, 120);
      y = ui_.Paragraph(y, error_);
      ui_.AddButton(kBack, y + ch * 2, "Back", 24);
    }
    for (const auto& b : ui_.buttons) y = std::max(y, b.r.y + b.r.h);
    content_h_ = y - top;
#if defined(H4_DESKTOP_APP)
    {  // ✕ at the top right: the fullscreen window has no title bar to close it with
      const float s = ch * 3.2f, m = ch;
      ui_.buttons.push_back({{ui_.safe.x + ui_.safe.w - s - m, ui_.safe.y + m, s, s}, "X", kQuit});
    }
#endif
    ui_.DrawButtons();
    if (room > 0) {  // where the view is: a thin bar at the right edge
      const float track = ui_.h, bar = std::max(ch * 3, track * ui_.safe.h / (content_h_ + 2 * ch));
      SDL_FRect r{ui_.w - ch * 0.6f, (track - bar) * scroll_ / room, ch * 0.4f, bar};
      ui_.Color(120, 96, 52);
      SDL_RenderFillRect(ui_.ren, &r);
    }
    SDL_RenderPresent(ui_.ren);
  }

  // Scrolling a screen taller than the window: a drag (a finger's, the mouse's), the wheel, the
  // arrow and page keys. true: the event was that (a drag's release is no button press).
  bool Scroll(SDL_Event& e) {
    const float ch = ui_.Ch(), page = std::max(ch * 4, ui_.safe.h - ch * 4);
    switch (e.type) {
      case SDL_EVENT_MOUSE_WHEEL:
        scroll_ -= e.wheel.y * ch * 4;
        return true;
      case SDL_EVENT_KEY_DOWN:
        if (e.key.key == SDLK_DOWN) scroll_ += ch * 4;
        else if (e.key.key == SDLK_UP) scroll_ -= ch * 4;
        else if (e.key.key == SDLK_PAGEDOWN || e.key.key == SDLK_SPACE) scroll_ += page;
        else if (e.key.key == SDLK_PAGEUP) scroll_ -= page;
        else return false;
        return true;
      case SDL_EVENT_MOUSE_BUTTON_DOWN:
        SDL_ConvertEventToRenderCoordinates(ui_.ren, &e);
        dragging_ = true;
        drag_y_ = e.button.y;
        dragged_ = 0;
        return true;
      case SDL_EVENT_MOUSE_MOTION:
        if (!dragging_) return false;
        SDL_ConvertEventToRenderCoordinates(ui_.ren, &e);
        scroll_ -= e.motion.y - drag_y_;
        dragged_ += std::fabs(e.motion.y - drag_y_);
        drag_y_ = e.motion.y;
        return true;
      case SDL_EVENT_MOUSE_BUTTON_UP:
        dragging_ = false;
        return dragged_ > ch * 1.5f;  // moved: a scroll; else the press goes on to the buttons
      case SDL_EVENT_FINGER_DOWN:
      case SDL_EVENT_FINGER_MOTION:
      case SDL_EVENT_FINGER_UP: {
        // A finger, when SDL does not make it the mouse (the runtime's SDL_HINT_TOUCH_MOUSE_EVENTS
        // off): the same drag, and a tap presses a button. (On, its mouse events do that.)
        if (SDL_GetHintBoolean(SDL_HINT_TOUCH_MOUSE_EVENTS, true)) return false;
        const float fx = e.tfinger.x * ui_.w, fy = e.tfinger.y * ui_.h;
        if (e.type == SDL_EVENT_FINGER_DOWN) {
          dragging_ = true;
          drag_y_ = fy;
          dragged_ = 0;
        } else if (e.type == SDL_EVENT_FINGER_MOTION && dragging_) {
          scroll_ -= fy - drag_y_;
          dragged_ += std::fabs(fy - drag_y_);
          drag_y_ = fy;
        } else if (e.type == SDL_EVENT_FINGER_UP) {
          dragging_ = false;
          if (dragged_ <= ch * 1.5f) OnPress(ui_.Hit(fx, fy));
        }
        return true;
      }
      default:
        return false;
    }
  }

  // A bar with a block moving to and fro (working, size unknown) at y; returns the y below it.
  float Indeterminate(float y) {
    const float ch = ui_.Ch();
    const float bw = ui_.safe.w * 0.8f, bx = ui_.safe.x + (ui_.safe.w - bw) / 2;
    SDL_FRect bar{bx, y, bw, ch * 2};
    ui_.Color(60, 48, 30);
    SDL_RenderFillRect(ui_.ren, &bar);
    const float k = 0.5f - 0.5f * std::cos(float(SDL_GetTicks() % 2000) / 2000.0f * 6.2831853f);
    SDL_FRect blk{bx + (bw - bw / 4) * k, y, bw / 4, ch * 2};
    ui_.Color(214, 172, 86);
    SDL_RenderFillRect(ui_.ren, &blk);
    return y + ch * 3.5f;
  }

  float content_h_ = 0;
  float scroll_ = 0;  // how far down the screen is scrolled (pixels)
  Screen scrolled_screen_ = Screen::Menu;
  bool dragging_ = false;
  float drag_y_ = 0, dragged_ = 0;

  std::string files_, autostart_;
  Ui ui_;
  Screen screen_ = Screen::Menu;
  gog::Session session_;
  std::unique_ptr<Task> task_;
  std::thread worker_;
  std::atomic<bool> finished_{false};
  bool ok_ = false, picked_ = false, picking_folder_ = false;
  bool quit_ = false;                  // closed (the window, or ✕ on the Mac and Linux)
  bool listing_languages_ = false;     // the job after signing in: GOG's languages
  std::vector<std::string> gog_langs_;  // they, for Screen::Language
  std::vector<std::string> own_installer_;  // the installer's files in files_, removed when done
  std::string error_, out_;
};

}  // namespace

std::string RunInstaller(const std::string& files_dir, const std::string& autostart) {
  if (IsDirectory(files_dir + "/game")) return files_dir + "/game";
  Installer in(files_dir, autostart);
  return in.Run();
}

namespace {

// The body of DownloadWithProgress (on a worker thread): ranges of 4 MB appended to <dest>.part.
bool Fetch(const std::string& url, uint64_t size, const std::string& sha256, const std::string& dest,
           Task* t, std::string* error) {
  constexpr uint64_t kPiece = 4 * kMB;
  const std::string part = dest + ".part";
  FILE* f = fopen(part.c_str(), "ab");
  if (!f) {
    *error = "cannot write " + part;
    return false;
  }
  fseek(f, 0, SEEK_END);
  uint64_t have = uint64_t(std::max(0L, ftell(f)));
  if (have > size) {  // not this file: start again
    fclose(f);
    f = fopen(part.c_str(), "wb");
    have = 0;
    if (!f) {
      *error = "cannot write " + part;
      return false;
    }
  }
  t->total = size;
  t->done = have;
  t->SetPhase("Downloading");
  while (have < size && !t->cancel) {
    const uint64_t to = std::min(size, have + kPiece) - 1;
    std::string body, err;
    bool ok = false;
    for (int attempt = 0; attempt < 5 && !ok && !t->cancel; attempt++) {
      if (attempt) SDL_Delay(Uint32(attempt) * 2000);
      int status = 0;
      body.clear();
      if (!HttpGet(url, "", &status, &body, &err, std::to_string(have) + "-" + std::to_string(to)))
        continue;
      if (status == 200 && have == 0 && body.size() == size) {
        ok = true;  // the server sent it all (no ranges)
      } else if (status == 206 && body.size() == to - have + 1) {
        ok = true;
      } else {
        err = status == 404 ? "the file is not on the server (404)"
                            : "the server answered " + std::to_string(status);
        if (status == 404) break;
      }
    }
    if (!ok) {
      fclose(f);
      *error = t->cancel ? "cancelled" : err;
      return false;
    }
    if (fwrite(body.data(), 1, body.size(), f) != body.size() || fflush(f) != 0) {
      fclose(f);
      *error = "cannot write the file (is the storage full?)";
      return false;
    }
    have += body.size();
    t->done = have;
  }
  fclose(f);
  if (t->cancel) {
    *error = "cancelled";
    return false;
  }
  t->SetPhase("Checking the download");
  Sha256 h;
  if (FILE* r = fopen(part.c_str(), "rb")) {
    std::vector<char> buf(1 << 20);
    for (size_t n; (n = fread(buf.data(), 1, buf.size(), r)) > 0;) h.Update(buf.data(), n);
    fclose(r);
  }
  if (h.HexDigest() != sha256) {
    remove(part.c_str());
    *error = "the downloaded file was damaged; try again";
    return false;
  }
  if (rename(part.c_str(), dest.c_str()) != 0) {
    *error = "cannot move the file into place";
    return false;
  }
  return true;
}

}  // namespace

std::string DownloadWithProgress(const std::string& title, const std::string& url, uint64_t size,
                                 const std::string& sha256, const std::string& dest) {
  const std::string why = RunWithProgress(title, [&](Task* task, std::string* error) {
    return Fetch(url, size, sha256, dest, task, error);
  });
  fprintf(stderr, "download %s: %s\n", url.c_str(), why.empty() ? "done" : why.c_str());
  return why;
}

std::string RunWithProgress(const std::string& title, const std::function<bool(Task*, std::string*)>& work) {
  PlatformInit();
  if (!SDL_InitSubSystem(SDL_INIT_VIDEO)) return "no video";
  Ui ui;
  ui.win = CreateUiWindow();
  ui.ren = ui.win ? SDL_CreateRenderer(ui.win, nullptr) : nullptr;
  if (!ui.ren) {
    if (ui.win) SDL_DestroyWindow(ui.win);
    SDL_QuitSubSystem(SDL_INIT_VIDEO);
    return "no window";
  }
  SDL_SetRenderVSync(ui.ren, 1);
  SDL_DisableScreenSaver();
  Task task;
  std::atomic<bool> finished{false};
  bool ok = false;
  std::string error;
  std::thread worker([&] {
    ok = work(&task, &error);
    finished = true;
  });
  float content_h = 0;
  while (!finished) {
    SDL_Event e;
    while (SDL_PollEvent(&e)) {
      if (e.type == SDL_EVENT_QUIT) task.cancel = true;
      if (e.type == SDL_EVENT_KEY_DOWN && e.key.key == SDLK_AC_BACK) task.cancel = true;
      if (e.type == SDL_EVENT_MOUSE_BUTTON_UP) {
        SDL_ConvertEventToRenderCoordinates(ui.ren, &e);
        if (ui.Hit(e.button.x, e.button.y) == kCancel) task.cancel = true;
      }
    }
    ui.Measure();
    ui.buttons.clear();
    ui.Color(18, 14, 10);
    SDL_RenderClear(ui.ren);
    const float ch = ui.Ch();
    const float top = ui.safe.y + std::max(ch, (ui.safe.h - content_h) / 2);
    float y = top;
    ui.Color(232, 190, 96);
    ui.Centered(y, "HEROES OF MIGHT AND MAGIC IV", 1.5f);
    y += ch * 3.5f;
    ui.Color(232, 220, 190);
    y = ui.Paragraph(y, title);
    y += ch * 0.5f;
    ui.Color(200, 190, 170);
    y = ui.Paragraph(y, task.Phase());
    y += ch;
    const float bw = ui.safe.w * 0.8f, bx = ui.safe.x + (ui.safe.w - bw) / 2;
    SDL_FRect bar{bx, y, bw, ch * 2};
    ui.Color(60, 48, 30);
    SDL_RenderFillRect(ui.ren, &bar);
    const uint64_t total = task.total, done = task.done;
    if (total) {
      SDL_FRect fill{bx, y, bw * float(std::min(done, total)) / float(total), ch * 2};
      ui.Color(214, 172, 86);
      SDL_RenderFillRect(ui.ren, &fill);
    }
    y += ch * 3.5f;
    ui.Color(200, 190, 170);
    if (total) y = ui.Paragraph(y, Megabytes(done) + " of " + Megabytes(total));
    ui.AddButton(kCancel, y + ch, task.cancel ? "Stopping..." : "Cancel", 24);
    for (const auto& b : ui.buttons) y = std::max(y, b.r.y + b.r.h);
    content_h = y - top;
    ui.DrawButtons();
    SDL_RenderPresent(ui.ren);
    SDL_Delay(16);
  }
  worker.join();
  SDL_EnableScreenSaver();
  SDL_DestroyRenderer(ui.ren);
  SDL_DestroyWindow(ui.win);
  SDL_QuitSubSystem(SDL_INIT_VIDEO);
  return ok ? "" : error.empty() ? "failed" : error;
}

}  // namespace h4::install
