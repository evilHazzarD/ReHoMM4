#include "install/gog.h"

#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>
#include <zlib.h>

#include <algorithm>
#include <cctype>
#include <condition_variable>
#include <ctime>
#include <deque>
#include <map>
#include <mutex>
#include <thread>
#include <vector>

#include "install/hash.h"
#include "install/json.h"
#include "install/manifest.h"
#include "install/platform.h"

namespace h4::install::gog {

namespace {

// GOG Galaxy's public client (the same as lgogdownloader, Heroic, minigalaxy).
constexpr const char* kClientId = "46899977096215655";
constexpr const char* kClientSecret =
    "9d85c43b1482497dbbce61f6e4aa173a433796eeae2ca8c5f6129f2dc4de46d9";
constexpr const char* kRedirect = "https://embed.gog.com/on_login_success?origin=client";
// The build the manifest was made from: "3.0 win11 GOG 0.2" (2024-09-11).
constexpr const char* kBuildId = "57915408093044786";
constexpr int kWorkers = 4;

std::string Enc(const std::string& s) {
  static const char* hex = "0123456789ABCDEF";
  std::string o;
  for (unsigned char c : s) {
    if (std::isalnum(c) || c == '-' || c == '_' || c == '.' || c == '~') {
      o.push_back(char(c));
    } else {
      o.push_back('%');
      o.push_back(hex[c >> 4]);
      o.push_back(hex[c & 15]);
    }
  }
  return o;
}

bool Inflate(const std::string& in, size_t expect, std::string* out) {
  z_stream z{};
  if (inflateInit(&z) != Z_OK) return false;
  out->assign(expect ? expect : in.size() * 4 + 1024, '\0');
  z.next_in = reinterpret_cast<Bytef*>(const_cast<char*>(in.data()));
  z.avail_in = uInt(in.size());
  size_t have = 0;
  int r;
  do {
    if (have == out->size()) out->resize(out->size() * 2);
    z.next_out = reinterpret_cast<Bytef*>(&(*out)[have]);
    z.avail_out = uInt(out->size() - have);
    r = inflate(&z, Z_NO_FLUSH);
    have = out->size() - z.avail_out;
  } while (r == Z_OK);
  inflateEnd(&z);
  out->resize(have);
  return r == Z_STREAM_END && (!expect || have == expect);
}

std::string MetaUrl(const std::string& h) {
  return "https://gog-cdn-fastly.gog.com/content-system/v2/meta/" + h.substr(0, 2) + "/" +
         h.substr(2, 2) + "/" + h;
}

// GET and parse JSON (zlib-compressed for content-system meta).
bool GetJson(const std::string& url, const std::string& bearer, bool zlib, Json* out,
             std::string* error) {
  int status = 0;
  std::string body;
  if (!HttpGet(url, bearer, &status, &body, error)) return false;
  if (status != 200) {
    *error = "GOG answered " + std::to_string(status);
    return false;
  }
  if (zlib) {
    std::string raw;
    if (!Inflate(body, 0, &raw)) {
      *error = "GOG sent a damaged file list";
      return false;
    }
    body.swap(raw);
  }
  if (!Json::Parse(body, out, error)) {
    *error = "GOG sent an unexpected answer";
    return false;
  }
  return true;
}

struct Chunk {
  std::string md5, cmd5;
  uint64_t size = 0, csize = 0, offset = 0;
  int file = 0;
};

struct PlanFile {
  std::string path;  // '/'-separated
  uint64_t size = 0;
  std::vector<Chunk> chunks;
};

// A depot item's chunks (offsets in the file); its size, their sum.
PlanFile Plan(const Json& it) {
  PlanFile pf;
  pf.path = it["path"].text();
  for (char& c : pf.path)
    if (c == '\\') c = '/';
  for (const Json& c : it["chunks"].arr) {
    Chunk ch;
    ch.md5 = c["md5"].text();
    ch.cmd5 = c["compressedMd5"].text();
    ch.size = uint64_t(c["size"].integer());
    ch.csize = uint64_t(c["compressedSize"].integer());
    ch.offset = pf.size;
    pf.size += ch.size;
    pf.chunks.push_back(ch);
  }
  return pf;
}

}  // namespace

std::string LoginUrl() {
  return std::string("https://auth.gog.com/auth?client_id=") + kClientId +
         "&redirect_uri=" + Enc(kRedirect) + "&response_type=code&layout=client2";
}

bool IsLoginSuccessUrl(const std::string& url) {
  return url.rfind("https://embed.gog.com/on_login_success", 0) == 0;
}

bool Session::Token(const std::string& query, std::string* error) {
  Json j;
  const std::string url = std::string("https://auth.gog.com/token?client_id=") + kClientId +
                          "&client_secret=" + kClientSecret + "&" + query;
  if (!GetJson(url, "", false, &j, error)) return false;
  if (j["access_token"].text().empty()) {
    *error = "GOG did not sign you in";
    return false;
  }
  access_ = j["access_token"].text();
  if (!j["refresh_token"].text().empty()) refresh_ = j["refresh_token"].text();
  expires_ = (long long)std::time(nullptr) + std::max<long long>(j["expires_in"].integer(), 600);
  SaveToken(refresh_);
  return true;
}

bool Session::SignIn(const std::string& code, std::string* error) {
  return Token("grant_type=authorization_code&code=" + Enc(code) + "&redirect_uri=" + Enc(kRedirect),
               error);
}

bool Session::Resume(std::string* error) {
  refresh_ = LoadToken();
  if (refresh_.empty()) {
    *error = "not signed in";
    return false;
  }
  if (Token("grant_type=refresh_token&refresh_token=" + Enc(refresh_), error)) return true;
  refresh_.clear();
  return false;
}

void Session::SignOut() {
  access_.clear();
  refresh_.clear();
  SaveToken("");
}

std::string Session::Access(std::string* error) {
  if (!access_.empty() && (long long)std::time(nullptr) < expires_ - 300) return access_;
  if (refresh_.empty()) {
    *error = "not signed in to GOG";
    return "";
  }
  if (!Token("grant_type=refresh_token&refresh_token=" + Enc(refresh_), error)) return "";
  return access_;
}

bool Session::OwnsGame(bool* owns, std::string* error) {
  const std::string token = Access(error);
  if (token.empty()) return false;
  Json j;
  if (!GetJson("https://embed.gog.com/user/data/games", token, false, &j, error)) return false;
  *owns = false;
  for (const Json& id : j["owned"].arr)
    if (id.text() == kProductId) *owns = true;
  return true;
}

// The build's metadata (its depots): the build the manifest was made from, else GOG's latest.
bool BuildMeta(Json* meta, std::string* error) {
  Json builds;
  if (!GetJson(std::string("https://content-system.gog.com/products/") + kProductId +
                   "/os/windows/builds?generation=2",
               "", false, &builds, error))
    return false;
  const Json* build = nullptr;
  for (const Json& b : builds["items"].arr)
    if (b["build_id"].text() == kBuildId) build = &b;
  if (!build && builds["items"].size()) build = &builds["items"][0];
  if (!build) {
    *error = "GOG has no Windows build of the game";
    return false;
  }
  return GetJson((*build)["link"].text(), "", true, meta, error);
}

bool Session::Languages(std::vector<std::string>* out, std::string* error) {
  Json meta;
  if (!BuildMeta(&meta, error)) return false;
  out->assign(1, "en-US");
  for (const Json& d : meta["depots"].arr)
    for (const Json& l : d["languages"].arr)
      if (d["productId"].text() == kProductId && l.text() != "*" &&
          std::find(out->begin(), out->end(), l.text()) == out->end())
        out->push_back(l.text());
  return true;
}

bool Session::Download(const std::string& dir, const std::string& lang, Task* task, std::string* error) {
  task->SetPhase("Getting the file list from GOG");
  Json meta;
  if (!BuildMeta(&meta, error)) return false;

  // The English files: every depot for en-US (or all languages); a later depot's file replaces an
  // earlier one's. Another language: every file of its depots, but heroes4.exe the English one (the
  // app's code is made from it; the data of another language works with it).
  std::vector<PlanFile> files;
  std::map<std::string, size_t> index;  // by lower-case path
  auto lower = [](std::string s) {
    for (char& c : s) c = char(std::tolower(static_cast<unsigned char>(c)));
    return s;
  };
  auto add = [&](PlanFile pf) {
    auto at = index.find(lower(pf.path));
    if (at == index.end()) {
      index[lower(pf.path)] = files.size();
      files.push_back(std::move(pf));
    } else {
      files[at->second] = std::move(pf);
    }
  };
  const bool english = lang == "en-US";
  for (const Json& d : meta["depots"].arr) {
    bool en = false, ours = false;
    for (const Json& l : d["languages"].arr) {
      en |= l.text() == "en-US" || l.text() == "*";
      ours |= l.text() == lang || l.text() == "*";
    }
    if ((!en && !ours) || d["productId"].text() != kProductId) continue;
    Json dm;
    if (!GetJson(MetaUrl(d["manifest"].text()), "", true, &dm, error)) return false;
    for (const Json& it : dm["depot"]["items"].arr) {
      if (it["type"].text() != "DepotFile") continue;
      PlanFile pf = Plan(it);
      const ManifestFile* m = FindInManifest(pf.path);
      if (english || lower(pf.path) == "heroes4.exe") {
        if (!en || !m || pf.size != m->size) continue;  // another version of the file: not ours
        pf.path = m->path;
        add(std::move(pf));
      } else if (ours) {
        if (m) pf.path = m->path;  // the English spelling
        add(std::move(pf));
      }
    }
  }
  if (english) {
    for (const ManifestFile& m : Manifest()) {
      if (!index.count(lower(m.path))) {
        *error = std::string("GOG's build has no ") + m.path +
                 " of the expected size (GOG may have updated the game)";
        return false;
      }
    }
  } else if (!index.count("heroes4.exe")) {
    *error = "GOG's build has no English heroes4.exe of the expected size (GOG may have updated the game)";
    return false;
  }
  uint64_t total = 0;
  for (const PlanFile& pf : files) total += pf.size;

  // What is already there (an earlier, interrupted download) and checks out
  // chunk by chunk is kept.
  task->SetPhase("Checking what is already downloaded");
  task->total = total;
  task->done = 0;
  std::deque<Chunk> todo;
  std::vector<int> fds(files.size(), -1);
  for (size_t fi = 0; fi < files.size(); fi++) {
    if (task->cancel) break;
    PlanFile& pf = files[fi];
    if (pf.path.find("..") != std::string::npos) continue;
    const std::string path = dir + "/" + pf.path;
    MakeDirs(path.substr(0, path.rfind('/')));
    int fd = open(path.c_str(), O_RDWR | O_CREAT, 0644);
    if (fd < 0) {
      *error = "cannot write " + path;
      for (int f : fds) if (f >= 0) close(f);
      return false;
    }
    fds[fi] = fd;
    struct stat st;
    const bool full = fstat(fd, &st) == 0 && uint64_t(st.st_size) == pf.size;
    if (!full && ftruncate(fd, off_t(pf.size)) != 0) {
      *error = "not enough space for " + pf.path;
      for (int f : fds) if (f >= 0) close(f);
      return false;
    }
    std::string buf;
    for (Chunk& ch : pf.chunks) {
      ch.file = int(fi);
      bool ok = false;
      if (full) {
        buf.resize(ch.size);
        ok = pread(fd, &buf[0], ch.size, off_t(ch.offset)) == ssize_t(ch.size) &&
             Md5Hex(buf.data(), buf.size()) == ch.md5;
      }
      if (ok) task->done += ch.size;
      else todo.push_back(ch);
    }
  }

  // GOG's CDN address for chunks, signed for a while.
  std::mutex mu;
  Json link;
  long long link_until = 0;
  auto RenewLink = [&](std::string* err) -> bool {
    const std::string token = Access(err);
    if (token.empty()) return false;
    Json j;
    if (!GetJson(std::string("https://content-system.gog.com/products/") + kProductId +
                     "/secure_link?generation=2&path=/&_version=2",
                 token, false, &j, err))
      return false;
    if (!j["urls"].size()) {
      *err = "GOG gave no download address";
      return false;
    }
    link = j["urls"][0];
    long long exp = link["parameters"]["expires_at"].integer();
    link_until = exp ? exp - 120 : (long long)std::time(nullptr) + 1800;
    return true;
  };
  auto ChunkUrl = [&](const std::string& cmd5, std::string* url, std::string* err) -> bool {
    std::lock_guard<std::mutex> l(mu);
    if (link.type == Json::Null || (long long)std::time(nullptr) > link_until)
      if (!RenewLink(err)) return false;
    std::string u = link["url_format"].text();
    for (const auto& [k, v] : link["parameters"].obj) {
      std::string val = v.text();
      if (k == "path") val += "/" + cmd5.substr(0, 2) + "/" + cmd5.substr(2, 2) + "/" + cmd5;
      const std::string key = "{" + k + "}";
      for (size_t at; (at = u.find(key)) != std::string::npos;) u.replace(at, key.size(), val);
    }
    *url = u;
    return true;
  };

  task->SetPhase("Downloading from GOG");
  std::string first_error;
  std::mutex qmu;
  auto Worker = [&] {
    for (;;) {
      Chunk ch;
      {
        std::lock_guard<std::mutex> l(qmu);
        if (todo.empty() || task->cancel || !first_error.empty()) return;
        ch = todo.front();
        todo.pop_front();
      }
      std::string err;
      bool ok = false;
      for (int attempt = 0; attempt < 5 && !ok && !task->cancel; attempt++) {
        if (attempt) std::this_thread::sleep_for(std::chrono::seconds(attempt * 2));
        std::string url, body, raw;
        int status = 0;
        if (!ChunkUrl(ch.cmd5, &url, &err)) continue;
        if (!HttpGet(url, "", &status, &body, &err)) continue;
        if (status == 401 || status == 403) {
          std::lock_guard<std::mutex> l(mu);
          link_until = 0;  // signed address expired: a new one next time
          err = "GOG refused the download address";
          continue;
        }
        if (status != 200) { err = "GOG's server answered " + std::to_string(status); continue; }
        if (body.size() != ch.csize || Md5Hex(body.data(), body.size()) != ch.cmd5) {
          err = "a downloaded part was damaged";
          continue;
        }
        if (!Inflate(body, ch.size, &raw) || Md5Hex(raw.data(), raw.size()) != ch.md5) {
          err = "a downloaded part was damaged";
          continue;
        }
        if (pwrite(fds[ch.file], raw.data(), raw.size(), off_t(ch.offset)) != ssize_t(raw.size())) {
          err = "cannot write the game files (is the storage full?)";
          break;
        }
        ok = true;
      }
      if (ok) {
        task->done += ch.size;
      } else if (!task->cancel) {
        std::lock_guard<std::mutex> l(qmu);
        if (first_error.empty()) first_error = err.empty() ? "download failed" : err;
        return;
      }
    }
  };
  std::vector<std::thread> workers;
  for (int i = 0; i < kWorkers; i++) workers.emplace_back(Worker);
  for (std::thread& t : workers) t.join();
  for (int f : fds) if (f >= 0) close(f);
  if (task->cancel) {
    *error = "cancelled";
    return false;
  }
  if (!first_error.empty()) {
    *error = first_error;
    return false;
  }
  return true;
}

}  // namespace h4::install::gog
