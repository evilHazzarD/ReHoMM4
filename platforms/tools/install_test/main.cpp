// Mac test harness for h4/src/install (CMakeLists.txt).
#include <cstdio>
#include <cstring>
#include <string>
#include <vector>

#include "install/gog.h"
#include "install/inno.h"
#include "install/manifest.h"
#include "install/platform.h"

namespace h4::install {
std::string GogLoginCode(const std::string&) { return ""; }
std::vector<PickedFile> PickFiles() { return {}; }
void EndPickedFiles() {}
std::string LoadToken() { return ""; }
void SaveToken(const std::string&) {}
}  // namespace h4::install

using namespace h4::install;

int main(int argc, char** argv) {
  if (argc < 3) return 2;
  Task t;
  std::string err;
  if (!std::strcmp(argv[1], "inno") && argc == 4) {
    std::string exe = argv[2], dir = argv[3];
    std::vector<PickedFile> files = inno::FilesIn(exe.substr(0, exe.rfind('/')));
    PickedFile e, b;
    if (!inno::FindInstaller(files, &e, &b, &err)) return fprintf(stderr, "find: %s\n", err.c_str()), 1;
    printf("installer %s + %s\n", e.name.c_str(), b.name.c_str());
    if (!inno::Extract(e, b, dir + ".links", dir, &t, &err)) return fprintf(stderr, "extract: %s\n", err.c_str()), 1;
    printf("unpacked %llu bytes\n", (unsigned long long)t.done.load());
    auto bad = VerifyDir(dir, nullptr, t.cancel);
    printf("verify: %zu bad of %zu\n", bad.size(), Manifest().size());
    for (auto& b2 : bad) printf("  bad %s\n", b2.c_str());
    return bad.empty() ? 0 : 1;
  }
  if (!std::strcmp(argv[1], "gog")) {
    gog::Session s;
    bool ok = s.Download(argv[2], &t, &err);
    printf("download: %s (%s); %llu of %llu bytes already good\n", ok ? "ok" : "stopped", err.c_str(),
           (unsigned long long)t.done.load(), (unsigned long long)t.total.load());
    return 0;
  }
  return 2;
}
