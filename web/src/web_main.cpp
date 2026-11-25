// The browser build's entry (web/CMakeLists.txt): runs on the module's main worker
// (PROXY_TO_PTHREAD), puts the browser's storage where the Linux app keeps its files, then
// starts the game's own main() (h4/src/main.cpp, compiled as h4_game_main).
//
//   /persist            the origin private file system (OPFS): kept between visits
//   /persist/ReHoMM4/game     the game files (the in-app installer puts them there)
//   /persist/ReHoMM4/player   saves, settings, the language, logs
//   /ui, /wings         the app's own pictures (preloaded with the module)

#include <emscripten/emscripten.h>
#include <emscripten/fetch.h>
#include <emscripten/wasmfs.h>
#include <dirent.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>

#include <SDL3/SDL.h>

#include <cerrno>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <thread>

int h4_game_main(int argc, char** argv);

namespace {

// OPFS needs a secure context and a browser that has it (every current one; not Firefox's
// private windows). Without it the game still runs, from memory: nothing is kept.
bool MountStorage() {
  backend_t opfs = wasmfs_create_opfs_backend();
  if (opfs && wasmfs_create_directory("/persist", 0777, opfs) == 0) {
    // A write that reaches the browser's storage: the mount alone does not tell.
    const char* probe = "/persist/.rehomm4-probe";
    if (FILE* f = std::fopen(probe, "w")) {
      const bool ok = std::fputs("ok", f) >= 0;
      std::fclose(f);
      std::remove(probe);
      if (ok) return true;
    }
  }
  std::fprintf(stderr, "h4-web: no origin private file system (%s): the game is kept in memory only\n",
               std::strerror(errno));
  mkdir("/persist", 0777);
  return false;
}

// H4_WEB_STREAM=<folder on this server> (index.html?env=H4_WEB_STREAM=game-files, with
// web/serve.py --game): the game read in place from the server, a few MB at a time as it is
// used, instead of from the browser's storage. For development and measuring: nothing is
// copied (a browser profile without 1 GB to spare runs it), nothing is kept (/web is memory).
bool MountStreamedGame(const char* base) {
  const std::string index = std::string(base) + "/index.json";
  emscripten_fetch_attr_t attr;
  emscripten_fetch_attr_init(&attr);
  std::strcpy(attr.requestMethod, "GET");
  attr.attributes = EMSCRIPTEN_FETCH_LOAD_TO_MEMORY | EMSCRIPTEN_FETCH_SYNCHRONOUS;
  emscripten_fetch_t* f = emscripten_fetch(&attr, index.c_str());
  if (!f || f->status != 200) {
    std::fprintf(stderr, "h4-web: no %s (%d)\n", index.c_str(), f ? f->status : 0);
    if (f) emscripten_fetch_close(f);
    return false;
  }
  const std::string list(f->data, size_t(f->numBytes));
  emscripten_fetch_close(f);
  mkdir("/web", 0777);
  mkdir("/web/ReHoMM4", 0777);
  mkdir("/web/ReHoMM4/player", 0777);
  // 4 MB pieces: the game's archives are read here and there, not through.
  backend_t fetch = wasmfs_create_fetch_backend(base, 4u << 20);
  if (!fetch || wasmfs_create_directory("/web/ReHoMM4/game", 0777, fetch) != 0) return false;
  // index.json (serve.py): {"files": [{"path": "data/x.h4r", "size": N}, ...]}; the names are
  // plain (no quotes or escapes in a game's file names).
  int n = 0;
  for (size_t at = 0; (at = list.find("\"path\": \"", at)) != std::string::npos;) {
    at += 9;
    const size_t end = list.find('"', at);
    if (end == std::string::npos) break;
    const std::string rel = list.substr(at, end - at);
    std::string path = "/web/ReHoMM4/game";
    for (size_t p = 0, q; (q = rel.find('/', p)) != std::string::npos; p = q + 1) {
      path += "/" + rel.substr(p, q - p);
      mkdir(path.c_str(), 0777);
    }
    const std::string file = "/web/ReHoMM4/game/" + rel;
    const int fd = open(file.c_str(), O_CREAT | O_WRONLY, 0666);
    if (fd >= 0) close(fd), n++;
    at = end;
  }
  std::printf("h4-web: the game streamed from %s (%d files)\n", base, n);
  return n > 0;
}

// The side panels (wings) are not in the module (heroes4.data): the site has them beside it, in
// wings/ (web/build.sh), fetched once into <files>/wings after the game has started, which the
// game shows from then on (H4_WINGS_DIR; black sides until they are there, or without them on the
// site). Each file is written under another name and renamed: the game never reads half a file.
bool FetchToMemory(const std::string& url, std::string* out) {
  emscripten_fetch_attr_t attr;
  emscripten_fetch_attr_init(&attr);
  std::strcpy(attr.requestMethod, "GET");
  attr.attributes = EMSCRIPTEN_FETCH_LOAD_TO_MEMORY | EMSCRIPTEN_FETCH_SYNCHRONOUS;
  emscripten_fetch_t* f = emscripten_fetch(&attr, url.c_str());
  const bool ok = f && f->status == 200 && f->numBytes > 0;
  if (ok) out->assign(f->data, size_t(f->numBytes));
  if (f) emscripten_fetch_close(f);
  return ok;
}

bool SameFile(const std::string& path, const std::string& data) {
  FILE* f = std::fopen(path.c_str(), "rb");
  if (!f) return false;
  std::string have(data.size() + 1, '\0');
  const size_t n = std::fread(have.data(), 1, have.size(), f);
  std::fclose(f);
  return n == data.size() && have.compare(0, n, data) == 0;
}

bool WriteFile(const std::string& path, const std::string& data) {
  const std::string part = path + ".part";
  FILE* f = std::fopen(part.c_str(), "wb");
  if (!f) return false;
  const bool ok = std::fwrite(data.data(), 1, data.size(), f) == data.size();
  if (std::fclose(f) != 0 || !ok || std::rename(part.c_str(), path.c_str()) != 0) {
    std::remove(part.c_str());
    return false;
  }
  return true;
}

void FetchWings(std::string dir) {
  sleep(5);  // after the game's own start (its files, the intro)
  std::string index;
  if (!FetchToMemory("wings/index.txt", &index)) return;  // none on this site: black sides
  if (SameFile(dir + "/index.txt", index)) return;          // there already
  mkdir(dir.c_str(), 0777);
  int n = 0;
  for (size_t at = 0; at < index.size();) {
    size_t nl = index.find('\n', at);
    if (nl == std::string::npos) nl = index.size();
    const std::string name = index.substr(at, nl - at);
    at = nl + 1;
    if (name.empty() || name.find('/') != std::string::npos || name[0] == '.') continue;
    std::string data;
    if (!FetchToMemory("wings/" + name, &data) || !WriteFile(dir + "/" + name, data)) {
      std::printf("h4-web: wings: %s not fetched\n", name.c_str());
      return;  // again at the next start
    }
    n++;
  }
  WriteFile(dir + "/index.txt", index);  // last: marks them all there
  std::printf("h4-web: wings: %d fetched into %s\n", n, dir.c_str());
  std::fflush(stdout);
}

}  // namespace

// main.cpp sends stdout and stderr to <player>/logs/h4.log. In the page they stay the
// browser's console (the page shows it on errors, and the console is where it is looked for).
extern "C" int __real_dup2(int oldfd, int newfd);
extern "C" int __wrap_dup2(int oldfd, int newfd) {
  if (newfd == 1 || newfd == 2) return newfd;
  return __real_dup2(oldfd, newfd);
}

// The game quits with ExitProcess, which ends the process (_Exit) from its thread: the page is told
// first (it shows that the game has ended; Module.onExit is not always called).
extern "C" [[noreturn]] void __real__Exit(int code);
extern "C" [[noreturn]] void __real__exit(int code);
namespace {
void TellExit(int code) {
  std::fflush(nullptr);
  MAIN_THREAD_EM_ASM({ if (Module.h4Status) Module.h4Status('exit', $0); }, code);
}
}  // namespace
extern "C" [[noreturn]] void __wrap__Exit(int code) {
  TellExit(code);
  __real__Exit(code);
}
extern "C" [[noreturn]] void __wrap__exit(int code) {
  TellExit(code);
  __real__exit(code);
}

// The origin private file system moves files but not folders (WasmFS: rename of a directory
// fails with EBUSY). The game and its installer finish their work by renaming a folder made
// beside the real one (the translation's maps, an install): such a folder is moved file by file.
extern "C" int __real_rename(const char* from, const char* to);
namespace {
int MoveTree(const std::string& from, const std::string& to) {
  if (mkdir(to.c_str(), 0777) != 0 && errno != EEXIST) return -1;
  DIR* d = opendir(from.c_str());
  if (!d) return -1;
  int r = 0;
  while (dirent* e = readdir(d)) {
    const std::string name = e->d_name;
    if (name == "." || name == "..") continue;
    const std::string a = from + "/" + name, b = to + "/" + name;
    struct stat st {};
    if (stat(a.c_str(), &st) != 0) { r = -1; break; }
    if (S_ISDIR(st.st_mode) ? MoveTree(a, b) != 0 : __real_rename(a.c_str(), b.c_str()) != 0) { r = -1; break; }
  }
  closedir(d);
  return r == 0 ? rmdir(from.c_str()) : r;
}
}  // namespace
extern "C" int __wrap_rename(const char* from, const char* to) {
  const int r = __real_rename(from, to);
  if (r == 0 || errno != EBUSY) return r;
  struct stat st {};
  if (stat(from, &st) != 0 || !S_ISDIR(st.st_mode)) {
    errno = EBUSY;
    return r;
  }
  return MoveTree(from, to);
}

int main(int argc, char** argv) {
  const bool kept = MountStorage();
  MAIN_THREAD_ASYNC_EM_ASM({ if (Module.h4Status) Module.h4Status('storage', $0); }, kept ? 1 : 0);
  // main.cpp's MobileDirs (the Linux app): $XDG_DATA_HOME/ReHoMM4.
  const char* stream = getenv("H4_WEB_STREAM");
  const char* home = stream && *stream && MountStreamedGame(stream) ? "/web" : "/persist";
  setenv("XDG_DATA_HOME", home, 1);
  setenv("HOME", home, 1);
  const std::string wings = std::string(home) + "/ReHoMM4/wings";
  setenv("H4_WINGS_DIR", wings.c_str(), 1);
  std::thread(FetchWings, wings).detach();
  // The soft device's frames are drawn by SDL's software renderer into the canvas (SDL proxies
  // the canvas calls to the page's thread); WebGL would need the canvas moved to this worker.
  SDL_SetHint(SDL_HINT_RENDER_DRIVER, "software");
  // Keys go to the game only while the canvas has the focus (the page's own controls keep theirs).
  SDL_SetHint(SDL_HINT_EMSCRIPTEN_KEYBOARD_ELEMENT, "#canvas");
  SDL_SetHint(SDL_HINT_EMSCRIPTEN_CANVAS_SELECTOR, "#canvas");
  chdir("/");
  std::printf("h4-web: starting (storage: %s)\n", kept ? "OPFS" : "memory");
  std::fflush(stdout);
  const int code = h4_game_main(argc, argv);
  std::printf("h4-web: the game ended (%d)\n", code);
  std::fflush(stdout);
  MAIN_THREAD_ASYNC_EM_ASM({ if (Module.h4Status) Module.h4Status('exit', $0); }, code);
  return code;
}

// menu_button.h: started again with the language just picked (it is read from the saved choice).
namespace h4 {
void RestartApp() {
  std::printf("h4: restarting\n");
  std::fflush(nullptr);
  MAIN_THREAD_ASYNC_EM_ASM({ location.reload(); });
  // The page goes; this thread waits for it instead of drawing on.
  for (;;) sleep(1);  // a worker may block
}
}  // namespace h4
