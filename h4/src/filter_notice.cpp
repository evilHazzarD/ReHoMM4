// filter_notice.h: the scaling filter F10 chose, kept and shown.

#include "filter_notice.h"

#include <algorithm>
#include <atomic>
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <mutex>
#include <thread>
#include <vector>

#include "random_map/font.h"
#include "random_map/locale.h"
#include "random_map_ui.h"
#include "runtime/gfx/device.h"

#if defined(_WIN32)
#include <stdlib.h>
// POSIX's: an existing variable kept unless `overwrite`.
static int setenv(const char* name, const char* value, int overwrite) {
  if (!overwrite && std::getenv(name)) return 0;
  return _putenv_s(name, value);
}
#endif

namespace h4 {
namespace {

// The runtime's names (gfx::SetPresentFilter) and how the notice calls them.
// In the runtime's order (F10's).
struct Name {
  const char* id;
  const char* shown;  // English; translated by random_map/translations.cpp
  const char* uk;     // the same in UTF-8, for the native settings
};
constexpr Name kNames[] = {
    {"nearest", "Nearest", "Без згладжування"},
    {"bilinear", "Bilinear", "Білінійне"},
    {"sharp", "Sharp bilinear", "Чітке білінійне"},
    {"bicubic", "Bicubic", "Бікубічне"},
    {"lanczos", "Lanczos", "Ланцош"},
    {"bicubic-sharp", "Sharp bicubic", "Чітке бікубічне"},
    {"fsr", "FSR", "FSR"},
};
constexpr int kCount = int(sizeof kNames / sizeof kNames[0]);

const Name* Find(const std::string& id) {
  for (const Name& n : kNames)
    if (id == n.id) return &n;
  return nullptr;
}

// " 3/7" for the third of the seven ("" for one not known).
std::string Number(const Name* n) {
  if (!n) return "";
  return " " + std::to_string(int(n - kNames) + 1) + "/" + std::to_string(kCount);
}

std::mutex g_mu;
std::string g_path;                 // where the choice is kept
std::string g_current;              // the filter in use ("" before it is known: H4_FILTER's)
std::atomic<uint32_t> g_shown{0};   // the notice's number: a later one hides it, not an older timer

// The notice's picture, `d` times finer than the frame: the text in the game's font on its dark
// panel colour, a gold edge.
std::vector<uint32_t> Picture(const random_map::Font& font, const std::string& text, float d, int* w, int* h) {
  const int pad_x = int(14 * d + 0.5f), pad_y = int(6 * d + 0.5f), edge = std::max(1, int(d + 0.5f));
  *w = font.Width(text) + 2 * pad_x;
  *h = font.Height() + 2 * pad_y;
  std::vector<uint32_t> px(size_t(*w) * size_t(*h), 0xFF1A1209u);
  for (int y = 0; y < *h; y++)
    for (int x = 0; x < *w; x++)
      if (x < edge || y < edge || x >= *w - edge || y >= *h - edge) px[size_t(y) * size_t(*w) + size_t(x)] = 0xFFB08D3Cu;
  font.Draw(px, *w, *h, pad_x, pad_y, text, 0xFFF0DCA0u);
  return px;
}

// Shows `name` for two seconds (on a thread of its own: the font may still be being read).
void Show(const std::string& name) {
  const uint32_t number = ++g_shown;
  std::thread([name, number] {
    const recomp::gfx::Presentation p = recomp::gfx::LastPresentation();
    // Screen pixels per frame pixel: the notice drawn that fine, in the biggest of the game's
    // fonts (prose_antique 16 ... 36) that is no bigger there than 16 in the frame.
    const float scale = p.src_w > 0 && p.rect.right > p.rect.left ? float(p.rect.right - p.rect.left) / float(p.src_w) : 1.0f;
    int size = 16;  // (then 20 to 36: no 18 is read)
    for (int s = 20; s <= 36 && float(s) <= 16.0f * scale + 1.0f; s += 2) size = s;
    const random_map::Font* font = nullptr;
    for (int i = 0; i < 60 && !(font = GameFont(size)); i++)  // (read once, on the generator's thread)
      std::this_thread::sleep_for(std::chrono::milliseconds(50));
    if (!font) font = GameFont(16), size = 16;
    if (!font || number != g_shown) return;
    const float d = float(size) / 16.0f;
    random_map::Locale locale;
    const char* lang = std::getenv("H4_LANG");
    locale.Set(lang ? lang : "");
    const Name* n = Find(name);
    const std::string text = locale.Text("Scaling") + Number(n) + ": " + locale.Text(n ? n->shown : name.c_str());
    int w = 0, h = 0;
    const std::vector<uint32_t> px = Picture(*font, text, d, &w, &h);
    const int fw = p.src_w > 0 ? p.src_w : 1024;
    recomp::gfx::SetPresentNoticeDense(px.data(), w, h, d);
    recomp::gfx::SetPresentNoticePos(std::max(0, (fw - int(float(w) / d)) / 2), 12, true);
    std::this_thread::sleep_for(std::chrono::seconds(2));
    if (number == g_shown) recomp::gfx::SetPresentNoticePos(0, 0, false);
  }).detach();
}

// The runtime's hook (on its presenting thread): F10 chose `name`.
void OnFilter(const char* name) {
  {
    std::lock_guard<std::mutex> l(g_mu);
    g_current = name;
    if (!g_path.empty())
      if (FILE* f = std::fopen(g_path.c_str(), "w")) {
        std::fputs(name, f);
        std::fclose(f);
      }
  }
  Show(name);
}

}  // namespace

void LoadFilter(const std::string& path) {
  {
    std::lock_guard<std::mutex> l(g_mu);
    g_path = path;
  }
  if (FILE* f = std::fopen(path.c_str(), "r")) {
    char buf[32] = {};
    const size_t n = std::fread(buf, 1, sizeof buf - 1, f);
    std::fclose(f);
    std::string id(buf, n);
    while (!id.empty() && (id.back() == '\n' || id.back() == '\r' || id.back() == ' ')) id.pop_back();
    if (Find(id)) setenv("H4_FILTER", id.c_str(), 0);  // (an H4_FILTER of the environment wins)
  }
  recomp::gfx::SetPresentFilterHook(OnFilter);
}

std::string FilterLabel(bool uk) {
  std::string id;
  {
    std::lock_guard<std::mutex> l(g_mu);
    id = g_current;
  }
  if (id.empty()) {
    const char* env = std::getenv("H4_FILTER");
    id = env ? env : "bilinear";  // (the runtime's own default)
  }
  const Name* n = Find(id);
  return std::string(uk ? "Масштабування" : "Scaling") + Number(n) + ": " + (n ? (uk ? n->uk : n->shown) : id);
}

void NextFilter() {
  {
    std::lock_guard<std::mutex> l(g_mu);
    std::string id = g_current;
    if (id.empty())
      if (const char* env = std::getenv("H4_FILTER")) id = env;
    int i = 0;
    while (i < kCount && id != kNames[i].id) i++;
    if (i == kCount) i = 1;  // none or unknown: the runtime's bilinear
    g_current = kNames[(i + 1) % kCount].id;  // shown at once; the runtime's hook agrees
  }
  recomp::gfx::RequestNextPresentFilter();
}

}  // namespace h4
