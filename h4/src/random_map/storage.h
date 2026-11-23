#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace h4::random_map {

// The folder of the generated maps, in the game's maps folder.
constexpr const char* kMapFolder = "RandomMaps";
// A generated map's title: rm_<yy-mm-dd>_<hh:mm:ss>_<seed>, the local time `when`.
std::string MapTitle(int64_t when, uint32_t seed);
// Its file name: the title, ':' as '-' (not in a Windows file name), .h4c.
std::string MapFileName(const std::string& title);
// The title back from such a file name ("" if it is not one).
std::string TitleOfFile(const std::string& name);
// Whether a file name is a generated map's: rm_... or the older _random_map_<seed>.h4c (any case:
// the game lists what the file system gives).
bool IsMapFileName(const std::string& name);

// Writes `bytes` to `path` as a new file: whole or not at all (a temporary file beside it, then
// moved in place), never over a file already there. false: `error` says why (EEXIST: "exists").
bool WriteNewFile(const std::string& path, const std::vector<uint8_t>& bytes, std::string* error);
// Writes `bytes` over `path` (a map edited again): whole or not at all (a temporary file beside
// it, then renamed over it), so the old map stays if anything fails.
bool ReplaceFile(const std::string& path, const std::vector<uint8_t>& bytes, std::string* error);

}  // namespace h4::random_map
