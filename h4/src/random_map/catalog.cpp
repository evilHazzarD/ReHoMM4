#include "catalog.h"

#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <iterator>
#include <set>

#include "resources.h"

namespace h4::random_map {

const char* const kFactionNames[kFactionCount] = {"life", "order", "death", "chaos", "nature", "might"};

namespace {

std::string Lower(std::string_view s) {
  std::string out(s);
  for (char& c : out)
    if (c >= 'A' && c <= 'Z') c = char(c - 'A' + 'a');
  return out;
}

bool StartsWith(std::string_view s, std::string_view p) { return s.substr(0, p.size()) == p; }

// Obstacles by terrain (model name prefixes, lower case), as Tihomir Ivanov's
// homm4-random-map-generator picks them (tools/generator.py, OBSTACLES).
const std::vector<std::vector<const char*>> kObstaclePrefixes = {
    {},
    {"trees.pine.", "trees.grass.", "rocks.grass."},                                  // grass
    {"mountains.rough.", "rocks.rough.", "trees.dead."},                              // rough
    {"trees.swamps.", "rocks.swamp."},                                                // swamp
    {"trees.dead.", "rocks.lava.", "mountains.lava."},                                // volcanic
    {"trees.snow.", "rocks.snow.", "mountains.snow."},                                // snow
    {"trees.desert.", "rocks.sand.", "mountains.desert.", "shrubs.cactus."},          // sand
    {"trees.grass.", "trees.pine.", "rocks.rough.", "mountains.rough."},              // dirt
    {"mountains.subterranean.", "rocks.subterranean."},                               // subterranean
};

// Decorations by group (generator.cpp's decor: where and how many, as the shipped maps have them).
const char* const kDecorPrefixes[] = {
    "shrubs.grass.", "shrubs.rough.", "shrubs.brush.", "shrubs.vines.", "shrubs.snow.", "shrubs.snow tufts.",
    "shrubs.reeds.", "shrubs.cattails.", "trees.autumn.", "trees.subterranean.", "farms.green.", "farms.snow.",
    "dunes.sand.", "dunes.snow.", "stumps.", "cracks-holes.lava.", "walls.fence.", "walls.regular.",
    "walls.evil.", "water objects.lily pads.", "water objects.kelp.", "rocks.water.",
    "water objects.ice flows.", "water objects.iceberg."};

// The edges of the underground's caves: internal.Walls.Cavern.<01-12><variant>.
constexpr const char* kCaveWallPrefix = "internal.walls.cavern.";

// The other models the generator places (generator.cpp); their footprints are read too.
const char* const kModels[] = {
    "movement modifiers.Tunnel_Down", "movement modifiers.Tunnel_Up",
    "Random creatures.RandomLv1", "Random creatures.RandomLv2", "Random creatures.RandomLv3",
    "Random creatures.RandomLv4", "mine.Crystal", "mine.Crystal R", "mine.Gem", "mine.Gem R",
    "mine.Sulfur", "mine.Sulfur R", "mine.Alchemists Lab", "mine.Alchemists Lab R", "mine.Sawmill",
    "mine.Sawmill R", "mine.ore pit", "mine.Ore Pit R", "mine.Gold", "mine.Gold R",
    "Resources.Random Resource", "Resources.Treasure Chest", "Resources.Campfire",
    "power ups.Fountain of Fortune", "power ups.Fountain of Strength", "power ups.Fountain of Vigor",
    "power ups.Spring of Speed", "power ups.Pool of Power", "Mana Rechargers.magic well",
    "movement modifiers.Water Trough", "moral-luck boosters.Adventurers Totem", "power ups.Ruby",
    "power ups.Emerald", "power ups.Opal", "power ups.Sapphire", "Skill Enhancer.Library",
    "Skill Enhancer.Witch's Hut", "Skill Enhancer.School of Magic", "Skill Enhancer.School of War",
    "Skill Enhancer.Veteran's_Guild", "Skill Enhancer.beastmaster's hut",
    "Skill Enhancer.Magic University", "Skill Enhancer.War College", "creature bank.Bandit Camp",
    "creature bank.Keep", "creature bank.Lair", "creature bank.Imp Cache", "creature bank.Graveyard",
    "creature bank.Crypt", "creature bank.Harpy Nest", "creature bank.Forest Glenn",
    "creature bank.Griffin Nest", "creature generators.Random Creature 2x1",
    "creature generators.Random Creature 2x2", "creature generators.Random Creature 2x3",
    "creature generators.Random Creature 3x3", "artifacts.random artifact.Minor",
    "artifacts.random artifact.Major", "artifacts.random artifact.Treasure",
    "artifacts.random artifact.Item", "shrines.Random 1", "shrines.Random 2", "shrines.Random 3",
    "Scholars.Random Scholar", "moral-luck boosters.Random Temple", "castle.Random Town",
    "castle.Random Town R", "castle.Haven.Fort", "castle.Haven.Fort R", "castle.Academy.Fort",
    "castle.Academy.Fort R", "castle.Necropolis.Fort", "castle.Necropolis.fort r",
    "castle.Asylum.Fort", "castle.Asylum.Fort R", "castle.Preserve.fort", "castle.Preserve.fort r",
    "castle.Stronghold.fort", "castle.Stronghold.fort r",
    // version 2's
    "Resources.Gold", "Resources.Wood", "Resources.Ore", "Resources.Gem", "Resources.Crystal",
    "Resources.Sulfur", "Resources.Mercury", "Resources.Traveler's backpack", "Resources.Mage's Chest",
    "Resources.Skeleton", "Resources.Medicine Wagon", "movement modifiers.Stables",
    "Mana Rechargers.magic spring", "Mana Rechargers.Magic Fountain", "Mana Rechargers.Magician's Pool",
    "moral-luck boosters.Faerie Ring", "moral-luck boosters.Clover Field", "moral-luck boosters.Rainbow",
    "moral-luck boosters.luck Stone", "mine.Weekly.windmill", "artifacts.random artifact.Potion",
    "miscellaneous.Sanctuary", "Shroud Removers.Tower", "moral-luck boosters.Temple of Light",
    "moral-luck boosters.Temple of Nature", "moral-luck boosters.Temple of Order",
    "moral-luck boosters.Temple of Chaos", "moral-luck boosters.Temple of Darkness",
    "power ups.Tree of Knowledge", "Power Ups.Learning Stone", "power ups.Training Grounds",
    "power ups.Mercenary Camp", "power ups.arena_might", "power ups.arena_magic", "power ups.Arena",
    "power ups.Sacred Fountain", "miscellaneous.Trading Post", "mine.Weekly.Leprechaun",
    "mine.Weekly.Imp Pit", "mine.Weekly.Apprentice's Lab", "mine.Weekly.Water Wheel",
    "mine.Weekly.Crystal Pool", "mine.Weekly.Miner's Guilds", "miscellaneous.Shipyard",
    "miscellaneous.Shipyard R", "water objects.flotsam", "water objects.Sea Chest", "water objects.buoy",
    "movement modifiers.whirlpool", "movement modifiers.Lighthouse", "movement modifiers.Bridge.wood.L",
    "movement modifiers.Bridge.wood.R", "water objects.Shipwreck Survivor", "water objects.Dolphin School",
    "creature bank.Dragon City",
    "creature bank.Ivory Tower",
    "creature bank.Labyrinth",
    "creature bank.Magic Lamp",
    "creature bank.Pyramid",
    "creature bank.Red Dwarf Mines",
    "creature bank.Ruins",
    "creature bank.Temple",
    "creature bank.Faerie Ring",
    "creature bank.Derelict Ship",
    "creature bank.Pirate Islands",
    "creature bank.Shipwreck",
    "creature bank.Shipwreck 2",
    "movement modifiers.Winter Lodge",
    "movement modifiers.Brimstone Lodge",
    "movement modifiers.Desert Hut",
    "movement modifiers.Barbarian's Camp",
    "movement modifiers.Wet Log Inn",
    "movement modifiers.Oasis",
    "movement modifiers.Rally Flag",
    "Shroud Removers.Redwood Tree",
    "Shroud Removers.Spires of Ice",
    "Shroud Removers.Cartographer",
    "Mana Rechargers.Lake of the Scarlet Swan",
    "Mana Rechargers.Mana Vortex",
    "power ups.Fountain of Youth",
    "power ups.Sphinx",
    "miscellaneous.BlackSmith",
    "miscellaneous.Obelisks.Gold", "miscellaneous.Obelisks.Black", "miscellaneous.Obelisks.Purple",
    "miscellaneous.Obelisks.Orange", "miscellaneous.Obelisks.Brown", "miscellaneous.Obelisks.Silver",
    "miscellaneous.Obelisks.Bronze",
    "miscellaneous.tavern",
    "Scholars.Combat",
    "Scholars.Tactician",
    "Scholars.Sorcerer",
    "Scholars.Druid",
    "Scholars.Priest",
    "Scholars.Scholar",
    "Scholars.Scout",
    "Scholars.Noble",
    "Scholars.Lich",
    "artifacts.random artifact.Staff",
    "artifacts.random artifact.Tome",
    "moral-luck boosters.Oyster"};

// The game's class keywords in its class table order (a hero record's class number) and their
// factions.
struct ClassInfo {
  const char* keyword;
  int faction;
};
const ClassInfo kClasses[] = {{"knight", kLife},        {"priest", kLife},    {"lord", kOrder},
                              {"mage", kOrder},         {"death_knight", kDeath},
                              {"necromancer", kDeath},  {"thief", kChaos},    {"sorcerer", kChaos},
                              {"archer", kNature},      {"druid", kNature},   {"barbarian", kMight}};

struct Bits {
  const std::vector<uint8_t>& d;
  size_t p;
  bool ok = true;
  // n bits from p (LSB first), p moved past their bytes.
  std::vector<uint8_t> Take(size_t n) {
    std::vector<uint8_t> out(n);
    const size_t bytes = (n + 7) / 8;
    if (p + bytes > d.size()) {
      ok = false;
      return out;
    }
    for (size_t i = 0; i < n; ++i) out[i] = d[p + i / 8] >> (i % 8) & 1;
    p += bytes;
    return out;
  }
};

}  // namespace

// After the footprint reader of homm4-random-map-generator (tools/footprints.py; field layout
// per the h4r notes, decomp/adv_object.md).
bool ParseFootprint(const std::vector<uint8_t>& d, Footprint& fp) {
  auto u16 = [&](size_t p) { return p + 2 <= d.size() ? uint16_t(d[p] | d[p + 1] << 8) : uint16_t(0); };
  if (d.size() < 9) return false;
  size_t p = 0;
  const int version = u16(0);
  p += 3;  // version, fps
  int edge = 0;
  if (version >= 3) {
    edge = u16(p);
    p += 2;
  }
  const int w = u16(p), h = u16(p + 2);
  p += 4;
  if (w <= 0 || h <= 0 || w > 64 || h > 64) return false;
  Bits bits{d, p};
  fp.w = w;
  fp.h = h;
  fp.blocked = bits.Take(size_t(w) * h);
  fp.blocked_count = int(std::count(fp.blocked.begin(), fp.blocked.end(), 1));
  std::set<std::array<int, 2>> entry;
  auto blocked = [&](int x, int y) { return fp.blocked[size_t(y) * w + x] != 0; };
  auto add = [&](int ax, int ay) {
    const bool inside = ax >= 0 && ax < w && ay >= 0 && ay < h;
    if (!inside || !blocked(ax, ay)) entry.insert({ax, ay});
  };
  if (edge) {
    const auto left = bits.Take(size_t(w) * (h + 1));
    const auto right = bits.Take(size_t(w + 1) * h);
    for (int y = 0; y < h; ++y)
      for (int x = 0; x < w; ++x) {
        const bool b = blocked(x, y);
        const bool m[4] = {left[x + w * y] != 0, left[x + w * (y + 1)] != 0, right[y + h * x] != 0,
                           right[y + h * (x + 1)] != 0};
        const bool border[4] = {y == 0, y == h - 1, x == 0, x == w - 1};
        bool trigger = false;
        for (int i = 0; i < 4; ++i) trigger |= m[i] && (border[i] || b);
        if (!trigger) continue;
        const int across[4][2] = {{x, y - 1}, {x, y + 1}, {x - 1, y}, {x + 1, y}};
        for (int i = 0; i < 4; ++i)
          if (m[i]) add(across[i][0], across[i][1]);
      }
  } else {
    const auto old = bits.Take(size_t(w) * h);
    for (int c = 0; c < w * h; ++c)
      if (old[c]) {
        const int x = c % w, y = c / w;
        add(x, y - 1);
        add(x, y + 1);
        add(x - 1, y);
        add(x + 1, y);
      }
  }
  if (!bits.ok) return false;
  p = bits.p;
  if (version >= 6)
    p += (2 * size_t(w) * (h + 1) + 7) / 8 + (2 * size_t(w + 1) * h + 7) / 8 + size_t(w + 1) * (h + 1);
  if (version >= 2) {
    p += 2;
    for (int i = 0; i < 3; ++i) {
      if (p + 4 > d.size()) return false;
      fp.kind_ids[i] = u16(p);
      const uint16_t n = u16(p + 2);
      if (p + 4 + n > d.size()) return false;
      fp.kind[i].assign(reinterpret_cast<const char*>(&d[p + 4]), n);
      p += 4 + n;
    }
  }
  fp.entry.assign(entry.begin(), entry.end());  // sorted, as upstream's
  return true;
}

std::vector<std::pair<std::string, int>> ExePortraits(const std::vector<uint8_t>& d) {
  std::vector<std::pair<std::string, int>> out;
  auto u32 = [&](size_t p) -> uint32_t {
    return p + 4 <= d.size() ? d[p] | d[p + 1] << 8 | d[p + 2] << 16 | uint32_t(d[p + 3]) << 24 : 0;
  };
  auto u16 = [&](size_t p) -> uint32_t { return p + 2 <= d.size() ? d[p] | d[p + 1] << 8 : 0; };
  const uint32_t pe = u32(0x3C);
  if (d.size() < 0x40 || pe + 0x78 > d.size() || std::memcmp(&d[pe], "PE\0\0", 4) != 0) return out;
  const uint32_t nsec = u16(pe + 6), opt = u16(pe + 20), base = u32(pe + 52);
  struct Sec {
    uint32_t va, raw_size, raw;
  };
  std::vector<Sec> secs;
  for (uint32_t i = 0; i < nsec; ++i) {
    const size_t s = pe + 24 + opt + 40 * i;
    secs.push_back({u32(s + 12), u32(s + 16), u32(s + 20)});
  }
  auto file_to_va = [&](size_t o) -> uint32_t {
    for (const Sec& s : secs)
      if (o >= s.raw && o < s.raw + s.raw_size) return uint32_t(base + s.va + o - s.raw);
    return 0;
  };
  auto va_to_file = [&](uint32_t a) -> int64_t {
    for (const Sec& s : secs)
      if (a >= base + s.va && a < base + s.va + s.raw_size) return int64_t(a) - base - s.va + s.raw;
    return -1;
  };
  static const char kFirst[] = "\0aidan";  // the first portrait's keyword, after a terminator
  auto it = std::search(d.begin(), d.end(), kFirst, kFirst + 7);  // (with its own '\0')
  if (it == d.end()) return out;
  const uint32_t va = file_to_va(size_t(it - d.begin()) + 1);
  if (!va) return out;
  const uint8_t ptr[4] = {uint8_t(va), uint8_t(va >> 8), uint8_t(va >> 16), uint8_t(va >> 24)};
  auto at = std::search(d.begin(), d.end(), ptr, ptr + 4);
  for (size_t p = size_t(at - d.begin()); at != d.end() && p + 8 <= d.size(); p += 8) {
    const int64_t o = va_to_file(u32(p));
    const uint32_t faction = u32(p + 4);
    if (o < 0 || faction > 5) break;
    const auto end = std::find(d.begin() + o, d.end(), 0);
    out.emplace_back(std::string(d.begin() + o, end), int(faction));
  }
  return out;
}

bool Catalog::Load(const std::string& game_dir, std::string* error) {
  Resources res;
  if (!res.Open(game_dir + "/Data", error) && !res.Open(game_dir + "/data", error)) return false;
  std::vector<uint8_t> exe;
  for (const char* n : {"/heroes4.exe", "/Heroes4.exe", "/HEROES4.EXE"})
    if (ReadFile(game_dir + n, exe)) break;
  return Load(res, exe, error);
}

bool ReadFile(const std::string& path, std::vector<uint8_t>& out) {
  std::FILE* f = std::fopen(path.c_str(), "rb");
  if (!f) return false;
  std::fseek(f, 0, SEEK_END);
  out.resize(size_t(std::max(0L, std::ftell(f))));
  std::fseek(f, 0, SEEK_SET);
  const bool ok = std::fread(out.data(), 1, out.size(), f) == out.size();
  std::fclose(f);
  if (!ok) out.clear();
  return ok;
}

bool Catalog::Load(const Resources& res, const std::vector<uint8_t>& exe, std::string* error) {

  std::set<std::string> wanted;
  for (const char* m : kModels) wanted.insert(Lower(m));
  const std::string prefix = "adv_object.";
  for (const std::string& name : res.Names(prefix)) {
    std::string model = name.substr(prefix.size());
    if (model.size() > 4 && model.compare(model.size() - 4, 4, ".h4d") == 0) model.resize(model.size() - 4);
    bool obstacle = false;
    for (const auto& list : kObstaclePrefixes)
      for (const char* p : list) obstacle |= StartsWith(model, p);
    const char* decor = nullptr;
    for (const char* p : kDecorPrefixes)
      if (StartsWith(model, p) && (!decor || std::strlen(p) > std::strlen(decor))) decor = p;
    if (!obstacle && !decor && !wanted.count(model) && !StartsWith(model, kCaveWallPrefix)) continue;
    std::vector<uint8_t> d;
    Footprint fp;
    if (!res.Read(name, d, 2048) || !ParseFootprint(d, fp)) {
      if (!res.Read(name, d) || !ParseFootprint(d, fp)) continue;
    }
    std::string spelled = res.Spelling(name);
    fp.model = spelled.size() > prefix.size() ? spelled.substr(prefix.size(), model.size()) : model;
    auto placed = footprints_.emplace(model, std::move(fp));
    if (decor) decor_names_.push_back({decor, placed.first->first});
  }
  for (const auto& [group, model] : decor_names_) decor_[group].push_back(&footprints_.at(model));
  for (const char* m : kModels)
    if (!footprints_.count(Lower(m))) {
      if (error) *error = std::string("the game's data have no ") + m;
      return false;
    }
  for (const auto& [model, fp] : footprints_) {
    if (!StartsWith(model, kCaveWallPrefix)) continue;
    const int n = std::atoi(model.c_str() + std::strlen(kCaveWallPrefix));
    if (n >= 1 && n <= 12 && fp.w == 1 && fp.h == 1) cave_walls_[size_t(n)].push_back(&fp);
  }
  for (size_t t = 1; t < kObstaclePrefixes.size(); ++t) {
    auto& pool = obstacles_[t];
    for (const auto& [model, fp] : footprints_) {
      bool match = false;
      for (const char* p : kObstaclePrefixes[t]) match |= StartsWith(model, p);
      if (match && fp.kind[0] == "decorative" && fp.w <= 3 && fp.h <= 3 && fp.blocked_count == fp.w * fp.h)
        pool.push_back(&fp);
    }
    if (pool.empty()) {
      if (error) *error = "the game's data have no obstacles";
      return false;
    }
  }

  // Heroes: table.heroes (Keyword, Name, Sex, Class, Biography, Allowed By Default).
  std::vector<uint8_t> d;
  if (!res.Read("table.heroes.h4d", d)) {
    if (error) *error = "the game's data have no hero table";
    return false;
  }
  const auto portraits = ExePortraits(exe);
  const auto rows = ReadTable(d);
  if (rows.empty()) return false;
  const auto& head = rows[0];
  auto column = [&](const char* name) {
    for (size_t i = 0; i < head.size(); ++i)
      if (head[i] == name) return int(i);
    return -1;
  };
  const int c_key = column("Keyword"), c_name = column("Name"), c_sex = column("Sex"),
            c_class = column("Class"), c_bio = column("Biography"), c_default = column("Allowed By Default");
  for (size_t r = 1; r < rows.size(); ++r) {
    const auto& row = rows[r];
    auto cell = [&](int c) { return c >= 0 && size_t(c) < row.size() ? row[c] : std::string(); };
    if (row.size() < 5 || row[0].empty() || cell(c_default) != "X") continue;
    HeroChoice h;
    h.keyword = cell(c_key);
    h.name = cell(c_name);
    h.biography = cell(c_bio);
    const std::string cls = Lower(cell(c_class)), sex = Lower(cell(c_sex));
    for (size_t i = 0; i < std::size(kClasses); ++i)
      if (cls == kClasses[i].keyword) {
        h.hero_class = uint8_t(i);
        h.faction = kClasses[i].faction;
      }
    if (h.hero_class == 0xFF) continue;
    h.sex = sex == "female" ? 0 : sex == "male" ? 1 : 0xFF;
    const std::string key = Lower(h.keyword);
    for (size_t i = 0; i < portraits.size(); ++i)
      if (Lower(portraits[i].first) == key) {
        h.portrait = int16_t(i);
        h.faction = portraits[i].second;  // the portrait sheet's faction, as upstream's
        break;
      }
    heroes_[h.faction].push_back(std::move(h));
  }
  for (auto& list : heroes_)
    std::stable_sort(list.begin(), list.end(), [](const HeroChoice& a, const HeroChoice& b) { return a.name < b.name; });
  // Artifacts: a row's number its artifact's (the maps' obelisk rewards: 0x39 Winged Sandals, 0xAE
  // Ring of Strength), by level. Not the two whose number takes a spell after it (scrolls, staffs).
  if (res.Read("table.artifacts.h4d", d)) {
    const auto arts = ReadTable(d);
    int c_level = -1;
    if (arts.size() > 1)
      for (size_t i = 0; i < arts[1].size(); ++i)
        if (arts[1][i] == "Level") c_level = int(i);
    // the original game's list only: it ends at an empty row (then the expansion's, numbered
    // otherwise); not the Grail
    for (size_t r = 2; c_level >= 0 && r < arts.size() && !arts[r].empty() && !arts[r][0].empty(); ++r) {
      const uint16_t id = uint16_t(r - 2);
      if (id == 0x7C || id == 0xA6 || arts[r][0] == "grail" || size_t(c_level) >= arts[r].size()) continue;
      artifacts_[arts[r][size_t(c_level)]].push_back(id);
    }
  }
  return true;
}

const Footprint* Catalog::Find(std::string_view model) const {
  auto it = footprints_.find(Lower(model));
  return it == footprints_.end() ? nullptr : &it->second;
}

}  // namespace h4::random_map
