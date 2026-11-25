// renderer.h: the GPU or the CPU draws the frame on phones and tablets.

#include "renderer.h"

#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <mutex>

#include "filter_notice.h"
#include "ui_text.h"

#if defined(_WIN32)
// POSIX's: an existing variable kept unless `overwrite`.
static int setenv(const char* name, const char* value, int overwrite) {
  if (!overwrite && std::getenv(name)) return 0;
  return _putenv_s(name, value);
}
#endif

namespace h4 {
namespace {

std::mutex g_mu;
std::string g_dir;           // the player folder
std::string g_choice = "auto";  // what <player>/renderer says
bool g_soft = false;         // the CPU draws now (H4_GFX=soft at start)
bool g_watch = false;        // auto on the GPU, nothing noted yet: presents are measured

std::string ReadWord(const std::string& path) {
  std::string w;
  if (FILE* f = std::fopen(path.c_str(), "r")) {
    char buf[16] = {};
    const size_t n = std::fread(buf, 1, sizeof buf - 1, f);
    std::fclose(f);
    w.assign(buf, n);
    while (!w.empty() && (w.back() == '\n' || w.back() == '\r' || w.back() == ' ')) w.pop_back();
  }
  return w;
}

void WriteWord(const std::string& path, const char* w) {
  if (FILE* f = std::fopen(path.c_str(), "w")) {
    std::fputs(w, f);
    std::fclose(f);
  }
}

// Auto's verdict: <player>/renderer_auto ("soft": the GPU was too slow; none: not known yet).
bool AutoSoft() { return ReadWord(g_dir + "/renderer_auto") == "soft"; }

}  // namespace

void LoadRenderer(const std::string& data_dir) {
  std::lock_guard<std::mutex> l(g_mu);
  g_dir = data_dir;
  const std::string c = ReadWord(data_dir + "/renderer");
  g_choice = c == "gpu" || c == "soft" ? c : "auto";
  const bool soft = g_choice == "soft" || (g_choice == "auto" && AutoSoft());
  if (soft) setenv("H4_GFX", "soft", 0);  // (an H4_GFX of the environment wins)
  const char* gfx = std::getenv("H4_GFX");
  g_soft = gfx && !std::strcmp(gfx, "soft");
  g_watch = g_choice == "auto" && !g_soft && !gfx;
  std::printf("h4: renderer %s (%s)%s\n", g_soft ? "CPU" : "GPU", g_choice.c_str(),
              g_watch ? ", the GPU's speed watched" : "");
}

void NotePresent(double ms) {
  // From 20 s after the start (the intro movies and the first loading done) for 60 s: a GPU
  // that holds the game's present 50 ms and more at least 30 times is too slow for it (the
  // Adreno 610 does so about every frame; a fast one only now and then, loading a screen).
  static const auto start = std::chrono::steady_clock::now();
  static int slow = 0;
  if (!g_watch) return;
  const double s = std::chrono::duration<double>(std::chrono::steady_clock::now() - start).count();
  if (s < 20) return;
  if (s > 80) {
    g_watch = false;  // fast enough: watched again at the next start
    std::printf("h4: renderer: the GPU is fast enough (%d slow frames)\n", slow);
    return;
  }
  if (ms >= 50 && ++slow >= 30) {
    g_watch = false;
    std::lock_guard<std::mutex> l(g_mu);
    WriteWord(g_dir + "/renderer_auto", "soft");
    std::printf("h4: renderer: the GPU is too slow (%d frames of 50 ms and more): the CPU from the next start\n", slow);
    ShowNotice("The graphics card is slow: the CPU draws the game from the next start (Settings: Renderer)", 6);
  }
}

std::vector<std::string> RendererChoices() { return {"auto", "gpu", "soft"}; }

std::string RendererChoice() {
  std::lock_guard<std::mutex> l(g_mu);
  return g_choice;
}

std::string RendererChoiceName(const std::string& choice, const std::string& lang) {
  std::lock_guard<std::mutex> l(g_mu);
  const bool auto_soft = AutoSoft();
  const char* what = choice == "gpu" ? "GPU" : choice == "soft" ? "CPU" : auto_soft ? "Auto (CPU)" : "Auto (GPU)";
  std::string name = TrIn(lang, what);
  const bool soft_next = choice == "soft" || (choice == "auto" && auto_soft);
  if (choice == g_choice && soft_next != g_soft) name = TrIn(lang, "{1} (from the next start)", {name});
  return name;
}

void SetRenderer(const std::string& choice) {
  if (choice != "auto" && choice != "gpu" && choice != "soft") return;
  std::lock_guard<std::mutex> l(g_mu);
  if (choice == g_choice) return;
  g_choice = choice;
  if (g_choice == "auto") std::remove((g_dir + "/renderer_auto").c_str());  // measured anew
  WriteWord(g_dir + "/renderer", g_choice.c_str());
}

namespace {
int g_layout = 1280;  // the layout in use
}

int CurrentLayout() { return g_layout; }

namespace {
std::string g_layout_file = "layout";
}

void LoadLayout(const std::string& data_dir, const std::string& fallback, const std::string& file) {
  std::lock_guard<std::mutex> l(g_mu);
  g_dir = data_dir;
  g_layout_file = file;
  const std::string saved = ReadWord(data_dir + "/" + file);
  const bool chosen = saved == "800" || saved == "1024" || saved == "1280";
  const std::string layout = chosen ? saved : fallback;
  setenv("H4_LAYOUT", layout.c_str(), 0);  // (the environment's wins)
  const char* e = std::getenv("H4_LAYOUT");
  g_layout = e && !std::strncmp(e, "1024", 4) ? 1024 : e && !std::strncmp(e, "800", 3) ? 800 : 1280;
  std::printf("h4: layout %s (%s, %s)\n", e ? e : "", chosen ? "chosen" : "this device's default", file.c_str());
}

int UseLayoutFile(const std::string& file, int fallback) {
  std::lock_guard<std::mutex> l(g_mu);
  g_layout_file = file;
  const std::string saved = ReadWord(g_dir + "/" + file);
  return saved == "800" ? 800 : saved == "1024" ? 1024 : saved == "1280" ? 1280 : fallback;
}

void SaveLayout(int width) {
  std::lock_guard<std::mutex> l(g_mu);
  g_layout = width;
  if (!g_dir.empty()) WriteWord(g_dir + "/" + g_layout_file, width == 800 ? "800" : width == 1024 ? "1024" : "1280");
}

}  // namespace h4
