// intro.h: the setting's file.
#include "intro.h"

#include <cstdio>
#include <cstdlib>

namespace h4 {

namespace {
std::string g_path;
bool g_on = false;
}  // namespace

void LoadSkipIntro(const std::string& path) {
  g_path = path;
  g_on = false;
  if (FILE* f = std::fopen(path.c_str(), "r")) {
    g_on = std::fgetc(f) == '1';
    std::fclose(f);
  } else if (const char* env = std::getenv("H4_SKIP_INTRO"); env && *env) {
    // H4_SKIP_INTRO=1 or 0, no setting yet: the browser page's choice of older versions, kept.
    SetSkipIntro(*env == '1');
  }
  SkipIntroMovies(g_on);
  std::printf("h4: intro movies %s\n", g_on ? "skipped (settings)" : "shown");
}

bool SkipIntro() { return g_on; }

void SetSkipIntro(bool on) {
  g_on = on;
  if (FILE* f = std::fopen(g_path.c_str(), "w")) {
    std::fputs(on ? "1" : "0", f);
    std::fclose(f);
  }
}

const std::string& SkipIntroPath() { return g_path; }

}  // namespace h4
