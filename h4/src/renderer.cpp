// renderer.h: the GPU or the CPU draws the frame on phones and tablets.

#include "renderer.h"

#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <mutex>

#include "filter_notice.h"

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
    ShowNotice("The graphics card is slow: the CPU draws the game from the next start (Settings: Renderer)",
               "Відеокарта повільна: з наступного запуску гру малюватиме процесор (Налаштування: Рендер)", 6);
  }
}

std::string RendererLabel(bool uk) {
  std::lock_guard<std::mutex> l(g_mu);
  std::string what;
  if (g_choice == "gpu") what = uk ? "Відеокарта" : "GPU";
  else if (g_choice == "soft") what = uk ? "Процесор" : "CPU";
  else what = std::string(uk ? "Авто" : "Auto") + " (" + (AutoSoft() ? (uk ? "процесор" : "CPU") : (uk ? "відеокарта" : "GPU")) + ")";
  const bool soft_next = g_choice == "soft" || (g_choice == "auto" && AutoSoft());
  std::string label = std::string(uk ? "Рендер: " : "Renderer: ") + what;
  if (soft_next != g_soft) label += uk ? " (з наступного запуску)" : " (from the next start)";
  return label;
}

void NextRenderer() {
  std::lock_guard<std::mutex> l(g_mu);
  // Auto, GPU, CPU, Auto, ... Choosing auto again forgets its verdict: it is measured anew.
  g_choice = g_choice == "auto" ? "gpu" : g_choice == "gpu" ? "soft" : "auto";
  if (g_choice == "auto") std::remove((g_dir + "/renderer_auto").c_str());
  WriteWord(g_dir + "/renderer", g_choice.c_str());
}

}  // namespace h4
