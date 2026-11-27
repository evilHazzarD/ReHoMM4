// Contains code from HoMM4Deadware (https://github.com/LeoDeadware/HoMM4Deadware): Copyright (c) 2026 LeoDeadware, MIT License.
// h4: Heroes of Might and Magic IV, natively.
//
//   h4 [--game-dir DIR] [-- GAME ARGS...]
//
// DIR is the game install (the folder with heroes4.exe, Data/, maps/).
// Defaults to $H4_GAME_DIR. Set H4_TRACE=1 to log every HLE call.
//
// iOS and Android have no launcher (platforms/macos-dmg/launcher on the Mac): MobileSetup
// does its job. The game is the folder "game" (or "game-<code>", a translated copy
// made by hand) in the app's Documents (iOS, Files app: On My iPhone > Heroes IV)
// or its external files dir (Android: Android/data/<package>/files, adb push).
// Without one, the in-app installer (install/installer.h) puts it there.

#include <dirent.h>
#include <cerrno>
#include <sys/stat.h>

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <initializer_list>
#include <string>

#if defined(__APPLE__)
#include <TargetConditionals.h>
#endif
#if defined(_WIN32)
#include <shlobj.h>
#include <direct.h>
// MinGW exposes the Microsoft spellings only. Keep the app code below POSIX-shaped.
// POSIX's: an existing variable kept unless `overwrite`.
static int setenv(const char* name, const char* value, int overwrite) {
  if (!overwrite && std::getenv(name)) return 0;
  return _putenv_s(name, value);
}
static int mkdir(const char* path, int) { return _mkdir(path); }
#endif
// The Linux (Steam Deck) and Mac app builds (H4_DESKTOP_APP, platforms/build.sh linux|macos-*) work
// the same way: the game installed by the app into ~/.local/share/ReHoMM4 (Linux) or
// ~/Library/Application Support/ReHoMM4 (Mac).
#if defined(__ANDROID__) || (defined(__APPLE__) && TARGET_OS_IOS) || defined(H4_DESKTOP_APP) || \
    defined(H4_SWITCH_APP)
#define H4_MOBILE 1
#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>  // main() is SDL's entry point there
#include <fcntl.h>
#include <unistd.h>
#endif
#if defined(H4_SWITCH_APP)
#include <switch.h>
#endif
#if defined(H4_WEB_APP)
#include <emscripten.h>
#endif

#if defined(H4_MOBILE)
#include "filter_notice.h"
#include "renderer.h"
#include "ui_text.h"
#include "install/installer.h"
#include "install/platform.h"
#include "intro.h"
#include "window_mode.h"
#include "pad_config.h"
#include "language.h"
#include "menu_button.h"
#include "save_sync.h"
#endif
#include "runtime/gfx/device.h"
#include "runtime/log.h"
#include "runtime/hle/kernel32_state.h"
#include "runtime/runtime.h"

#if defined(__APPLE__) && TARGET_OS_IOS
namespace h4 {
bool IosSafeAreaMargin(float* fraction);
bool IosPhone();     // an iPhone (not an iPad)
bool IosIpadMini();  // an iPad mini
bool IosBigPhone();  // a Plus or Pro Max iPhone
}
#endif
#if defined(__ANDROID__)
namespace h4 {
bool AndroidSafeAreaMargin(float* fraction);  // menu_button_android.cpp: the camera notch's side
bool AndroidSmallScreen();  // a phone's screen (smallest width under 600 dp)
}
#endif

#if defined(H4_SWITCH_APP)
namespace h4 {
// In the hands (not in the dock): resolution.cpp follows a change while the game runs.
bool SwitchHandheld() { return appletGetOperationMode() == AppletOperationMode_Handheld; }
}  // namespace h4
#endif

namespace {

bool IsDir(const std::string& p) {
  struct stat st;
  return stat(p.c_str(), &st) == 0 && S_ISDIR(st.st_mode);
}

bool IsFile(const std::string& p) {
  struct stat st;
  return stat(p.c_str(), &st) == 0 && S_ISREG(st.st_mode);
}

// For the log: what is at `p` ("folder", "file, N bytes", or why there is nothing).
[[maybe_unused]] std::string WhatIs(const std::string& p) {
  struct stat st;
  if (stat(p.c_str(), &st) != 0) return std::string("nothing (") + std::strerror(errno) + ")";
  if (S_ISDIR(st.st_mode)) return "folder";
  if (S_ISREG(st.st_mode)) return "file, " + std::to_string((long long)st.st_size) + " bytes";
  return "something else (mode " + std::to_string(unsigned(st.st_mode)) + ")";
}

#if defined(H4_WINDOWS_APP)
// The earlier Windows preview kept its downloaded game under LocalAppData.  Keep
// that location readable after the portable release started living beside the
// game's own heroes4.exe.
std::string WindowsStorageDir() {
  PWSTR known = nullptr;
  if (SUCCEEDED(SHGetKnownFolderPath(FOLDERID_LocalAppData, 0, nullptr, &known))) {
    const int n = WideCharToMultiByte(CP_UTF8, 0, known, -1, nullptr, 0, nullptr, nullptr);
    std::string base(n > 0 ? n - 1 : 0, '\0');
    if (n > 1) WideCharToMultiByte(CP_UTF8, 0, known, -1, base.data(), n, nullptr, nullptr);
    CoTaskMemFree(known);
    return base + "/ReHoMM4";
  }
  return "./ReHoMM4";
}
#endif

#if defined(H4_MOBILE)
void MakeDirs(const std::string& p) {
  for (size_t i = 1; i <= p.size(); i++)
    if (i == p.size() || p[i] == '/') mkdir(p.substr(0, i).c_str(), 0755);
}

// The side panels the app carries (platforms/wings.sh): the bundle's wings/ on
// iOS; on Android the APK's assets, copied once into private storage (the
// game reads them as files). "" without them.
std::string AppWings() {
#if defined(__ANDROID__)
  auto read = [](const char* path) {
    std::string s;
    size_t n = 0;
    if (void* d = SDL_LoadFile(path, &n)) {  // relative: the internal storage's, else the APK's assets
      s.assign(static_cast<char*>(d), n);
      SDL_free(d);
    }
    return s;
  };
  // Not <internal>/wings: SDL opens a relative path from the internal storage before the APK's
  // assets, so a copy there named like the assets ("wings/index.txt") was read in their place
  // (an older app's BMP wings kept the JPEG ones from ever being copied). That old copy goes.
  const char* in = SDL_GetAndroidInternalStoragePath();
  const std::string legacy = std::string(in ? in : ".") + "/wings";
  if (DIR* d = opendir(legacy.c_str())) {
    while (dirent* e = readdir(d))
      if (e->d_name[0] != '.') std::remove((legacy + "/" + e->d_name).c_str());
    closedir(d);
    rmdir(legacy.c_str());
  }
  const std::string index = read("wings/index.txt");
  if (index.empty()) return "";
  const std::string dir = std::string(in ? in : ".") + "/app-wings";
  auto each = [](const std::string& list, auto&& f) {
    for (size_t at = 0; at < list.size();) {
      size_t nl = list.find('\n', at);
      if (nl == std::string::npos) nl = list.size();
      const std::string name = list.substr(at, nl - at);
      at = nl + 1;
      if (!name.empty() && !f(name)) return false;
    }
    return true;
  };
  const std::string old = read((dir + "/index.txt").c_str());
  // Copied before: the same list, and every file of it there (an index left without its files
  // was taken for a whole copy, and the wings never shown).
  if (old == index && each(index, [&](const std::string& name) {
        struct stat st;
        return stat((dir + "/" + name).c_str(), &st) == 0 && st.st_size > 0;
      }))
    return dir;
  // An older app's copies (another format, say) go first.
  each(old, [&](const std::string& name) { return std::remove((dir + "/" + name).c_str()), true; });
  std::remove((dir + "/index.txt").c_str());
  MakeDirs(dir);
  if (!each(index, [&](const std::string& name) {
        const std::string data = read(("wings/" + name).c_str());
        if (!data.empty() && SDL_SaveFile((dir + "/" + name).c_str(), data.data(), data.size())) return true;
        std::printf("h4: wings: %s not copied to %s: %s\n", name.c_str(), dir.c_str(), SDL_GetError());
        return false;
      }))
    return "";
  SDL_SaveFile((dir + "/index.txt").c_str(), index.data(), index.size());  // last: marks it whole
  std::printf("h4: wings: copied to %s\n", dir.c_str());
  return dir;
#else
  const char* base = SDL_GetBasePath();
  const std::string dir = std::string(base ? base : "./") + "wings";
  return IsDir(dir) ? dir : "";
#endif
}

// The game's language: the one chosen with the language button over the main
// menu (menu_button.h; saved in <data>/language, English at first), or
// H4_LANG. English is the GOG data as it is; a translation is its language pack
// (language.h), in the app or downloaded to <data>/packs/<code>-<hash>.h4p the
// first time it is chosen (and again when a new translations release changes it),
// applied to the game folder in place. A "game-<code>" folder made by hand is used
// as it is.
void ChooseLanguage(const std::string& game, const std::string& data) {
  // A game installed from GOG in another language (install/installer.cpp: its data with the English
  // heroes4.exe; the folder says which): played as it is. The translations' packs are made from the
  // English archives, so the settings offer no languages then.
  if (FILE* f = std::fopen((game + "/rehomm4-language").c_str(), "r")) {
    char b[16] = {};
    std::string own;
    if (std::fgets(b, sizeof b, f)) own.assign(b, std::strcspn(b, " \r\n"));
    std::fclose(f);
    if (!own.empty()) {
      std::printf("h4: language %s (the game's own, from GOG)\n", own.c_str());
      recomp::SetTranslationFiles(h4::TranslationFiles("", data + "/translation"));
      setenv("H4_LANG", own.c_str(), 1);
      h4::SetGogLanguage(own);
      h4::SetLanguageChoice(own, "");
      std::fflush(stdout);
      return;
    }
  }
  h4::LoadLanguages(data);
  h4::RefreshLanguages(data);  // a newer release: for the menu now, its packs at the next start
  const std::string saved_path = data + "/language";
  const std::string packs = data + "/packs";
  // Every pack at hand: in the app, downloaded (this version or an older one) and the
  // Ukrainian one of the apps before the language list (<data>/uk.h4p).
  std::vector<std::string> all;
  for (const h4::LanguageInfo& l : h4::Languages())
    if (std::string b = h4::BundledPack(l.code); !b.empty()) all.push_back(b);
  if (DIR* d = opendir(packs.c_str())) {
    while (dirent* e = readdir(d)) {
      const std::string n = e->d_name;
      if (n.size() > 4 && n.compare(n.size() - 4, 4, ".h4p") == 0) all.push_back(packs + "/" + n);
    }
    closedir(d);
  }
  if (IsFile(data + "/uk.h4p")) all.push_back(data + "/uk.h4p");
  auto downloaded = [&](const h4::LanguageInfo& l) {
    return h4::DownloadedPack(data, l);
  };
  std::string lang = "en";
  if (FILE* f = std::fopen(saved_path.c_str(), "r")) {
    char b[32] = {};
    if (std::fgets(b, sizeof b, f)) lang.assign(b, std::strcspn(b, " \r\n"));
    std::fclose(f);
  }
  if (const char* env = std::getenv("H4_LANG"); env && *env) lang = env;
  const h4::LanguageInfo info = h4::FindLanguage(lang);
  lang = info.code;  // English when the app does not have it
  std::string patch;
  if (lang != "en") {
    patch = h4::BundledPack(lang);
    if (patch.empty() && IsFile(downloaded(info))) patch = downloaded(info);
    if (patch.empty() && !info.url.empty()) {
      MakeDirs(packs);
      const std::string why = h4::install::DownloadWithProgress(
          // The progress screen's debug font is ASCII only: the native name would be "?????".
          info.english + " translation", info.url, info.size, info.sha256, downloaded(info));
      if (why.empty()) {
        patch = downloaded(info);
        all.push_back(patch);
      } else if (why != "cancelled") {
        SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_WARNING, "ReHoMM4",
                                 ("The " + info.english + " translation could not be downloaded (" + why +
                                  "). The game starts in English; choose " + info.name +
                                  " in the main menu to try again.").c_str(),
                                 nullptr);
      }
    }
    if (patch.empty()) {
      lang = "en";
      if (FILE* f = std::fopen(saved_path.c_str(), "w")) {  // not again at every start
        std::fputs("en", f);
        std::fclose(f);
      }
    }
  }
  std::string err;
  const uint64_t t0 = SDL_GetTicks();
  if (h4::SetGameLanguage(game, lang, patch, all, &err)) {
    std::printf("h4: language %s (%llu ms)\n", lang.c_str(), (unsigned long long)(SDL_GetTicks() - t0));
    // Only the chosen translation is in the archives now: older downloads go.
    if (lang != "en")
      for (const std::string& p : all) {
        if (p == patch || p.compare(0, packs.size(), packs) != 0) continue;
        bool current = false;
        for (const h4::LanguageInfo& l : h4::Languages()) current |= l.code != "en" && p == downloaded(l);
        if (!current) std::remove(p.c_str());
      }
    if (lang != "en") std::remove((data + "/uk.h4p").c_str());
  } else {
    std::printf("h4: language %s not set: %s\n", lang.c_str(), err.c_str());
    if (lang != "en")
      SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_WARNING, "ReHoMM4",
                               ("The " + info.english + " translation cannot be applied: " + err +
                                ". The game starts in English.").c_str(), nullptr);
    lang = "en";
  }
  // Its files outside the archives (single scenarios), read in place of the game's.
  recomp::SetTranslationFiles(h4::TranslationFiles(lang != "en" ? patch : "", data + "/translation"));
  setenv("H4_LANG", lang.c_str(), 1);
#if !defined(H4_TRANSLATIONS_URL)
  if (h4::Languages().size() < 2) return;  // no translations and nowhere to get them: no button
#endif
  h4::SetLanguageChoice(lang, saved_path);
  std::fflush(stdout);
}

// Where the player puts the game, and where saves/settings/logs go.
void MobileDirs(std::string* user_files, std::string* data) {
#if defined(H4_SWITCH_APP)
  // The SD card: sdmc:/switch/ReHoMM4 (beside the .nro), the game in game/, saves and
  // settings in player/; the installer files put there are found by the installer.
  *user_files = "sdmc:/switch/ReHoMM4";
  *data = *user_files + "/player";
#elif defined(H4_DESKTOP_APP)
  // .../ReHoMM4: the game in game/, saves and settings in player/. Linux:
  // $XDG_DATA_HOME (~/.local/share); Mac: ~/Library/Application Support.
  const char* home = std::getenv("HOME");
#if defined(_WIN32)
#if defined(H4_WINDOWS_APP)
  // The portable executable is put beside heroes4.exe.  Player data stays out
  // of the game installation, so an update never touches saves or settings.
  const char* base = SDL_GetBasePath();
  *user_files = base ? base : ".";
  *data = WindowsStorageDir() + "/player";
#else
  PWSTR known = nullptr;
  if (SUCCEEDED(SHGetKnownFolderPath(FOLDERID_LocalAppData, 0, nullptr, &known))) {
    const int n = WideCharToMultiByte(CP_UTF8, 0, known, -1, nullptr, 0, nullptr, nullptr);
    std::string base(n > 0 ? n - 1 : 0, '\0');
    if (n > 1) WideCharToMultiByte(CP_UTF8, 0, known, -1, base.data(), n, nullptr, nullptr);
    CoTaskMemFree(known);
    *user_files = base + "/ReHoMM4";
  } else {
    *user_files = "./ReHoMM4";
  }
  *data = *user_files + "/player";
#endif
#elif defined(__APPLE__)
  *user_files = std::string(home ? home : ".") + "/Library/Application Support/ReHoMM4";
#else
  const char* xdg = std::getenv("XDG_DATA_HOME");
  *user_files = (xdg && *xdg ? std::string(xdg) : std::string(home ? home : ".") + "/.local/share") +
                "/ReHoMM4";
#endif
#if !defined(H4_WINDOWS_APP)
  *data = *user_files + "/player";
#endif
#elif defined(__ANDROID__)
  // The app's external files dir: adb push/pull reach it (a release build's internal storage is
  // closed to adb), and it is removed with the app like the internal one.
  const char* ext = SDL_GetAndroidExternalStoragePath();
  const char* in = SDL_GetAndroidInternalStoragePath();
  *user_files = ext ? ext : (in ? in : ".");
  *data = *user_files + "/player";
#else
  const char* home = std::getenv("HOME");  // the app's sandbox container
  *user_files = std::string(home ? home : ".") + "/Documents";
  *data = std::string(home ? home : ".") + "/Library/Application Support/player";
#endif
}

// The launcher's work (platforms/macos-dmg/launcher/launcher.c): player data dir, log,
// screen defaults, the game folder. Returns the game dir or "".
std::string g_install_auto;  // --install-auto=files|gog|folder:<path> (testing)

std::string MobileSetup() {
  std::string files, data;
  MobileDirs(&files, &data);
  // Testing the installer beside an existing install: H4_FILES_DIR=<subfolder> (relative to the
  // app's files folder) is where the game is looked for and installed.
  if (const char* sub = std::getenv("H4_FILES_DIR"); sub && *sub)
    files = sub[0] == '/' ? std::string(sub) : files + "/" + sub;
  MakeDirs(data + "/data");
  MakeDirs(data + "/logs");
  MakeDirs(files);
  setenv("XDG_DATA_HOME", (data + "/data").c_str(), 0);
  const std::string log = data + "/logs/h4.log";
  rename(log.c_str(), (log + ".1").c_str());  // the previous run's log survives one start
  int fd = open(log.c_str(), O_WRONLY | O_CREAT | O_TRUNC, 0644);
  if (fd >= 0) {
    dup2(fd, 1);
    dup2(fd, 2);
    close(fd);
  }
#if defined(__EMSCRIPTEN__)
  // Every line at once (a file is otherwise written in blocks): a game that hangs in the browser
  // leaves its last lines in the log.
  setvbuf(stdout, nullptr, _IOLBF, 0);
  setvbuf(stderr, nullptr, _IOLBF, 0);
#endif
#if defined(H4_SWITCH_APP)
  // Every line on the card at once: a crash there leaves no other trace.
  setvbuf(stdout, nullptr, _IOLBF, 0);
  setvbuf(stderr, nullptr, _IOLBF, 0);
  // Where it starts (the first lines of a report): the console's mode and what the card has.
  std::printf("h4: ReHoMM4 %s, applet type %d, %s\n", H4_VERSION_STRING, int(appletGetAppletType()),
              appletGetOperationMode() == AppletOperationMode_Handheld ? "handheld" : "docked");
  std::printf("h4: log %s (%s)\n", log.c_str(), fd >= 0 ? "written" : std::strerror(errno));
  for (const std::string& p : {std::string("sdmc:/switch"), files, files + "/game", files + "/game/heroes4.exe",
                               files + "/game/Data", files + "/game/Data/heroes4.h4r", data})
    std::printf("h4: %s: %s\n", p.c_str(), WhatIs(p).c_str());
#endif
  // Testing on a device whose environment cannot be set (a phone): <player>/env, one KEY=VALUE a
  // line (H4_GFX=soft, H4_FPS_REPORT=5...), read before anything looks at them.
  if (FILE* f = std::fopen((data + "/env").c_str(), "r")) {
    char line[256];
    while (std::fgets(line, sizeof line, f)) {
      std::string s(line);
      while (!s.empty() && (s.back() == '\n' || s.back() == '\r' || s.back() == ' ')) s.pop_back();
      const size_t eq = s.find('=');
      if (s.empty() || s[0] == '#' || eq == std::string::npos || eq == 0) continue;
      setenv(s.substr(0, eq).c_str(), s.substr(eq + 1).c_str(), 1);
      std::printf("h4: env %s\n", s.c_str());
    }
    std::fclose(f);
  }
  h4::LoadSkipIntro(data + "/skip_intro");
  h4::LoadUiLanguageChoice(data);  // the interface's language, if not the game's (ui_text.h)
#if defined(__ANDROID__) || defined(__APPLE__)
  h4::LoadRenderer(data);  // before the graphics device is made
#endif
  // The layout when none is chosen: 800x600 on phones (in their browsers too), 1024x768 on
  // the big iPhones (Plus, Pro Max), Android tablets and the iPad mini, 1280x1024 on the other iPads and on desktops whose screen has the pixels for it (else 1024x768); the Switch 800x600 in the
  // hands and 1024x768 on the TV, each kept apart.
#if defined(__ANDROID__)
  h4::LoadLayout(data, h4::AndroidSmallScreen() ? "800" : "1024");
#elif defined(__APPLE__) && TARGET_OS_IOS
  h4::LoadLayout(data, h4::IosPhone() ? (h4::IosBigPhone() ? "1024" : "800") : h4::IosIpadMini() ? "1024" : "1280");
#elif defined(H4_SWITCH_APP)
  if (h4::SwitchHandheld())
    h4::LoadLayout(data, "800", "layout_handheld");
  else
    h4::LoadLayout(data, "1024", "layout_docked");
#elif defined(H4_WEB_APP)
  {
    const int kind = MAIN_THREAD_EM_ASM_INT({
      const touch = matchMedia('(pointer: coarse)').matches;
      const short_side = Math.min(screen.width, screen.height);
      return touch && short_side < 600 ? 0 : touch ? 1 : 2;  // a phone, a tablet, a computer
    });
    // (a computer: 1280x1024, as it always was; the window is scaled to the page anyway)
    h4::LoadLayout(data, kind == 0 ? "800" : kind == 1 ? "1024" : "1280");
  }
#else  // the desktop apps
  h4::LoadLayout(data, "desktop");
#endif
  h4::LoadFilter(data + "/filter");  // before the default below
  h4::LoadPadConfig(data + "/gamepad.conf");
#if defined(H4_DESKTOP_APP)
  h4::LoadWindowMode(data + "/windowed");
#endif
  setenv("H4_RESOLUTION", "wide", 0);
  setenv("H4_FILTER", "fsr", 0);
  setenv("H4_DEBAND", "1", 0);
  setenv("H4_SHARPNESS", "0.2", 0);
#if defined(H4_DESKTOP_APP)
  // Saves in iCloud Drive/ReHOMM4 (Mac, save_sync_apple.mm) or ~/ReHOMM4 (Linux, save_sync.cpp),
  // with an rclone remote around it when sync.conf names one.
  h4::StartSaveSync(data);
#endif
#if defined(__APPLE__) && TARGET_OS_IOS
  h4::StartSaveSync(data);  // saves in iCloud Drive/ReHOMM4, once a folder was picked
  // Keep the game clear of the notch and the rounded corners: its edge
  // buttons are hard to press there. H4_SAFE_AREA=0 uses the whole screen.
  const char* sa = std::getenv("H4_SAFE_AREA");
  float margin = 0;
  if (!(sa && !std::strcmp(sa, "0")) && h4::IosSafeAreaMargin(&margin) && margin > 0) {
    recomp::gfx::SetOutputInsets(margin, margin);
    std::printf("h4: safe area: %.1f%% of the width kept clear on each side\n", margin * 100);
  }
#endif
#if defined(__ANDROID__)
  {  // The same off the camera notch (the app draws under it: windowLayoutInDisplayCutoutMode).
    const char* sa = std::getenv("H4_SAFE_AREA");
    float margin = 0;
    if (!(sa && !std::strcmp(sa, "0")) && h4::AndroidSafeAreaMargin(&margin) && margin > 0) {
      recomp::gfx::SetOutputInsets(margin, margin);
      std::printf("h4: safe area: %.1f%% of the width kept clear on each side (the notch)\n", margin * 100);
    } else {
      std::printf("h4: safe area: no notch\n");
    }
  }
#endif
  setenv("H4_SIDE_PANELS_FILE", (data + "/side-panels.txt").c_str(), 0);
  // H4_SIDE_PANELS (black, wings) with no setting yet: the browser page's choice of older versions
  // (or its default, black), kept as the setting.
  if (const char* sp = std::getenv("H4_SIDE_PANELS"); sp && *sp) {
    if (FILE* f = std::fopen(h4::SidePanelsPath(), "r")) std::fclose(f);
    else h4::SaveBlackSidePanels(std::strcmp(sp, "black") == 0);
  }
  // The player's own wings/ folder first, else the app's.
  if (IsDir(files + "/wings")) setenv("H4_WINGS_DIR", (files + "/wings").c_str(), 0);
  else if (const std::string w = AppWings(); !w.empty()) setenv("H4_WINGS_DIR", w.c_str(), 0);
  auto use_game = [&](const std::string& game) {
    std::printf("h4: game %s, player files %s\n", game.c_str(), data.c_str());
    ChooseLanguage(game, data);
    h4::SetGameFolder(game);
    std::fflush(stdout);
    return game;
  };
  // The portable Windows release is unpacked directly into an existing GOG
  // installation, so its launcher and heroes4.exe are siblings.
#if defined(H4_WINDOWS_APP)
  if (IsFile(files + "/heroes4.exe")) return use_game(files);
#endif
  // A translated copy made by hand (game-<code>) when that language is asked for, else game/.
  const char* lang = std::getenv("H4_LANG");
  const std::string own = lang && *lang && std::strcmp(lang, "en") ? std::string("game-") + lang : "";
  for (const std::string& d : {own, std::string("game")}) {
    if (!d.empty() && IsDir(files + "/" + d)) {
      return use_game(files + "/" + d);
    }
#if defined(H4_SWITCH_APP)
    if (!d.empty()) std::printf("h4: looked for the game in %s/%s: %s\n", files.c_str(), d.c_str(), WhatIs(files + "/" + d).c_str());
#endif
  }
  std::string install_files = files;
#if defined(H4_WINDOWS_APP)
  // Existing preview installs remain usable.  New installs still go to the
  // private storage, not into a possibly read-only GOG directory.
  install_files = WindowsStorageDir();
  if (install_files != files) {
    for (const std::string& d : {own, std::string("game")})
      if (!d.empty() && IsDir(install_files + "/" + d)) return use_game(install_files + "/" + d);
  }
#endif
  // No game yet: the in-app installer (GOG.com download or GOG's offline installer). Where it
  // was looked for, and what is there, in the log (a device's card: the folder named otherwise).
  std::printf("h4: no game in %s/game; %s holds:", files.c_str(), files.c_str());
  if (DIR* d = opendir(files.c_str())) {
    while (const dirent* e = readdir(d))
      if (std::strcmp(e->d_name, ".") && std::strcmp(e->d_name, "..")) std::printf(" %s", e->d_name);
    closedir(d);
  } else {
    std::printf(" (cannot be read: %s)", std::strerror(errno));
  }
  std::printf("\n");
  std::fflush(stdout);
  const std::string installed = h4::install::RunInstaller(install_files, g_install_auto);
  if (!installed.empty()) {
    return use_game(installed);
  }
  return "";
}
#endif

}  // namespace

#if defined(H4_DESKTOP_APP) && !defined(H4_WINDOWS_APP)
namespace h4 {
void KeepStartArgs(int argc, char** argv);
}
#endif

int main(int argc, char** argv) {
#if defined(H4_DESKTOP_APP) && !defined(H4_WINDOWS_APP)
  h4::KeepStartArgs(argc, argv);  // for RestartApp (menu_button.h)
#endif
#if defined(H4_SWITCH_APP)
  romfsInit();  // the wings, the language button's pictures, the translation (in the .nro)
  mkdir("sdmc:/switch/ReHoMM4", 0777);
#endif
  std::string game_dir, game_args;
  if (const char* e = std::getenv("H4_GAME_DIR")) game_dir = e;
#if defined(__ANDROID__)
  // The Java side (HttpGet, the file picker, ...): looked up here, on SDL's main thread, which
  // can see the app's classes; the installer did it only when it ran, so with the game already
  // installed the settings' network calls (What's new, the update check) had none.
  h4::install::PlatformInit();
#endif
#if defined(H4_MOBILE)
  // Testing the installer on a device: --install-auto=files|gog presses that button (folder:<path>:
  // that folder as the installed game picked; files-fd:
  // files, read through descriptors as picked files are);
  // --files-dir=<subfolder> is H4_FILES_DIR, --lang=<code> H4_LANG (Android has no
  // environment to pass).
  for (int i = 1; i < argc; i++)
    if (!std::strncmp(argv[i], "--install-auto=", 15) || !std::strncmp(argv[i], "--files-dir=", 12) ||
        !std::strncmp(argv[i], "--lang=", 7) || !std::strncmp(argv[i], "--zoom=", 7)) {
      if (argv[i][2] == 'i') g_install_auto = argv[i] + 15;
      else if (argv[i][2] == 'z') setenv("H4_TEST_ZOOM", argv[i] + 7, 1);
      else if (argv[i][2] == 'l') setenv("H4_LANG", argv[i] + 7, 1);
      else setenv("H4_FILES_DIR", argv[i] + 12, 1);
      for (int k = i; k + 1 < argc; k++) argv[k] = argv[k + 1];
      argc--, i--;
    }
  if (const char* sub = std::getenv("H4_INSTALL_AUTO")) g_install_auto = sub;
  if (game_dir.empty()) game_dir = MobileSetup();
  if (game_dir.empty()) return 2;
  // The game is landscape only (Info.plist / the Android manifest allow portrait for the
  // installer screen).
  SDL_SetHint(SDL_HINT_ORIENTATIONS, "LandscapeLeft LandscapeRight");
#endif
  for (int i = 1; i < argc; i++) {
    if (!std::strcmp(argv[i], "--game-dir") && i + 1 < argc) {
      game_dir = argv[++i];
    } else if (!std::strcmp(argv[i], "--")) {
      for (i++; i < argc; i++) {
        if (!game_args.empty()) game_args += ' ';
        game_args += argv[i];
      }
    } else {
      std::fprintf(stderr, "usage: %s [--game-dir DIR] [-- GAME ARGS...]\n", argv[0]);
      return 2;
    }
  }
  if (game_dir.empty()) {
    std::fprintf(stderr, "h4: pass --game-dir or set H4_GAME_DIR\n");
    return 2;
  }
  while (game_dir.size() > 1 && game_dir.back() == '/') game_dir.pop_back();
  if (const char* t = std::getenv("H4_TRACE")) recomp::g_trace = *t && *t != '0';

  std::string exe = game_dir + "/heroes4.exe";
  struct stat st;
  if (stat(exe.c_str(), &st) != 0) {
    std::fprintf(stderr, "h4: %s not found\n", exe.c_str());
    return 2;
  }

  recomp::GameConfig game;
  game.id = "h4";
  game.guest_dir = "C:\\Heroes4";
  game.exe_name = "heroes4.exe";
  // What the retail installer writes (the GOG install writes nothing).
  const std::string key = "HKLM\\SOFTWARE\\New World Computing\\Heroes of Might and Magic IV\\1.0";
  // The game's own settings key: without a "resolution" (the screen width)
  // it takes the desktop's size, which its interface does not support past
  // 1280x1024. 1024 is its 1024x768 mode.
  const std::string settings = "HKCU\\Software\\New World Computing\\Heroes of Might and Magic IV";
  game.registry = {{key, "AppPath", game.guest_dir},
                   {settings, "resolution", "1024", 4}};
  // The game draws its own cursor where GetCursorPos says, in exclusive
  // mode too, and never re-centres it.
  game.own_cursor = true;
  // 60 frames a second at most, on 120 Hz screens too: the game's motion
  // (smooth.cpp) is updated at most every 16 ms, so faster only costs GPU
  // work and battery. H4_MAX_FPS=0 shows every refresh.
  game.max_frame_rate = 60;
#if H4_WEB_DYNAMIC
  extern bool h4_prepare_dynamic_game(const char* path);
  if (!h4_prepare_dynamic_game(exe.c_str())) return 2;
#endif
  return recomp::RunProcess(game, exe, game_dir, game_args);
}

// menu_button.h: the game's folder, for the settings.
namespace h4 {
namespace {
std::string g_game_folder;
}
const std::string& GameFolder() { return g_game_folder; }
void SetGameFolder(const std::string& path) { g_game_folder = path; }
}  // namespace h4

#if defined(H4_DESKTOP_APP) && !defined(H4_WINDOWS_APP)
#if defined(__APPLE__)
#include <mach-o/dyld.h>
#endif
#include <climits>
#include <vector>

// menu_button.h: the app started again, as it was started (after a language pick).
namespace h4 {
namespace {
std::vector<std::string> g_start_args;
}  // namespace

void KeepStartArgs(int argc, char** argv) {
  for (int i = 0; i < argc; i++)
    if (std::strncmp(argv[i], "--lang=", 7)) g_start_args.push_back(argv[i]);  // as H4_LANG: not again
}

void RestartApp() {
  char exe[PATH_MAX] = {};
#if defined(__APPLE__)
  uint32_t size = sizeof exe;
  if (_NSGetExecutablePath(exe, &size) != 0) return;
#else
  const ssize_t n = readlink("/proc/self/exe", exe, sizeof exe - 1);
  if (n <= 0) return;
  exe[n] = 0;
#endif
  // The language is read again from the saved choice: never H4_LANG (ChooseLanguage sets it, and
  // one the app was started with, a test's, would win over the language just picked).
  unsetenv("H4_LANG");
  std::vector<char*> args;
  for (std::string& a : g_start_args) args.push_back(a.data());
  args.push_back(nullptr);
  std::fflush(nullptr);
  std::printf("h4: restarting\n");
  std::fflush(nullptr);
  // Nothing of this process may stay open in the new one (archives, the log, sockets).
  for (int fd = 3; fd < 1024; fd++) {
    const int flags = fcntl(fd, F_GETFD);
    if (flags >= 0) fcntl(fd, F_SETFD, flags | FD_CLOEXEC);
  }
  execv(exe, args.data());
  std::printf("h4: restart failed\n");
}
}  // namespace h4
#endif
