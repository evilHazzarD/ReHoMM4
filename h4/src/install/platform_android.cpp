// install/platform.h on Android: the static h4* methods of HeroesActivity
// (platforms/android/.../HeroesActivity.java) through JNI, and the token in the
// app's private storage.
#include <SDL3/SDL.h>
#include <fcntl.h>
#include <jni.h>
#include <sys/stat.h>
#include <unistd.h>

#include <cstdio>
#include <cstdlib>
#include <string>
#include <vector>

#include "install/platform.h"

namespace h4::install {

namespace {

jclass g_cls = nullptr;  // HeroesActivity, found on the main (Java-created) thread
std::vector<int> g_fds;  // descriptors of picked files

JNIEnv* Env() { return static_cast<JNIEnv*>(SDL_GetAndroidJNIEnv()); }

jstring Str(JNIEnv* env, const std::string& s) { return env->NewStringUTF(s.c_str()); }

std::string Std(JNIEnv* env, jstring s) {
  if (!s) return "";
  const char* c = env->GetStringUTFChars(s, nullptr);
  std::string out = c ? c : "";
  if (c) env->ReleaseStringUTFChars(s, c);
  return out;
}

std::string TokenPath() {
  const char* in = SDL_GetAndroidInternalStoragePath();  // private to the app
  return std::string(in ? in : ".") + "/gog.token";
}

}  // namespace

std::string ScratchDir() {
  const char* in = SDL_GetAndroidInternalStoragePath();
  return std::string(in ? in : ".") + "/scratch";
}

void PlatformInit() {
  if (g_cls) return;
  JNIEnv* env = Env();
  jclass c = env->FindClass("games/hazzard/heroes4/HeroesActivity");
  if (!c) {
    env->ExceptionClear();
    return;
  }
  g_cls = static_cast<jclass>(env->NewGlobalRef(c));
  env->DeleteLocalRef(c);
}

bool HttpGet(const std::string& url, const std::string& bearer, int* status, std::string* body,
             std::string* error, const std::string& range) {
  JNIEnv* env = Env();
  if (!g_cls || !env) {
    *error = "no Java";
    return false;
  }
  static jmethodID m = env->GetStaticMethodID(
      g_cls, "h4Http", "(Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;)[B");
  jstring ju = Str(env, url), jb = Str(env, bearer), jr = Str(env, range);
  auto arr = static_cast<jbyteArray>(env->CallStaticObjectMethod(g_cls, m, ju, jb, jr));
  env->DeleteLocalRef(ju);
  env->DeleteLocalRef(jb);
  env->DeleteLocalRef(jr);
  if (env->ExceptionCheck()) env->ExceptionClear();
  if (!arr) {
    *error = "network error (no connection?)";
    return false;
  }
  const jsize n = env->GetArrayLength(arr);
  std::string all(size_t(n), '\0');
  env->GetByteArrayRegion(arr, 0, n, reinterpret_cast<jbyte*>(&all[0]));
  env->DeleteLocalRef(arr);
  if (all.size() < 3) {
    *error = "network error";
    return false;
  }
  *status = std::atoi(all.substr(0, 3).c_str());
  body->assign(all, 3, std::string::npos);
  return true;
}

bool CanSignInToGog() { return true; }

std::string GogLoginCode(const std::string& url) {
  JNIEnv* env = Env();
  if (!g_cls) return "";
  jmethodID m = env->GetStaticMethodID(g_cls, "h4GogLogin", "(Ljava/lang/String;)Ljava/lang/String;");
  jstring ju = Str(env, url);
  auto r = static_cast<jstring>(env->CallStaticObjectMethod(g_cls, m, ju));
  env->DeleteLocalRef(ju);
  if (env->ExceptionCheck()) env->ExceptionClear();
  std::string code = Std(env, r);
  if (r) env->DeleteLocalRef(r);
  return code;
}

bool BeginPickFiles(bool folder) {
  EndPickedFiles();
  JNIEnv* env = Env();
  if (!g_cls) return false;
  jmethodID m = env->GetStaticMethodID(g_cls, "h4PickFilesBegin", "(Z)Z");
  const bool ok = env->CallStaticBooleanMethod(g_cls, m, jboolean(folder));
  if (env->ExceptionCheck()) {
    env->ExceptionClear();
    return false;
  }
  return ok;
}

bool PickedFiles(std::vector<PickedFile>* result) {
  std::vector<PickedFile>& out = *result;
  out.clear();
  JNIEnv* env = Env();
  if (!g_cls) return true;
  static jmethodID m = env->GetStaticMethodID(g_cls, "h4PickFilesPoll", "()[Ljava/lang/String;");
  auto arr = static_cast<jobjectArray>(env->CallStaticObjectMethod(g_cls, m));
  if (env->ExceptionCheck()) {
    env->ExceptionClear();
    return true;
  }
  if (!arr) return false;  // still picking
  for (jsize i = 0, n = env->GetArrayLength(arr); i < n; i++) {
    auto s = static_cast<jstring>(env->GetObjectArrayElement(arr, i));
    const std::string line = Std(env, s);
    env->DeleteLocalRef(s);
    const size_t tab = line.find('\t');
    if (tab == std::string::npos) continue;
    PickedFile f{line.substr(0, tab), line.substr(tab + 1)};
    g_fds.push_back(std::atoi(f.path.c_str() + f.path.rfind('/') + 1));
    out.push_back(f);
  }
  env->DeleteLocalRef(arr);
  return true;
}

void EndPickedFiles() {
  for (int fd : g_fds) close(fd);
  g_fds.clear();
}

std::string LoadToken() {
  std::string t;
  if (FILE* f = fopen(TokenPath().c_str(), "rb")) {
    char buf[4096];
    size_t n = fread(buf, 1, sizeof buf, f);
    t.assign(buf, n);
    fclose(f);
  }
  return t;
}

void SaveToken(const std::string& token) {
  const std::string p = TokenPath();
  if (token.empty()) {
    unlink(p.c_str());
    return;
  }
  int fd = open(p.c_str(), O_WRONLY | O_CREAT | O_TRUNC, 0600);
  if (fd < 0) return;
  ssize_t w = write(fd, token.data(), token.size());
  (void)w;
  close(fd);
}

}  // namespace h4::install
