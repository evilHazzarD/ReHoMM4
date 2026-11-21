// install/platform.h for the desktop app builds (H4_DESKTOP_APP: Linux and
// the Steam Deck, and the Mac): HTTPS through the system's libcurl (loaded at
// run time: macOS, SteamOS and every desktop distribution have it), files
// picked with the system's own dialog (macOS: osascript's "choose file";
// Linux: kdialog on KDE, as the Deck's desktop, else zenity), the token in
// the player's data.
// GOG's sign-in page needs a web view inside the app, which this build has
// not got (CanSignInToGog): the installer takes the installer files.
#include <dlfcn.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>

#include <SDL3/SDL.h>
#include <signal.h>

#include <atomic>
#include <cctype>
#include <memory>
#include <cstdio>
#include <cstdlib>
#include <mutex>
#include <filesystem>
#include <string>
#include <thread>
#include <vector>

#include "install/platform.h"

namespace h4::install {

namespace {

std::string DataHome() {
  const char* h = std::getenv("HOME");
#if defined(__APPLE__)
  return std::string(h ? h : ".") + "/Library/Application Support";
#else
  const char* x = std::getenv("XDG_DATA_HOME");
  if (x && *x) return x;
  return std::string(h ? h : ".") + "/.local/share";
#endif
}

std::string TokenPath() { return DataHome() + "/ReHoMM4/player/gog.token"; }

// The few libcurl calls used, from libcurl.so.4.
struct Curl {
  void* (*easy_init)();
  int (*easy_setopt)(void*, int, ...);
  int (*easy_perform)(void*);
  int (*easy_getinfo)(void*, int, ...);
  void (*easy_cleanup)(void*);
  const char* (*easy_strerror)(int);
  void* (*slist_append)(void*, const char*);
  void (*slist_free_all)(void*);
  bool ok = false;
};

const Curl& Lib() {
  static Curl c = [] {
    Curl k{};
#if defined(__APPLE__)
    void* h = dlopen("/usr/lib/libcurl.4.dylib", RTLD_NOW);
#else
    void* h = dlopen("libcurl.so.4", RTLD_NOW);
    if (!h) h = dlopen("libcurl-gnutls.so.4", RTLD_NOW);
#endif
    if (!h) return k;
    k.easy_init = reinterpret_cast<void* (*)()>(dlsym(h, "curl_easy_init"));
    k.easy_setopt = reinterpret_cast<int (*)(void*, int, ...)>(dlsym(h, "curl_easy_setopt"));
    k.easy_perform = reinterpret_cast<int (*)(void*)>(dlsym(h, "curl_easy_perform"));
    k.easy_getinfo = reinterpret_cast<int (*)(void*, int, ...)>(dlsym(h, "curl_easy_getinfo"));
    k.easy_cleanup = reinterpret_cast<void (*)(void*)>(dlsym(h, "curl_easy_cleanup"));
    k.easy_strerror = reinterpret_cast<const char* (*)(int)>(dlsym(h, "curl_easy_strerror"));
    k.slist_append = reinterpret_cast<void* (*)(void*, const char*)>(dlsym(h, "curl_slist_append"));
    k.slist_free_all = reinterpret_cast<void (*)(void*)>(dlsym(h, "curl_slist_free_all"));
    k.ok = k.easy_init && k.easy_setopt && k.easy_perform && k.easy_getinfo && k.easy_cleanup &&
           k.slist_append && k.slist_free_all;
    return k;
  }();
  return c;
}

// libcurl's option and info numbers (curl/curl.h; stable ABI).
enum : int {
  kOptUrl = 10002,
  kOptWriteData = 10001,
  kOptWriteFunction = 20011,
  kOptFollowLocation = 52,
  kOptHttpHeader = 10023,
  kOptRange = 10007,
  kOptConnectTimeout = 78,
  kOptLowSpeedLimit = 19,
  kOptLowSpeedTime = 20,
  kOptNoSignal = 99,
  kOptUserAgent = 10018,
  kInfoResponseCode = 0x200002,
};

size_t Append(char* p, size_t size, size_t n, void* out) {
  static_cast<std::string*>(out)->append(p, size * n);
  return size * n;
}

// The picker's run (a thread): the paths the dialog printed, one a line.
std::mutex g_pick_mu;
std::vector<PickedFile> g_picked;
std::atomic<bool> g_pick_done{true};

std::string Quote(const std::string& s) {
  std::string q = "'";
  for (char c : s) q += c == '\'' ? std::string("'\\''") : std::string(1, c);
  return q + "'";
}

bool Have(const char* tool) {
  return std::system((std::string("command -v ") + tool + " >/dev/null 2>&1").c_str()) == 0;
}

}  // namespace


std::string ScratchDir() {
  const char* h = std::getenv("HOME");
#if defined(__APPLE__)
  return std::string(h ? h : "/tmp") + "/Library/Caches/ReHoMM4";
#else
  const char* x = std::getenv("XDG_CACHE_HOME");
  return (x && *x ? std::string(x) : std::string(h ? h : "/tmp") + "/.cache") + "/ReHoMM4";
#endif
}

void PlatformInit() {}

bool HttpGet(const std::string& url, const std::string& bearer, int* status, std::string* body,
             std::string* error, const std::string& range) {
  const Curl& c = Lib();
  if (!c.ok) {
    *error = "no libcurl on this system";
    return false;
  }
  void* h = c.easy_init();
  if (!h) {
    *error = "libcurl failed";
    return false;
  }
  void* headers = nullptr;
  if (!bearer.empty()) headers = c.slist_append(headers, ("Authorization: Bearer " + bearer).c_str());
  body->clear();
  c.easy_setopt(h, kOptUrl, url.c_str());
  c.easy_setopt(h, kOptFollowLocation, 1L);
  c.easy_setopt(h, kOptNoSignal, 1L);
  c.easy_setopt(h, kOptConnectTimeout, 30L);
  c.easy_setopt(h, kOptLowSpeedLimit, 1L);  // stalled for 60 s: give up
  c.easy_setopt(h, kOptLowSpeedTime, 60L);
  c.easy_setopt(h, kOptUserAgent, "ReHoMM4");
  c.easy_setopt(h, kOptWriteFunction, &Append);
  c.easy_setopt(h, kOptWriteData, body);
  if (headers) c.easy_setopt(h, kOptHttpHeader, headers);
  if (!range.empty()) c.easy_setopt(h, kOptRange, range.c_str());
  const int r = c.easy_perform(h);
  long code = 0;
  c.easy_getinfo(h, kInfoResponseCode, &code);
  c.easy_cleanup(h);
  if (headers) c.slist_free_all(headers);
  if (r != 0) {
    *error = std::string("network error: ") + (c.easy_strerror ? c.easy_strerror(r) : std::to_string(r));
    return false;
  }
  *status = int(code);
  return true;
}

#if !defined(__APPLE__)  // the Mac's: gog_login_mac.mm (a web view)
// Linux has no web view for the app: GOG's login page opens in the browser,
// and the address it ends on (https://embed.gog.com/on_login_success?...
// code=...) comes back pasted into a dialog, or just copied: the clipboard is
// watched meanwhile.
bool CanSignInToGog() { return Have("xdg-open") && (Have("kdialog") || Have("zenity")); }

namespace {
// The code in a pasted login-success address (or a bare code).
std::string CodeIn(const std::string& text) {
  std::string t = text;
  while (!t.empty() && std::isspace((unsigned char)t.back())) t.pop_back();
  while (!t.empty() && std::isspace((unsigned char)t.front())) t.erase(0, 1);
  const size_t at = t.find("code=");
  if (at != std::string::npos) {
    if (t.find("on_login_success") == std::string::npos) return "";
    size_t e = at + 5;
    while (e < t.size() && t[e] != '&' && t[e] != '#' && !std::isspace((unsigned char)t[e])) e++;
    return t.substr(at + 5, e - at - 5);
  }
  if (t.size() < 20 || t.size() > 400) return "";
  for (char c : t)
    if (!std::isalnum((unsigned char)c) && c != '-' && c != '_' && c != '.') return "";
  return t;
}
}  // namespace

std::string GogLoginCode(const std::string& url) {
  std::system(("xdg-open " + Quote(url) + " >/dev/null 2>&1 &").c_str());
  const std::string text =
      "GOG's sign-in page has opened in your browser. Sign in there; when it ends on a blank "
      "page, copy that page's address (Ctrl+L, Ctrl+C) - the game picks it up - or paste it here.";
  const std::string dialog =
      Have("kdialog") ? "kdialog --title 'Sign in to GOG.com' --inputbox " + Quote(text)
                      : "zenity --entry --title='Sign in to GOG.com' --width=520 --text=" + Quote(text);
  // The dialog's run (its pid first, so it can be closed once the clipboard has the address).
  struct Run {
    std::mutex mu;
    std::string out;
    int pid = 0;
    std::atomic<bool> done{false};
  };
  auto run = std::make_shared<Run>();
  std::thread([run, dialog] {
    FILE* p = popen(("sh -c 'echo $$; exec " + dialog + " 2>/dev/null'").c_str(), "r");
    if (p) {
      char buf[4096];
      if (std::fgets(buf, sizeof buf, p)) {
        std::lock_guard<std::mutex> l(run->mu);
        run->pid = std::atoi(buf);
      }
      std::string out;
      while (std::fgets(buf, sizeof buf, p)) out += buf;
      pclose(p);
      std::lock_guard<std::mutex> l(run->mu);
      run->out = out;
    }
    run->done = true;
  }).detach();
  std::string code;
  char* before = SDL_GetClipboardText();
  const std::string old = before ? before : "";
  SDL_free(before);
  uint64_t next_check = 0;
  while (!run->done) {
    SDL_PumpEvents();
    if (SDL_GetTicks() >= next_check) {
      next_check = SDL_GetTicks() + 500;
      char* c = SDL_GetClipboardText();
      const std::string clip = c ? c : "";
      SDL_free(c);
      if (clip != old && !(code = CodeIn(clip)).empty()) {
        std::lock_guard<std::mutex> l(run->mu);
        if (run->pid > 0) kill(run->pid, SIGTERM);
        break;
      }
    }
    SDL_Delay(30);
  }
  if (code.empty()) {
    std::lock_guard<std::mutex> l(run->mu);
    code = CodeIn(run->out);
  }
  SDL_FlushEvents(SDL_EVENT_FIRST, SDL_EVENT_LAST);
  return code;
}
#endif

bool BeginPickFiles(bool folder) {
  std::string cmd;
  const char* home = std::getenv("HOME");
  const std::string start = std::string(home ? home : "/") + (folder ? "" : "/Downloads");
  const std::string title = folder ? "Pick the folder of an installed Heroes IV (with heroes4.exe)"
                                   : "Pick both GOG installer files (.exe and -1.bin)";
#if defined(__APPLE__)
  if (folder)
    cmd = "osascript -e 'POSIX path of (choose folder with prompt \"" + title + "\" default location (POSIX file \"" +
          start + "\"))'";
  else
  cmd = "osascript -e 'set fs to choose file with prompt \"" + title +
        "\" default location (POSIX file \"" + start +
        "\") with multiple selections allowed' -e 'set out to \"\"' -e 'repeat with f in fs' "
        "-e 'set out to out & POSIX path of f & linefeed' -e 'end repeat' -e 'return out'";
#else
  if (folder && Have("kdialog"))
    cmd = "kdialog --title " + Quote(title) + " --getexistingdirectory " + Quote(start);
  else if (folder && Have("zenity"))
    cmd = "zenity --file-selection --directory --title=" + Quote(title) + " --filename=" + Quote(start + "/");
  else if (Have("kdialog"))
    cmd = "kdialog --title " + Quote(title) + " --multiple --separate-output --getopenfilename " +
          Quote(start) + " 'GOG installer (*.exe *.bin)'";
  else if (Have("zenity"))
    cmd = "zenity --file-selection --multiple --separator='\n' --title=" + Quote(title) +
          " --filename=" + Quote(start + "/") + " --file-filter='GOG installer | *.exe *.bin'";
  else
    return false;
#endif
  {
    std::lock_guard<std::mutex> l(g_pick_mu);
    g_picked.clear();
  }
  g_pick_done = false;
  std::thread([cmd, folder] {
    std::vector<PickedFile> out;
    if (FILE* p = popen((cmd + " 2>/dev/null").c_str(), "r")) {
      char line[4096];
      while (fgets(line, sizeof line, p)) {
        std::string s = line;
        while (!s.empty() && (s.back() == '\n' || s.back() == '\r')) s.pop_back();
        if (s.empty()) continue;
        const size_t slash = s.rfind('/');
        out.push_back({s, slash == std::string::npos ? s : s.substr(slash + 1)});
      }
      pclose(p);
    }
    if (folder && !out.empty()) {  // the files in it, by their paths inside it
      std::string root = out[0].path;
      while (root.size() > 1 && root.back() == '/') root.pop_back();
      out.clear();
      std::error_code ec;
      for (std::filesystem::recursive_directory_iterator it(root, ec), end; !ec && it != end; it.increment(ec))
        if (it->is_regular_file())
          out.push_back({it->path().string(), it->path().lexically_relative(root).generic_string()});
      if (out.empty()) out.push_back({root, ""});  // picked, but nothing in it
    }
    std::lock_guard<std::mutex> l(g_pick_mu);
    g_picked = std::move(out);
    g_pick_done = true;
  }).detach();
  return true;
}

bool PickedFiles(std::vector<PickedFile>* out) {
  if (!g_pick_done) return false;
  std::lock_guard<std::mutex> l(g_pick_mu);
  *out = g_picked;
  return true;
}

void EndPickedFiles() {}

std::string LoadToken() {
  std::string t;
  if (FILE* f = fopen(TokenPath().c_str(), "rb")) {
    char buf[4096];
    t.assign(buf, fread(buf, 1, sizeof buf, f));
    fclose(f);
  }
  return t;
}

void SaveToken(const std::string& token) {
  const std::string p = TokenPath();
  if (token.empty()) {
    unlink(p.c_str());
    return;
  }
  const int fd = open(p.c_str(), O_WRONLY | O_CREAT | O_TRUNC, 0600);
  if (fd < 0) return;
  const ssize_t w = write(fd, token.data(), token.size());
  (void)w;
  close(fd);
}

}  // namespace h4::install
