// language.h: the language packs applied in place.
#include "language.h"

#include <SDL3/SDL.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>

#include <algorithm>
#include <cstdint>
#include <cstdio>
#include <filesystem>
#include <cstring>
#include <map>
#include <memory>
#include <mutex>
#include <thread>
#include <vector>

#include "install/json.h"
#include "install/platform.h"

namespace h4 {

namespace {

struct Entry {
  uint32_t pos;
  uint8_t en[12], tr[12];  // the header in English, and translated
};

struct Archive {
  std::string name;
  uint64_t size = 0, appended = 0;
  Sint64 data_at = 0;  // where its appended bytes start in the delta
  std::vector<Entry> entries;
};

struct Closer {
  void operator()(SDL_IOStream* io) const { SDL_CloseIO(io); }
};
using Io = std::unique_ptr<SDL_IOStream, Closer>;

bool Read(SDL_IOStream* io, void* p, size_t n) { return SDL_ReadIO(io, p, n) == n; }

template <typename T>
bool ReadLe(SDL_IOStream* io, T* v) {
  uint8_t b[sizeof(T)];
  if (!Read(io, b, sizeof b)) return false;
  *v = 0;
  for (size_t i = 0; i < sizeof b; i++) *v |= T(b[i]) << (8 * i);
  return true;
}

bool ReadIndex(SDL_IOStream* io, std::vector<Archive>* out) {
  char magic[4];
  uint32_t count = 0;
  if (!Read(io, magic, 4) || std::memcmp(magic, "H4P1", 4) || !ReadLe(io, &count) || count > 64)
    return false;
  for (uint32_t i = 0; i < count; i++) {
    Archive a;
    uint16_t n = 0;
    uint32_t entries = 0;
    if (!ReadLe(io, &n) || n == 0 || n > 64) return false;
    a.name.resize(n);
    if (!Read(io, &a.name[0], n) || a.name.find('/') != std::string::npos) return false;
    if (!ReadLe(io, &a.size) || !ReadLe(io, &a.appended) || !ReadLe(io, &entries) ||
        entries > 100000)
      return false;
    a.entries.resize(entries);
    for (Entry& e : a.entries)
      if (!ReadLe(io, &e.pos) || !Read(io, e.en, 12) || !Read(io, e.tr, 12)) return false;
    a.data_at = SDL_TellIO(io);
    if (a.data_at < 0 || SDL_SeekIO(io, Sint64(a.appended), SDL_IO_SEEK_CUR) < 0) return false;
    out->push_back(std::move(a));
  }
  return true;
}

// A file of the translation outside the archives (H4P1 then "H4PF", u32 count, per file: u16
// path length, the path below the game folder ('/', e.g. "maps/X.h4c"), u64 size, the bytes).
// Older apps read only the archives and stop there.
struct File {
  std::string path;
  uint64_t size = 0;
  Sint64 data_at = 0;
};

bool SafePath(const std::string& p) {
  if (p.empty() || p[0] == '/' || p.find('\\') != std::string::npos || p.find(':') != std::string::npos) return false;
  for (size_t s = 0; s <= p.size();) {
    const size_t e = std::min(p.find('/', s), p.size());
    const std::string part = p.substr(s, e - s);
    if (part.empty() || part == "." || part == "..") return false;
    s = e + 1;
  }
  return true;
}

// After ReadIndex: the files, if the pack has them (none: true, empty).
bool ReadFiles(SDL_IOStream* io, std::vector<File>* out) {
  char magic[4];
  if (!Read(io, magic, 4)) return true;  // an older pack: archives only
  uint32_t count = 0;
  if (std::memcmp(magic, "H4PF", 4) || !ReadLe(io, &count) || count > 100000) return false;
  for (uint32_t i = 0; i < count; i++) {
    File f;
    uint16_t n = 0;
    if (!ReadLe(io, &n) || n == 0 || n > 1024) return false;
    f.path.resize(n);
    if (!Read(io, &f.path[0], n) || !SafePath(f.path) || !ReadLe(io, &f.size)) return false;
    f.data_at = SDL_TellIO(io);
    if (f.data_at < 0 || SDL_SeekIO(io, Sint64(f.size), SDL_IO_SEEK_CUR) < 0) return false;
    out->push_back(std::move(f));
  }
  return true;
}

bool PRead(int fd, void* p, size_t n, uint64_t at) { return pread(fd, p, n, off_t(at)) == ssize_t(n); }
bool PWrite(int fd, const void* p, size_t n, uint64_t at) {
  return pwrite(fd, p, n, off_t(at)) == ssize_t(n);
}

struct Pack {
  std::string path;
  Io io;
  std::vector<Archive> archives;
};

bool LoadPack(const std::string& path, Pack* p) {
  p->path = path;
  p->io.reset(SDL_IOFromFile(path.c_str(), "rb"));
  return p->io && ReadIndex(p->io.get(), &p->archives);
}

// Whether the start of `a`'s appended data in pack `p` is what follows the original part of
// the archive open as fd (`size` bytes): that pack's translation is in it.
bool DataIn(Pack& p, const Archive& a, int fd, uint64_t size) {
  if (a.appended == 0 || size <= a.size) return false;
  const size_t n = size_t(std::min<uint64_t>({4096, a.appended, size - a.size}));
  std::vector<uint8_t> x(n), y(n);
  return PRead(fd, x.data(), n, a.size) && SDL_SeekIO(p.io.get(), a.data_at, SDL_IO_SEEK_SET) >= 0 &&
         Read(p.io.get(), y.data(), n) && x == y;
}

struct Fd {
  int fd = -1;
  ~Fd() {
    if (fd >= 0) close(fd);
  }
};

// The other pack's translation taken back out: its entries English again and, when another
// translation is to be put in (`truncate`), its data cut off the archives.
bool TakeOut(Pack& p, const std::string& game_dir, std::map<std::string, Fd>& files, bool truncate,
             std::string* error) {
  for (const Archive& a : p.archives) {
    Fd& f = files[a.name];
    if (f.fd < 0) f.fd = open((game_dir + "/Data/" + a.name).c_str(), O_RDWR);
    struct stat st {};
    if (f.fd < 0 || fstat(f.fd, &st) != 0) continue;  // checked by the pack put in
    const bool data = DataIn(p, a, f.fd, uint64_t(st.st_size));
    bool english = true;
    for (const Entry& e : a.entries) {
      uint8_t now[12];
      if (!PRead(f.fd, now, 12, e.pos)) {
        english = false;
        continue;
      }
      if (!std::memcmp(now, e.tr, 12) && std::memcmp(now, e.en, 12)) {
        if (!PWrite(f.fd, e.en, 12, e.pos)) {
          *error = "cannot write Data/" + a.name;
          return false;
        }
      } else if (std::memcmp(now, e.en, 12)) {
        english = false;  // neither: not this pack's archive
      }
    }
    if (data && truncate && english) {
      fsync(f.fd);
      if (ftruncate(f.fd, off_t(a.size)) != 0) {
        *error = "cannot write Data/" + a.name;
        return false;
      }
      std::printf("h4: language: %s taken out of Data/%s\n", p.path.c_str(), a.name.c_str());
    }
    fsync(f.fd);
  }
  return true;
}

}  // namespace

namespace {

// Never destroyed: the refresh thread may still be running when the app exits.
std::mutex& g_langs_mu = *new std::mutex;
std::vector<LanguageInfo>& g_langs = *new std::vector<LanguageInfo>;  // after English

const LanguageInfo& English() {
  static const LanguageInfo& en = *new LanguageInfo{
      "en", "English", "English", "Restart the game to apply the new language.", "", "", 0};
  return en;
}

// The app's own list (platforms/languages.py).
const std::vector<LanguageInfo>& Baked() {
  static const std::vector<LanguageInfo>& all = *new std::vector<LanguageInfo>{
#if defined(H4_LANGUAGES_INC)
#include H4_LANGUAGES_INC
#endif
  };
  return all;
}

bool Hex64(const std::string& s) {
  return s.size() == 64 && s.find_first_not_of("0123456789abcdef") == std::string::npos;
}
bool Web(const std::string& s) { return s.rfind("https://", 0) == 0 || s.rfind("http://", 0) == 0; }

// languages.json of a release -> its languages (only well-formed entries); false if it is not one.
bool ParseRelease(const std::string& text, std::vector<LanguageInfo>* out) {
  install::Json j;
  if (!install::Json::Parse(text, &j) || j["languages"].type != install::Json::Array) return false;
  for (size_t i = 0; i < j["languages"].size(); i++) {
    const install::Json& e = j["languages"][i];
    LanguageInfo l{e["code"].text(), e["name"].text(), e["english"].text(), e["note"].text(),
                   e["url"].text(), e["sha256"].text(), uint64_t(e["size"].integer())};
    if (l.code.empty() || l.code.size() > 12 || l.code == "en" ||
        l.code.find_first_not_of("abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ-") != std::string::npos ||
        l.name.empty() || !Web(l.url) || !Hex64(l.sha256) || l.size == 0)
      continue;
    if (l.english.empty()) l.english = l.name;
    out->push_back(std::move(l));
  }
  return true;
}

std::string ReadFile(const std::string& path) {
  std::string s;
  if (FILE* f = std::fopen(path.c_str(), "rb")) {
    char b[65536];
    for (size_t n; (n = std::fread(b, 1, sizeof b, f)) > 0;) s.append(b, n);
    std::fclose(f);
  }
  return s;
}

[[maybe_unused]] bool WriteFile(const std::string& path, const std::string& data) {
  const std::string tmp = path + ".tmp";
  FILE* f = std::fopen(tmp.c_str(), "wb");
  if (!f) return false;
  const bool ok = std::fwrite(data.data(), 1, data.size(), f) == data.size();
  if (std::fclose(f) != 0 || !ok) return false;
  return std::rename(tmp.c_str(), path.c_str()) == 0;
}

// The release's list, with the app's own packs (built in for testing: no address) kept first.
std::vector<LanguageInfo> Merge(const std::vector<LanguageInfo>& release) {
  std::vector<LanguageInfo> out;
  for (const LanguageInfo& l : Baked())
    if (l.url.empty()) out.push_back(l);
  for (const LanguageInfo& l : release) {
    bool have = false;
    for (const LanguageInfo& o : out) have |= o.code == l.code;
    if (!have) out.push_back(l);
  }
  return out;
}

}  // namespace

void LoadLanguages(const std::string& data_dir) {
  std::vector<LanguageInfo> release;
  const bool cached = ParseRelease(ReadFile(data_dir + "/languages.json"), &release);
  std::lock_guard<std::mutex> l(g_langs_mu);
  g_langs = cached ? Merge(release) : Baked();
}

std::vector<LanguageInfo> Languages() {
  std::lock_guard<std::mutex> l(g_langs_mu);
  std::vector<LanguageInfo> out{English()};
  out.insert(out.end(), g_langs.begin(), g_langs.end());
  return out;
}

LanguageInfo FindLanguage(const std::string& code) {
  for (const LanguageInfo& l : Languages())
    if (l.code == code) return l;
  return English();
}

void RefreshLanguages(const std::string& data_dir) {
#if defined(H4_TRANSLATIONS_URL)
  std::thread([data_dir] {
    int status = 0;
    std::string body, error;
    std::vector<LanguageInfo> release;
    if (!install::HttpGet(H4_TRANSLATIONS_URL, "", &status, &body, &error) || status != 200 ||
        !ParseRelease(body, &release)) {
      std::printf("h4: languages: no release list (%s)\n", error.empty() ? std::to_string(status).c_str() : error.c_str());
      return;
    }
    const std::string path = data_dir + "/languages.json";
    if (ReadFile(path) == body) return;  // the release already seen
    if (!WriteFile(path, body)) return;
    std::printf("h4: languages: a new release: %zu translations\n", release.size());
    std::lock_guard<std::mutex> l(g_langs_mu);
    g_langs = Merge(release);
  }).detach();
#else
  (void)data_dir;
#endif
}

std::string BundledPack(const std::string& code) {
  if (code == "en") return "";
#if defined(__ANDROID__)
  // In the APK's assets: SDL_IOFromFile reads them by a relative path.
  const std::string p = code + ".h4p";
  if (SDL_IOStream* io = SDL_IOFromFile(p.c_str(), "rb")) {
    SDL_CloseIO(io);
    return p;
  }
  return "";
#elif defined(H4_SWITCH_APP)
  const std::string p = "romfs:/" + code + ".h4p";
#else
  const char* base = SDL_GetBasePath();
  const std::string p = std::string(base ? base : "./") + code + ".h4p";
#endif
#if !defined(__ANDROID__)
  struct stat st {};
  return stat(p.c_str(), &st) == 0 ? p : "";
#endif
}

std::string TranslationFiles(const std::string& pack, const std::string& dir) {
  namespace fs = std::filesystem;
  std::error_code ec;
  std::vector<File> files;
  Io io;
  if (!pack.empty()) {
    io.reset(SDL_IOFromFile(pack.c_str(), "rb"));
    std::vector<Archive> archives;
    if (!io || !ReadIndex(io.get(), &archives) || !ReadFiles(io.get(), &files)) files.clear();
  }
  if (files.empty()) {
    fs::remove_all(dir, ec);
    return "";
  }
  // Which pack's files are there: its name, size and file list.
  uint64_t h = 1469598103934665603ull;
  auto mix = [&](const std::string& s) {
    for (unsigned char ch : s) h = (h ^ ch) * 1099511628211ull;
  };
  mix(pack.substr(pack.find_last_of('/') + 1) + " " + std::to_string(SDL_GetIOSize(io.get())));
  for (const File& f : files) mix(f.path + " " + std::to_string(f.size) + "\n");
  char want[32];
  std::snprintf(want, sizeof want, "%016llx", (unsigned long long)h);
  const std::string stamp = dir + "/.pack";
  if (FILE* s = std::fopen(stamp.c_str(), "r")) {
    char have[32] = {};
    const bool same = std::fgets(have, sizeof have, s) && !std::strcmp(have, want);
    std::fclose(s);
    if (same) return dir;
  }
  const std::string tmp = dir + ".tmp";
  fs::remove_all(tmp, ec);
  std::vector<uint8_t> buf(1 << 20);
  for (const File& f : files) {
    const fs::path to = fs::path(tmp) / f.path;
    fs::create_directories(to.parent_path(), ec);
    FILE* o = std::fopen(to.string().c_str(), "wb");
    bool ok = o && SDL_SeekIO(io.get(), f.data_at, SDL_IO_SEEK_SET) >= 0;
    for (uint64_t done = 0; ok && done < f.size;) {
      const size_t n = size_t(std::min<uint64_t>(buf.size(), f.size - done));
      ok = Read(io.get(), buf.data(), n) && std::fwrite(buf.data(), 1, n, o) == n;
      done += n;
    }
    if (o) ok = std::fclose(o) == 0 && ok;
    if (!ok) {
      std::printf("h4: language: cannot write %s\n", to.string().c_str());
      fs::remove_all(tmp, ec);
      return "";
    }
  }
  if (FILE* s = std::fopen((tmp + "/.pack").c_str(), "w")) {
    std::fputs(want, s);
    std::fclose(s);
  }
  fs::remove_all(dir, ec);
  fs::rename(tmp, dir, ec);
  if (ec) return "";
  std::printf("h4: language: %zu files of %s in %s\n", files.size(), pack.c_str(), dir.c_str());
  return dir;
}

bool SetGameLanguage(const std::string& game_dir, const std::string& code, const std::string& patch,
                     const std::vector<std::string>& others, std::string* error) {
  const bool translated = code != "en";
  // 0. Another translation in the archives: taken back out first (not a copy of this one: an
  //    older download of the same pack).
  {
    Pack self;
    const bool have_self = !patch.empty() && LoadPack(patch, &self);
    auto same = [&](const Pack& p) {
      if (!have_self || p.archives.size() != self.archives.size()) return false;
      for (size_t i = 0; i < p.archives.size(); i++) {
        const Archive &a = p.archives[i], &b = self.archives[i];
        if (a.name != b.name || a.size != b.size || a.appended != b.appended || a.entries.size() != b.entries.size())
          return false;
        for (size_t k = 0; k < a.entries.size(); k++)
          if (a.entries[k].pos != b.entries[k].pos || std::memcmp(a.entries[k].tr, b.entries[k].tr, 12)) return false;
      }
      // The same layout: the same data too? (Read only then: an upgrade's first start.)
      std::vector<uint8_t> x(1 << 20), y(1 << 20);
      for (size_t i = 0; i < p.archives.size(); i++) {
        const Archive &a = p.archives[i], &b = self.archives[i];
        if (SDL_SeekIO(p.io.get(), a.data_at, SDL_IO_SEEK_SET) < 0 ||
            SDL_SeekIO(self.io.get(), b.data_at, SDL_IO_SEEK_SET) < 0)
          return false;
        for (uint64_t done = 0; done < a.appended;) {
          const size_t n = size_t(std::min<uint64_t>(x.size(), a.appended - done));
          if (!Read(p.io.get(), x.data(), n) || !Read(self.io.get(), y.data(), n) ||
              std::memcmp(x.data(), y.data(), n))
            return false;
          done += n;
        }
      }
      return true;
    };
    std::map<std::string, Fd> fds;
    for (const std::string& o : others) {
      if (o == patch) continue;
      Pack p;
      if (!LoadPack(o, &p)) continue;  // not a pack: nothing of it can be in the archives
      if (same(p)) continue;
      if (!TakeOut(p, game_dir, fds, translated, error)) return false;
    }
  }
  if (!translated) {
    if (patch.empty()) return true;
    Pack p;
    std::map<std::string, Fd> fds;
    if (!LoadPack(patch, &p)) return true;
    return TakeOut(p, game_dir, fds, false, error);
  }
  Io io(SDL_IOFromFile(patch.c_str(), "rb"));
  std::vector<Archive> archives;
  if (!io || !ReadIndex(io.get(), &archives)) {
    *error = "the translation file is damaged";
    return false;
  }
  // 1. Check: every archive is the original (maybe with the data already appended) and every
  //    entry reads as English or translated. Nothing is changed before all are.
  std::vector<Fd> files(archives.size());
  std::vector<bool> appended(archives.size());
  for (size_t i = 0; i < archives.size(); i++) {
    const Archive& a = archives[i];
    const std::string path = game_dir + "/Data/" + a.name;
    files[i].fd = open(path.c_str(), O_RDWR);
    struct stat st {};
    if (files[i].fd < 0 || fstat(files[i].fd, &st) != 0) {
      *error = "cannot open Data/" + a.name;
      return false;
    }
    if (uint64_t(st.st_size) == a.size + a.appended) {
      appended[i] = true;
    } else if (uint64_t(st.st_size) < a.size || uint64_t(st.st_size) > a.size + a.appended) {
      // (In between: an append that was interrupted; it is written again from the start.)
      *error = "Data/" + a.name + " is not the GOG version the translation is for";
      return false;
    }
    for (const Entry& e : a.entries) {
      uint8_t now[12];
      if (!PRead(files[i].fd, now, 12, e.pos) ||
          (std::memcmp(now, e.en, 12) && std::memcmp(now, e.tr, 12))) {
        *error = "Data/" + a.name + " is not the GOG version the translation is for";
        return false;
      }
      // English entries pointing into the appended part cannot happen; translated ones need it.
      if (!std::memcmp(now, e.tr, 12) && std::memcmp(now, e.en, 12) && !appended[i]) {
        *error = "Data/" + a.name + " is damaged (translated without its data)";
        return false;
      }
    }
  }
  // 2. The data appended once (written after the original end, so English is not touched; an
  //    interrupted append is written again over its part).
  std::vector<uint8_t> buf(1 << 20);
  for (size_t i = 0; i < archives.size(); i++) {
    const Archive& a = archives[i];
    if (appended[i] || a.appended == 0) continue;
    if (SDL_SeekIO(io.get(), a.data_at, SDL_IO_SEEK_SET) < 0) {
      *error = "the translation file is damaged";
      return false;
    }
    for (uint64_t done = 0; done < a.appended;) {
      const size_t n = size_t(std::min<uint64_t>(buf.size(), a.appended - done));
      if (!Read(io.get(), buf.data(), n) || !PWrite(files[i].fd, buf.data(), n, a.size + done)) {
        if (ftruncate(files[i].fd, off_t(a.size)) != 0) {}
        *error = "cannot write Data/" + a.name + " (no space?)";
        return false;
      }
      done += n;
    }
    fsync(files[i].fd);
  }
  // 3. The entries.
  for (size_t i = 0; i < archives.size(); i++) {
    for (const Entry& e : archives[i].entries) {
      uint8_t now[12];
      if (PRead(files[i].fd, now, 12, e.pos) && !std::memcmp(now, e.tr, 12)) continue;
      if (!PWrite(files[i].fd, e.tr, 12, e.pos)) {
        *error = "cannot write Data/" + archives[i].name;
        return false;
      }
    }
    fsync(files[i].fd);
  }
  return true;
}

}  // namespace h4

// menu_button.h's FrameToView (shared by both platforms' buttons).
#include "menu_button.h"
#include "runtime/gfx/device.h"

namespace h4 {
void FrameToView(int x, int y, float w, float h, float* vx, float* vy) {
  const recomp::gfx::Presentation p = recomp::gfx::LastPresentation();
  if (p.src_w <= 0 || p.src_h <= 0) {
    *vx = *vy = -1;
    return;
  }
  const recomp::gfx::RectF r =
      recomp::gfx::ZoomedRect(recomp::gfx::FitRect(p.src_w, p.src_h, int(w), int(h)));
  *vx = r.l + float(x) * (r.r - r.l) / float(p.src_w);
  *vy = r.t + float(y) * (r.b - r.t) / float(p.src_h);
}
}  // namespace h4
