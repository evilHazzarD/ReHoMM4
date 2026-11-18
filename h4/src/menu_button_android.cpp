// menu_button.h on Android: HeroesActivity.h4LanguageButton (Buttons over
// SDL's view: the language one, a tap opens the list of languages, the one picked is saved; the ☁️ one, the
// save sync dialog of SaveSync.java), called through JNI from the game's thread.
#include <SDL3/SDL.h>
#include <jni.h>

#include <cstdio>
#include <string>

#include "language.h"
#include "menu_button.h"

namespace h4 {

namespace {
std::string g_lang, g_saved;
bool g_shown = false;
}  // namespace

void SetLanguageChoice(const std::string& lang, const std::string& saved_path) {
  g_lang = lang;
  g_saved = saved_path;
}

void ShowLanguageButton(bool shown, int x, int y, int left) {
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
  if (!cls) {
    std::fprintf(stderr, "h4: language button: no HeroesActivity class\n");
    return;
  }
  static jmethodID m =
      env->GetStaticMethodID(cls, "h4LanguageButton", "(ZFFFLjava/lang/String;Ljava/lang/String;Ljava/lang/String;)V");
  if (!m) {
    std::fprintf(stderr, "h4: language button: no h4LanguageButton\n");
    env->ExceptionClear();
    return;
  }
  std::string list;  // as it is now (a new translations release can add to it)
  for (const LanguageInfo& lang : Languages()) list += lang.code + "\t" + lang.name + "\t" + lang.note + "\n";
  jstring l = env->NewStringUTF(g_lang.c_str()), p = env->NewStringUTF(g_saved.c_str()),
          all = env->NewStringUTF(list.c_str());
  env->CallStaticVoidMethod(cls, m, jboolean(shown), jfloat(vx), jfloat(vy), jfloat(lx), l, p, all);
  env->DeleteLocalRef(l);
  env->DeleteLocalRef(p);
  env->DeleteLocalRef(all);
  if (env->ExceptionCheck()) env->ExceptionClear();
}

}  // namespace h4
