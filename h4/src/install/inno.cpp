#include "install/inno.h"

#include <fcntl.h>
#include <vector>
#include <dirent.h>
#include <unistd.h>
#if defined(_WIN32)
#include <windows.h>
#endif

#include <algorithm>
#include <cctype>
#include <chrono>
#include <cstdio>
#include <cstdlib>

#include "install/gog.h"
#include "install/manifest.h"
#include "recomp_innoextract.h"

namespace h4::install::inno {

namespace {

std::string Lower(std::string s) {
  for (char& c : s) c = char(std::tolower((unsigned char)c));
  return s;
}

bool EndsWith(const std::string& s, const std::string& e) {
  return s.size() >= e.size() && !s.compare(s.size() - e.size(), e.size(), e);
}

struct Progress {
  Task* task;
};

int OnProgress(void* ctx, const char* current, uint64_t bytes) {
  Task* task = static_cast<Progress*>(ctx)->task;
  if (current) task->SetPhase(std::string("Unpacking ") + current);
  task->done = std::min<uint64_t>(bytes, task->total);
  return task->cancel ? 0 : 1;
}

}  // namespace

bool FindInstaller(const std::vector<PickedFile>& files, PickedFile* exe, PickedFile* bin,
                   std::string* error) {
  const PickedFile* e = nullptr;
  for (const PickedFile& f : files) {
    const std::string n = Lower(f.name);
    if (n.rfind("setup_", 0) == 0 && EndsWith(n, ".exe")) e = &f;
  }
  if (!e) {
    for (const PickedFile& f : files)
      if (EndsWith(Lower(f.name), "-1.bin")) {
        *error = "Pick " + f.name.substr(0, f.name.size() - 6) +
                 ".exe together with " + f.name + " (select both files).";
        return false;
      }
    *error =
        "No GOG installer was picked. Pick both setup_heroes_of_might_and_magic_4_complete_...exe "
        "and its ...-1.bin file.";
    return false;
  }
  const std::string want = Lower(e->name.substr(0, e->name.size() - 4) + "-1.bin");
  for (const PickedFile& f : files) {
    if (Lower(f.name) == want) {
      *exe = *e;
      *bin = f;
      return true;
    }
  }
  *error = "Pick " + e->name.substr(0, e->name.size() - 4) + "-1.bin together with " + e->name +
           " (select both files). If your browser renamed it (\"... (1).bin\"), rename it back.";
  return false;
}

std::vector<PickedFile> FilesIn(const std::string& dir) {
  std::vector<PickedFile> out;
  if (DIR* d = opendir(dir.c_str())) {
    while (dirent* e = readdir(d)) {
      const std::string n = Lower(e->d_name);
      if (n.rfind("._", 0) == 0) continue;  // macOS's metadata beside files copied to exFAT/FAT cards
      // (and the English heroes4.exe beside an installer in another language: Installer, StartUnpack)
      if ((n.rfind("setup_", 0) == 0 && EndsWith(n, ".exe")) || EndsWith(n, ".bin") || n == "heroes4.exe")
        out.push_back({dir + "/" + e->d_name, e->d_name});
    }
    closedir(d);
  }
  return out;
}

bool Extract(const PickedFile& exe, const PickedFile& bin, const std::string& stage,
             const std::string& dir, Task* task, std::string* error) {
  // innoextract finds "<name>-1.bin" next to "<name>.exe": links with those
  // names, whatever the picked files are called or wherever they are.
  RemoveTree(stage);
  MakeDirs(stage);
  const std::string sexe = stage + "/setup.exe", sbin = stage + "/setup-1.bin";
  // Android's picked files are descriptors (/proc/self/fd/N) that cannot be opened again by
  // path: empty stand-ins, read through the descriptors (recomp_inno_map_file).
  // Where there are no symlinks (the Switch's FAT/exFAT SD card): the same, through descriptors
  // opened here (closed when done).
  std::vector<int> opened;
  auto stand_in = [](const std::string& name, int fd) {
    FILE* fp = fopen(name.c_str(), "wb");
    if (!fp) return false;
    fclose(fp);
    recomp_inno_map_file(name.c_str(), fd);
    return true;
  };
  auto stage_file = [&](const PickedFile& f, const std::string& name) {
    const std::string fd_prefix = "/proc/self/fd/";
    if (f.path.compare(0, fd_prefix.size(), fd_prefix) == 0)
      return stand_in(name, atoi(f.path.c_str() + fd_prefix.size()));
#if defined(_WIN32)
    // Symlinks need a privilege on many Windows installs. A hard link is
    // equivalent for innoextract and avoids duplicating a multi-gigabyte
    // offline installer; CopyFile is the cross-volume fallback.
    if (CreateHardLinkA(name.c_str(), f.path.c_str(), nullptr) ||
        CopyFileA(f.path.c_str(), name.c_str(), TRUE)) return true;
#elif !defined(__EMSCRIPTEN__)
    if (symlink(f.path.c_str(), name.c_str()) == 0) return true;
#endif
    // (The browser: through descriptors too, read 64 KiB at a time with pread and counted in the
    // log, where a link would be read through a stream's few-KiB buffer.)
    const int fd = open(f.path.c_str(), O_RDONLY);
    if (fd < 0) return false;
    opened.push_back(fd);
    return stand_in(name, fd);
  };
  struct Unmap {
    std::vector<int>& fds;
    ~Unmap() {
      recomp_inno_map_file(nullptr, -1);
      for (int fd : fds) close(fd);
    }
  } unmap{opened};
  if (!stage_file(exe, sexe) || !stage_file(bin, sbin)) {
    *error = "cannot prepare the installer files";
    return false;
  }
  task->SetPhase("Reading the installer");
  char id[64] = "", err[512] = "";
  if (recomp_inno_gog_id(sexe.c_str(), id, sizeof id, err, sizeof err) != RECOMP_INNO_OK) {
    *error = std::string("This is not a GOG installer that can be read: ") + err;
    RemoveTree(stage);
    return false;
  }
  if (std::string(id) != gog::kProductId) {
    *error = exe.name + " is not GOG's Heroes of Might and Magic IV: Complete installer.";
    RemoveTree(stage);
    return false;
  }
  std::vector<const char*> paths;
  for (const ManifestFile& f : Manifest()) paths.push_back(f.path);
  MakeDirs(dir);
  task->total = ManifestBytes();
  task->done = 0;
  task->SetPhase("Unpacking");
  Progress p{task};
  const auto unpack_start = std::chrono::steady_clock::now();
  int r = recomp_inno_extract(sexe.c_str(), dir.c_str(), paths.data(), paths.size(), OnProgress, &p,
                              err, sizeof err);
  std::fprintf(stderr, "installer: unpacking took %.1f s\n",
               std::chrono::duration<double>(std::chrono::steady_clock::now() - unpack_start).count());
  RemoveTree(stage);
  if (r == RECOMP_INNO_CANCELLED) {
    *error = "cancelled";
    return false;
  }
  if (r != RECOMP_INNO_OK) {
    *error = std::string("Unpacking failed: ") + err;
    return false;
  }
  return true;
}

}  // namespace h4::install::inno
