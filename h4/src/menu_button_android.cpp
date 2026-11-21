// menu_button.h on Android: HeroesActivity.h4LanguageButton (buttons over SDL's view: ⚙︎, a tap
// opens the settings, the languages and the checkbox that shows the ☁️ one, the save sync dialog
// of SaveSync.java, the gamepad's: showGamepad), called through JNI from the game's thread; and
// the gamepad dialog's natives (h4PadState, h4PadSet).
#include <SDL3/SDL.h>
#include <jni.h>

#include <cstdio>
#include <cstdlib>
#include <string>

#include "language.h"
#include "intro.h"
#include "menu_button.h"
#include "pad_config.h"
#include "runtime/hle/user32_window.h"

namespace h4 {

namespace {
std::string g_lang, g_saved;
bool g_shown = false;

jclass Activity(JNIEnv* env) {
  static jclass cls = [&] {
    jclass c = env->FindClass("games/hazzard/heroes4/HeroesActivity");
    if (!c) {
      env->ExceptionClear();
      return jclass(nullptr);
    }
    auto g = static_cast<jclass>(env->NewGlobalRef(c));
    env->DeleteLocalRef(c);
    return g;
  }();
  return cls;
}

// A gamepad's clicks (recomp::SetPadEventHook) reach the game only, never ⚙︎, ☕ and ☁️ (views of
// their own over SDL's): a click (A, on release) on one of them is its tap
// (HeroesActivity.h4PadClick); its press is not the game's either. The settings' dialogs take the
// pad themselves (Android's focus: the D-pad and the stick move it, A presses, B goes back).
float g_pad_x = -1, g_pad_y = -1;  // its pointer (its button events carry no place)
bool g_pad_down_on = false;        // the left button went down on one of them
bool PadEvent(const SDL_Event& e) {
  if (e.type == SDL_EVENT_MOUSE_MOTION) {
    g_pad_x = e.motion.x;
    g_pad_y = e.motion.y;
    return false;
  }
  if ((e.type != SDL_EVENT_MOUSE_BUTTON_DOWN && e.type != SDL_EVENT_MOUSE_BUTTON_UP) || !g_shown || g_pad_x < 0)
    return false;
  auto* env = static_cast<JNIEnv*>(SDL_GetAndroidJNIEnv());
  jclass cls = env ? Activity(env) : nullptr;
  static jmethodID m = cls ? env->GetStaticMethodID(cls, "h4PadClick", "(FFZ)Z") : nullptr;
  if (!m) {
    if (env) env->ExceptionClear();
    return false;
  }
  // Pixels of the window (SDL's view fills it): the pad's points are pixels on Android.
  const bool up = e.type == SDL_EVENT_MOUSE_BUTTON_UP;
  const bool hit = env->CallStaticBooleanMethod(cls, m, jfloat(g_pad_x), jfloat(g_pad_y),
                                                jboolean(up && e.button.button == SDL_BUTTON_LEFT && g_pad_down_on));
  if (env->ExceptionCheck()) env->ExceptionClear();
  if (!up) g_pad_down_on = hit && e.button.button == SDL_BUTTON_LEFT;
  return hit;
}
}  // namespace

void SetLanguageChoice(const std::string& lang, const std::string& saved_path) {
  g_lang = lang;
  g_saved = saved_path;
}

void ShowLanguageButton(bool shown, int x, int y, int left) {
  static bool hooked = false;
  if (!hooked) {
    hooked = true;
    recomp::SetPadEventHook(PadEvent);
  }
  static int last_x = -1, last_y = -1, last_left = -1;
  if (shown == g_shown && (!shown || (x == last_x && y == last_y && left == last_left))) return;
  g_shown = shown;
  last_x = x;
  last_y = y;
  last_left = left;
  // Pixels of the window (SDL's view fills it; points are pixels on Android).
  float vx = -1, vy = -1, lx = -1, ly = -1;
  if (shown) {
    int ww = 0, wh = 0;
    int n = 0;
    if (SDL_Window** wins = SDL_GetWindows(&n)) {  // the game's one window
      if (n > 0) SDL_GetWindowSizeInPixels(wins[0], &ww, &wh);
      SDL_free(wins);
    }
    if (ww > 0) {
      FrameToView(x, y, float(ww), float(wh), &vx, &vy);
      FrameToView(left >= 0 ? left : 0, y, float(ww), float(wh), &lx, &ly);
    }
    if (vx < 0) {
      g_shown = false;  // nothing shown yet: tried again next pass
      return;
    }
    static bool logged = false;
    if (!logged) std::fprintf(stderr, "h4: language button at %.0f,%.0f of %dx%d\n", vx, vy, ww, wh);
    logged = true;
  }
  auto* env = static_cast<JNIEnv*>(SDL_GetAndroidJNIEnv());
  if (!env) return;
  jclass cls = Activity(env);
  if (!cls) {
    std::fprintf(stderr, "h4: language button: no HeroesActivity class\n");
    return;
  }
  static jmethodID m =
      env->GetStaticMethodID(cls, "h4LanguageButton", "(ZFFFLjava/lang/String;Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;)V");
  if (!m) {
    std::fprintf(stderr, "h4: language button: no h4LanguageButton\n");
    env->ExceptionClear();
    return;
  }
  std::string list;  // as it is now (a new translations release can add to it)
  for (const LanguageInfo& lang : Languages()) list += lang.code + "\t" + lang.name + "\t" + lang.note + "\n";
  jstring l = env->NewStringUTF(g_lang.c_str()), p = env->NewStringUTF(g_saved.c_str()),
          all = env->NewStringUTF(list.c_str()), skip = env->NewStringUTF(SkipIntroPath().c_str());
  env->CallStaticVoidMethod(cls, m, jboolean(shown), jfloat(vx), jfloat(vy), jfloat(lx), l, p, all, skip);
  env->DeleteLocalRef(skip);
  env->DeleteLocalRef(l);
  env->DeleteLocalRef(p);
  env->DeleteLocalRef(all);
  if (env->ExceptionCheck()) env->ExceptionClear();
}

}  // namespace h4

// The gamepad's dialog (HeroesActivity.showGamepad), on the UI thread: what it shows, and its changes.
// h4PadState: lines of tab-separated fields: "enabled 1", "style <index> <its names...>",
// "pointer <value> <min> <max>", "scroll <value> <min> <max>", "actions <name>...", then
// "button <name> <action>" for each button (pad_config.h's order).
extern "C" JNIEXPORT jstring JNICALL Java_games_hazzard_heroes4_HeroesActivity_h4PadState(JNIEnv* env, jclass,
                                                                                         jboolean uk) {
  std::string s = "enabled\t" + std::to_string(h4::PadEnabled() ? 1 : 0) + "\n";
  s += "style\t" + std::to_string(h4::PadStyle());
  for (int i = 0; i < h4::kPadStyles; i++) s += std::string("\t") + h4::PadStyleName(i);
  char b[128];
  std::snprintf(b, sizeof b, "\npointer\t%g\t%g\t%g\nscroll\t%g\t%g\t%g\nactions", h4::PadPointerSpeed(),
                h4::kPointerMin, h4::kPointerMax, h4::PadScrollSpeed(), h4::kScrollMin, h4::kScrollMax);
  s += b;
  for (int a = 0; a < h4::PadActionCount(); a++) s += std::string("\t") + h4::PadActionName(a, uk);
  s += "\n";
  for (int i = 0; i < h4::kPadButtons; i++)
    s += std::string("button\t") + h4::PadButtonName(i, uk) + "\t" + std::to_string(h4::PadBinding(i)) + "\n";
  return env->NewStringUTF(s.c_str());
}

// The game's folder (GameFolder, main.cpp), shown in the settings; "" before it is found.
extern "C" JNIEXPORT jstring JNICALL Java_games_hazzard_heroes4_HeroesActivity_h4GameFolder(JNIEnv* env, jclass) {
  return env->NewStringUTF(h4::GameFolder().c_str());
}

// key: "enabled" (value 0 or 1), "pointer", "scroll", "preset" (its index), "reset", or "button<i>"
// (value: the action's index).
extern "C" JNIEXPORT void JNICALL Java_games_hazzard_heroes4_HeroesActivity_h4PadSet(JNIEnv* env, jclass, jstring key,
                                                                                    jfloat value) {
  const char* k = env->GetStringUTFChars(key, nullptr);
  const std::string name = k ? k : "";
  if (k) env->ReleaseStringUTFChars(key, k);
  if (name == "enabled") h4::SetPadEnabled(value != 0);
  else if (name == "pointer") h4::SetPadPointerSpeed(value);
  else if (name == "scroll") h4::SetPadScrollSpeed(value);
  else if (name == "preset") h4::ApplyPadPreset(int(value));
  else if (name == "reset") h4::ResetPadConfig();
  else if (name.rfind("button", 0) == 0) h4::SetPadBinding(std::atoi(name.c_str() + 6), int(value));
}
