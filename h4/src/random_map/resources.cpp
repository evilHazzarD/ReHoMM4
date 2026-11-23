#include "resources.h"

#include <algorithm>
#include <cstdio>
#include <cstring>
#include <map>

#include <zlib.h>

namespace h4::random_map {
namespace {

// Newest first: the game opens these after heroes4/storm/x2 and each one overrides the ones
// before it (heroes4.exe's list).
const char* const kOrder[] = {"new.h4r", "x2_updates.h4r", "x2_override.h4r", "storm_updates.h4r",
                              "storm_override.h4r", "updates.h4r", "text.h4r", "music.h4r",
                              "movies.h4r", "x2.h4r", "storm.h4r", "heroes4.h4r"};

std::string Lower(std::string_view s) {
  std::string out(s);
  for (char& c : out)
    if (c >= 'A' && c <= 'Z') c = char(c - 'A' + 'a');
  return out;
}

uint32_t U32(const uint8_t* p) { return p[0] | p[1] << 8 | p[2] << 16 | uint32_t(p[3]) << 24; }
uint16_t U16(const uint8_t* p) { return uint16_t(p[0] | p[1] << 8); }

}  // namespace

bool Gunzip(const uint8_t* in, size_t size, std::vector<uint8_t>& out, size_t limit) {
  out.clear();
  size_t pos = 0;
  while (pos < size) {
    z_stream z{};
    if (inflateInit2(&z, 16 + MAX_WBITS) != Z_OK) return false;
    z.next_in = const_cast<Bytef*>(in + pos);
    z.avail_in = uInt(size - pos);
    uint8_t chunk[1 << 15];
    int r;
    do {
      z.next_out = chunk;
      z.avail_out = sizeof chunk;
      r = inflate(&z, Z_NO_FLUSH);
      if (r != Z_OK && r != Z_STREAM_END) {
        inflateEnd(&z);
        return false;
      }
      out.insert(out.end(), chunk, chunk + (sizeof chunk - z.avail_out));
      if (limit && out.size() >= limit) {
        inflateEnd(&z);
        return true;
      }
    } while (r != Z_STREAM_END && (z.avail_in || !z.avail_out));
    const size_t used = size - pos - z.avail_in;
    inflateEnd(&z);
    if (r != Z_STREAM_END) return false;
    pos += used;
    // Padding after the last member ends it.
    if (pos + 2 > size || in[pos] != 0x1F || in[pos + 1] != 0x8B) break;
  }
  return true;
}

struct Resources::Archive {
  std::FILE* file = nullptr;
  struct Entry {
    uint32_t offset, packed;
    std::string alias, spelling;
  };
  std::map<std::string, Entry> entries;  // lower-case name ->
  ~Archive() {
    if (file) std::fclose(file);
  }
};

Resources::Resources() = default;
Resources::~Resources() = default;

bool Resources::Open(const std::string& data_dir, std::string* error) {
  return Open([&](const std::string& path) { return data_dir + "/" + path.substr(5); }, error);
}

bool Resources::Open(const PathOf& path_of, std::string* error) {
  archives_.clear();
  for (const char* name : kOrder) {
    auto a = std::make_unique<Archive>();
    a->file = std::fopen(path_of(std::string("data/") + name).c_str(), "rb");
    if (!a->file) continue;
    uint8_t head[8];
    if (std::fread(head, 1, 8, a->file) != 8 || std::memcmp(head, "H4R\x05", 4) != 0) {
      if (error) *error = std::string(name) + " is not a Heroes IV archive";
      return false;
    }
    std::fseek(a->file, 0, SEEK_END);
    const long end = std::ftell(a->file);
    const uint32_t dir = U32(head + 4);
    if (end < 0 || dir >= uint32_t(end)) {
      if (error) *error = std::string(name) + ": no directory";
      return false;
    }
    // The directory only, read as far as it goes: it is at the start of some archives (offset 8),
    // with the whole archive after it (heroes4.h4r: 600 MB, which a browser cannot take at once).
    const size_t avail = size_t(end) - dir;
    std::vector<uint8_t> d;
    std::fseek(a->file, long(dir), SEEK_SET);
    auto have = [&](size_t n) {  // the directory's first n bytes read
      while (d.size() < n) {
        const size_t want = std::min(avail - d.size(), std::max<size_t>(n - d.size(), 1 << 16));
        if (!want) return false;
        const size_t at = d.size();
        d.resize(at + want);
        if (std::fread(&d[at], 1, want, a->file) != want) return false;
      }
      return true;
    };
    if (!have(4)) {
      if (error) *error = std::string(name) + ": unreadable directory";
      return false;
    }
    size_t p = 4;
    const uint32_t count = U32(d.data());
    auto bad = [&] {
      if (error) *error = std::string(name) + ": bad directory";
      return false;
    };
    for (uint32_t i = 0; i < count; ++i) {
      if (!have(p + 16)) return bad();
      Archive::Entry e{U32(&d[p]), U32(&d[p + 4]), {}, {}};
      p += 16;
      std::string strs[3];
      for (auto& s : strs) {
        if (!have(p + 2)) return bad();
        const uint16_t n = U16(&d[p]);
        if (!have(p + 2 + n)) return bad();
        s.assign(reinterpret_cast<const char*>(&d[p + 2]), n);
        p += 2 + n;
      }
      p += 4;
      e.alias = Lower(strs[2]);
      e.spelling = strs[0];
      a->entries.emplace(Lower(strs[0]), std::move(e));
    }
    archives_.push_back(std::move(a));
  }
  if (archives_.empty()) {
    if (error) *error = "no game archives in " + path_of("data/");
    return false;
  }
  return true;
}

std::vector<std::string> Resources::Names(std::string_view prefix) const {
  std::vector<std::string> out;
  for (const auto& a : archives_)
    for (auto it = a->entries.lower_bound(std::string(prefix));
         it != a->entries.end() && it->first.compare(0, prefix.size(), prefix) == 0; ++it)
      out.push_back(it->first);
  std::sort(out.begin(), out.end());
  out.erase(std::unique(out.begin(), out.end()), out.end());
  return out;
}

std::string Resources::Spelling(std::string_view lower_name) const {
  for (const auto& a : archives_) {
    auto it = a->entries.find(std::string(lower_name));
    if (it != a->entries.end()) return it->second.spelling;
  }
  return {};
}

bool Resources::Read(std::string_view name, std::vector<uint8_t>& out, size_t limit) const {
  std::string key = Lower(name);
  for (int hop = 0; hop < 8; ++hop) {
    const Archive* found = nullptr;
    const Archive::Entry* e = nullptr;
    for (const auto& a : archives_) {
      auto it = a->entries.find(key);
      if (it != a->entries.end()) {
        found = a.get();
        e = &it->second;
        break;
      }
    }
    if (!found) return false;
    if (!e->alias.empty()) {
      key = e->alias;
      continue;
    }
    // A gzip stream's start gives its output's start: a little more than `limit` packed bytes
    // is plenty for one that compresses.
    const size_t want = limit ? std::min<size_t>(e->packed, limit + 4096) : e->packed;
    std::vector<uint8_t> packed(want);
    if (std::fseek(found->file, long(e->offset), SEEK_SET) != 0 ||
        std::fread(packed.data(), 1, packed.size(), found->file) != packed.size())
      return false;
    // Stored as it is, not gzipped (a translation's files: language.h's packs).
    if (packed.size() < 2 || packed[0] != 0x1F || packed[1] != 0x8B) {
      out.assign(packed.begin(), packed.begin() + std::min(packed.size(), limit ? limit : packed.size()));
      return true;
    }
    if (Gunzip(packed.data(), packed.size(), out, limit)) return true;
    if (want == e->packed) return false;
    packed.resize(e->packed);  // (it did not: all of it)
    if (std::fseek(found->file, long(e->offset), SEEK_SET) != 0 ||
        std::fread(packed.data(), 1, packed.size(), found->file) != packed.size())
      return false;
    return Gunzip(packed.data(), packed.size(), out, limit);
  }
  return false;
}

std::vector<std::vector<std::string>> ReadTable(const std::vector<uint8_t>& d) {
  std::vector<std::vector<std::string>> rows;
  if (d.size() < 4) return rows;
  size_t p = 4;
  const uint32_t count = U32(d.data());
  for (uint32_t i = 0; i < count && p + 2 <= d.size(); ++i) {
    const uint16_t n = U16(&d[p]);
    p += 2;
    auto& row = rows.emplace_back();
    for (uint16_t j = 0; j < n && p + 2 <= d.size(); ++j) {
      const uint16_t k = U16(&d[p]);
      if (p + 2 + k > d.size()) return rows;
      row.emplace_back(reinterpret_cast<const char*>(&d[p + 2]), k);
      p += 2 + k;
    }
  }
  return rows;
}

}  // namespace h4::random_map
