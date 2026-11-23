#pragma once

#include <cstdint>
#include <string_view>
#include <utility>
#include <vector>

namespace h4::random_map {

// Object bodies (what follows an object's model and kind) built from the map format, with the
// editor's defaults: what Tihomir Ivanov's homm4-random-map-generator copies from the game's
// shipped maps (data/templates.json, object_catalog.json), written out field by field instead.
// Layout per its tools/h4objects.py and research/format.md.

// The body of a placed object of `type` and `subtype` (its kind's first two names): empty if the
// generator cannot make one.
std::vector<uint8_t> ObjectBody(std::string_view type, std::string_view subtype);

// A town's body: a random town (faction < 0: "random_town", v7) or one of `faction` ("town",
// v4 with the two spell sets), `owner` = player colour, default buildings, no garrison.
std::vector<uint8_t> TownBody(int faction, uint8_t owner);

// A neutral army of one kind of creature (its number in the game's creature order, 0-based),
// its count the game's own (0), as the shipped maps' guards on the water have it.
std::vector<uint8_t> CreatureArmy(int creature);

struct HeroRecord {
  int16_t portrait = -1;   // -1: random
  uint8_t hero_class = 0xFF, sex = 0xFF;  // 0xFF: random
  std::string_view name, biography;       // cp1252
  std::vector<std::pair<int16_t, uint16_t>> army;  // (creature number, count), up to 6
};
// A player's hero on the map: a v9 army record (what follows the 13-byte position/army header).
std::vector<uint8_t> HeroArmy(uint8_t owner, const HeroRecord& hero);

// The faction's starting army (two level-1 stacks), as upstream's heroinfo.START_ARMY.
std::vector<std::pair<int16_t, uint16_t>> StartArmy(int faction);

}  // namespace h4::random_map
