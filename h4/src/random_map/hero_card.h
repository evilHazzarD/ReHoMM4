#pragma once

#include <cstdint>
#include <map>
#include <string>
#include <string_view>
#include <vector>

namespace h4::random_map {

class Resources;

// A picture of the game's (ARGB, top-down; alpha 0 transparent).
struct Image {
  int w = 0, h = 0;
  std::vector<uint32_t> argb;
};

// layers.*.h4d, a group of paletted pictures: u16 count, then per layer u16 palette size,
// u16 block alpha, the palette (B, G, R), a u16-prefixed name, u8 alpha depth, i32 left, top,
// right, bottom, a row (u16 x0, u16 x1, u32 offset) per line, N palette indices (N: the last
// row's offset + width), then with alpha depth 4 or more (N + 1) / 2 bytes of 4-bit alpha (the even
// pixel in the low nibble) and with block alpha (N + 131) / 128 more; alpha depth 1: index 0 is
// transparent. (After homm4-random-map-generator's tools/h4data.py.) Names lower case.
std::map<std::string, Image> ReadLayers(const std::vector<uint8_t>& bytes);

// What the setup's hero card shows of a starting hero, from the game's own data (its archives:
// table.skills, table.creatures and the layers of portraits and icons), after
// homm4-random-map-generator's tools/heroinfo.py. The game's texts are its bytes (its language).
class HeroArt {
 public:
  struct Creature {
    std::string plural;
    int level = 0, attack = 0, defense = 0, hit_points = 0, low = 0, high = 0, speed = 0, move = 0;
  };

  bool Load(const Resources& res);  // the tables; pictures when first asked for
  // class: a hero record's class number (Catalog's HeroChoice::hero_class).
  static std::vector<std::string> ClassSkills(int hero_class);  // skill keywords ("tactics", ...)
  static const char* ClassTitle(int hero_class);                // "Knight", ... (English)
  // The faction's starting army (bodies.cpp's StartArmy): creature keywords and counts.
  static std::vector<std::pair<std::string, int>> Army(int faction);
  // A level 1 hero's statistics (kStats' order), as heroes4.exe's t_hero works them out (its
  // skills all basic, no artifacts); the damage's low end (its high end: kDamageHigh).
  static constexpr int kStatCount = 9;
  static const char* const kStats[kStatCount];       // icon names: "melee_attack", ...
  static const char* const kStatTitles[kStatCount];  // "Melee Attack", ... (English)
  static std::vector<int> Stats(int hero_class);
  static constexpr int kDamageHigh = 24;

  std::string SkillName(const std::string& skill) const;  // its basic level's ("Basic Tactics")
  const Creature* FindCreature(const std::string& keyword) const;

  // nullptr: none in the game's data.
  const Image* Portrait(int faction, const std::string& keyword, const std::string& name) const;
  const Image* SkillIcon(const std::string& skill) const;
  const Image* CreatureIcon(const std::string& keyword) const;
  const Image* MoodIcon(const char* what) const;  // "morale", "luck": its neutral (0) one
  const Image* StatIcon(const char* stat) const;  // a kStats one

 private:
  const Resources* res_ = nullptr;
  std::map<std::string, std::string> skill_names_;  // "<skill>_basic" ->
  std::map<std::string, Creature> creatures_;       // lower-case keyword ->
  mutable std::map<std::string, std::map<std::string, Image>> sheets_;
  const Image* Find(const std::string& sheet, const std::string& name) const;
};

}  // namespace h4::random_map
