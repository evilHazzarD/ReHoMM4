#pragma once

#include <array>
#include <cstdint>
#include <map>
#include <string>
#include <string_view>
#include <vector>

namespace h4::random_map {

// An object model's footprint, from the game's adv_object.<model>.h4d (in its archives).
// Axes: x runs along map rows, y along columns; cell index = y * w + x.
struct Footprint {
  std::string model;                 // as the archive spells it
  int w = 0, h = 0;
  std::vector<uint8_t> blocked;      // w * h: 1 = blocked
  int blocked_count = 0;
  std::vector<std::array<int, 2>> entry;  // (x, y) cells a hero visits from (may be outside)
  std::array<std::string, 3> kind;   // type, major, minor subtype
  std::array<uint16_t, 3> kind_ids{};
};

enum Faction { kLife, kOrder, kDeath, kChaos, kNature, kMight, kFactionCount };
extern const char* const kFactionNames[kFactionCount];  // "life", ...

// A hero of table.heroes the map can start with.
struct HeroChoice {
  std::string keyword, name, biography;  // name and biography: cp1252, the game's language
  int faction = -1;
  int16_t portrait = -1;                 // index in heroes4.exe's portrait table
  uint8_t hero_class = 0xFF, sex = 0xFF;
};

// What the generator needs from the player's own game: read from its archives and heroes4.exe
// at run time, so nothing of the game is carried by ReHoMM4.
class Resources;

class Catalog {
 public:
  // game_dir: the folder with heroes4.exe and Data/.
  bool Load(const std::string& game_dir, std::string* error);
  // From open archives and heroes4.exe's bytes.
  bool Load(const Resources& res, const std::vector<uint8_t>& exe, std::string* error);

  const Footprint* Find(std::string_view model) const;  // any case
  // Fully blocking decorations of at most 3 x 3 for a terrain (1 grass ... 8 subterranean).
  const std::vector<const Footprint*>& Obstacles(int terrain) const { return obstacles_[terrain]; }
  // The edge of a cave (rock) of the underground: the variants of wall number n (1 to 12), whose
  // number says which of its 8 neighbours are rock (generator.cpp's kCaveWall).
  const std::vector<const Footprint*>& CaveWalls(int n) const { return cave_walls_[size_t(n)]; }
  // The heroes allowed by default, by faction, sorted by name.
  const std::vector<HeroChoice>& Heroes(int faction) const { return heroes_[faction]; }
  // Artifacts' numbers (table.artifacts' row order) of a level: "Major", "Relic", ...
  const std::vector<uint16_t>& Artifacts(const std::string& level) const {
    static const std::vector<uint16_t> none;
    auto it = artifacts_.find(level);
    return it == artifacts_.end() ? none : it->second;
  }

  // Decorations of a group ("shrubs.grass.", "rocks.water.", ...: catalog.cpp's kDecorPrefixes).
  const std::vector<const Footprint*>& Decor(const std::string& group) const {
    static const std::vector<const Footprint*> none;
    auto it = decor_.find(group);
    return it == decor_.end() ? none : it->second;
  }

 private:
  std::map<std::string, std::vector<uint16_t>> artifacts_;
  std::vector<std::pair<std::string, std::string>> decor_names_;
  std::map<std::string, std::vector<const Footprint*>> decor_;
  std::map<std::string, Footprint> footprints_;  // lower-case model ->
  std::array<std::vector<const Footprint*>, 9> obstacles_;
  std::array<std::vector<const Footprint*>, 13> cave_walls_;
  std::array<std::vector<HeroChoice>, kFactionCount> heroes_;
};

// A whole file (false: none, unreadable).
bool ReadFile(const std::string& path, std::vector<uint8_t>& out);
// Parses an adv_object resource's footprint (the start of the file).
bool ParseFootprint(const std::vector<uint8_t>& d, Footprint& fp);
// heroes4.exe's portrait table: (keyword, faction), from the pointer to "aidan" on.
std::vector<std::pair<std::string, int>> ExePortraits(const std::vector<uint8_t>& exe);

}  // namespace h4::random_map
