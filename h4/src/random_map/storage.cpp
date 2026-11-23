#include "storage.h"

#include <sys/stat.h>
#include <unistd.h>
#if defined(_WIN32)
#include <io.h>
#include <windows.h>
#undef ReplaceFile  // WinBase macro would rename this public C++ function.
#endif

#include <cctype>
#include <cerrno>
#include <cstdio>
#include <cstring>
#include <ctime>
#include <string_view>

namespace h4::random_map {

std::string MapTitle(int64_t when, uint32_t seed) {
  const std::time_t t = std::time_t(when);
  std::tm tm{};
#if defined(_WIN32)
  localtime_s(&tm, &t);
#else
  localtime_r(&t, &tm);
#endif
  char s[48];
  std::snprintf(s, sizeof s, "rm_%02d-%02d-%02d_%02d:%02d:%02d_%u", tm.tm_year % 100, tm.tm_mon + 1, tm.tm_mday,
                tm.tm_hour, tm.tm_min, tm.tm_sec, seed);
  return s;
}

std::string MapFileName(const std::string& title) {
  std::string name = title;
  for (char& ch : name)
    if (ch == ':') ch = '-';
  return name + ".h4c";
}

namespace {
// rm_yy-mm-dd_hh-mm-ss_<seed>.h4c: the digits' and separators' places
bool IsNewName(const std::string& name) {
  constexpr std::string_view kShape = "rm_00-00-00_00-00-00_";
  if (name.size() <= kShape.size() + 4 || name.size() > kShape.size() + 10 + 4) return false;
  for (size_t i = 0; i < kShape.size(); ++i) {
    const char want = kShape[i], ch = char(std::tolower(static_cast<unsigned char>(name[i])));
    if (want == '0' ? !std::isdigit(static_cast<unsigned char>(ch)) : ch != want) return false;
  }
  for (size_t i = kShape.size(); i < name.size() - 4; ++i)
    if (!std::isdigit(static_cast<unsigned char>(name[i]))) return false;
  std::string tail = name.substr(name.size() - 4);
  for (char& ch : tail) ch = char(std::tolower(static_cast<unsigned char>(ch)));
  return tail == ".h4c";
}
bool IsOldName(const std::string& name);
}  // namespace

std::string TitleOfFile(const std::string& name) {
  if (IsNewName(name)) {
    std::string t = name.substr(0, name.size() - 4);
    t[0] = 'r', t[1] = 'm', t[14] = ':', t[17] = ':';
    return t;
  }
  if (IsOldName(name)) return "_Random_Map_" + name.substr(12, name.size() - 16);
  return {};
}

bool IsMapFileName(const std::string& name) { return IsNewName(name) || IsOldName(name); }

namespace {
bool IsOldName(const std::string& name) {
  constexpr std::string_view kHead = "_random_map_", kTail = ".h4c";
  if (name.size() <= kHead.size() + kTail.size() || name.size() > kHead.size() + 10 + kTail.size()) return false;
  auto same = [&](size_t at, std::string_view s) {
    for (size_t i = 0; i < s.size(); ++i)
      if (std::tolower(static_cast<unsigned char>(name[at + i])) != s[i]) return false;
    return true;
  };
  if (!same(0, kHead) || !same(name.size() - kTail.size(), kTail)) return false;
  for (size_t i = kHead.size(); i < name.size() - kTail.size(); ++i)
    if (name[i] < '0' || name[i] > '9') return false;
  return true;
}
}  // namespace

namespace {

// `bytes` into `path`.part, flushed to the disk; false: `error`, nothing left behind.
bool WriteTemp(const std::string& tmp, const std::vector<uint8_t>& bytes, std::string* error) {
  std::FILE* f = std::fopen(tmp.c_str(), "wb");
  if (!f) {
    if (error) *error = std::strerror(errno);
    return false;
  }
  const bool written = std::fwrite(bytes.data(), 1, bytes.size(), f) == bytes.size();
  const bool flushed = std::fflush(f) == 0 &&
#if defined(_WIN32)
                       _commit(_fileno(f)) == 0;
#else
                       fsync(fileno(f)) == 0;
#endif
  if (std::fclose(f) != 0 || !written || !flushed) {
    if (error) *error = std::strerror(errno ? errno : EIO);
    std::remove(tmp.c_str());
    return false;
  }
  return true;
}

}  // namespace

bool ReplaceFile(const std::string& path, const std::vector<uint8_t>& bytes, std::string* error) {
  const std::string tmp = path + ".part";
  if (!WriteTemp(tmp, bytes, error)) return false;
  if (std::rename(tmp.c_str(), path.c_str()) != 0) {
    if (error) *error = std::strerror(errno);
    std::remove(tmp.c_str());
    return false;
  }
  return true;
}

bool WriteNewFile(const std::string& path, const std::vector<uint8_t>& bytes, std::string* error) {
  auto fail = [&](const std::string& why) {
    if (error) *error = why;
    return false;
  };
  struct stat st;
  if (stat(path.c_str(), &st) == 0) return fail("exists");
  const std::string tmp = path + ".part";
  if (!WriteTemp(tmp, bytes, error)) return false;
  // A hard link never replaces a file; where there are none (FAT, the browser's storage) the
  // check above and rename have to do.
#if defined(_WIN32)
  // MoveFileEx without MOVEFILE_REPLACE_EXISTING retains the same no-clobber
  // contract as link().  WRITE_THROUGH makes the save durable before return.
  if (MoveFileExA(tmp.c_str(), path.c_str(), MOVEFILE_WRITE_THROUGH)) return true;
#else
  if (link(tmp.c_str(), path.c_str()) == 0) {
    std::remove(tmp.c_str());
    return true;
  }
#endif
  if (errno == EEXIST || stat(path.c_str(), &st) == 0) {
    std::remove(tmp.c_str());
    return fail("exists");
  }
  if (std::rename(tmp.c_str(), path.c_str()) != 0) {
    const std::string why = std::strerror(errno);
    std::remove(tmp.c_str());
    return fail(why);
  }
  return true;
}

}  // namespace h4::random_map
