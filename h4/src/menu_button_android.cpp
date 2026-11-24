// Android's side of the app: what the settings (drawn into the game's frame, menu_button_drawn.cpp)
// ask of the system (app_ui/native.h: the browser, the Files app, the save sync's folder picker,
// the update's download, the app started again) and what main.cpp asks (a phone's screen, the
// notch), through HeroesActivity's static methods (JNI, from the game's thread); and HeroesActivity.tr.
#include <SDL3/SDL.h>
#include <jni.h>

#include <cstdint>
#include <cstdio>
#include <string>
#include <vector>

#include "app_ui/native.h"
#include "menu_button.h"
#include "ui_text.h"

static jstring JString(JNIEnv* env, const std::string& s);
static std::string Utf8(JNIEnv* env, jstring s);

namespace {

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

// HeroesActivity.<name>(<sig>) called; false when it is not there (or threw).
template <typename F>
bool Call(const char* name, const char* sig, F&& call) {
  auto* env = static_cast<JNIEnv*>(SDL_GetAndroidJNIEnv());
  jclass cls = env ? Activity(env) : nullptr;
  jmethodID m = cls ? env->GetStaticMethodID(cls, name, sig) : nullptr;
  if (!m) {
    if (env) env->ExceptionClear();
    std::fprintf(stderr, "h4: no HeroesActivity.%s\n", name);
    return false;
  }
  call(env, cls, m);
  if (env->ExceptionCheck()) {
    env->ExceptionClear();
    return false;
  }
  return true;
}

bool CallBool(const char* name, const std::string& arg) {
  bool r = false;
  Call(name, "(Ljava/lang/String;)Z", [&](JNIEnv* env, jclass cls, jmethodID m) {
    jstring a = JString(env, arg);
    r = env->CallStaticBooleanMethod(cls, m, a);
    env->DeleteLocalRef(a);
  });
  return r;
}

std::string CallString(const char* name) {
  std::string r;
  Call(name, "()Ljava/lang/String;", [&](JNIEnv* env, jclass cls, jmethodID m) {
    auto s = static_cast<jstring>(env->CallStaticObjectMethod(cls, m));
    r = Utf8(env, s);
    if (s) env->DeleteLocalRef(s);
  });
  return r;
}

void CallVoid(const char* name) {
  Call(name, "()V", [](JNIEnv* env, jclass cls, jmethodID m) { env->CallStaticVoidMethod(cls, m); });
}

}  // namespace

namespace h4 {

// A phone's screen: Android's smallest width under 600 dp (tablets have 600 and more).
bool AndroidSmallScreen() {
  auto* env = static_cast<JNIEnv*>(SDL_GetAndroidJNIEnv());
  jclass cls = env ? Activity(env) : nullptr;
  jmethodID m = cls ? env->GetStaticMethodID(cls, "h4SmallestWidthDp", "()I") : nullptr;
  if (!m) {
    if (env) env->ExceptionClear();
    return true;
  }
  const int dp = env->CallStaticIntMethod(cls, m);
  if (env->ExceptionCheck()) {
    env->ExceptionClear();
    return true;
  }
  std::printf("h4: the screen's smallest width %d dp\n", dp);
  return dp <= 0 || dp < 600;
}

// HeroesActivity.h4CutoutMargin: the camera notch's side, a fraction of the screen's width.
bool AndroidSafeAreaMargin(float* fraction) {
  auto* env = static_cast<JNIEnv*>(SDL_GetAndroidJNIEnv());
  jclass cls = env ? Activity(env) : nullptr;
  jmethodID m = cls ? env->GetStaticMethodID(cls, "h4CutoutMargin", "()F") : nullptr;
  if (!m) {
    if (env) env->ExceptionClear();
    return false;
  }
  const float f = env->CallStaticFloatMethod(cls, m);
  if (env->ExceptionCheck()) {
    env->ExceptionClear();
    return false;
  }
  *fraction = f;
  return f > 0;
}

void RestartApp() { CallVoid("h4Restart"); }

}  // namespace h4

namespace h4::native {

bool OpenUrl(const std::string& url) { return CallBool("h4OpenUrl", url); }
bool RevealFolder(const std::string& path) { return CallBool("h4OpenFolder", path); }
bool HasSyncChoice() { return true; }
bool SyncOn() {
  bool on = false;
  Call("h4SyncOn", "()Z", [&](JNIEnv* env, jclass cls, jmethodID m) { on = env->CallStaticBooleanMethod(cls, m); });
  return on;
}
void SetSync(bool on) {
  Call("h4SetSync", "(Z)V", [on](JNIEnv* env, jclass cls, jmethodID m) { env->CallStaticVoidMethod(cls, m, jboolean(on)); });
}
std::string SavesFolder() { return CallString("h4SavesFolder"); }
std::string SyncedFolder() { return CallString("h4SyncedFolder"); }

// The ☁️ panel (SaveSync.state: "<folder>\n<status>", the folder "" when off).
SyncPanel SyncPanelFor(const std::string& lang) {
  const std::string st = CallString("h4SyncState");
  const size_t nl = st.find('\n');
  const std::string folder = st.substr(0, nl), status = nl == std::string::npos ? "" : st.substr(nl + 1);
  SyncPanel p;
  p.title = h4::TrIn(lang, "Save sync");
  if (folder.empty()) {
    p.text = h4::TrIn(lang, "Pick a folder that Syncthing, Round Sync (rclone), Nextcloud or another app keeps in sync: "
                            "the game makes a ReHOMM4 folder in it and copies saved games there and back.");
    p.buttons.push_back({h4::TrIn(lang, "Choose a folder…"), [] { SetSync(true); }});
  } else {
    p.text = h4::TrIn(lang, "Folder: {1}", {folder}) + "\n" + status;
    p.buttons.push_back({h4::TrIn(lang, "Sync now"), [] { CallVoid("h4SyncNow"); }});
    p.buttons.push_back({h4::TrIn(lang, "Another folder…"), [] { SetSync(true); }});
    p.buttons.push_back({h4::TrIn(lang, "Stop syncing"), [] { SetSync(false); }});
  }
  return p;
}

void DownloadApk(const std::string& url, const std::string& tag) {
  Call("h4DownloadApk", "(Ljava/lang/String;Ljava/lang/String;)Z", [&](JNIEnv* env, jclass cls, jmethodID m) {
    jstring u = JString(env, url), t = JString(env, tag);
    env->CallStaticBooleanMethod(cls, m, u, t);
    env->DeleteLocalRef(u);
    env->DeleteLocalRef(t);
  });
}

}  // namespace h4::native

// A Java string of UTF-8 `s` (by UTF-16: NewStringUTF takes Java's modified UTF-8, which has no
// 4-byte sequences).
static jstring JString(JNIEnv* env, const std::string& s) {
  std::u16string u;
  for (size_t i = 0; i < s.size();) {
    const unsigned char c = static_cast<unsigned char>(s[i]);
    const int n = c < 0x80 ? 1 : c < 0xE0 ? 2 : c < 0xF0 ? 3 : 4;
    uint32_t cp = n == 1 ? c : n == 2 ? c & 0x1F : n == 3 ? c & 0x0F : c & 0x07;
    for (int k = 1; k < n && i + size_t(k) < s.size(); k++) cp = (cp << 6) | (static_cast<unsigned char>(s[i + size_t(k)]) & 0x3F);
    i += size_t(n);
    if (cp >= 0x10000) {
      cp -= 0x10000;
      u += char16_t(0xD800 + (cp >> 10));
      u += char16_t(0xDC00 + (cp & 0x3FF));
    } else {
      u += char16_t(cp);
    }
  }
  return env->NewString(reinterpret_cast<const jchar*>(u.data()), jsize(u.size()));
}

static std::string Utf8(JNIEnv* env, jstring s) {
  if (!s) return "";
  const char* c = env->GetStringUTFChars(s, nullptr);
  std::string out = c ? c : "";
  if (c) env->ReleaseStringUTFChars(s, c);
  return out;
}

// HeroesActivity.tr: `english` in `lang` (ui_text.h), {1}, {2}... from `args`.
extern "C" JNIEXPORT jstring JNICALL Java_games_hazzard_heroes4_HeroesActivity_h4Tr(JNIEnv* env, jclass, jstring lang,
                                                                                   jstring english, jobjectArray args) {
  std::vector<std::string> a;
  for (jsize i = 0, n = args ? env->GetArrayLength(args) : 0; i < n; i++) {
    jstring s = static_cast<jstring>(env->GetObjectArrayElement(args, i));
    a.push_back(Utf8(env, s));
    if (s) env->DeleteLocalRef(s);
  }
  const std::string e = Utf8(env, english);
  return JString(env, h4::TrIn(Utf8(env, lang), e.c_str(), a));
}
