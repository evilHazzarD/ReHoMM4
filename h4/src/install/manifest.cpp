#include "install/manifest.h"

#include <dirent.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/statvfs.h>
#include <unistd.h>

#include <algorithm>
#include <cctype>
#include <cstring>
#include <vector>

#include "install/hash.h"

namespace h4::install {

namespace {

const std::vector<ManifestFile> kManifest = {
#include "install/manifest_en.inc"
};

std::string Folded(const std::string& p) {
  std::string s = p;
  for (char& c : s) c = c == '\\' ? '/' : char(std::tolower((unsigned char)c));
  return s;
}

}  // namespace

const std::vector<ManifestFile>& Manifest() { return kManifest; }

uint64_t ManifestBytes() {
  uint64_t n = 0;
  for (const ManifestFile& f : kManifest) n += f.size;
  return n;
}

const ManifestFile* FindInManifest(const std::string& path) {
  const std::string want = Folded(path);
  for (const ManifestFile& f : kManifest)
    if (Folded(f.path) == want) return &f;
  return nullptr;
}

std::vector<std::string> VerifyDir(const std::string& dir,
                                   const std::function<void(uint64_t, uint64_t)>& progress,
                                   const std::atomic<bool>& cancel) {
  std::vector<std::string> bad;
  const uint64_t total = ManifestBytes();
  uint64_t done = 0;
  std::vector<uint8_t> buf(1 << 20);
  for (const ManifestFile& f : kManifest) {
    if (cancel) break;
    const std::string p = dir + "/" + f.path;
    struct stat st;
    if (stat(p.c_str(), &st) != 0 || uint64_t(st.st_size) != f.size) {
      bad.push_back(f.path);
      done += f.size;
      continue;
    }
    int fd = open(p.c_str(), O_RDONLY);
    if (fd < 0) {
      bad.push_back(f.path);
      done += f.size;
      continue;
    }
    Sha256 h;
    ssize_t n;
    while (!cancel && (n = read(fd, buf.data(), buf.size())) > 0) {
      h.Update(buf.data(), size_t(n));
      done += uint64_t(n);
      if (progress) progress(done, total);
    }
    close(fd);
    if (h.HexDigest() != f.sha256) bad.push_back(f.path);
  }
  return bad;
}

bool MakeDirs(const std::string& p) {
  for (size_t i = 1; i <= p.size(); i++)
    if (i == p.size() || p[i] == '/') mkdir(p.substr(0, i).c_str(), 0755);
  return IsDirectory(p);
}

bool IsDirectory(const std::string& p) {
  struct stat st;
  return stat(p.c_str(), &st) == 0 && S_ISDIR(st.st_mode);
}

bool IsFile(const std::string& p) {
  struct stat st;
  return stat(p.c_str(), &st) == 0 && S_ISREG(st.st_mode);
}

long long FreeBytes(const std::string& dir) {
  struct statvfs s;
  if (statvfs(dir.c_str(), &s) != 0) return -1;
  return (long long)s.f_bavail * (long long)s.f_frsize;
}

bool RemoveTree(const std::string& p) {
  struct stat st;
  if (lstat(p.c_str(), &st) != 0) return true;
  if (S_ISDIR(st.st_mode)) {
    if (DIR* d = opendir(p.c_str())) {
      while (dirent* e = readdir(d)) {
        if (!std::strcmp(e->d_name, ".") || !std::strcmp(e->d_name, "..")) continue;
        RemoveTree(p + "/" + e->d_name);
      }
      closedir(d);
    }
    return rmdir(p.c_str()) == 0;
  }
  return unlink(p.c_str()) == 0;
}

}  // namespace h4::install
