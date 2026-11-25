// install/platform.h in the browser (web), and the app pieces the Linux build takes from
// files a browser cannot use (save_sync.cpp's rclone, platform_desktop.cpp's curl and dialogs).
//
// - HTTPS: the Fetch API (emscripten_fetch, synchronous: this is never the page's thread).
//   Only addresses that allow other sites' pages (CORS) can be read; GOG's API does not, so
//   there is no "Sign in to GOG.com" (CanSignInToGog).
// - Picking files: the page's file input (web_pre.js h4PickFiles). The page copies what was
//   picked into the origin private file system, <files>/picked/, where the installer reads it
//   like any file, and writes the list of them last (<files>/picked/.done, one per line:
//   the path inside picked/); an empty list is a cancel. EndPickedFiles removes the copies.
#include <emscripten/emscripten.h>
#include <emscripten/fetch.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <string>
#include <vector>

#include "install/platform.h"

namespace h4::install {

namespace {

std::string FilesDir() {
  const char* x = std::getenv("XDG_DATA_HOME");
  return std::string(x && *x ? x : "/persist") + "/ReHoMM4";
}
std::string PickDir() { return FilesDir() + "/picked"; }
std::string TokenPath() { return FilesDir() + "/player/gog.token"; }

void RemoveAll(const std::string& p) {
  std::error_code ec;
  std::filesystem::remove_all(p, ec);
}

}  // namespace

namespace {

bool Fetch(const std::string& url, const std::string& bearer, int* status, std::string* body,
           std::string* error, const std::string& range);

// Release files on GitHub (the translations' list and packs): GitHub's downloads do not allow
// other sites' pages to read them (no CORS headers). Where the page finds them instead, in order:
//   translations/<file>   the site's own copy (web/build.sh, unless --no-translations);
//   /<repository>/<file>  that repository's own GitHub Pages site, which a site on the same
//                         <owner>.github.io reads as its own (ReHoMM4-translations publishes its
//                         latest release there: .github/workflows/pages.yml in that repository).
std::vector<std::string> OnThisSite(const std::string& url) {
  const size_t gh = url.find("://github.com/");
  if (gh == std::string::npos || url.find("/releases/") == std::string::npos) return {};
  const std::string file = url.substr(url.rfind('/') + 1);
  std::vector<std::string> at{"translations/" + file};
  const size_t owner = gh + 14, repo = url.find('/', owner) + 1, end = url.find('/', repo);
  if (repo > owner && end != std::string::npos && end > repo)
    at.push_back(url.substr(repo - 1, end - repo + 1) + "/" + file);
  return at;
}

}  // namespace

bool HttpGet(const std::string& url, const std::string& bearer, int* status, std::string* body,
             std::string* error, const std::string& range) {
  for (const std::string& here : OnThisSite(url))
    if (Fetch(here, "", status, body, error, range) && (*status == 200 || *status == 206)) return true;
  return Fetch(url, bearer, status, body, error, range);
}

namespace {

bool Fetch(const std::string& url, const std::string& bearer, int* status, std::string* body,
           std::string* error, const std::string& range) {
  emscripten_fetch_attr_t attr;
  emscripten_fetch_attr_init(&attr);
  std::strcpy(attr.requestMethod, "GET");
  attr.attributes = EMSCRIPTEN_FETCH_LOAD_TO_MEMORY | EMSCRIPTEN_FETCH_SYNCHRONOUS;
  std::vector<std::string> kv;
  if (!bearer.empty()) kv.insert(kv.end(), {"Authorization", "Bearer " + bearer});
  if (!range.empty()) kv.insert(kv.end(), {"Range", "bytes=" + range});
  std::vector<const char*> headers;
  for (const std::string& s : kv) headers.push_back(s.c_str());
  headers.push_back(nullptr);
  attr.requestHeaders = headers.data();
  emscripten_fetch_t* f = emscripten_fetch(&attr, url.c_str());
  if (!f) {
    if (error) *error = "the request could not be made";
    return false;
  }
  // 0: the browser refused it (offline, or the site does not allow pages of other sites).
  const bool ok = f->status != 0;
  if (ok) {
    *status = f->status;
    body->assign(f->data ? f->data : "", size_t(f->numBytes));
  } else if (error) {
    *error = "no connection, or the site does not allow it from a web page";
  }
  emscripten_fetch_close(f);
  return ok;
}

}  // namespace

std::string GogLoginCode(const std::string&) { return ""; }
bool CanSignInToGog() { return false; }

bool BeginPickFiles(bool folder) {
  RemoveAll(PickDir());
  mkdir(FilesDir().c_str(), 0755);
  // The page needs the click that led here to open a picker: the click reached this thread a
  // moment ago, well within the browser's few seconds of "user activation".
  MAIN_THREAD_ASYNC_EM_ASM({ Module.h4PickFiles($0 != 0); }, folder ? 1 : 0);
  return true;
}

bool PickedFiles(std::vector<PickedFile>* out) {
  const std::string dir = PickDir();
  FILE* f = std::fopen((dir + "/.done").c_str(), "r");
  if (!f) return false;
  out->clear();
  char line[4096];
  while (std::fgets(line, sizeof line, f)) {
    std::string s = line;
    while (!s.empty() && (s.back() == '\n' || s.back() == '\r')) s.pop_back();
    if (s.empty()) continue;
    // Installer files by their own names; a folder's files by their paths inside it.
    out->push_back({dir + "/" + s, s});
  }
  std::fclose(f);
  std::remove((dir + "/.done").c_str());
  return true;
}

void EndPickedFiles() { RemoveAll(PickDir()); }

std::string LoadToken() {
  std::string t;
  if (FILE* f = std::fopen(TokenPath().c_str(), "rb")) {
    char buf[4096];
    t.assign(buf, std::fread(buf, 1, sizeof buf, f));
    std::fclose(f);
  }
  return t;
}

void SaveToken(const std::string& token) {
  const std::string p = TokenPath();
  if (token.empty()) {
    unlink(p.c_str());
    return;
  }
  if (FILE* f = std::fopen(p.c_str(), "wb")) {
    std::fwrite(token.data(), 1, token.size(), f);
    std::fclose(f);
  }
}

std::string ScratchDir() {
  const std::string d = "/tmp/h4-scratch";
  mkdir("/tmp", 0777);
  mkdir(d.c_str(), 0700);
  return d;
}

void PlatformInit() {}

}  // namespace h4::install

// save_sync.h (H4_DESKTOP_APP, not Apple): the browser keeps the saves in its own storage; there
// is no folder outside it to keep in step with.
namespace h4 {
void StartSaveSync(const std::string&) {}
}  // namespace h4
