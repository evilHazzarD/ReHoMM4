#pragma once

#include <cstdint>
#include <functional>
#include <memory>
#include <string>
#include <string_view>
#include <vector>

namespace h4::random_map {

// The game's .h4r archives in <game>/Data, read-only: "H4R\x05", u32 directory offset, gzip
// blobs; the directory is u32 count + (u32 offset, packed size, size, time; name, source, alias
// strings; u32 type). The first archive in the game's override order that has a name wins.
// Format after the h4r project's notes (github.com/demen1n/h4r), as Tihomir Ivanov's
// homm4-random-map-generator reads it (tools/h4r.py).
class Resources {
 public:
  Resources();
  ~Resources();
  Resources(const Resources&) = delete;
  Resources& operator=(const Resources&) = delete;
  // Where a file of the game is, by its path in the install ("data/new.h4r", "heroes4.exe"):
  // the game's own view of it (ReHoMM4's overlay over the install), any case.
  using PathOf = std::function<std::string(const std::string& path)>;
  bool Open(const PathOf& path_of, std::string* error);
  bool Open(const std::string& data_dir, std::string* error);  // <game>/Data
  // Every name (lower case) that starts with `prefix`, sorted.
  std::vector<std::string> Names(std::string_view prefix) const;
  // The resource's bytes (aliases followed); false if there is none. `limit`: at most about
  // that many (its start), 0 = all.
  bool Read(std::string_view name, std::vector<uint8_t>& out, size_t limit = 0) const;
  // The name as the archive spells it ("" if there is none).
  std::string Spelling(std::string_view lower_name) const;

 private:
  struct Archive;
  std::vector<std::unique_ptr<Archive>> archives_;
};

// table.*.h4d: u32 row count; each row u16 cell count + u16-prefixed cp1252 strings.
std::vector<std::vector<std::string>> ReadTable(const std::vector<uint8_t>& bytes);

// Gunzips `in` (one or more gzip members); with a `limit`, stops once it has that many bytes.
bool Gunzip(const uint8_t* in, size_t size, std::vector<uint8_t>& out, size_t limit = 0);

}  // namespace h4::random_map
