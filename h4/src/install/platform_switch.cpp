// install/platform.h on the Nintendo Switch (homebrew, H4_SWITCH_APP): HTTPS
// through libcurl (devkitPro's switch-curl, linked in; the sockets started
// here), no file picker and no web view: no install there (the installer
// screen only says where to copy an installed game: sdmc:/switch/ReHoMM4/game),
// and no GOG sign-in.
#include <curl/curl.h>
#include <fcntl.h>
#include <pthread.h>
#include <switch.h>
#include <sys/stat.h>
#include <unistd.h>

#include <cstdio>
#include <string>
#include <vector>

#include "install/platform.h"

// newlib has no pread/pwrite: lseek + read/write, one at a time (the
// installer's download and the translation use them from worker threads).
#include <mutex>
namespace {
std::mutex g_positional_io;
}
extern "C" ssize_t pread(int fd, void* buf, size_t n, off_t at) {
  std::lock_guard<std::mutex> l(g_positional_io);
  if (lseek(fd, at, SEEK_SET) == off_t(-1)) return -1;
  return read(fd, buf, n);
}
extern "C" ssize_t pwrite(int fd, const void* buf, size_t n, off_t at) {
  std::lock_guard<std::mutex> l(g_positional_io);
  if (lseek(fd, at, SEEK_SET) == off_t(-1)) return -1;
  return write(fd, buf, n);
}

// Threads started without a stack size (std::thread: the installer's worker, the game's helpers)
// get 8 MB as on a PC, not libnx's 128 KB, which the installer's unpacking overflows (the
// link wraps pthread_create: h4/CMakeLists.txt). Those that ask for a size keep it.
extern "C" int __real_pthread_create(pthread_t*, const pthread_attr_t*, void* (*)(void*), void*);
extern "C" int __wrap_pthread_create(pthread_t* t, const pthread_attr_t* attr, void* (*fn)(void*),
                                     void* arg) {
  size_t size = 0;
  if (attr && pthread_attr_getstacksize(attr, &size) == 0 && size) return __real_pthread_create(t, attr, fn, arg);
  pthread_attr_t big = attr ? *attr : pthread_attr_t{};
  if (!attr) pthread_attr_init(&big);
  pthread_attr_setstacksize(&big, 8u << 20);
  return __real_pthread_create(t, &big, fn, arg);
}

namespace h4::install {

namespace {

const char kHome[] = "sdmc:/switch/ReHoMM4";

std::string TokenPath() { return std::string(kHome) + "/player/gog.token"; }

size_t Append(char* p, size_t size, size_t n, void* out) {
  static_cast<std::string*>(out)->append(p, size * n);
  return size * n;
}

}  // namespace

void PlatformInit() {
  socketInitializeDefault();
  curl_global_init(CURL_GLOBAL_DEFAULT);
}

bool CanSignInToGog() { return false; }

std::string GogLoginCode(const std::string&) { return ""; }

bool HttpGet(const std::string& url, const std::string& bearer, int* status, std::string* body,
             std::string* error, const std::string& range) {
  CURL* h = curl_easy_init();
  if (!h) {
    *error = "libcurl failed";
    return false;
  }
  curl_slist* headers = nullptr;
  if (!bearer.empty()) headers = curl_slist_append(headers, ("Authorization: Bearer " + bearer).c_str());
  body->clear();
  curl_easy_setopt(h, CURLOPT_URL, url.c_str());
  curl_easy_setopt(h, CURLOPT_FOLLOWLOCATION, 1L);
  curl_easy_setopt(h, CURLOPT_NOSIGNAL, 1L);
  curl_easy_setopt(h, CURLOPT_CONNECTTIMEOUT, 30L);
  curl_easy_setopt(h, CURLOPT_LOW_SPEED_LIMIT, 1L);  // stalled for 60 s: give up
  curl_easy_setopt(h, CURLOPT_LOW_SPEED_TIME, 60L);
  curl_easy_setopt(h, CURLOPT_USERAGENT, "ReHoMM4");
  curl_easy_setopt(h, CURLOPT_WRITEFUNCTION, &Append);
  curl_easy_setopt(h, CURLOPT_WRITEDATA, body);
  if (headers) curl_easy_setopt(h, CURLOPT_HTTPHEADER, headers);
  if (!range.empty()) curl_easy_setopt(h, CURLOPT_RANGE, range.c_str());
  const CURLcode r = curl_easy_perform(h);
  long code = 0;
  curl_easy_getinfo(h, CURLINFO_RESPONSE_CODE, &code);
  curl_easy_cleanup(h);
  if (headers) curl_slist_free_all(headers);
  if (r != CURLE_OK) {
    *error = std::string("network error: ") + curl_easy_strerror(r);
    return false;
  }
  *status = int(code);
  return true;
}

// No picker: the installer files are found in the app's folder on the SD card.
bool BeginPickFiles(bool) { return false; }
bool PickedFiles(std::vector<PickedFile>* out) {
  out->clear();
  return true;
}
void EndPickedFiles() {}

std::string ScratchDir() { return std::string(kHome) + "/tmp"; }

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

}  // namespace h4::install
