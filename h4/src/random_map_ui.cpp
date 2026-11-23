// The random map generator in the scenario list (random_map_ui.h).
#include "random_map_ui.h"

#include <SDL3/SDL.h>
#include <sys/stat.h>

#include <algorithm>
#include <atomic>
#include <cstdio>
#include <cstdlib>
#include <cerrno>
#include <cstring>
#include <ctime>
#include <filesystem>
#include <map>
#include <memory>
#include <mutex>
#include <random>
#include <sstream>
#include <thread>

#include "random_map/catalog.h"
#include "random_map/font.h"
#include "random_map/generator.h"
#include "random_map/hero_card.h"
#include "random_map/map_settings.h"
#include "random_map/panel.h"
#include "random_map/resources.h"
#include "random_map/storage.h"
#include "random_map/template.h"
#include "runtime/gfx/device.h"
#include "runtime/hle/kernel32_state.h"
#include "runtime/hle/user32_window.h"

#if defined(__EMSCRIPTEN__)
#include <emscripten/em_asm.h>
#include <emscripten/threading.h>
#endif

namespace h4 {
namespace {

using random_map::Panel;

// The game's install as the game sees it (main.cpp's GameConfig::guest_dir).
constexpr const char* kGuestInstall = "C:\\Heroes4\\";
constexpr const char* kFontName = "font.prose_antique.16.h4d";

// What is read from the game once (on a thread of its own; the dialog shows meanwhile):
// the font first (then the button shows), then the generator's catalogue.
struct Data {
  random_map::Resources res;
  random_map::Font font;
  random_map::Catalog catalog;
  random_map::HeroArt art;
};
Data* g_data = nullptr;               // set before g_load is 2, never freed
std::atomic<int> g_load{0};           // 0 not started, 1 reading, 2 font read, 3 all, -1 failed
std::string g_load_error;             // (set before g_load is -1)

// A map being made: the worker's result, taken by RandomMapPass.
struct Job {
  std::atomic<bool> cancel{false}, done{false};
  bool ok = false;
  std::string message;  // the game's bytes
};
std::shared_ptr<Job> g_job;

// Shared with the input hook (the SDL thread): the panel, where it is and what was clicked.
std::mutex g_mu;
Panel g_panel;
bool g_panel_ready = false;
bool g_shown = false;      // the button or the panel is on the frame
bool g_open = false;       // the panel (else the button)
int g_x = 0, g_y = 0, g_w = 0, g_h = 0;  // the picture's place in the frame
int g_pressed = 0;         // the button pressed (Panel::ButtonAt; a click is its release over it)
bool g_open_wanted = false;
bool g_remove_wanted = false;  // the remove button clicked
bool g_edit_wanted = false;    // the edit one
std::string g_edit_guest;      // the map the panel edits (its guest path), "" a new one
std::string g_made_folder;     // where the map being made goes (the list is shown there after)
// The remove button: the random map selected in the list (its file name) and whether the next
// click removes it (the first only asks, for a few seconds).
std::string g_selected;
bool g_remove_armed = false;
uint64_t g_armed_at = 0;
Panel::Action g_action = Panel::kNone;
const void* g_badge = nullptr;  // the picture last given to SetPresentBadge
bool g_badge_ours = false;

uint32_t RandomSeed() {
  std::random_device rd;
  return uint32_t((uint64_t(rd()) << 16 ^ rd()) % 1000000000u);
}

std::string GuestToHost(const std::string& rel, recomp::PathAccess access = recomp::PathAccess::kRead) {
  std::string g = rel;
  std::replace(g.begin(), g.end(), '/', '\\');
  return recomp::HostPath(g.rfind("C:\\", 0) == 0 ? g : kGuestInstall + g, access);
}

bool Exists(const std::string& host) {
  struct stat st;
  return stat(host.c_str(), &st) == 0;
}

std::string RandomMapsFolder(const std::string& folder);

// The templates for the panel (g_mu held): the built-in ones, then the player's files in the
// generated maps' folder's Templates\ (the player's data and the install's, as the game lists
// folders), by name. A file that does not read is left out (stderr says why). The folder made
// with an example in it the first time, so the player finds it and its format.
void LoadTemplates(const std::string& list_folder) {
  std::vector<random_map::TemplateInfo> list = random_map::BuiltInTemplates();
  const std::string guest = RandomMapsFolder(list_folder) + "Templates\\";
  namespace fs = std::filesystem;
  std::error_code ec;
  const std::string own = GuestToHost(guest, recomp::PathAccess::kWrite);
  if (!own.empty()) {
    // Our files: written where they are not; a newer version of one over the older, unless the
    // player changed it (.shipped: the sum of each file as we last wrote it).
    const bool fresh = !fs::exists(own, ec);
    fs::create_directories(own, ec);
    const fs::path stamp = fs::path(own) / ".shipped";
    std::map<std::string, std::string> wrote;  // file -> its sum as written
    {
      std::vector<uint8_t> b;
      if (random_map::ReadFile(stamp.string(), b)) {
        std::istringstream in(std::string(b.begin(), b.end()));
        for (std::string name, sum; in >> name >> sum;) wrote[name] = sum;
      }
    }
    bool changed = false;
    auto put = [&](const char* name, const std::string& text) {
      const fs::path p = fs::path(own) / name;
      const std::string sum = random_map::TemplateSum(text);
      std::vector<uint8_t> old;
      const bool there = random_map::ReadFile(p.string(), old);
      if (there) {
        const std::string now = random_map::TemplateSum(std::string(old.begin(), old.end()));
        if (now == sum) {  // ours, as it is now
          changed |= wrote[name] != sum;
          wrote[name] = sum;
          return;
        }
        const auto it = wrote.find(name);
        if (it == wrote.end() || it->second != now) return;  // the player's own, or changed by them
      }
      const std::vector<uint8_t> bytes(text.begin(), text.end());
      if (there ? random_map::ReplaceFile(p.string(), bytes, nullptr) : random_map::WriteNewFile(p.string(), bytes, nullptr)) {
        wrote[name] = sum;
        changed = true;
      }
    };
    for (int i = 0; i < random_map::kShippedTemplateCount; ++i)
      put(random_map::kShippedTemplates[i].file, random_map::kShippedTemplates[i].text);
    if (fresh || wrote.count(random_map::kTemplateExampleName))  // (removed by the player: left so)
      if (fresh || fs::exists(fs::path(own) / random_map::kTemplateExampleName, ec))
        put(random_map::kTemplateExampleName, random_map::kTemplateExample);
    if (changed) {
      std::string text;
      for (const auto& [name, sum] : wrote) text += name + " " + sum + "\n";
      const std::vector<uint8_t> bytes(text.begin(), text.end());
      if (!random_map::ReplaceFile(stamp.string(), bytes, nullptr)) random_map::WriteNewFile(stamp.string(), bytes, nullptr);
    }
  }
  std::vector<std::string> seen;
  for (const std::string& dir : {own, GuestToHost(guest)}) {
    if (dir.empty() || !fs::is_directory(dir, ec)) continue;
    std::vector<fs::path> files;
    for (const auto& e : fs::directory_iterator(dir, ec)) {
      std::string ext = e.path().extension().string();
      for (char& ch : ext) ch = char(std::tolower(static_cast<unsigned char>(ch)));
      if (e.is_regular_file(ec) && ext == ".txt") files.push_back(e.path());
    }
    std::sort(files.begin(), files.end());
    for (const fs::path& f : files) {
      if (std::find(seen.begin(), seen.end(), f.filename().string()) != seen.end()) continue;
      seen.push_back(f.filename().string());
      std::vector<uint8_t> bytes;
      if (!random_map::ReadFile(f.string(), bytes)) continue;
      const std::string text(bytes.begin(), bytes.end());
      std::vector<random_map::Template> parsed;
      std::string err;
      if (!random_map::ParseTemplates(text, parsed, &err)) {
        std::fprintf(stderr, "h4: random map template %s: %s\n", f.filename().string().c_str(), err.c_str());
        continue;
      }
      for (random_map::TemplateInfo& t : random_map::TemplatesOf(text)) {
        bool taken = false;  // (a name once: the first file's)
        for (const auto& o : list) taken |= o.name == t.name;
        if (!taken) list.push_back(std::move(t));
      }
    }
  }
  g_panel.SetTemplates(std::move(list));
}

void WakeIdleGame();  // (below)

void Load() {
  auto* d = new Data;
  std::string err;
  const random_map::Resources::PathOf path_of = [](const std::string& p) { return GuestToHost(p); };
  if (!d->res.Open(path_of, &err) || !d->font.Load(d->res, kFontName)) {
    g_load_error = err.empty() ? std::string("no ") + kFontName : err;
    std::fprintf(stderr, "h4: random map: %s\n", g_load_error.c_str());
    g_load = -1;
    return;
  }
  g_data = d;
  g_load = 2;
  WakeIdleGame();  // the button can show now
  std::vector<uint8_t> exe;
  random_map::ReadFile(GuestToHost("heroes4.exe"), exe);
  const uint64_t t0 = SDL_GetTicks();
  if (!d->catalog.Load(d->res, exe, &err)) {
    g_load_error = err;
    std::fprintf(stderr, "h4: random map: %s\n", err.c_str());
    g_load = -1;
    return;
  }
  if (!d->art.Load(d->res)) std::fprintf(stderr, "h4: random map: no skills or creatures for the hero card\n");
  std::printf("h4: random map: the game's data read (%llu ms)\n", (unsigned long long)(SDL_GetTicks() - t0));
  g_load = 3;
  WakeIdleGame();
}

// A point of the window (the event's) in the frame's pixels.
bool ToFrame(SDL_WindowID id, float vx, float vy, int* fx, int* fy) {
  const recomp::gfx::Presentation p = recomp::gfx::LastPresentation();
  SDL_Window* win = SDL_GetWindowFromID(id);
  int ww = 0, wh = 0;
  if (win) {
    SDL_GetWindowSize(win, &ww, &wh);
  } else if (SDL_Window** wins = SDL_GetWindows(nullptr)) {
    if (wins[0]) SDL_GetWindowSize(wins[0], &ww, &wh);
    SDL_free(wins);
  }
  if (p.src_w <= 0 || p.src_h <= 0 || ww <= 0 || wh <= 0) return false;
  const recomp::gfx::RectF r = recomp::gfx::ZoomedRect(recomp::gfx::FitRect(p.src_w, p.src_h, ww, wh));
  if (r.r <= r.l || r.b <= r.t) return false;
  *fx = int((vx - r.l) * float(p.src_w) / (r.r - r.l));
  *fy = int((vy - r.t) * float(p.src_h) / (r.b - r.t));
  return true;
}

int KeyOf(const SDL_KeyboardEvent& k) {
  switch (k.scancode) {
    case SDL_SCANCODE_ESCAPE: return 27;
    case SDL_SCANCODE_RETURN:
    case SDL_SCANCODE_KP_ENTER: return '\r';
    case SDL_SCANCODE_BACKSPACE: return '\b';
    case SDL_SCANCODE_0:
    case SDL_SCANCODE_KP_0: return '0';
    default: break;
  }
  if (k.scancode >= SDL_SCANCODE_1 && k.scancode <= SDL_SCANCODE_9) return '1' + (k.scancode - SDL_SCANCODE_1);
  if (k.scancode >= SDL_SCANCODE_KP_1 && k.scancode <= SDL_SCANCODE_KP_9) return '1' + (k.scancode - SDL_SCANCODE_KP_1);
  return 0;
}

bool OnInputLocked(const SDL_Event& e);

// recomp::SetAppInputHook: every mouse (touch, the gamepad's pointer) and key event before the
// game. The button takes its clicks; the open panel takes every click, wheel and key (the game's
// pointer still moves).
// Where the pointer is in the frame: from its moves, as a button's own place is not always given
// (the gamepad's, a test's); -1 unknown.
int g_px = -1, g_py = -1;
// x86recomp's test input (<ID>_TEST_INPUT "pointer"): places in the frame's pixels already.
constexpr SDL_MouseID kTestPointer = 0xD51D51;

// The game waits for its input between frames when nothing moves: an event the panel takes (the
// game never sees it) would show only with the next one. A pointer move where it is, after it, has
// the game run its loop (and the panel drawn) at once.
void WakeGame(const SDL_Event& e) {
  SDL_Event m{};
  m.type = SDL_EVENT_MOUSE_MOTION;
  m.motion.windowID = e.type == SDL_EVENT_MOUSE_BUTTON_DOWN || e.type == SDL_EVENT_MOUSE_BUTTON_UP ? e.button.windowID
                                                                                                    : e.key.windowID;
  m.motion.which = e.type == SDL_EVENT_MOUSE_BUTTON_DOWN || e.type == SDL_EVENT_MOUSE_BUTTON_UP ? e.button.which : 0;
  if (m.motion.which == kTestPointer) return;  // (a test's input: its own pace)
  float x = 0, y = 0;
  SDL_GetMouseState(&x, &y);
  m.motion.x = x;
  m.motion.y = y;
  SDL_PushEvent(&m);
}

// Something changed on the panel's side with no input (the game's data read, a map made, the
// scenario list just opened): the game, idle and waiting for input (the phones), would show it
// only after a touch. A pointer move where the pointer is has it draw a frame. Any thread.
void WakeIdleGame() {
  SDL_Window* w = SDL_GetMouseFocus();
  if (!w) w = SDL_GetKeyboardFocus();
  if (!w) return;
  SDL_Event m{};
  m.type = SDL_EVENT_MOUSE_MOTION;
  m.motion.windowID = SDL_GetWindowID(w);
  float x = 0, y = 0;
  SDL_GetMouseState(&x, &y);
  m.motion.x = x;
  m.motion.y = y;
  SDL_PushEvent(&m);
}

bool OnInput(const SDL_Event& e) {
  const bool taken = OnInputLocked(e);
  if (taken && (e.type == SDL_EVENT_MOUSE_BUTTON_DOWN || e.type == SDL_EVENT_MOUSE_BUTTON_UP ||
                e.type == SDL_EVENT_KEY_DOWN))
    WakeGame(e);
  return taken;
}

bool OnInputLocked(const SDL_Event& e) {
  std::lock_guard<std::mutex> l(g_mu);
  if (e.type == SDL_EVENT_MOUSE_MOTION) {
    if (e.motion.which == kTestPointer) {
      g_px = int(e.motion.x);
      g_py = int(e.motion.y);
    } else if (!ToFrame(e.motion.windowID, e.motion.x, e.motion.y, &g_px, &g_py)) {
      g_px = g_py = -1;
    }
    return false;
  }
  if (!g_shown) return false;
  switch (e.type) {
    case SDL_EVENT_MOUSE_BUTTON_DOWN:
    case SDL_EVENT_MOUSE_BUTTON_UP: {
      const int fx = g_px, fy = g_py;
      const bool in = fx >= g_x && fy >= g_y && fx < g_x + g_w && fy < g_y + g_h;
      const bool down = e.type == SDL_EVENT_MOUSE_BUTTON_DOWN;
      if (g_open) {
        if (down && in && (e.button.button == SDL_BUTTON_LEFT || e.button.button == SDL_BUTTON_RIGHT)) {
          const Panel::Action a = g_panel.Click(fx - g_x, fy - g_y, e.button.button == SDL_BUTTON_RIGHT);
          if (a != Panel::kNone) g_action = a;
        }
        return true;
      }
      const int which = in ? g_panel.ButtonAt(fx - g_x, fy - g_y) : 0;
      if (down && which && e.button.button == SDL_BUTTON_LEFT) {
        g_pressed = which;
        return true;
      }
      if (!down && g_pressed && e.button.button == SDL_BUTTON_LEFT) {
        if (which == g_pressed)
          (which == Panel::kOpenButton ? g_open_wanted : which == Panel::kEditButton ? g_edit_wanted : g_remove_wanted) = true;
        g_pressed = 0;
        return true;
      }
      return false;
    }
    case SDL_EVENT_MOUSE_WHEEL:
      return g_open;
    case SDL_EVENT_TEXT_INPUT:  // an on-screen keyboard's digits (with text input on: not as keys)
      if (!g_open) return false;
      for (const char* t = e.text.text; t && *t; ++t)
        if (*t >= '0' && *t <= '9') g_panel.Key(*t);
      return true;
    case SDL_EVENT_KEY_DOWN:
      if (!g_open) return false;
      if (const int k = KeyOf(e.key)) {
        // With text input on, a digit comes as its text too (above): not twice.
        SDL_Window* w = SDL_GetWindowFromID(e.key.windowID);
        if (k >= '0' && k <= '9' && w && SDL_TextInputActive(w)) return true;
        const Panel::Action a = g_panel.Key(k);
        if (a != Panel::kNone) g_action = a;
      }
      return true;
    case SDL_EVENT_KEY_UP:
      return g_open;
    default:
      return false;
  }
}

// Starts making `setup`'s map into `path` (g_mu held): a new file, or over the edited one.
void StartJob(const random_map::MapSetup& setup, const std::string& path, bool replace) {
  auto job = std::make_shared<Job>();
  g_job = job;
  const std::string title = setup.title.empty() ? "_Random_Map_" + std::to_string(setup.seed) : setup.title;
  g_panel.SetMode(Panel::Mode::kWorking, g_panel.Text("Creating") + " " + title + "...");
  const std::string created = g_panel.Text(replace ? "Updated" : "Created") + " " + title + ": " +
                              g_panel.Text("it is at the top of the list.");
  const std::string failed = g_panel.Text("Could not create the map:") + " ";
  const random_map::Catalog* catalog = &g_data->catalog;
  std::thread([job, setup, path, replace, created, failed, catalog] {
    random_map::GeneratedMap map;
    std::string err;
    const uint64_t t0 = SDL_GetTicks();
    bool ok = random_map::GenerateMap(*catalog, setup, map, &err, &job->cancel);
    if (ok && !job->cancel)
      ok = replace ? random_map::ReplaceFile(path, map.file, &err) : random_map::WriteNewFile(path, map.file, &err);
    std::printf("h4: random map: seed %u, size %d, %zu players: %s (%llu ms, %d attempts) %s\n", setup.seed,
                setup.size, setup.players.size(), ok ? (replace ? "updated" : "made") : err.c_str(),
                (unsigned long long)(SDL_GetTicks() - t0), map.attempts, ok ? path.c_str() : "");
    job->ok = ok && !job->cancel;
    job->message = job->ok ? created : failed + err;
    job->done = true;
    WakeIdleGame();  // its message, and the map in the list
  }).detach();
}

// Whether the game's data is there to make a map with and a player is human; else says why.
bool ReadyToMake() {
  const int load = g_load;
  if (load == -1) {
    g_panel.SetMode(Panel::Mode::kEdit, g_panel.Text("Could not create the map:") + " " + g_load_error);
    return false;
  }
  if (load != 3) {
    g_panel.SetMode(Panel::Mode::kEdit, g_panel.Text("Reading the game's data..."));
    return false;
  }
  const random_map::MapSetup& setup = g_panel.setup();
  if (std::none_of(setup.players.begin(), setup.players.end(), [](const auto& p) { return p.human; })) {
    g_panel.SetMode(Panel::Mode::kEdit, g_panel.Text("At least one player must be human."));
    return false;
  }
  return true;
}

// Starts making a new map (g_mu held).
// The generated maps' folder for the list's `folder`: its maps folder's RandomMaps.
std::string RandomMapsFolder(const std::string& folder) {
  if (IsRandomMapsFolder(folder)) return folder;
  std::string lower = folder;
  for (char& ch : lower) ch = char(std::tolower(static_cast<unsigned char>(ch)));
  const size_t at = lower.rfind("\\maps\\");
  const std::string base = at == std::string::npos ? folder : folder.substr(0, at + 6);
  return base + random_map::kMapFolder + "\\";
}

void Create(const std::string& list_folder) {
  if (!ReadyToMake()) return;
  random_map::MapSetup setup = g_panel.setup();
  const std::string folder = RandomMapsFolder(list_folder);
  // The file: named by the time and seed; a seed the player chose is never changed, a rolled one
  // is rolled again.
  setup.title = random_map::MapTitle(int64_t(std::time(nullptr)), setup.seed);
  std::string guest = folder + random_map::MapFileName(setup.title);
  for (int i = 0; Exists(GuestToHost(guest)); ++i) {
    if (!g_panel.rolled() || i == 100) {
      g_panel.SetMode(Panel::Mode::kEdit, random_map::MapFileName(setup.title) + " " +
                                              g_panel.Text("already exists: choose another seed."));
      return;
    }
    g_panel.SetSeed(RandomSeed());
    setup.seed = g_panel.setup().seed;
    setup.title = random_map::MapTitle(int64_t(std::time(nullptr)), setup.seed);
    guest = folder + random_map::MapFileName(setup.title);
  }
  const std::string host = GuestToHost(guest, recomp::PathAccess::kWrite);
  std::error_code ec;  // the folder, in the player's data (the game lists it with the install's)
  std::filesystem::create_directories(std::filesystem::path(host).parent_path(), ec);
  g_made_folder = folder;
  StartJob(setup, host, false);
}

// Makes the edited map again over its file (g_mu held).
void Update() {
  if (!ReadyToMake()) return;
  g_made_folder.clear();  // (its title kept: it stays where it is in the list)
  StartJob(g_panel.setup(), GuestToHost(g_edit_guest, recomp::PathAccess::kWrite), true);
}

// Opens the panel on the random map selected in the list (g_mu held): its settings read back.
void EditSelected(const std::string& folder) {
  if (!random_map::IsMapFileName(g_selected)) return;
  if (g_load != 3) {  // (the button shows once the font is read; the rest follows)
    ReadyToMake();
    g_open = true;
    return;
  }
  const std::string guest = folder + g_selected;
  std::vector<uint8_t> file;
  random_map::MapSetup setup;
  if (!random_map::ReadFile(GuestToHost(guest), file) || !random_map::ReadSetup(g_data->catalog, file, &setup)) {
    g_panel.NewMap(RandomSeed());
    g_panel.SetMode(Panel::Mode::kEdit, g_panel.Text("Could not read the map's settings."));
  } else {
    g_edit_guest = guest;
    if (setup.title.empty()) setup.title = random_map::TitleOfFile(g_selected);
    LoadTemplates(folder);
    g_panel.EditMap(setup, setup.title);
    // made by another version, or its template's file changed since: not the same map again
    const random_map::MapSetup& now = g_panel.setup();
    if (setup.generator_version != random_map::kGeneratorVersion)
      g_panel.SetMode(Panel::Mode::kEdit, g_panel.Text("Made by another version of the generator: updated, it will not be the same map."));
    else if (!setup.template_sum.empty() &&
             (now.template_text.empty() || random_map::TemplateSum(now.template_text) != setup.template_sum))
      g_panel.SetMode(Panel::Mode::kEdit, g_panel.Text("Its template's file has changed or is gone: updated, it will not be the same map."));
  }
  g_open = true;
}

// Removes the random map selected in the list (g_mu held); true: it is gone.
bool RemoveSelected(const std::string& folder) {
  if (!random_map::IsMapFileName(g_selected) || g_open) return false;
  const std::string guest = folder + g_selected;
  // Where the game reads it from: the player's data, or (an older copy) the install.
  for (int i = 0; i < 3; ++i) {
    const std::string host = GuestToHost(guest);
    if (!Exists(host)) break;
    if (std::remove(host.c_str()) != 0) {
      std::fprintf(stderr, "h4: random map: could not remove %s: %s\n", host.c_str(), std::strerror(errno));
      return false;
    }
    std::printf("h4: random map: removed %s\n", host.c_str());
  }
  return !Exists(GuestToHost(guest));
}

}  // namespace

bool RandomMapTyping() {
  std::lock_guard<std::mutex> lock(g_mu);
  return g_shown && g_open && g_panel.typing();
}

bool IsRandomMapsFolder(const std::string& folder) {
  const std::string tail = std::string("\\") + random_map::kMapFolder + "\\";
  if (folder.size() < tail.size()) return false;
  for (size_t i = 0; i < tail.size(); ++i)
    if (std::tolower(static_cast<unsigned char>(folder[folder.size() - tail.size() + i])) !=
        std::tolower(static_cast<unsigned char>(tail[i])))
      return false;
  return true;
}

bool RandomMapPass(bool shown, int l, int t, int r, int b, const std::string& folder,
                   const std::string& selected, std::string* open) {
  static bool hooked = false;
  if (!hooked) {
    hooked = true;
    recomp::SetAppInputHook(OnInput);
  }
  if (shown && g_load == 0) {
    g_load = 1;
    std::thread(Load).detach();
  }
  std::lock_guard<std::mutex> lock(g_mu);
  const int load = g_load;
  bool written = false;
  {  // what the log says: the dialog seen or gone, the game's data read or not
    static int said = -2;
    const int now = shown ? load : -2;
    if (now != said) {
      std::printf("h4: random map: %s (data: %s)\n", shown ? "the scenario list" : "no scenario list",
                  load == 3 ? "read" : load == 2 ? "font read" : load == 1 ? "reading" : load == -1 ? "failed" : "not read");
      std::fflush(stdout);
      said = now;
    }
  }
#if defined(__EMSCRIPTEN__)
  {  // the browser: the page's on-screen keyboard (index.html) while the seed is typed in
    static bool up = false;
    const bool want = shown && g_shown && g_open && g_panel.typing();
    if (want != up) {
      up = want;
      MAIN_THREAD_ASYNC_EM_ASM({ if (Module.recompScreenKeyboard) Module.recompScreenKeyboard(!!$0); }, want ? 1 : 0);
    }
  }
#endif
  if (g_job && g_job->done) {
    written = g_job->ok;
    if (written && open && !g_made_folder.empty()) *open = g_made_folder;
    if (g_job->cancel) g_panel.SetMode(Panel::Mode::kEdit, "");
    else g_panel.SetMode(written ? Panel::Mode::kDone : Panel::Mode::kEdit, g_job->message);
    g_job.reset();
  }
  if (!shown || !(load >= 2 || (load == -1 && g_data))) {
    if (g_shown && g_badge_ours) recomp::gfx::SetPresentBadgePos(0, 0, false);
    g_shown = g_open = g_remove_wanted = g_remove_armed = false;
    g_pressed = 0;
    g_badge_ours = false;
    g_badge = nullptr;
    return written;
  }
  if (!g_panel_ready) {
    // The game's language (main.cpp's H4_LANG: a translation's, or a GOG install's own).
    const char* lang = std::getenv("H4_LANG");
    g_panel.Init(&g_data->font, lang ? lang : "", RandomSeed());
    g_panel_ready = true;
  }
  static bool catalog_given = false;
  if (load == 3 && !catalog_given) {
    g_panel.SetCatalog(&g_data->catalog);
    g_panel.SetHeroArt(&g_data->art);
    catalog_given = true;
  }

  if (g_edit_wanted) {
    g_edit_wanted = false;
    if (!g_open && !g_job) EditSelected(folder);
  }
  if (g_open_wanted) {
    g_open_wanted = false;
    LoadTemplates(folder);
    if (!g_job) g_panel.NewMap(RandomSeed());  // (an edited map's settings left)
    g_open = true;
    if (g_panel.mode() == Panel::Mode::kDone) g_panel.SetMode(Panel::Mode::kEdit, "");
  }
  // The remove button: for a random map selected, asking first.
  const std::string sel = random_map::IsMapFileName(selected) ? selected : std::string();
  if (sel != g_selected || (g_remove_armed && SDL_GetTicks() - g_armed_at > 5000)) g_remove_armed = false;
  g_selected = sel;
  if (g_remove_wanted) {
    g_remove_wanted = false;
    if (!g_selected.empty() && !g_open) {
      if (!g_remove_armed) {
        g_remove_armed = true;
        g_armed_at = SDL_GetTicks();
      } else {
        g_remove_armed = false;
        if (RemoveSelected(folder)) written = true;
      }
    }
  }
  const Panel::Action action = g_action;
  g_action = Panel::kNone;
  switch (action) {
    case Panel::kCreate:
      if (g_panel.editing()) Update();
      else Create(folder);
      break;
    case Panel::kCancel:
      if (g_job) g_job->cancel = true;
      break;
    case Panel::kClose:
      if (!g_job) {
        g_open = false;
        if (g_panel.mode() == Panel::Mode::kDone) g_panel.SetMode(Panel::Mode::kEdit, "");
      }
      break;
    case Panel::kNewSeedAction: g_panel.SetSeed(RandomSeed()); break;
    default: break;
  }

  int w = 0, h = 0;
  bool changed = false;
  const int remove = g_selected.empty() ? 0 : g_remove_armed ? 2 : 1;
  static int shown_remove = -1;  // what the buttons' picture given showed
  if (!g_open && remove != shown_remove) changed = true;
  if (!g_open) shown_remove = remove;
  const std::vector<uint32_t>& pic =
      g_open ? g_panel.Picture(&w, &h, &changed) : g_panel.ButtonPicture(&w, &h, remove);
  if (changed || g_badge != &pic || !g_badge_ours) {
    recomp::gfx::SetPresentBadge(pic.data(), w, h);
    g_badge = &pic;
    g_badge_ours = true;
  }
  const recomp::gfx::Presentation p = recomp::gfx::LastPresentation();
  const int fw = p.src_w > 0 ? p.src_w : r, fh = p.src_h > 0 ? p.src_h : b;
  int x, y;
  if (g_open) {  // over the dialog's middle
    x = (l + r - w) / 2;
    y = (t + b - h) / 2;
  } else {  // under the dialog's bottom left corner, else over its top one, else inside it
    x = l + 8;
    y = b + 6 + h <= fh ? b + 6 : t - 6 - h >= 0 ? t - 6 - h : b - h - 8;
  }
  x = std::clamp(x, 0, std::max(0, fw - w));
  y = std::clamp(y, 0, std::max(0, fh - h));
  g_x = x, g_y = y, g_w = w, g_h = h;
  if (!g_shown) WakeIdleGame();  // (one more frame: the button on screen now, not after a touch)
  g_shown = true;
  recomp::gfx::SetPresentBadgePos(x, y, true);
  return written;
}

}  // namespace h4
