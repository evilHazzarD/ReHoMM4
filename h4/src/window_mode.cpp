// window_mode.h
#include "window_mode.h"

#include <cstdio>
#include <cstdlib>

#include <SDL3/SDL.h>

#include <chrono>

#include "runtime/hle/user32_window.h"

namespace h4 {

namespace {
std::string g_path;
bool g_full = true;
}  // namespace

void LoadWindowMode(const std::string& path) {
  g_path = path;
  g_full = true;
  if (FILE* f = std::fopen(path.c_str(), "r")) {
    g_full = std::fgetc(f) != '1';
    std::fclose(f);
  }
  recomp::SetWindowed(!g_full);  // no windows yet: only the choice
  std::printf("h4: %s (settings)\n", g_full ? "fullscreen" : "in a window");
}

bool Fullscreen() { return g_full; }

void SetFullscreen(bool on) {
  g_full = on;
  if (FILE* f = std::fopen(g_path.c_str(), "w")) {
    std::fputs(on ? "0" : "1", f);
    std::fclose(f);
  }
  recomp::SetWindowed(!on);
  std::fprintf(stderr, "h4: %s now\n", on ? "fullscreen" : "in a window");
}

void TestWindowMode() {
  static const char* e = std::getenv("H4_TEST_WINDOWED");
  if (!e || !*e) return;
  using C = std::chrono::steady_clock;
  static const C::time_point start = C::now();
  static int done = 0;  // steps taken: odd ones to a window, even ones back
  const double s = std::chrono::duration<double>(C::now() - start).count(), every = std::atof(e);
  if (every <= 0) return;
  static bool logged = true;
  const int step = int(s / every);
  if (step > done && step <= 4) {
    done = step;
    recomp::SetWindowed(step % 2 == 1);
    std::fprintf(stderr, "h4: test: %s\n", step % 2 == 1 ? "to a window" : "to fullscreen");
    logged = false;
  }
  if (!logged && s >= done * every + 1.5) {
    logged = true;
    int n = 0;
    SDL_Window** ws = SDL_GetWindows(&n);
    for (int i = 0; i < n; i++) {
      int w = 0, h = 0, x = 0, y = 0;
      SDL_GetWindowSize(ws[i], &w, &h);
      SDL_GetWindowPosition(ws[i], &x, &y);
      std::fprintf(stderr, "h4: test: window %d: %s, %dx%d at %d,%d, Windowed() %d\n", i,
                   (SDL_GetWindowFlags(ws[i]) & SDL_WINDOW_FULLSCREEN) ? "fullscreen" : "a window", w, h, x, y,
                   int(recomp::Windowed()));
    }
    SDL_free(ws);
  }
}

}  // namespace h4
