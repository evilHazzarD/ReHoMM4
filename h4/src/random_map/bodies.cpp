#include "bodies.h"

#include <string>

namespace h4::random_map {
namespace {

struct Out {
  std::vector<uint8_t> b;
  Out& u8(uint32_t v) {
    b.push_back(uint8_t(v));
    return *this;
  }
  Out& u16(uint32_t v) { return u8(v).u8(v >> 8); }
  Out& u32(uint32_t v) { return u16(v).u16(v >> 16); }
  Out& str(std::string_view s) {
    u16(uint32_t(s.size()));
    b.insert(b.end(), s.begin(), s.end());
    return *this;
  }
  Out& raw(std::initializer_list<uint8_t> v) {
    b.insert(b.end(), v);
    return *this;
  }
  Out& fill(size_t n, uint8_t v) {
    b.insert(b.end(), n, v);
    return *this;
  }
  // A built-in event as the editor leaves it: version 0, an empty script (v2, "seq" of nothing),
  // no message, no resources (7 x i32), every player (0x7F), flags 3.
  Out& empty_event() {
    u16(0).u16(2).str("seq").u16(0).str("");
    return fill(28, 0).u8(0x7F).u8(3);
  }
  // A random object's kind: none.
  Out& kind_none() { return u16(0).str("none"); }
};

}  // namespace

std::vector<uint8_t> ObjectBody(std::string_view type, std::string_view subtype) {
  // The skills each school teaches, as the editor sets them (36 bits; a second, empty set).
  struct School {
    const char* subtype;
    uint8_t skills[5];
  };
  static const School kSchools[] = {
      {"library", {0xF0, 0x01, 0xE0, 0xFF, 0x0F}},       {"magic_university", {0x00, 0x00, 0xE0, 0xFF, 0x0F}},
      {"school_of_magic", {0xF0, 0x01, 0x00, 0x00, 0x00}}, {"witchs_hut", {0xF0, 0x01, 0x00, 0x00, 0x00}},
      {"school_of_war", {0x0F, 0x00, 0x00, 0x00, 0x00}},   {"beastmasters_hut", {0x0F, 0x00, 0x00, 0x00, 0x00}},
      {"war_institute", {0x0F, 0xFE, 0x1F, 0x00, 0x00}},   {"war_college", {0x00, 0xFE, 0x1F, 0x00, 0x00}},
  };
  Out o;
  if (type == "school") {
    for (const School& s : kSchools)
      if (subtype == s.subtype) {
        o.u16(1).u8(0);
        for (uint8_t b : s.skills) o.u8(b);
        o.u8(0).fill(5, 0);
        return o.b;
      }
    return {};
  }
  if (type == "decorative" || type == "campfire" || type == "random_material_pile" ||
      type == "treasure" || type == "random_artifact" || type == "creature_bank" ||
      type == "idol_of_fortune" || type == "movement_booster" || type == "magic_gem" ||
      type == "fountain" || type == "mana_recharger" || type == "system_object" ||
      type == "subterranean_gate" || type == "material_pile" || type == "corpse" ||
      type == "medicine_wagon" || type == "tree_of_knowledge" || type == "academy" ||
      type == "faerie_ring" || type == "clover_field" || type == "rainbow" || type == "blattner_stone" ||
      type == "random_potion" || type == "sanctuary" || type == "tower" || type == "mercenary_camp" ||
      type == "training_grounds" || type == "coliseum" || type == "sacred_grove" || type == "arena" ||
      type == "trading_post" || type == "temple" || type == "bridge" || type == "flotsam" ||
      type == "buoy" || type == "whirlpool" || type == "shipwreck_survivor" || type == "dolphin_school" ||
      type == "pathfinder" || type == "sphinx" || type == "cartographer" || type == "blacksmith" ||
      type == "tavern" || type == "teacher" || type == "random_staff" || type == "oyster" ||
      type == "nothing" || type == "obelisk" || type == "exhausted_mine") {
    o.u16(0);  // as every shipped map has them
  } else if (type == "weekly_material_generator" || type == "windmill" || type == "shipyard" ||
             type == "lighthouse") {  // v2, nobody's
    o.u16(2).u8(6).u16(0);
  } else if (type == "mine") {  // v2, nobody's, 4 built-in events, no events of its own, no guards
    o.u16(2).u8(6);
    for (int i = 0; i < 4; ++i) o.empty_event();
    o.u16(0).u16(0).u16(0).u8(0);
  } else if (type == "random_monster") {  // v4, no flags, 3 built-in events, any kind, i32 0 + 0
    o.u16(4).u8(0);
    for (int i = 0; i < 3; ++i) o.empty_event();
    o.u16(0).u16(0).u16(0).kind_none().u32(0).u32(0);
  } else if (type == "random_creature_dwelling") {  // v3, nobody's, no events, any kind
    o.u16(3).u8(6).u16(0).kind_none();
  } else if (type == "random_temple") {
    o.u16(1).kind_none();
  } else if (type == "random_shrine") {  // v3, a v2 spell set (every spell allowed), any kind
    o.u16(3).u8(2).raw({0xFF, 0xFF, 0xFF, 0xFF, 0x9F}).fill(18, 0xFF).u8(0).kind_none();
  } else if (type == "random_teacher") {
    o.u16(1).u16(0x01FF).kind_none();
  } else {
    return {};
  }
  return o.b;
}

std::vector<uint8_t> CreatureArmy(int creature) {
  Out o;
  // map army v9, nobody's (6); the army: u16 0, a creature slot (v1: type, count 0, no
  // artifacts), 6 empty slots
  o.u16(9).u8(6).u16(0).u8(0).u16(1).u16(uint32_t(creature)).u16(0).u16(0).fill(6, 0xFF);
  o.u8(0);  // no patrol
  for (int i = 0; i < 3; ++i) o.empty_event();
  return o.raw({0, 0, 0, 0, 0, 0, 0xFF, 0, 0}).b;
}

std::vector<uint8_t> TownBody(int faction, uint8_t owner) {
  Out o;
  if (faction < 0) {
    o.u16(7).kind_none();
  } else {  // v4, the mage guild's two spell sets: none required, every one allowed
    o.u16(4).u8(2).fill(24, 0).u8(2).fill(23, 0xFF).u8(0x0F);
  }
  // Town data v8: the owner, no name, no garrison, default buildings, 4 built-in events, no
  // events of its own, no skill sets, 0.
  o.u16(8).u8(owner).str("").u8(0).u8(0).u8(1);
  for (int i = 0; i < 4; ++i) o.empty_event();
  o.u16(0).u16(0).u16(0).u8(0).u8(0);
  return o.b;
}

std::vector<uint8_t> HeroArmy(uint8_t owner, const HeroRecord& h) {
  Out o;
  o.u16(9).u8(owner).u16(0);  // map army v9, the army's version
  // Slot 1: the hero, record v10.
  o.u8(1).u16(10).u16(uint16_t(h.portrait)).u8(h.hero_class).u8(h.sex).u8(0);
  o.u16(0).u16(0);
  o.str(h.biography).str(h.name);  // (the map keeps the biography first)
  o.u8(0);                         // no skills of its own
  o.u8(2).fill(24, 0);             // spell set v2: none
  o.fill(14, 0);                   // no artifacts worn
  o.u16(0);                        // none carried
  for (int i = 0; i < 3; ++i) o.empty_event();
  o.u16(0).u16(0).u16(0);
  o.u8(0);  // v9
  // The other six slots: creature stacks (v1, type, count, no artifacts), then empty ones.
  size_t n = 0;
  for (; n < h.army.size() && n < 6; ++n)
    o.u8(0).u16(1).u16(uint16_t(h.army[n].first)).u16(h.army[n].second).u16(0);
  for (; n < 6; ++n) o.u8(0xFF);
  // v5+: no patrol (flag 0); v6+: 3 built-in events and none of its own; v7-v9: three bytes.
  o.u8(0);
  for (int i = 0; i < 3; ++i) o.empty_event();
  o.u16(0).u16(0).u16(0);
  o.u8(0xFF).u8(0).u8(1);
  return o.b;
}

std::vector<std::pair<int16_t, uint16_t>> StartArmy(int faction) {
  // Creature numbers: the exe's creature list order (upstream's heroinfo.CREATURES).
  switch (faction) {
    case 0: return {{54, 16}, {12, 6}};  // squire, crossbowman
    case 1: return {{18, 14}, {29, 9}};  // dwarf, halfling
    case 2: return {{53, 19}, {33, 11}};  // skeleton, imp
    case 3: return {{46, 12}, {3, 10}};  // orc, bandit
    case 4: return {{65, 11}, {55, 9}};  // wolf, sprite
    case 5: return {{6, 14}, {9, 4}};    // berserker, centaur
    default: return {};
  }
}

}  // namespace h4::random_map
