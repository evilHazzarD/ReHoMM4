// save_sync.h: the saved games kept in step with a folder that a sync tool
// carries to the player's other devices (Syncthing shares it; rclone copies it
// to and from a cloud remote), on the desktop apps (H4_DESKTOP_APP).
//
// Linux: the folder is ~/ReHOMM4 (made when missing), or `folder=` in
// <data>/sync.conf; `rclone=<remote:path>` there (or H4_SYNC_RCLONE) adds a
// remote: before each pass `rclone copy --update` brings newer files down
// into the folder, after it sends them up. `off` in sync.conf stops it all.
// The Mac app syncs iCloud Drive/ReHOMM4 (save_sync_apple.mm) and takes the
// same `rclone=` line, run around its own passes (RcloneAround).
//
// A pass, as on iOS: file by file, the newer copy wins (times copied along),
// one side's file is copied to the other, nothing is deleted; a save changed
// in the last 3 s waits. Here: the install overlay's games/ and the saves the
// game wrote beside it (<data>/data/h4/install).
#include "save_sync.h"

#if defined(_WIN32)
#include <windows.h>
#include <shlobj.h>
#else
#include <spawn.h>
#include <sys/wait.h>
#include <unistd.h>
#endif

#include <cerrno>
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <filesystem>
#include <fstream>
#include <map>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

#if !defined(_WIN32)
extern char** environ;
#endif

namespace fs = std::filesystem;

namespace h4 {

namespace {

std::string Trim(std::string s) {
  while (!s.empty() && (s.back() == ' ' || s.back() == '\t' || s.back() == '\r')) s.pop_back();
  size_t i = 0;
  while (i < s.size() && (s[i] == ' ' || s[i] == '\t')) i++;
  return s.substr(i);
}

bool IEquals(const std::string& a, const char* b) {
#if defined(_WIN32)
  return _stricmp(a.c_str(), b) == 0;
#else
  return !strcasecmp(a.c_str(), b);
#endif
}

bool IsSave(const fs::path& p) {
  std::string e = p.extension().string();
  return IEquals(e, ".h4s");
}

#if defined(_WIN32)
std::string WindowsDocuments() {
  PWSTR p = nullptr;
  if (FAILED(SHGetKnownFolderPath(FOLDERID_Documents, 0, nullptr, &p))) return ".";
  const int n = WideCharToMultiByte(CP_UTF8, 0, p, -1, nullptr, 0, nullptr, nullptr);
  std::string out(n > 0 ? n - 1 : 0, '\0');
  if (n > 1) WideCharToMultiByte(CP_UTF8, 0, p, -1, out.data(), n, nullptr, nullptr);
  CoTaskMemFree(p);
  return out;
}
#endif

}  // namespace

SyncConfig ReadSyncConfig(const std::string& data_dir) {
  SyncConfig c;
  std::ifstream in(data_dir + "/sync.conf");
  for (std::string line; std::getline(in, line);) {
    line = Trim(line);
    if (line.empty() || line[0] == '#') continue;
    if (IEquals(line, "off")) {
      c.off = true;
      continue;
    }
    const size_t eq = line.find('=');
    if (eq == std::string::npos) continue;
    const std::string k = Trim(line.substr(0, eq)), v = Trim(line.substr(eq + 1));
    if (IEquals(k, "folder")) c.folder = v;
    if (IEquals(k, "rclone")) c.rclone = v;
  }
  if (const char* r = std::getenv("H4_SYNC_RCLONE"); r && *r) c.rclone = r;
  if (const char* f = std::getenv("H4_SYNC_FOLDER"); f && *f) c.folder = f;
  if (!c.folder.empty() && c.folder[0] == '~')
    if (const char* home = std::getenv("HOME")) c.folder = home + c.folder.substr(1);
  return c;
}

// rclone: the one in PATH, else where Homebrew and the install script put it (a Mac app
// started from Finder does not have the shell's PATH).
#if defined(_WIN32)
std::string RcloneBinary() { return ""; }
#else
std::string RcloneBinary() {
  std::vector<std::string> dirs;
  if (const char* p = std::getenv("PATH"))
    for (const char* s = p; *s;) {
      const char* e = std::strchr(s, ':');
      dirs.emplace_back(s, e ? size_t(e - s) : std::strlen(s));
      if (!e) break;
      s = e + 1;
    }
  for (const char* d : {"/opt/homebrew/bin", "/usr/local/bin", "/usr/bin"}) dirs.emplace_back(d);
  if (const char* home = std::getenv("HOME")) dirs.push_back(std::string(home) + "/.local/bin");
  for (const std::string& d : dirs)
    if (!d.empty() && access((d + "/rclone").c_str(), X_OK) == 0) return d + "/rclone";
  return "";
}
#endif

// `rclone copy from to --update`: files newer on `from` (or missing on `to`) copied, with their
// times; nothing deleted. False when rclone is missing or failed.
bool RcloneCopy(const std::string& from, const std::string& to, std::string* err) {
#if defined(_WIN32)
  (void)from;
  (void)to;
  *err = "rclone remote sync is not available in this Windows build";
  return false;
#else
  const std::string bin = RcloneBinary();
  if (bin.empty()) {
    *err = "rclone is not installed";
    return false;
  }
  std::vector<std::string> args = {bin,          "copy",           from,        to,
                                   "--update",   "--contimeout",   "15s",       "--timeout",
                                   "30s",        "--retries",      "1",         "--low-level-retries",
                                   "2",          "--exclude",      ".*",        "-q"};
  std::vector<char*> argv;
  for (std::string& a : args) argv.push_back(a.data());
  argv.push_back(nullptr);
  pid_t pid = 0;
  if (posix_spawn(&pid, bin.c_str(), nullptr, nullptr, argv.data(), environ) != 0) {
    *err = "rclone did not start";
    return false;
  }
  int st = 0;
  while (waitpid(pid, &st, 0) < 0 && errno == EINTR) {
  }
  if (WIFEXITED(st) && WEXITSTATUS(st) == 0) return true;
  *err = "rclone failed (exit " + std::to_string(WIFEXITED(st) ? WEXITSTATUS(st) : -1) + ")";
  return false;
#endif
}

std::string RcloneAround(const std::string& remote, const std::string& folder, bool down) {
  if (remote.empty()) return "";
  std::string err;
  if (down ? RcloneCopy(remote, folder, &err) : RcloneCopy(folder, remote, &err)) return "";
  std::fprintf(stderr, "h4: save sync: %s (%s)\n", err.c_str(), remote.c_str());
  return err;
}

std::string SyncFolderPass(const std::string& data_dir, const std::string& folder, int* up_out,
                           int* down_out) {
  std::error_code ec;
  const fs::path install = fs::path(data_dir) / "data/h4/install";
  fs::path games = install / "games";
  if (fs::is_directory(install, ec))  // "games" as the game spelled it
    for (const auto& e : fs::directory_iterator(install, ec))
      if (e.is_directory(ec) && IEquals(e.path().filename().string(), "games")) games = e.path();
  fs::create_directories(games, ec);
  const fs::path remote = folder;
  fs::create_directories(remote, ec);
  if (!fs::is_directory(remote, ec)) return "the folder " + folder + " cannot be made";

  auto list = [&](const fs::path& dir, bool saves_only, std::map<std::string, fs::path>* out) {
    std::error_code e2;
    for (const auto& e : fs::directory_iterator(dir, e2)) {
      const std::string n = e.path().filename().string();
      if (n.empty() || n[0] == '.' || !e.is_regular_file(e2)) continue;
      if (saves_only && !IsSave(e.path())) continue;
      auto it = out->find(n);
      if (it == out->end() || fs::last_write_time(e.path(), e2) > fs::last_write_time(it->second, e2))
        (*out)[n] = e.path();
    }
  };
  std::map<std::string, fs::path> here, there;
  list(games, false, &here);
  list(install, true, &here);  // saves the game wrote beside games/
  list(remote, false, &there);

  std::map<std::string, int> names;
  for (auto& [n, p] : here) names[n] |= 1;
  for (auto& [n, p] : there) names[n] |= 2;
  const auto now = fs::file_time_type::clock::now();
  int up = 0, down = 0, failed = 0;
  for (const auto& entry : names) {
    const std::string n = entry.first;
    const bool l = entry.second & 1, r = entry.second & 2;
    std::error_code e;
    const auto lt = l ? fs::last_write_time(here[n], e) : fs::file_time_type{};
    const auto rt = r ? fs::last_write_time(there[n], e) : fs::file_time_type{};
    const double diff = (l && r) ? std::chrono::duration<double>(lt - rt).count() : 0;
    auto copy = [&](const fs::path& from, const fs::path& to, fs::file_time_type t) {
      std::error_code ce;
      const fs::path tmp = to.parent_path() / (".sync-" + n);
      fs::copy_file(from, tmp, fs::copy_options::overwrite_existing, ce);
      if (ce) return false;
      fs::last_write_time(tmp, t, ce);
      fs::rename(tmp, to, ce);
      return !ce;
    };
    if (r && (!l || diff < -2)) {  // down, into games/ (or over the save where it is)
      const fs::path to = l && here[n].parent_path() == games ? here[n] : games / n;
      copy(there[n], to, rt) ? down++ : failed++;
    } else if (l && (!r || diff > 2) && std::chrono::duration<double>(now - lt).count() > 3) {
      copy(here[n], remote / n, lt) ? up++ : failed++;
    }
  }
  if (up_out) *up_out = up;
  if (down_out) *down_out = down;
  char b[200];
  const std::time_t t = std::time(nullptr);
  char hm[8];
  std::strftime(hm, sizeof hm, "%H:%M", std::localtime(&t));
  std::snprintf(b, sizeof b, "%s: %d save(s) in %s, %d sent, %d received%s", hm, int(names.size()),
                remote.filename().string().c_str(), up, down,
                failed ? (", " + std::to_string(failed) + " failed").c_str() : "");
  if (up || down || failed) std::fprintf(stderr, "h4: save sync %s\n", b);
  return b;
}

#if defined(H4_DESKTOP_APP) && !defined(__APPLE__)

namespace {
std::mutex g_pass;
std::string g_data_dir;
SyncConfig g_conf;
}  // namespace

// Linux: one pass; the remote (slower, over the network) around every third one, at quitting,
// and right after a save went into the folder.
static void LinuxPass(bool remote) {
  std::lock_guard<std::mutex> lock(g_pass);
  const bool rc = !g_conf.rclone.empty();
  if (rc && remote) RcloneAround(g_conf.rclone, g_conf.folder, true);
  int up = 0;
  SyncFolderPass(g_data_dir, g_conf.folder, &up, nullptr);
  if (rc && (remote || up)) RcloneAround(g_conf.rclone, g_conf.folder, false);
}

void StartSaveSync(const std::string& data_dir) {
  g_data_dir = data_dir;
  g_conf = ReadSyncConfig(data_dir);
  if (g_conf.off) {
    std::fprintf(stderr, "h4: save sync off (sync.conf)\n");
    return;
  }
  if (g_conf.folder.empty()) {
#if defined(_WIN32)
    g_conf.folder = WindowsDocuments() + "/ReHOMM4";
#else
    const char* home = std::getenv("HOME");
    g_conf.folder = std::string(home ? home : ".") + "/ReHOMM4";
#endif
  }
  std::fprintf(stderr, "h4: save sync with %s%s%s\n", g_conf.folder.c_str(),
               g_conf.rclone.empty() ? "" : " and rclone ", g_conf.rclone.c_str());
  std::thread([] {
    for (unsigned i = 0;; i++) {
      LinuxPass(i % 3 == 0);
      std::this_thread::sleep_for(std::chrono::seconds(20));
    }
  }).detach();
  std::atexit([] { LinuxPass(true); });  // a save made just before quitting goes up too
}

#endif

}  // namespace h4
