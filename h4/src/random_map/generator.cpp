// The generator of Tihomir Ivanov's homm4-random-map-generator (tools/generator.py), step by
// step: zones, walls, passages, towns and heroes, content, paths, guards, obstacles, tiles, the
// checks. Built with -ffp-contract=off (h4/CMakeLists.txt) and without the C library's
// sin/cos, std::*_distribution or unstable sorts, so that a seed gives the same map everywhere.
#include "generator.h"

#include <algorithm>
#include <array>
#include <cctype>
#include <cmath>
#include <cstring>
#include <map>
#include <queue>
#include <random>
#include <set>
#include <string_view>

#include "bodies.h"
#include "map_settings.h"
#include "objects.h"
#include "scenario.h"
#include "template.h"
#include "tiles.h"
#include "transitions.h"

namespace h4::random_map {
namespace {

constexpr double kPi = 3.14159265358979323846;

// sin with + - * / only (IEEE: the same result everywhere): reduced to [-pi/2, pi/2], then the
// Taylor series to x^21.
double Sin(double x) {
  x = std::fmod(x, 2 * kPi);  // (exact)
  if (x > kPi) x -= 2 * kPi;
  if (x < -kPi) x += 2 * kPi;
  if (x > kPi / 2) x = kPi - x;
  if (x < -kPi / 2) x = -kPi - x;
  const double x2 = x * x;
  double term = x, sum = x;
  for (int n = 1; n <= 10; ++n) {
    term = -term * x2 / double((2 * n) * (2 * n + 1));
    sum += term;
  }
  return sum;
}
double Cos(double x) { return Sin(x + kPi / 2); }

// Python's random.Random-like helpers on mt19937 (whose output the C++ standard fixes).
class Rng {
 public:
  explicit Rng(uint32_t seed) : e_(seed) {}
  uint32_t Bits() { return e_(); }
  double Real() {  // [0, 1), 53 bits
    const uint32_t a = e_() >> 5, b = e_() >> 6;
    return (a * 67108864.0 + b) * (1.0 / 9007199254740992.0);
  }
  double Uniform(double lo, double hi) { return lo + (hi - lo) * Real(); }
  uint32_t Below(uint32_t n) {  // [0, n), unbiased
    if (n <= 1) return 0;
    const uint32_t limit = uint32_t(-n) % n;
    for (;;) {
      const uint32_t r = e_();
      if (r >= limit) return r % n;
    }
  }
  template <typename T>
  const T& Choice(const std::vector<T>& v) { return v[Below(uint32_t(v.size()))]; }
  template <typename T>
  void Shuffle(std::vector<T>& v) {
    for (size_t i = v.size(); i > 1; --i) std::swap(v[i - 1], v[Below(uint32_t(i))]);
  }

 private:
  std::mt19937 e_;
};

struct Pt {
  double x, y;
};
double Dist(Pt a, Pt b) { return std::sqrt((a.x - b.x) * (a.x - b.x) + (a.y - b.y) * (a.y - b.y)); }

enum : uint8_t { FREE, WALL, BLOCK, OCC, RES };
bool Walkable(uint8_t s) { return s == FREE || s == OCC || s == RES; }

constexpr int kDir8[8][2] = {{-1, -1}, {-1, 0}, {-1, 1}, {0, -1}, {0, 1}, {1, -1}, {1, 0}, {1, 1}};
constexpr int kDir4[4][2] = {{-1, 0}, {1, 0}, {0, -1}, {0, 1}};

enum : uint8_t { WATER = 0, GRASS, ROUGH, SWAMP, VOLCANIC, SNOW, SAND, DIRT, SUBTERRANEAN, RIVER, LAVA_RIVER, ICE_RIVER };
const std::vector<uint8_t> kLand = {GRASS, ROUGH, SWAMP, VOLCANIC, SNOW, SAND, DIRT, SUBTERRANEAN};
const std::vector<uint8_t> kStartTerrains = {GRASS, DIRT, ROUGH, SNOW, SAND, SWAMP, VOLCANIC};
constexpr uint8_t kRoadDirt = 2, kRoadCobble = 3;

using Models = std::vector<std::string_view>;
const char* const kMonster[5] = {"", "Random creatures.RandomLv1", "Random creatures.RandomLv2",
                                 "Random creatures.RandomLv3", "Random creatures.RandomLv4"};
const std::vector<std::array<const char*, 2>> kRareMines = {
    {"mine.Crystal", "mine.Crystal R"}, {"mine.Gem", "mine.Gem R"},
    {"mine.Sulfur", "mine.Sulfur R"}, {"mine.Alchemists Lab", "mine.Alchemists Lab R"}};
const std::array<const char*, 2> kSawmill = {"mine.Sawmill", "mine.Sawmill R"};
const std::array<const char*, 2> kOrePit = {"mine.ore pit", "mine.Ore Pit R"};
const std::vector<std::string_view> kGoldMine = {"mine.Gold", "mine.Gold R"};
constexpr const char* kResource = "Resources.Random Resource";
constexpr const char* kChest = "Resources.Treasure Chest";
constexpr const char* kCampfire = "Resources.Campfire";
const Models kFountains = {"power ups.Fountain of Fortune", "power ups.Fountain of Strength",
                           "power ups.Fountain of Vigor", "power ups.Spring of Speed",
                           "power ups.Pool of Power", "Mana Rechargers.magic well",
                           "movement modifiers.Water Trough", "moral-luck boosters.Adventurers Totem"};
const Models kGems = {"power ups.Ruby", "power ups.Emerald", "power ups.Opal", "power ups.Sapphire"};
const Models kSchools = {"Skill Enhancer.Library", "Skill Enhancer.Witch's Hut",
                         "Skill Enhancer.School of Magic", "Skill Enhancer.School of War",
                         "Skill Enhancer.Veteran's_Guild", "Skill Enhancer.beastmaster's hut",
                         "Skill Enhancer.Magic University", "Skill Enhancer.War College"};
const Models kBanks = {"creature bank.Bandit Camp", "creature bank.Keep", "creature bank.Lair",
                       "creature bank.Imp Cache", "creature bank.Graveyard", "creature bank.Crypt",
                       "creature bank.Harpy Nest", "creature bank.Forest Glenn",
                       "creature bank.Griffin Nest", "creature bank.Ruins", "creature bank.Temple",
                       "creature bank.Red Dwarf Mines", "creature bank.Faerie Ring", "creature bank.Labyrinth",
                       "creature bank.Ivory Tower"};
// the richest zones' too
const Models kRichBanks = {"creature bank.Dragon City", "creature bank.Pyramid", "creature bank.Keep",
                           "creature bank.Labyrinth", "creature bank.Temple"};
// more objects. Unguarded: small finds and boosts; guarded: the better ones.
const Models kPiles = {"Resources.Gold", "Resources.Gold", "Resources.Wood", "Resources.Ore", "Resources.Gem",
                       "Resources.Crystal", "Resources.Sulfur", "Resources.Mercury", "Resources.Random Resource"};
const Models kTreasures = {"Resources.Treasure Chest", "Resources.Treasure Chest", "Resources.Treasure Chest",
                           "Resources.Traveler's backpack", "Resources.Mage's Chest"};
const Models kSmall = {"Resources.Campfire", "Resources.Skeleton", "Resources.Medicine Wagon",
                       "movement modifiers.Water Trough", "movement modifiers.Stables",
                       "Mana Rechargers.magic well", "Mana Rechargers.magic spring",
                       "Mana Rechargers.Magic Fountain", "Mana Rechargers.Magician's Pool",
                       "moral-luck boosters.Faerie Ring", "moral-luck boosters.Clover Field",
                       "moral-luck boosters.Rainbow", "moral-luck boosters.luck Stone",
                       "moral-luck boosters.Adventurers Totem", "mine.Weekly.windmill",
                       "artifacts.random artifact.Potion", "miscellaneous.Sanctuary", "Shroud Removers.Tower",
                       "power ups.Fountain of Fortune", "power ups.Fountain of Strength",
                       "power ups.Fountain of Vigor", "power ups.Spring of Speed", "power ups.Pool of Power",
                       "power ups.Ruby", "power ups.Emerald", "power ups.Opal", "power ups.Sapphire",
                       "moral-luck boosters.Temple of Light", "moral-luck boosters.Temple of Order",
                       "moral-luck boosters.Temple of Chaos", "moral-luck boosters.Temple of Darkness",
                       "power ups.Fountain of Youth", "movement modifiers.Rally Flag",
                       "Mana Rechargers.Lake of the Scarlet Swan", "Mana Rechargers.Mana Vortex",
                       "miscellaneous.BlackSmith", "miscellaneous.tavern"};
const Models kGuarded = {"power ups.Tree of Knowledge", "Power Ups.Learning Stone", "power ups.Training Grounds",
                         "power ups.Mercenary Camp", "power ups.arena_might", "power ups.arena_magic",
                         "power ups.Arena", "power ups.Sacred Fountain", "miscellaneous.Trading Post",
                         "mine.Weekly.Leprechaun", "mine.Weekly.Imp Pit", "mine.Weekly.Apprentice's Lab",
                         "mine.Weekly.Water Wheel", "mine.Weekly.Crystal Pool", "mine.Weekly.Miner's Guilds",
                         "Scholars.Random Scholar", "shrines.Random 1", "shrines.Random 2", "power ups.Sphinx",
                         "Shroud Removers.Cartographer", "Scholars.Combat", "Scholars.Tactician",
                         "Scholars.Sorcerer", "Scholars.Druid", "Scholars.Priest", "Scholars.Scholar",
                         "Scholars.Scout", "Scholars.Noble", "Scholars.Lich", "artifacts.random artifact.Staff",
                         "creature bank.Ruins", "creature bank.Red Dwarf Mines", "creature bank.Faerie Ring"};
// Of a land: by a zone's terrain (index: the terrain number), a find or two each.
const std::vector<Models> kThemed = {
    {},
    {"Shroud Removers.Redwood Tree", "creature bank.Forest Glenn"},                  // grass
    {"movement modifiers.Barbarian's Camp"},                                          // rough
    {"movement modifiers.Wet Log Inn"},                                               // swamp
    {"movement modifiers.Brimstone Lodge"},                                           // volcanic
    {"movement modifiers.Winter Lodge", "Shroud Removers.Spires of Ice"},             // snow
    {"movement modifiers.Desert Hut", "movement modifiers.Oasis", "creature bank.Pyramid"},  // sand
    {"movement modifiers.Barbarian's Camp", "Shroud Removers.Redwood Tree"},          // dirt
    {"creature bank.Red Dwarf Mines", "creature bank.Imp Cache"},                     // subterranean
};

const char* const kDwelling[5] = {"", "creature generators.Random Creature 2x1",
                                  "creature generators.Random Creature 2x2",
                                  "creature generators.Random Creature 2x3",
                                  "creature generators.Random Creature 3x3"};
constexpr const char* kArtifactMinor = "artifacts.random artifact.Minor";
constexpr const char* kArtifactMajor = "artifacts.random artifact.Major";
constexpr const char* kArtifactTreasure = "artifacts.random artifact.Treasure";
constexpr const char* kArtifactItem = "artifacts.random artifact.Item";
// Town models (left, right) with a fort, as the shipped maps' start towns.
const char* const kFactionTown[kFactionCount][2] = {
    {"castle.Haven.Fort", "castle.Haven.Fort R"},         {"castle.Academy.Fort", "castle.Academy.Fort R"},
    {"castle.Necropolis.Fort", "castle.Necropolis.fort r"}, {"castle.Asylum.Fort", "castle.Asylum.Fort R"},
    {"castle.Preserve.fort", "castle.Preserve.fort r"},   {"castle.Stronghold.fort", "castle.Stronghold.fort r"}};
const char* const kRandomTown[2] = {"castle.Random Town", "castle.Random Town R"};

// The underground: its caves' terrains (mostly subterranean), the passages between the levels
// (the one going down on the surface, up underground, on the same cells).
const std::vector<uint8_t> kCaveTerrains = {SUBTERRANEAN, SUBTERRANEAN, SUBTERRANEAN, DIRT, ROUGH, VOLCANIC};
constexpr const char* kTunnelDown = "movement modifiers.Tunnel_Down";
constexpr const char* kTunnelUp = "movement modifiers.Tunnel_Up";
// A cave's edge (a rock cell next to open ground) has a wall object, internal.Walls.Cavern.<n>,
// whose n says which neighbours are rock. By the 8 neighbours (bit i: kWallDirs[i] is rock or off
// the map), as the shipped maps have them (the most common n for each pattern seen in them; the
// others: the nearest pattern seen).
constexpr int kWallDirs[8][2] = {{-1, -1}, {-1, 0}, {-1, 1}, {0, 1}, {1, 1}, {1, 0}, {1, -1}, {0, -1}};
constexpr uint8_t kCaveWall[256] = {
    1,  1,  3,  3,  1,  1,  3,  1,  3,  3,  3,  1,  1,  1, 12, 12,
    3,  1,  3,  3,  1,  1,  1,  3,  3,  1,  1,  1,  1,  3, 12, 12,
    1,  1,  1,  3,  1,  1,  1,  1,  3,  3,  1,  1,  3, 11,  3,  1,
    3,  3,  1,  3,  1,  1,  3,  3, 11, 11,  1,  3, 11, 11,  7,  7,
    1,  1,  1,  3,  1,  1,  3,  3,  3,  3,  3, 12,  1,  1, 12, 12,
    1,  1,  3,  3,  1,  3,  1,  3,  1,  3,  3,  1,  3,  3, 12, 12,
    1,  3,  1,  3,  1,  3,  3,  1,  1, 11,  1,  3,  3, 11,  1,  3,
    3,  3,  1,  3,  3,  3,  3,  3, 11, 11,  1,  3, 11, 11,  7,  7,
    1,  1,  3,  9,  1,  1,  3,  9,  3,  3,  1,  1,  3,  1,  1,  1,
    1,  1,  3,  9,  1,  1,  9,  9,  3,  1,  1,  1,  3,  3,  1,  1,
    3,  1,  3,  3,  3,  3,  1,  1,  1,  3,  1,  1,  1,  1,  1,  1,
    1,  3,  3,  1,  1,  3,  3,  1,  3,  3,  5,  1,  1,  1,  7,  8,
    1,  1,  9,  9,  1,  3,  1,  9,  3,  3,  3,  1,  3,  1,  1,  1,
    3,  3,  1,  9,  3,  3,  1,  9,  1,  3,  1,  1,  1,  1,  3,  1,
   10, 10,  3,  3, 10, 10,  3,  3,  1,  3,  1,  3,  1,  1,  1,  2,
   10, 10,  3,  3, 10, 10,  3,  3,  5,  5,  5,  4,  5,  5,  6,  3};

// The newest map editor's type numbers (the game finds a type by its name; the editor's
// versions numbered them differently), for the types the generator writes.
// A forest's obstacle: trees or shrubs (the models' names, any case).
bool TreeLike(std::string_view model) {
  auto starts = [&](std::string_view p) {
    if (model.size() < p.size()) return false;
    for (size_t i = 0; i < p.size(); ++i)
      if (std::tolower(static_cast<unsigned char>(model[i])) != p[i]) return false;
    return true;
  };
  return starts("trees.") || starts("shrubs.");
}

uint16_t TypeId(std::string_view type, uint16_t fallback) {
  static const std::map<std::string_view, uint16_t> kIds = {
      {"campfire", 12},       {"creature_bank", 20},    {"decorative", 22},
      {"fountain", 31},       {"idol_of_fortune", 35},  {"magic_gem", 39},
      {"mana_recharger", 40}, {"mine", 44},             {"movement_booster", 45},
      {"random_artifact", 60}, {"random_creature_dwelling", 61}, {"random_material_pile", 63},
      {"random_monster", 64}, {"random_shrine", 67},    {"random_teacher", 69},
      {"random_temple", 70},  {"random_town", 72},      {"school", 76},
      {"town", 95},           {"treasure", 98},         {"subterranean_gate", 86},
      {"system_object", 87},  {"academy", 2},           {"arena", 3},
      {"blattner_stone", 7},  {"bridge", 10},           {"clover_field", 16},
      {"coliseum", 17},       {"corpse", 19},           {"faerie_ring", 28},
      {"material_pile", 41},  {"medicine_wagon", 42},   {"mercenary_camp", 43},
      {"rainbow", 59},        {"random_potion", 65},    {"sacred_grove", 74},
      {"sanctuary", 75},      {"temple", 92},           {"tower", 94},
      {"trading_post", 96},   {"training_grounds", 97}, {"tree_of_knowledge", 99},
      {"weekly_material_generator", 102}, {"windmill", 103}, {"shipyard", 81},
      {"flotsam", 30},        {"buoy", 11},             {"whirlpool", 104},
      {"lighthouse", 38},     {"shipwreck_survivor", 80}, {"dolphin_school", 24},
      {"pathfinder", 54},     {"sphinx", 84},           {"cartographer", 14},
      {"blacksmith", 6},      {"tavern", 88},           {"teacher", 89},
      {"random_staff", 68},   {"oyster", 51},           {"obelisk", 47}};
  auto it = kIds.find(type);
  return it == kIds.end() ? fallback : it->second;
}

struct Object {
  int r = 0, c = 0, lv = 0;
  bool army = false, removed = false;
  std::string model;
  std::array<std::pair<uint16_t, std::string>, 3> kind;
  std::vector<uint8_t> body;
};

struct Failure {
  std::string what;
};

struct Link {
  std::array<int, 2> zones;
  std::array<int, 2> ends;  // the corridor's end in zones[0], zones[1]
  std::vector<int> corridor;
  int guard;
  int strength = -1;  // the guard's level (a template's link); -1: by the zones; 0: none
};

struct Item {
  std::vector<int> entries;
  int guard;
  size_t object;
};

class Generator {
 public:
  Generator(const Catalog& cat, const MapSetup& setup, uint32_t seed, const std::atomic<bool>* cancel,
            const Template* tpl = nullptr)
      : cat_(cat), setup_(setup), seed_(seed), size_(setup.size), players_(int(setup.players.size())),
        levels_(setup.underground ? 2 : 1), area_(setup.size * setup.size), rng_(seed), cancel_(cancel), tpl_(tpl) {
    const int s = size_;
    first_.resize(s);
    end_.resize(s);
    for (int r = 0; r < s; ++r) {
      const int d = r < s / 2 ? r : s - 1 - r;
      first_[r] = s / 2 - 1 - d;
      end_[r] = s / 2 + 1 + d;
    }
    // A cell: level * size * size + row * size + column (the surface's are the classic numbers).
    for (int lv = 0; lv < levels_; ++lv)
      for (int r = 0; r < s; ++r)
        for (int c = first_[r]; c < end_[r]; ++c) cells_.push_back(lv * area_ + r * s + c);
    zone_.assign(size_t(area_) * levels_, -1);
    state_.assign(size_t(area_) * levels_, FREE);
    rock_.assign(size_t(area_) * levels_, 0);
    ground_.assign(size_t(area_) * levels_, kZoneGround);
  }

  std::vector<std::string> log;
  std::vector<Object> objects;
  int zones() const { return int(centres_.size()); }

  // Throws Failure; returns the problems of the checks (none: the map is good).
  std::vector<std::string> Generate(std::vector<uint8_t>& file, const std::string& title,
                                    const std::string& description) {
    MakeZones();
    Step();
    MakeWalls();
    if (islands_) MakeSea();
    if (water_ == 1 && (size_ > 76 || rng_.Real() < 0.75)) {
      MakeCoast();  // normal water: a sea, as a rule
      if (tpl_) KeepLargestPieces();  // (a template's zones by the shore: no piece cut off)
    }
    MakeLinks();
    Step();
    FillZones();
    MakeMainPaths();
    if (water_ == 1) MakeLakes();  // before the content: room for them, the content round them
    Step();
    FillContents();
    MarkTownViews();
    Step();
    PlaceLinkGuards();
    MakePaths();
    Step();
    if (water_ == 1) {
      MakeRivers();
      Step();
    }
    FillWalls();
    Step();
    if (islands_ || (coast_ && !shipyards_.empty())) FillSea();
    FillForests();
    FillInterior(islands_ ? 0.12 : 0.08);
    FillRuins();
    FillDecor();
    FillWaterDecor();
    Step();
    std::vector<Tile> tiles = BuildGrid();
    std::vector<std::string> problems = Check();
    if (!problems.empty()) return problems;

    // every player may be a human's (the game's new game window sets who is, as on its own
    // multiplayer maps); the setup's humans and computers are in the description (map_settings)
    std::vector<PlayerSlot> slots;
    for (const PlayerSetup& p : setup_.players)
      slots.push_back({true, uint8_t(p.faction >= 0 ? 1 << p.faction : 0x3F)});
    ScenarioRules rules;
    rules.size = uint16_t(size_);
    rules.levels = uint8_t(levels_);
    rules.terrain_seed = rng_.Bits();
    rules.players = slots;
    rules.title = title;
    rules.description = description;
    rules.obelisk_artifacts = obelisk_rewards_;
    std::vector<int> numbers;  // the teams in their first player's order
    for (size_t i = 0; i < setup_.players.size(); ++i) {
      const int t = TeamOf(setup_.players, i);
      size_t k = 0;
      while (k < numbers.size() && numbers[k] != t) ++k;
      if (k == numbers.size()) {
        numbers.push_back(t);
        rules.teams.push_back(0);
      }
      rules.teams[k] |= uint8_t(1u << i);
    }
    std::vector<MapObject> records;
    for (const Object& o : objects) {
      if (o.removed) continue;
      MapObject m;
      m.row = o.r;
      m.column = o.c;
      m.level = o.lv;
      m.army = o.army;
      m.model = o.model;
      for (int i = 0; i < 3; ++i) m.kind[i] = {o.kind[i].first, o.kind[i].second};
      m.body = o.body;
      records.push_back(std::move(m));
    }
    std::vector<uint8_t> heights(HeightCount(), 0);
    if (!PackGeneratedScenario(rules, records, tiles, heights, file)) throw Failure{"the map could not be written"};
    written_ = int(records.size());
    return {};
  }
  int written() const { return written_; }

 private:
  const Catalog& cat_;
  const MapSetup& setup_;
  // "random" settings chosen by the seed (the map's own, not the retries'): the same on an edit
  static int ByChoice(int value, uint32_t seed, uint32_t salt) {
    if (value != 3) return value;
    uint32_t h = seed * 2654435761u ^ salt;
    h ^= h >> 15;
    h *= 2246822519u;
    h ^= h >> 13;
    return int(h % 3);
  }
  const int monsters_ = ByChoice(setup_.monsters, setup_.seed, 0x6D6F6E);
  const int water_ = ByChoice(setup_.water, setup_.seed, 0x776174);
  const bool islands_ = water_ == 2;
  uint32_t seed_;
  int size_, players_;
  int levels_, area_;  // 1 or 2 (the underground); the cells of a level (size * size)
  Rng rng_;
  const std::atomic<bool>* cancel_;
  const Template* tpl_;           // none: the classic layout
  std::vector<double> power_;     // a template's zones' power diagram offsets (Power)
  std::vector<bool> town_;        // a template's zone with a neutral town
  std::vector<int> tpl_tier_;     // a template's zone's riches
  std::vector<int> first_, end_;  // each row's columns [first, end)
  std::vector<int> cells_;        // the diamond, row by row
  std::vector<int> zone_;         // -1 outside the diamond
  std::vector<uint8_t> state_;
  std::vector<uint8_t> rock_;     // the underground's solid rock (a tile's rock byte)
  // A cell's own terrain (water, a river) instead of its zone's; kZoneGround: the zone's.
  static constexpr uint8_t kZoneGround = 0xFF;
  std::vector<uint8_t> ground_;
  // The passages between the levels: from an entrance cell to the other end's (guarded: past the
  // guard on it).
  struct Jump {
    int to;
    bool guarded;
  };
  std::map<int, std::vector<Jump>> jumps_;
  std::vector<Pt> centres_;
  struct Roamer {
    int cell, zone, level;
  };
  std::vector<Roamer> roamers_;
  std::vector<std::vector<int>> lakes_;
  std::vector<std::pair<int, std::vector<int>>> road_paths_;  // more roads: (zone, path)
  struct Islet {
    int zone, centre;
  };
  std::vector<Islet> islets_;
  std::vector<int> shipyards_;  // their objects
  std::vector<uint8_t> decor_;  // cells under a passable decoration
  std::vector<uint8_t> view_;   // in front of a town (MarkTownViews): no trees there
  std::vector<uint8_t> path_side_;  // along the paths (MarkPathSides): no forest, no obstacle
  std::array<std::vector<uint16_t>, 12> obelisk_rewards_;  // by the obelisks' colour
  std::vector<uint8_t> sea_zone_;   // islands: a zone of the surface all sea, with islets
  std::set<int> sea_links_;         // normal water: zones linked by sea (a shipyard each)
  std::vector<uint8_t> shore_;      // land next to the sea: no obstacles (the way to the boats)
  bool coast_ = false;          // normal water with a sea
  std::set<int> sea_used_;  // monsters on their own, placed with the guards
  std::vector<bool> start_;       // a start zone
  std::vector<int> zone_level_;   // each zone's level
  std::vector<uint8_t> terrain_;
  std::vector<int> tier_;
  std::vector<Link> links_;
  std::vector<std::vector<int>> zone_cells_;
  std::vector<int> hubs_;
  std::vector<std::vector<Item>> items_;
  std::vector<int> town_gates_;
  std::vector<double> bias_;      // path cost factor per cell (0: none)
  std::vector<std::pair<int, std::vector<int>>> main_paths_;
  std::set<int> guards_;
  struct Pending {
    int cell, level;
    std::vector<int> path;
  };
  std::vector<Pending> pending_guards_;
  std::vector<std::pair<int, uint8_t>> undo_;
  int written_ = 0;

  void Step() {
    if (cancel_ && cancel_->load()) throw Failure{"cancelled"};
  }

  int R(int cell) const { return cell % area_ / size_; }
  int C(int cell) const { return cell % size_; }
  int L(int cell) const { return cell / area_; }
  bool Inside(int r, int c) const { return r >= 0 && r < size_ && c >= first_[r] && c < end_[r]; }
  // The cell at (r, c) of level lv, -1 outside the diamond.
  int At(int r, int c, int lv) const { return Inside(r, c) ? lv * area_ + r * size_ + c : -1; }
  // The cell (dr, dc) from `cell` on its level, -1 outside; its zone (-1 outside).
  int Nb(int cell, int dr, int dc) const { return At(R(cell) + dr, C(cell) + dc, L(cell)); }
  int ZoneNb(int cell, int dr, int dc) const {
    const int nb = Nb(cell, dr, dc);
    return nb < 0 ? -1 : zone_[nb];
  }
  int ZoneLevel(int z) const { return zone_level_[size_t(z)]; }
  double CellDist(int a, int b) const {
    const double dr = R(a) - R(b), dc = C(a) - C(b);
    return std::sqrt(dr * dr + dc * dc);
  }
  Pt Screen(int cell) const {
    return {double(C(cell) - R(cell)) / size_, double(C(cell) + R(cell) - size_ + 1) / size_};
  }
  // A screen point -> (row, column), not necessarily on the map.
  std::array<int, 2> FromScreen(Pt p) const {
    const double cr = p.x * size_, cs = p.y * size_ + size_ - 1;
    return {int(std::nearbyint((cs - cr) / 2)), int(std::nearbyint((cs + cr) / 2))};
  }
  size_t HeightCount() const {
    size_t n = 0;
    int previous = 0;
    for (int r = 0; r < size_; ++r) {
      const int w = end_[r] - first_[r];
      n += size_t(std::max(w, previous)) + 1;
      previous = w;
    }
    return (n + size_t(previous) + 1) * size_t(levels_);
  }

  const Footprint& Foot(std::string_view model) const {
    const Footprint* fp = cat_.Find(model);
    if (!fp) throw Failure{"no footprint for " + std::string(model)};
    return *fp;
  }

  Object MakeObject(std::string_view model, int r, int c, int lv) const {
    const Footprint& fp = Foot(model);
    Object o;
    o.r = r;
    o.c = c;
    o.lv = lv;
    o.model = std::string(model);
    for (int i = 0; i < 3; ++i) o.kind[i] = {fp.kind_ids[i], fp.kind[i]};
    o.kind[0].first = TypeId(fp.kind[0], fp.kind_ids[0]);
    o.body = ObjectBody(fp.kind[0], fp.kind[1]);
    if (o.body.empty()) throw Failure{"no body for " + std::string(model)};
    return o;
  }

  // A random monster of a level (1-4). Its army's strength: the game's own (normal), or, weak or
  // strong, set in the body's last two i32 (heroes4.exe: t_random_monster, 0x7f3710 reads them,
  // 0x7f3910 builds the army): its least and most, in peasants (their experience, 4). The game's
  // own is 0.8 to 1.2 times 155 x 2, 310 x 2.5, 620 x 3, 1240 x 4 (0x7f4530), the creatures'
  // experience; the number of creatures: about 2/3 of it over the creature's experience.
  Object MakeMonster(int level, int cell) const {
    Object o = MakeObject(kMonster[level], R(cell), C(cell), L(cell));
    if (monsters_ == 1) return o;
    constexpr double kOwn[5] = {0, 155 * 2.0, 310 * 2.5, 620 * 3.0, 1240 * 4.0};
    constexpr double kPeasant = 4;
    const double scale = monsters_ == 0 ? 0.6 : 1.5;
    const auto low = uint32_t(std::lround(kOwn[level] * 0.8 * scale / kPeasant));
    const auto high = uint32_t(std::lround(kOwn[level] * 1.2 * scale / kPeasant));
    if (o.body.size() < 8) throw Failure{"a random monster's body"};
    uint8_t* tail = o.body.data() + o.body.size() - 8;
    for (int i = 0; i < 4; ++i) {
      tail[i] = uint8_t(low >> (8 * i));
      tail[4 + i] = uint8_t(high >> (8 * i));
    }
    return o;
  }

  Object MakeTown(int faction, int variant, int owner, int r, int c) const {
    Object o;
    o.r = r;
    o.c = c;
    o.model = faction >= 0 ? kFactionTown[faction][variant] : kRandomTown[variant];
    const char* side = variant ? "right" : "left";
    if (faction >= 0)
      o.kind = {{{TypeId("town", 95), "town"}, {uint16_t(faction), kFactionNames[faction]}, {uint16_t(variant), side}}};
    else
      o.kind = {{{TypeId("random_town", 72), "random_town"}, {uint16_t(variant), side}, {0, ""}}};
    o.body = TownBody(faction, uint8_t(owner));
    return o;
  }

  // -- 1. zones

  // A template's zones: where it puts them (turned round the middle by the seed), the others laid
  // out by their links (linked zones pulled together, all pushed apart).
  void TemplateZones() {
    const Template& t = *tpl_;
    const int n = int(t.zones.size());
    const double turn = rng_.Real();
    const bool mirror = rng_.Below(2);
    std::vector<Pt> p(static_cast<size_t>(n));
    std::vector<bool> fixed(static_cast<size_t>(n));
    int starts = 0;
    for (const TemplateZone& z : t.zones) starts += z.start;
    int start_no = 0;
    for (int i = 0; i < n; ++i) {
      const TemplateZone& z = t.zones[size_t(i)];
      if (z.polar) {
        const double a = 2 * kPi * (z.turn + turn);
        p[size_t(i)] = {z.r * Cos(a), z.r * Sin(a)};
        fixed[size_t(i)] = true;
      } else if (z.placed) {  // turned by a quarter, mirrored: the map stays a square
        double x = mirror ? -z.x : z.x, y = z.y;
        for (int q = int(turn * 4); q-- > 0;) {
          const double nx = -y;
          y = x;
          x = nx;
        }
        p[size_t(i)] = {x, y};
        fixed[size_t(i)] = true;
      } else if (z.start) {  // round the map, in their order
        const double a = 2 * kPi * (turn + double(start_no) / starts);
        p[size_t(i)] = {0.34 * Cos(a), 0.34 * Sin(a)};
      }
      if (z.start) ++start_no;
    }
    // the rest first between the zones they link to (inwards: towards the middle), so the layout
    // starts near its shape
    std::vector<bool> set(static_cast<size_t>(n));
    for (int i = 0; i < n; ++i) set[size_t(i)] = fixed[size_t(i)] || t.zones[size_t(i)].start;
    for (bool more = true; more;) {
      more = false;
      std::vector<bool> now = set;
      for (int i = 0; i < n; ++i) {
        if (set[size_t(i)]) continue;
        Pt sum{0, 0};
        int k = 0;
        for (const TemplateLink& l : t.links) {
          const int o = l.a == i ? l.b : l.b == i ? l.a : -1;
          if (o >= 0 && set[size_t(o)]) sum.x += p[size_t(o)].x, sum.y += p[size_t(o)].y, ++k;
        }
        if (!k) continue;
        p[size_t(i)] = {0.7 * sum.x / k + rng_.Uniform(-0.03, 0.03), 0.7 * sum.y / k + rng_.Uniform(-0.03, 0.03)};
        now[size_t(i)] = more = true;
      }
      set = now;
    }
    for (int i = 0; i < n; ++i)  // (none linked to a start: anywhere)
      if (!set[size_t(i)]) p[size_t(i)] = {rng_.Uniform(-0.25, 0.25), rng_.Uniform(-0.25, 0.25)};
    const double spacing = 0.8 * std::sqrt(0.64 / n);
    std::set<std::pair<int, int>> linked;
    for (const TemplateLink& l : t.links) linked.insert({std::min(l.a, l.b), std::max(l.a, l.b)});
    for (int it = 0; it < 800; ++it) {
      const double step = 0.02 * (1 - it / 800.0) + 0.002;
      std::vector<Pt> force(static_cast<size_t>(n), Pt{0, 0});
      for (int i = 0; i < n; ++i)
        for (int j = 0; j < n; ++j) {
          if (i == j) continue;
          double dx = p[size_t(i)].x - p[size_t(j)].x, dy = p[size_t(i)].y - p[size_t(j)].y;
          double d = std::sqrt(dx * dx + dy * dy);
          if (d < 1e-6) dx = 1e-3 * (i - j), dy = 0, d = 1e-3;
          // linked ones may come closer (to border each other), the others are kept apart
          const double want = spacing * (std::sqrt(t.zones[size_t(i)].size) + std::sqrt(t.zones[size_t(j)].size)) / 2 *
                              (linked.count({std::min(i, j), std::max(i, j)}) ? 0.7 : 1.15);
          if (d < want) {  // pushed apart when closer than their sizes ask
            force[size_t(i)].x += (want - d) / d * dx;
            force[size_t(i)].y += (want - d) / d * dy;
          }
        }
      for (const TemplateLink& l : t.links) {  // pulled together when farther
        const int a = l.a, b = l.b;
        const double dx = p[size_t(b)].x - p[size_t(a)].x, dy = p[size_t(b)].y - p[size_t(a)].y;
        const double d = std::sqrt(dx * dx + dy * dy);
        const double want =
            0.85 * spacing * (std::sqrt(t.zones[size_t(a)].size) + std::sqrt(t.zones[size_t(b)].size)) / 2;
        if (d <= want) continue;
        const double k = (d - want) / d * 0.5;
        force[size_t(a)].x += k * dx, force[size_t(a)].y += k * dy;
        force[size_t(b)].x -= k * dx, force[size_t(b)].y -= k * dy;
      }
      for (int i = 0; i < n; ++i) {
        if (fixed[size_t(i)]) continue;
        const double fx = force[size_t(i)].x, fy = force[size_t(i)].y, f = std::sqrt(fx * fx + fy * fy);
        if (f > 1e-9) {
          const double m = std::min(f, step) / f;
          p[size_t(i)].x = std::clamp(p[size_t(i)].x + fx * m, -0.4, 0.4);
          p[size_t(i)].y = std::clamp(p[size_t(i)].y + fy * m, -0.4, 0.4);
        }
      }
    }
    // The power diagram's offsets. Zones where the template puts them: a size w zone about w times
    // a size 1 one's area. Zones laid out here: each one's share of the map as its size asks (a few
    // rounds of measuring the shares on a coarse grid and correcting them).
    power_.assign(static_cast<size_t>(n), 0.0);
    double total = 0;
    for (const TemplateZone& z : t.zones) total += z.size;
    const bool all_fixed = std::all_of(fixed.begin(), fixed.end(), [](bool f) { return f; });
    for (int i = 0; i < n && all_fixed; ++i) power_[size_t(i)] = (t.zones[size_t(i)].size - 1) * 0.64 / (kPi * n);
    constexpr int kGrid = 48;
    for (int round = 0; round < (all_fixed ? 0 : 40); ++round) {
      std::vector<int> count(static_cast<size_t>(n));
      std::vector<Pt> mass(static_cast<size_t>(n), Pt{0, 0});
      for (int gy = 0; gy < kGrid; ++gy)
        for (int gx = 0; gx < kGrid; ++gx) {
          const double x = (gx + 0.5) / kGrid - 0.5, y = (gy + 0.5) / kGrid - 0.5;
          int best = 0;
          double best_d = 1e18;
          for (int i = 0; i < n; ++i) {
            const double dx = x - p[size_t(i)].x, dy = y - p[size_t(i)].y, d = dx * dx + dy * dy - power_[size_t(i)];
            if (d < best_d) best_d = d, best = i;
          }
          ++count[size_t(best)];
          mass[size_t(best)].x += x, mass[size_t(best)].y += y;
        }
      for (int i = 0; i < n; ++i) {
        const double share = double(count[size_t(i)]) / (kGrid * kGrid), want = t.zones[size_t(i)].size / total;
        power_[size_t(i)] += 0.25 * (want - share);
        if (count[size_t(i)]) {  // and towards its area's middle (where its neighbours are its links')
          p[size_t(i)].x = 0.8 * p[size_t(i)].x + 0.2 * mass[size_t(i)].x / count[size_t(i)];
          p[size_t(i)].y = 0.8 * p[size_t(i)].y + 0.2 * mass[size_t(i)].y / count[size_t(i)];
        }
      }
    }
    for (int i = 0; i < n; ++i) {
      const TemplateZone& z = t.zones[size_t(i)];
      centres_.push_back(p[size_t(i)]);
      start_.push_back(z.start);
      zone_level_.push_back(0);
      town_.push_back(z.town && !z.start);
      tpl_tier_.push_back(z.start ? 0 : std::clamp(z.tier, 1, 2));
    }
  }

  void MakeZones() {
    const int P = players_;
    if (tpl_) TemplateZones();
    const int neutral = tpl_ ? 0 : size_ == 76 ? std::max(3, P) : size_ == 152 ? 2 * P + 1 : 3 * P + 1;
    const double a0 = tpl_ ? 0 : rng_.Uniform(0, 2 * kPi);
    for (int i = 0; i < (tpl_ ? 0 : P); ++i) {
      const double a = a0 + 2 * kPi * i / P;
      centres_.push_back({0.34 * Cos(a), 0.34 * Sin(a)});
      start_.push_back(true);
      zone_level_.push_back(0);
    }
    for (int i = 0; i < neutral; ++i) {
      Pt best{0, 0};
      double best_d = -1;
      for (int k = 0; k < 300; ++k) {
        const double x = rng_.Uniform(-0.4, 0.4);
        const double y = rng_.Uniform(-0.4, 0.4);
        double d = 1e9;
        for (Pt q : centres_) d = std::min(d, Dist({x, y}, q));
        if (d > best_d) {
          best = {x, y};
          best_d = d;
        }
      }
      centres_.push_back(best);
      start_.push_back(false);
      zone_level_.push_back(0);
    }
    if (levels_ == 2) {  // the caves: spread out as the surface's zones
      const int caves = size_ == 76 ? std::max(2, P) : size_ == 152 ? P + 2 : size_ == 228 ? 2 * P + 1 : 2 * P + 2;
      for (int i = 0; i < caves; ++i) {
        Pt best{0, 0};
        double best_d = -1;
        for (int k = 0; k < 300; ++k) {
          const Pt p{rng_.Uniform(-0.4, 0.4), rng_.Uniform(-0.4, 0.4)};
          double d = 1e9;
          for (size_t z = 0; z < centres_.size(); ++z)
            if (zone_level_[z] == 1) d = std::min(d, Dist(p, centres_[z]));
          if (d > best_d) {
            best = p;
            best_d = d;
          }
        }
        centres_.push_back(best);
        start_.push_back(false);
        zone_level_.push_back(1);
      }
    }
    const double amp = 0.035, f = rng_.Uniform(9, 14);
    double ph[4];
    for (double& p : ph) p = rng_.Uniform(0, 2 * kPi);
    for (int cell : cells_) {
      const Pt s = Screen(cell);
      const Pt p{s.x + amp * Sin(f * s.y + ph[0]) + amp * 0.5 * Sin(2.3 * f * (s.x + s.y) + ph[1]),
                 s.y + amp * Sin(f * s.x + ph[2]) + amp * 0.5 * Sin(2.1 * f * (s.x - s.y) + ph[3])};
      int best = 0;
      double best_d = 1e18;
      for (size_t z = 0; z < centres_.size(); ++z) {
        if (zone_level_[z] != L(cell)) continue;
        const double dx = p.x - centres_[z].x, dy = p.y - centres_[z].y, d = dx * dx + dy * dy - Power(int(z));
        if (d < best_d) {
          best_d = d;
          best = int(z);
        }
      }
      zone_[cell] = best;
    }
    FixFragments();

    std::vector<uint8_t> pool = kStartTerrains;
    rng_.Shuffle(pool);
    for (size_t z = 0; z < centres_.size(); ++z) {
      if (start_[z]) {
        terrain_.push_back(pool.back());
        pool.pop_back();
      } else if (ZoneLevel(int(z)) == 1) {
        terrain_.push_back(rng_.Choice(kCaveTerrains));
      } else {
        terrain_.push_back(rng_.Choice(kLand));
      }
    }
    const auto pairs = ZonePairs();
    for (size_t z = 0; z < centres_.size(); ++z) {
      if (start_[z]) {
        tier_.push_back(0);
        continue;
      }
      if (z < tpl_tier_.size()) {
        tier_.push_back(tpl_tier_[z]);
        continue;
      }
      if (ZoneLevel(int(z)) == 1) {  // the caves: mostly the richest (and best guarded)
        tier_.push_back(rng_.Below(3) ? 2 : 1);
        continue;
      }
      bool near = false;
      for (const auto& [key, cells] : pairs)
        if ((key[0] == int(z) && start_[key[1]]) || (key[1] == int(z) && start_[key[0]])) near = true;
      tier_.push_back(near ? 1 : 2);
    }
  }

  // Each zone's largest connected piece is kept; the other pieces go to a neighbour.
  void FixFragments() {
    const int n = int(centres_.size());
    for (int pass = 0; pass < 4; ++pass) {
      bool changed = false;
      std::vector<std::vector<std::vector<int>>> pieces(n);
      std::vector<uint8_t> seen(zone_.size());
      for (int cell : cells_) {
        if (seen[cell]) continue;
        const int z = zone_[cell];
        std::vector<int> piece, stack{cell};
        seen[cell] = 1;
        while (!stack.empty()) {
          const int cur = stack.back();
          stack.pop_back();
          piece.push_back(cur);
          for (const auto& d : kDir4) {
            const int nb = Nb(cur, d[0], d[1]);
            if (nb >= 0 && !seen[nb] && zone_[nb] == z) {
              seen[nb] = 1;
              stack.push_back(nb);
            }
          }
        }
        pieces[z].push_back(std::move(piece));
      }
      for (int z = 0; z < n; ++z) {
        auto& ps = pieces[z];
        std::stable_sort(ps.begin(), ps.end(), [](const auto& a, const auto& b) { return a.size() > b.size(); });
        for (size_t i = 1; i < ps.size(); ++i) {
          std::vector<int> votes(n);
          for (int cell : ps[i])
            for (const auto& d : kDir4) {
              const int nz = ZoneNb(cell, d[0], d[1]);
              if (nz >= 0 && nz != z) ++votes[nz];
            }
          const int best = int(std::max_element(votes.begin(), votes.end()) - votes.begin());
          if (!votes[best]) continue;
          for (int cell : ps[i]) zone_[cell] = best;
          changed = true;
        }
      }
      if (!changed) break;
    }
  }

  // Zone pairs with at least 4 bordering cell pairs (4-neighbours): (a, b) a < b -> [(in a, in b)].
  std::map<std::array<int, 2>, std::vector<std::array<int, 2>>> ZonePairs() const {
    std::map<std::array<int, 2>, std::vector<std::array<int, 2>>> pairs;
    for (int cell : cells_) {
      const int za = zone_[cell];
      for (const auto& d : {std::array<int, 2>{1, 0}, std::array<int, 2>{0, 1}}) {
        const int nb = Nb(cell, d[0], d[1]);
        if (nb < 0) continue;
        const int zb = zone_[nb];
        if (zb == za) continue;
        if (za < zb) pairs[{za, zb}].push_back({cell, nb});
        else pairs[{zb, za}].push_back({nb, cell});
      }
    }
    for (auto it = pairs.begin(); it != pairs.end();)
      it = it->second.size() < 4 ? pairs.erase(it) : std::next(it);
    return pairs;
  }

  // -- 2. walls and passages

  void MakeWalls() {
    for (int cell : cells_) {
      const int z = zone_[cell];
      for (const auto& d : kDir8)
        if (ZoneNb(cell, d[0], d[1]) != z) {
          state_[cell] = WALL;
          break;
        }
    }
    if (levels_ == 2) {  // the caves' rock: thicker and ragged
      std::vector<int> grow;
      for (int cell : cells_) {
        if (L(cell) != 1 || state_[cell] != FREE) continue;
        for (const auto& d : kDir8) {
          const int nb = Nb(cell, d[0], d[1]);
          if (nb >= 0 && state_[nb] == WALL) {
            grow.push_back(cell);
            break;
          }
        }
      }
      for (int cell : grow)
        if (rng_.Real() < 0.6) state_[cell] = WALL;
    }
    KeepLargestPieces();
  }

  // A narrow neck (or the sea) can split a zone's open area: the largest piece stays, the rest is
  // walled.
  void KeepLargestPieces() {
    std::vector<uint8_t> seen(zone_.size());
    std::map<int, std::vector<int>> best;
    for (int cell : cells_) {
      if (seen[cell] || state_[cell] != FREE) continue;
      const int z = zone_[cell];
      std::vector<int> piece, stack{cell};
      seen[cell] = 1;
      while (!stack.empty()) {
        const int cur = stack.back();
        stack.pop_back();
        piece.push_back(cur);
        for (const auto& d : kDir8) {
          const int nb = Nb(cur, d[0], d[1]);
          if (nb >= 0 && !seen[nb] && zone_[nb] == z && state_[nb] == FREE) {
            seen[nb] = 1;
            stack.push_back(nb);
          }
        }
      }
      auto& kept = best[z];
      if (piece.size() > kept.size()) {
        for (int old : kept) state_[old] = WALL;
        kept = std::move(piece);
      } else {
        for (int old : piece) state_[old] = WALL;
      }
    }
  }

  void MakeLinks() {
    const auto pairs = ZonePairs();
    const int n = int(centres_.size());
    auto weight = [&](const std::array<int, 2>& k) {
      double w = Dist(centres_[k[0]], centres_[k[1]]);
      if (start_[k[0]] && start_[k[1]]) w += 10;  // no direct start-to-start links if possible
      return w;
    };
    std::vector<std::array<int, 2>> keys;
    for (const auto& [k, v] : pairs) keys.push_back(k);
    std::stable_sort(keys.begin(), keys.end(), [&](const auto& a, const auto& b) { return weight(a) < weight(b); });
    std::vector<int> parent(n);
    for (int i = 0; i < n; ++i) parent[i] = i;
    auto find = [&](int x) {
      while (parent[x] != x) x = parent[x] = parent[parent[x]];
      return x;
    };
    if (islands_)  // the surface's zones: islands, no ways between them but by boat
      std::erase_if(keys, [&](const auto& k) { return ZoneLevel(k[0]) == 0 && ZoneLevel(k[1]) == 0; });
    if (tpl_)  // a template's links first, where their zones border each other
      for (const TemplateLink& tl : tpl_->links) {
        if (islands_) break;
        const std::array<int, 2> k{std::min(tl.a, tl.b), std::max(tl.a, tl.b)};
        const auto it = pairs.find(k);
        Link link;
        if (it == pairs.end() || !MakePassage(k, it->second, link)) {
          log.push_back("template link " + tpl_->zones[size_t(tl.a)].id + "-" + tpl_->zones[size_t(tl.b)].id +
                        (it == pairs.end() ? ": the zones do not border each other" : ": no room for the passage"));
          continue;
        }
        link.strength = tl.guard;
        parent[size_t(find(k[0]))] = find(k[1]);
        links_.push_back(std::move(link));
      }
    for (const auto& k : keys) {  // Kruskal's spanning tree (a template's: what joins its parts)
      const int a = find(k[0]), b = find(k[1]);
      if (a == b) continue;
      Link link;
      if (MakePassage(k, pairs.at(k), link)) {
        parent[a] = b;
        links_.push_back(std::move(link));
      }
    }
    std::vector<int> shore(static_cast<size_t>(n));  // each zone's open cells by the sea (normal water's coast)
    if (coast_)
      for (int cell : cells_) {
        if (state_[cell] != FREE) continue;
        for (const auto& d : kDir4) {
          const int nb = Nb(cell, d[0], d[1]);
          if (nb >= 0 && ground_[nb] == WATER && L(nb) == 0) {
            ++shore[size_t(zone_[cell])];
            break;
          }
        }
      }
    for (const auto& [k, v] : pairs) {  // and a few more
      if (islands_ && ZoneLevel(k[0]) == 0 && ZoneLevel(k[1]) == 0) continue;
      bool linked = false;
      for (const Link& l : links_) linked |= l.zones == k;
      if (linked || (start_[k[0]] && start_[k[1]])) continue;
      if (coast_ && ZoneLevel(k[0]) == 0 && shore[size_t(k[0])] >= 25 && shore[size_t(k[1])] >= 25 &&
          rng_.Real() < 0.6) {
        sea_links_.insert(k[0]);  // by sea: a shipyard on each side
        sea_links_.insert(k[1]);
        continue;
      }
      if (!tpl_ && rng_.Real() < 0.35) {  // (a template has the links it says)
        Link link;
        if (MakePassage(k, v, link)) links_.push_back(std::move(link));
      }
    }
    if (levels_ == 2) MakeTunnels();
  }

  // The passages between the levels: one into each group of linked caves at least, more to some.
  void MakeTunnels() {
    const int n = zones();
    std::vector<int> parent(n);
    for (int i = 0; i < n; ++i) parent[i] = i;
    auto find = [&](int x) {
      while (parent[x] != x) x = parent[x] = parent[parent[x]];
      return x;
    };
    for (const Link& l : links_) parent[find(l.zones[0])] = find(l.zones[1]);
    std::vector<int> caves;
    std::set<int> groups, joined;
    for (int z = 0; z < n; ++z)
      if (ZoneLevel(z) == 1) {
        caves.push_back(z);
        groups.insert(find(z));
      }
    rng_.Shuffle(caves);
    for (int uz : caves) {
      const int g = find(uz);
      if (joined.count(g) && rng_.Real() >= 0.6) continue;
      // the surface's zones above it, the most of them first
      std::map<int, int> overlap;
      for (int cell : cells_)
        if (L(cell) == 1 && zone_[cell] == uz && state_[cell] == FREE && state_[cell - area_] == FREE)
          ++overlap[zone_[cell - area_]];
      std::vector<std::pair<int, int>> order(overlap.begin(), overlap.end());
      std::stable_sort(order.begin(), order.end(), [](const auto& a, const auto& b) { return a.second > b.second; });
      for (const auto& [sz, count] : order) {
        if (count < 16) break;
        Link link;
        if (MakeTunnel(sz, uz, link)) {
          links_.push_back(std::move(link));
          joined.insert(g);
          break;
        }
      }
    }
    if (joined.size() < groups.size()) throw Failure{"the underground cannot be reached"};
  }

  // A passage from the surface's zone sz down to the cave uz, both ends on the same cells.
  bool MakeTunnel(int sz, int uz, Link& out) {
    const Footprint& down = Foot(kTunnelDown);
    const Footprint& up = Foot(kTunnelUp);
    std::vector<int> cand;
    double cr = 0, cc = 0;
    for (int cell : cells_)
      if (L(cell) == 1 && zone_[cell] == uz && state_[cell] == FREE && zone_[cell - area_] == sz &&
          state_[cell - area_] == FREE) {
        cand.push_back(cell);
        cr += R(cell);
        cc += C(cell);
      }
    if (cand.empty()) return false;
    cr /= double(cand.size());
    cc /= double(cand.size());
    std::vector<double> score;
    for (int cell : cand)
      score.push_back(std::sqrt((R(cell) - cr) * (R(cell) - cr) + (C(cell) - cc) * (C(cell) - cc)) + rng_.Uniform(0, 4));
    std::vector<size_t> order(cand.size());
    for (size_t i = 0; i < order.size(); ++i) order[i] = i;
    std::stable_sort(order.begin(), order.end(), [&](size_t x, size_t y) { return score[x] < score[y]; });
    for (size_t i = 0; i < order.size() && i < 300; ++i) {
      const int cell = cand[order[i]];
      const int r0 = R(cell) - down.w / 2, c0 = C(cell) - down.h / 2;
      std::vector<int> e0, e1;
      if (!TryPlace(down, r0, c0, sz, e0)) continue;
      const auto first = undo_;
      if (!TryPlace(up, r0, c0, uz, e1)) {
        undo_ = first;
        Undo();
        continue;
      }
      objects.push_back(MakeObject(kTunnelDown, r0, c0, 0));
      objects.push_back(MakeObject(kTunnelUp, r0, c0, 1));
      for (int a : e0)
        for (int b : e1) {
          jumps_[a].push_back({b, true});
          jumps_[b].push_back({a, true});
        }
      out.zones = {sz, uz};
      out.ends = {e0[0], e1[0]};
      out.guard = e1[0];  // underground, at the passage's foot
      return true;
    }
    return false;
  }

  bool MakePassage(const std::array<int, 2>& key, const std::vector<std::array<int, 2>>& cell_pairs, Link& out) {
    const int a = key[0], b = key[1];
    const Pt mid{(centres_[a].x + centres_[b].x) / 2, (centres_[a].y + centres_[b].y) / 2};
    auto clean = [&](int cell, int radius) {  // no third zone (or the map's edge) around it
      for (int dr = -radius; dr <= radius; ++dr)
        for (int dc = -radius; dc <= radius; ++dc) {
          const int z = ZoneNb(cell, dr, dc);
          if (z != a && z != b) return false;
        }
      return true;
    };
    std::vector<std::array<int, 2>> cand;
    for (int radius : {2, 1}) {
      for (const auto& p : cell_pairs)
        if (clean(p[0], radius) && clean(p[1], radius)) cand.push_back(p);
      if (!cand.empty()) break;
    }
    if (cand.empty()) return false;
    std::vector<double> score;
    for (const auto& p : cand) score.push_back(Dist(Screen(p[0]), mid) + rng_.Uniform(0, 0.03));
    std::vector<size_t> order(cand.size());
    for (size_t i = 0; i < order.size(); ++i) order[i] = i;
    std::stable_sort(order.begin(), order.end(), [&](size_t x, size_t y) { return score[x] < score[y]; });
    for (size_t i = 0; i < order.size() && i < 12; ++i) {
      const int ca = cand[order[i]][0], cb = cand[order[i]][1];
      const int dr = R(ca) - R(cb), dc = C(ca) - C(cb);
      std::vector<int> corridor{ca, cb};
      std::array<int, 2> ends{};
      const int starts[2] = {ca, cb}, sign[2] = {1, -1}, zs[2] = {a, b};
      for (int side = 0; side < 2; ++side) {
        int cur = starts[side];
        for (int step = 0; step < 6; ++step) {
          if (state_[cur] != WALL) break;
          const int nxt = Nb(cur, sign[side] * dr, sign[side] * dc);
          if (nxt < 0 || zone_[nxt] != zs[side]) break;
          corridor.push_back(nxt);
          cur = nxt;
        }
        ends[side] = cur;
      }
      if (state_[ends[0]] != FREE || state_[ends[1]] != FREE) continue;
      for (int cell : corridor) state_[cell] = RES;
      out.zones = key;
      out.ends = ends;
      out.corridor = std::move(corridor);
      out.guard = tier_[b] >= tier_[a] ? cb : ca;  // on the border cell of the more valuable zone
      return true;
    }
    return false;
  }

  // -- 3. content

  // Places the footprint with its cell (0, 0) at (r0, c0) in zone z: its entrances, or none if it
  // does not fit (the grid unchanged). undo_ then has what it changed.
  bool TryPlace(const Footprint& fp, int r0, int c0, int z, std::vector<int>& entries) {
    std::vector<std::pair<int, bool>> cells;
    for (int fy = 0; fy < fp.h; ++fy)
      for (int fx = 0; fx < fp.w; ++fx) {
        const int cell = At(r0 + fx, c0 + fy, ZoneLevel(z));
        if (cell < 0 || zone_[cell] != z || state_[cell] != FREE) return false;
        cells.push_back({cell, fp.blocked[size_t(fy) * fp.w + fx] != 0});
      }
    auto own = [&](int r, int c) { return r >= r0 && r < r0 + fp.w && c >= c0 && c < c0 + fp.h; };
    for (const auto& [cell, b] : cells)  // a gap to other objects
      for (const auto& d : kDir8) {
        const int r = R(cell) + d[0], c = C(cell) + d[1];
        const int nb = At(r, c, ZoneLevel(z));
        if (nb >= 0 && !own(r, c) && (state_[nb] == BLOCK || state_[nb] == OCC)) return false;
      }
    entries.clear();
    for (const auto& e : fp.entry) {
      const int r = r0 + e[0], c = c0 + e[1];
      if (own(r, c)) continue;
      const int cell = At(r, c, ZoneLevel(z));
      if (cell >= 0 && zone_[cell] == z && (state_[cell] == FREE || state_[cell] == RES))
        entries.push_back(cell);
    }
    if (!fp.entry.empty() && entries.empty()) return false;
    undo_.clear();
    for (const auto& [cell, b] : cells) undo_.push_back({cell, state_[cell]});
    for (int cell : entries) undo_.push_back({cell, state_[cell]});
    for (const auto& [cell, b] : cells) state_[cell] = b ? BLOCK : OCC;
    for (int cell : entries) state_[cell] = RES;
    return true;
  }

  void Undo() {
    for (auto it = undo_.rbegin(); it != undo_.rend(); ++it) state_[it->first] = it->second;
  }

  // Places `model` in zone z as close to `target` as it can; the object's index or -1.
  template <typename Make, typename Validate>
  long PlaceNear(std::string_view model, int z, int target, double spread, Make make, Validate validate,
                 std::vector<int>& entries) {
    const Footprint& fp = Foot(model);
    std::vector<int> cand;
    for (int cell : zone_cells_[z])
      if (state_[cell] == FREE) cand.push_back(cell);
    std::vector<double> score;
    for (int cell : cand) {
      const double dr = R(cell) - R(target), dc = C(cell) - C(target);
      score.push_back(std::sqrt(dr * dr + dc * dc) + rng_.Uniform(0, spread));
    }
    std::vector<size_t> order(cand.size());
    for (size_t i = 0; i < order.size(); ++i) order[i] = i;
    std::stable_sort(order.begin(), order.end(), [&](size_t x, size_t y) { return score[x] < score[y]; });
    for (size_t i = 0; i < order.size() && i < 400; ++i) {
      const int cell = cand[order[i]];
      const int r0 = R(cell) - fp.w / 2, c0 = C(cell) - fp.h / 2;
      if (!TryPlace(fp, r0, c0, z, entries)) continue;
      if (!validate(entries)) {
        Undo();
        continue;
      }
      objects.push_back(make(r0, c0));
      objects.back().lv = ZoneLevel(z);
      return long(objects.size() - 1);
    }
    log.push_back("zone " + std::to_string(z) + ": no room for " + std::string(model));
    return -1;
  }
  long PlaceNear(std::string_view model, int z, int target, double spread, std::vector<int>& entries) {
    return PlaceNear(
        model, z, target, spread, [&](int r, int c) { return MakeObject(model, r, c, ZoneLevel(z)); },
        [](const std::vector<int>&) { return true; }, entries);
  }

  int RandomSpot(int z) {
    std::vector<int> cells;
    for (int cell : zone_cells_[z])
      if (state_[cell] == FREE) cells.push_back(cell);
    rng_.Shuffle(cells);
    return cells.empty() ? -1 : cells[0];
  }

  int ZoneCentreCell(int z) const {
    const auto t = FromScreen(centres_[z]);
    int best = -1;
    double best_d = 1e18;
    for (int cell : zone_cells_[z]) {
      if (state_[cell] != FREE) continue;
      const double dr = R(cell) - t[0], dc = C(cell) - t[1], d = std::sqrt(dr * dr + dc * dc);
      if (d < best_d) {
        best_d = d;
        best = cell;
      }
    }
    if (best < 0) throw Failure{"zone " + std::to_string(z) + " has no room"};
    return best;
  }

  // Can a hero walk from `start` to every goal inside zone z?
  bool Reaches(int z, int start, const std::vector<int>& goals) const {
    std::vector<uint8_t> seen(zone_.size());
    std::vector<int> stack{start};
    seen[start] = 1;
    const std::set<int> want(goals.begin(), goals.end());
    size_t left = want.size() - want.count(start);
    while (!stack.empty() && left) {
      const int cur = stack.back();
      stack.pop_back();
      for (const auto& d : kDir8) {
        const int nb = Nb(cur, d[0], d[1]);
        if (nb < 0 || seen[nb] || zone_[nb] != z || !Walkable(state_[nb])) continue;
        seen[nb] = 1;
        stack.push_back(nb);
        if (want.count(nb)) --left;
      }
    }
    return left == 0;
  }

  void FillZones() {
    const int n = int(centres_.size());
    zone_cells_.assign(n, {});
    for (int cell : cells_) zone_cells_[zone_[cell]].push_back(cell);
    if (tpl_) {  // (for a template's author: how big its zones came out)
      std::string line = "template zones:";
      for (size_t z = 0; z < tpl_->zones.size(); ++z) {
        int open = 0;
        for (int cell : zone_cells_[z]) open += state_[cell] == FREE;
        line += " " + tpl_->zones[z].id + "=" + std::to_string(open);
      }
      log.push_back(line);
    }
    hubs_.assign(n, -1);
    items_.assign(n, {});
    int owner = 0;
    for (int z = 0; z < n; ++z) {
      if (SeaZone(z)) continue;  // its islets are filled on their own
      const int centre = ZoneCentreCell(z);
      if (!start_[z]) {
        state_[centre] = RES;
        hubs_[z] = centre;
        continue;
      }
      const int player = owner++;
      const int variant = int(rng_.Below(2));  // left, right
      const PlayerSetup& ps = setup_.players[player];
      const int faction = ps.faction;
      const std::string_view model = faction >= 0 ? kFactionTown[faction][variant] : kRandomTown[variant];
      std::vector<int> ends;
      for (const Link& l : links_)
        for (int side = 0; side < 2; ++side)
          if (l.zones[side] == z) ends.push_back(l.ends[side]);
      std::vector<int> entries;
      const long town = PlaceNear(
          model, z, centre, 0, [&](int r, int c) { return MakeTown(faction, variant, player, r, c); },
          [&](const std::vector<int>& e) { return !e.empty() && Reaches(z, e[0], ends); }, entries);
      if (town < 0) throw Failure{"no room for the town in zone " + std::to_string(z)};
      const int gate = entries[0];
      const int fr = R(gate) + (variant ? 0 : 1), fc = C(gate) + (variant ? 1 : 0);  // in front of it
      std::vector<std::array<int, 2>> spots;
      for (int dr = -3; dr <= 3; ++dr)
        for (int dc = -3; dc <= 3; ++dc) spots.push_back({fr + dr, fc + dc});
      auto d2 = [&](const std::array<int, 2>& p) {
        const double a = p[0] - fr, b = p[1] - fc;
        return std::sqrt(a * a + b * b);
      };
      std::stable_sort(spots.begin(), spots.end(), [&](const auto& x, const auto& y) { return d2(x) < d2(y); });
      spots.insert(spots.begin(), {fr, fc});
      int hero = -1;
      for (const auto& p : spots) {
        const int cell = At(p[0], p[1], ZoneLevel(z));
        if (cell >= 0 && cell != gate && state_[cell] == FREE && zone_[cell] == z) {
          hero = cell;
          break;
        }
      }
      if (hero < 0) throw Failure{"no room for the hero in zone " + std::to_string(z)};
      state_[hero] = BLOCK;
      HeroRecord rec;
      const HeroChoice* choice = nullptr;
      if (faction >= 0 && ps.hero >= 0 && size_t(ps.hero) < cat_.Heroes(faction).size())
        choice = &cat_.Heroes(faction)[ps.hero];
      if (choice) {
        rec.portrait = choice->portrait;
        rec.hero_class = choice->hero_class;
        rec.sex = choice->sex;
        rec.name = choice->name;
        rec.biography = choice->biography;
      }
      // no army: the game gives the starting hero a week's growth of the town's level 1
      // creatures (hero_card.h's HeroArt::Army)
      Object o;
      o.r = R(hero);
      o.c = C(hero);
      o.lv = L(hero);
      o.army = true;
      o.body = HeroArmy(uint8_t(player), rec);
      objects.push_back(std::move(o));
      hubs_[z] = gate;
      town_gates_.push_back(gate);
    }
  }

  void Add(int z, std::string_view model, int near, double spread, int guard = 0) {
    std::vector<int> entries;
    const long obj = PlaceNear(model, z, near, spread, entries);
    if (obj >= 0) items_[z].push_back({std::move(entries), guard, size_t(obj)});
  }
  int Far(int z) {
    const int spot = RandomSpot(z);
    return spot >= 0 ? spot : hubs_[z];
  }
  std::string_view Pick(const Models& m) { return m[rng_.Below(uint32_t(m.size()))]; }
  std::string_view PickMine(const std::vector<std::array<const char*, 2>>& m) {
    const auto& pair = m[rng_.Below(uint32_t(m.size()))];
    return pair[rng_.Below(2)];
  }

  void FillContents() {
    for (const Islet& i : islets_) FillIslet(i.zone, i.centre);
    for (int z = 0; z < int(centres_.size()); ++z) {
      if (SeaZone(z)) continue;
      if (islands_ && ZoneLevel(z) == 0) PlaceShipyard(z);
      if (coast_ && ZoneLevel(z) == 0 && Shore(z) >= 25 && (sea_links_.count(z) || rng_.Real() < 0.7))
        PlaceShipyard(z, false);
      if (ZoneLevel(z) == 1 && size_ > 76 && rng_.Real() < 0.35) PlaceCaveTown(z);
      if (Town(z)) PlaceCaveTown(z);  // a template's neutral town (as a cave's: well guarded)
      if (start_[z]) FillStart(z, hubs_[z]);
      else FillNeutral(z, hubs_[z], tier_[z]);
      // bigger zones (medium maps and up) get more rounds of smaller content
      const int extra = int(std::nearbyint(zone_cells_[z].size() / 700.0)) - 1;
      for (int i = 0; i < extra; ++i) FillExtra(z, tier_[z]);
      FillMore(z);
      Step();
    }
    if ((islands_ || coast_) && size_ > 76) PlaceLighthouse();
    PlaceObelisks();
  }

  // Obelisks: gold ones (each shows a part of the puzzle map to the Grail), and, as the setup
  // asks, a few of other colours, each colour's own map to a treasure (artifacts dug up).
  void PlaceObelisks() {
    std::vector<int> zs;
    for (int z = 0; z < zones(); ++z)
      if (!SeaZone(z) && !start_[z]) zs.push_back(z);
    if (zs.empty())
      for (int z = 0; z < zones(); ++z)
        if (!SeaZone(z)) zs.push_back(z);
    if (zs.empty()) return;
    auto spread = [&](std::string_view model, int n) {
      rng_.Shuffle(zs);
      for (int i = 0; i < n; ++i) {
        const int z = zs[size_t(i) % zs.size()];
        Add(z, model, Far(z), 5);
      }
    };
    const int k = size_ / 76;  // 1 small ... 4 extra large
    if (setup_.obelisks) spread("miscellaneous.Obelisks.Gold", k == 1 ? 4 : k == 2 ? 6 : k == 3 ? 7 : 8);
    if (!setup_.treasures) return;
    struct Colour {
      const char* model;
      int slot;
    };
    std::vector<Colour> colours = {{"miscellaneous.Obelisks.Black", 4},  {"miscellaneous.Obelisks.Purple", 6},
                                   {"miscellaneous.Obelisks.Orange", 7}, {"miscellaneous.Obelisks.Brown", 9},
                                   {"miscellaneous.Obelisks.Silver", 10}, {"miscellaneous.Obelisks.Bronze", 11}};
    rng_.Shuffle(colours);
    auto pick = [&](const char* level) -> int {
      const auto& arts = cat_.Artifacts(level);
      return arts.empty() ? -1 : arts[rng_.Below(uint32_t(arts.size()))];
    };
    for (int i = 0; i < std::clamp(setup_.treasures, 0, 4); ++i) {
      const Colour& c = colours[size_t(i)];
      std::vector<uint16_t> rewards;
      const int best = pick(k >= 3 && rng_.Real() < 0.4 ? "Relic" : "Major");
      if (best >= 0) rewards.push_back(uint16_t(best));
      if (rng_.Real() < 0.5)
        if (const int more = pick("Minor"); more >= 0) rewards.push_back(uint16_t(more));
      if (rewards.empty()) continue;
      obelisk_rewards_[size_t(c.slot)] = rewards;
      spread(c.model, 3 + int(rng_.Below(2)));
    }
  }

  void FillExtra(int z, int tier) {
    const int level = std::min(4, tier + 1);
    std::string_view art = kArtifactTreasure;
    if (tier < 2) art = rng_.Below(2) ? kArtifactItem : kArtifactMinor;
    Add(z, art, Far(z), 5, level);
    Add(z, kDwelling[std::min(4, tier + 1)], Far(z), 5, tier ? level : 0);
    for (int i = 0; i < 2; ++i) {
      Add(z, kChest, Far(z), 5);
      Add(z, kResource, Far(z), 5);
    }
    Models any = kFountains;
    any.insert(any.end(), kGems.begin(), kGems.end());
    const auto m = Pick(any);
    Add(z, m, Far(z), 5);
    if (rng_.Real() < 0.5) {
      const auto b = Pick(kBanks);
      Add(z, b, Far(z), 5);
    }
  }

  // as many more objects again, as the shipped maps have (per 1000 open cells: about 7
  // piles, 3 chests, 9 small finds, 3 guarded better ones, 3 monsters on their own).
  void FillMore(int z) {
    int open = 0;
    for (int cell : zone_cells_[z]) open += state_[cell] == FREE;
    const double k = open / 1000.0;
    const int tier = tier_[z], level = std::min(4, tier + 1);
    auto count = [&](double per) { return int(per * k + rng_.Real()); };
    for (int i = count(7); i-- > 0;) Add(z, Pick(kPiles), Far(z), 5);
    for (int i = count(3); i-- > 0;) Add(z, Pick(kTreasures), Far(z), 5, tier ? level : 0);
    for (int i = count(9); i-- > 0;) Add(z, Pick(kSmall), Far(z), 5);
    for (int i = count(3); i-- > 0;) Add(z, Pick(kGuarded), Far(z), 5, level);
    const auto& themed = kThemed[terrain_[z]];
    for (int i = count(1.5); i-- > 0 && !themed.empty();) Add(z, Pick(themed), Far(z), 5);
    for (int i = count(3); i-- > 0;) {  // the roaming ones: a cell of their own, kept open
      const int cell = RandomSpot(z);
      if (cell < 0) break;
      state_[cell] = BLOCK;  // no path through it; open again with its monster (MakePaths)
      roamers_.push_back({cell, z, std::max(1, level - int(rng_.Below(2)))});
    }
  }

  void FillStart(int z, int gate) {
    Add(z, kSawmill[rng_.Below(2)], gate, 14);
    Add(z, kOrePit[rng_.Below(2)], gate, 14);
    for (int i = 0; i < 3; ++i) Add(z, kResource, gate, 30);
    for (int i = 0; i < 2; ++i) Add(z, kChest, gate, 30);
    Add(z, kCampfire, gate, 30);
    Add(z, kDwelling[1], gate, 20);
    Add(z, Pick(kFountains), gate, 30);
    Add(z, kArtifactMinor, gate, 40, 1);
    if (size_ >= 152) {
      Add(z, PickMine(kRareMines), gate, 30, 1);
      Add(z, kDwelling[2], gate, 30);
      for (int i = 0; i < 3; ++i) Add(z, kResource, gate, 60);
    }
  }

  void FillNeutral(int z, int hub, int tier) {
    (void)hub;
    if (tier == 1) {
      auto m = PickMine(kRareMines);
      Add(z, m, Far(z), 5, 2);
      if (rng_.Real() < 0.5) {
        const auto& pair = rng_.Below(2) ? kOrePit : kSawmill;
        m = pair[rng_.Below(2)];
        Add(z, m, Far(z), 5, 1);
      }
      Add(z, kDwelling[2], Far(z), 5, 2);
      for (int i = 0; i < 2; ++i) Add(z, kArtifactMinor, Far(z), 5, 2);
      Add(z, "shrines.Random 1", Far(z), 5);
      m = rng_.Below(2) ? "moral-luck boosters.Random Temple" : "Scholars.Random Scholar";
      Add(z, m, Far(z), 5);
      m = Pick(kGems);
      Add(z, m, Far(z), 5);
    } else {
      auto m = Pick(kGoldMine);
      Add(z, m, Far(z), 5, 3);
      m = PickMine(kRareMines);
      Add(z, m, Far(z), 5, 3);
      m = kDwelling[rng_.Below(2) ? 4 : 3];
      Add(z, m, Far(z), 5, 3);
      Add(z, kArtifactMajor, Far(z), 5, 4);
      Add(z, kArtifactTreasure, Far(z), 5, 3);
      m = rng_.Below(2) ? "shrines.Random 3" : "shrines.Random 2";
      Add(z, m, Far(z), 5);
      m = Pick(kSchools);
      Add(z, m, Far(z), 5);
      m = rng_.Below(2) ? Pick(kRichBanks) : Pick(kBanks);
      Add(z, m, Far(z), 5);
    }
    for (int i = 0; i < 2 + tier; ++i) Add(z, kChest, Far(z), 5);
    for (int i = 0; i < 3 + tier; ++i) Add(z, kResource, Far(z), 5);
    const auto m = Pick(kFountains);
    Add(z, m, Far(z), 5);
  }

  // -- 4. paths

  // A* in zone z from start to the nearest goal, over walkable cells; empty if none.
  std::vector<int> AStar(int z, int start, const std::vector<int>& goals) const {
    std::set<int> goal(goals.begin(), goals.end());
    if (goal.count(start)) return {start};
    if (goal.empty()) return {};
    struct Q {
      double f, g;
      int cell;
      bool operator>(const Q& o) const {
        if (f != o.f) return f > o.f;
        if (g != o.g) return g > o.g;
        return cell > o.cell;
      }
    };
    std::priority_queue<Q, std::vector<Q>, std::greater<Q>> open;
    std::map<int, int> came;
    std::map<int, double> g;
    open.push({0, 0, start});
    came[start] = -1;
    g[start] = 0;
    const size_t nh = std::min<size_t>(8, goals.size());
    while (!open.empty()) {
      const Q q = open.top();
      open.pop();
      if (goal.count(q.cell)) {
        std::vector<int> path;
        for (int cur = q.cell; cur != -1; cur = came[cur]) path.push_back(cur);
        std::reverse(path.begin(), path.end());
        return path;
      }
      if (q.g > g[q.cell]) continue;
      for (const auto& d : kDir8) {
        const int nb = Nb(q.cell, d[0], d[1]);
        if (nb < 0 || zone_[nb] != z || !Walkable(state_[nb])) continue;
        double step = d[0] && d[1] ? 1.41 : 1.0;
        if (bias_[nb] > 0) step *= bias_[nb];
        const double ng = q.g + step;
        auto it = g.find(nb);
        if (it != g.end() && ng >= it->second) continue;
        g[nb] = ng;
        came[nb] = q.cell;
        int h = 1 << 30;
        for (size_t i = 0; i < nh; ++i)
          h = std::min(h, std::max(std::abs(R(nb) - R(goals[i])), std::abs(C(nb) - C(goals[i]))));
        open.push({ng + h * 0.5, ng, nb});
      }
    }
    return {};
  }

  void MakeMainPaths() {
    bias_.assign(zone_.size(), 0);
    for (const Link& l : links_)
      for (int side = 0; side < 2; ++side) {
        const int z = l.zones[side];
        auto path = AStar(z, hubs_[z], {l.ends[side]});
        if (path.empty()) {
          log.push_back("zone " + std::to_string(z) + ": a passage cannot be reached");
          continue;
        }
        for (int cell : path) {
          state_[cell] = RES;
          bias_[cell] = 0.4;
        }
        main_paths_.push_back({z, std::move(path)});
      }
  }

  void MakePaths() {
    std::vector<int> uses(zone_.size());
    for (int z = 0; z < int(items_.size()); ++z)
      for (const Item& it : items_[z]) {
        auto path = AStar(z, hubs_[z], it.entries);
        if (path.empty()) {
          log.push_back("zone " + std::to_string(z) + ": left out " + objects[it.object].model + " (unreachable)");
          objects[it.object].removed = true;
          continue;
        }
        for (int cell : path) {
          state_[cell] = RES;
          if (bias_[cell] == 0) bias_[cell] = 0.7;
          ++uses[cell];
        }
        // roads to the shipyards and towns, the mines, dwellings and material generators, and
        // (not far) the places a hero goes back to: the side roads off the ways between the zones
        const std::string_view type = objects[it.object].kind[0].second;
        const bool major = type == "shipyard" || type == "random_town" || type == "mine" ||
                           type == "random_creature_dwelling" || type == "weekly_material_generator" ||
                           type == "windmill";
        const bool minor = type == "creature_bank" || type == "random_shrine" || type == "random_temple" ||
                           type == "trading_post" || type == "tavern" || type == "random_teacher" ||
                           type == "academy" || type == "blacksmith" || type == "mana_recharger";
        if (major || (minor && path.size() <= 24)) road_paths_.push_back({z, path});
        if (it.guard) pending_guards_.push_back({path.back(), it.guard, path});
      }
    std::vector<uint8_t> main(zone_.size());
    for (const auto& [z, p] : main_paths_)
      for (int cell : p) main[cell] = 1;
    // guards on the objects' entrances, unless that cell is on another path
    for (const Pending& p : pending_guards_) {
      int cell = p.cell;
      if (uses[cell] > 1 || main[cell] || guards_.count(cell)) {
        cell = -1;  // step back along the path to a cell only this path uses
        for (size_t i = p.path.size(); i-- > 1;)
          if (uses[p.path[i]] == 1 && !main[p.path[i]]) {
            cell = p.path[i];
            break;
          }
        if (cell < 0) continue;
      }
      objects.push_back(MakeMonster(p.level, cell));
      guards_.insert(cell);
    }
    for (const Roamer& m : roamers_) {
      state_[m.cell] = OCC;
      objects.push_back(MakeMonster(m.level, m.cell));
    }
  }

  // -- islands

  // The surface's zones made islands: their borders (and the map's edge) the sea, a little wider.
  static constexpr int kSeaRoom = 300;  // MakeSea: a smallest island's cells per pass of more sea

  void MakeSea() {
    std::vector<int> sea;
    for (int cell : cells_)
      if (L(cell) == 0 && state_[cell] == WALL) sea.push_back(cell);
    for (int cell : cells_) {  // the map's edge
      if (L(cell) != 0) continue;
      const int r = R(cell), c = C(cell);
      const int d = std::min({r, size_ - 1 - r, c - first_[r], end_[r] - 1 - c});
      if (d < (size_ > 76 ? 3 : 2)) sea.push_back(cell);
    }
    // some neutral zones all sea: open water with islets (one in five, none on small maps; on the
    // classic extra large ones one in five to one in three, by the seed: a template's zones are its
    // author's, and the large maps' start islands would be left too small)
    sea_zone_.assign(size_t(zones()), 0);
    std::vector<int> neutral;
    for (int z = 0; z < zones(); ++z)
      if (ZoneLevel(z) == 0 && !start_[z]) neutral.push_back(z);
    rng_.Shuffle(neutral);
    // (the widths' draws: their own sequence, the rest of the map as before)
    Rng wide(seed_ ^ 0x736561u);
    const double open_share = size_ >= 228 && !tpl_ ? 0.2 + 0.13 * wide.Real() : 0.2;
    const size_t open_seas = size_ > 76 ? size_t(double(neutral.size()) * open_share + 0.4) : 0;
    for (size_t i = 0; i < open_seas && i < neutral.size(); ++i) sea_zone_[size_t(neutral[i])] = 1;
    for (int cell : cells_)
      if (L(cell) == 0 && sea_zone_[size_t(zone_[cell])]) sea.push_back(cell);
    for (int cell : sea) ground_[cell] = WATER;
    // wider, ragged (less on the smaller maps: their zones are small; on the large and extra large
    // ones up to 3 or 4 cells more, by the seed, where the smallest island keeps room for a town)
    const int base = size_ >= 228 ? 6 : size_ >= 152 ? 4 : players_ > 4 ? 0 : players_ > 2 ? 1 : 2;
    const int extra = size_ >= 228 ? int(wide.Below(5)) : size_ >= 152 ? int(wide.Below(4)) : 0;
    int least = 1 << 30;  // the smallest land zone's cells
    {
      std::vector<int> area(static_cast<size_t>(zones()));
      for (int cell : cells_)
        if (L(cell) == 0) ++area[size_t(zone_[cell])];
      for (int z = 0; z < zones(); ++z)
        if (ZoneLevel(z) == 0 && !sea_zone_[size_t(z)]) least = std::min(least, area[size_t(z)]);
    }
    int passes = base;
    if (tpl_)  // a template's small zones: a narrower sea (room on each island for its shipyard)
      passes = std::clamp(least / 150, 0, base);
    else
      passes = std::max(base, std::min(base + extra, least / kSeaRoom));
    for (int pass = 0; pass < passes; ++pass) {
      std::vector<int> grow;
      for (int cell : cells_) {
        if (L(cell) != 0 || ground_[cell] == WATER) continue;
        for (const auto& d : kDir4) {
          const int nb = Nb(cell, d[0], d[1]);
          if (nb >= 0 && ground_[nb] == WATER) {
            grow.push_back(cell);
            break;
          }
        }
      }
      for (int cell : grow)
        if (rng_.Real() < (pass + 1 < passes ? 0.95 : 0.5)) ground_[cell] = WATER;  // the last: ragged
    }
    for (int cell : cells_)
      if (ground_[cell] == WATER) state_[cell] = BLOCK;
    // the islets: sand, far from the shores
    for (int z = 0; z < zones(); ++z) {
      if (!sea_zone_[size_t(z)]) continue;
      std::vector<int> cand;
      for (int cell : zone_cells_of(z))
        if (Clear(cell, 6)) cand.push_back(cell);
      rng_.Shuffle(cand);
      int made = 0;
      for (int centre : cand) {
        if (made >= 3) break;
        if (!Clear(centre, 6)) continue;  // an islet made since
        const int rad = 2 + int(rng_.Below(2));
        for (int dr = -rad; dr <= rad; ++dr)
          for (int dc = -rad; dc <= rad; ++dc) {
            const int nb = Nb(centre, dr, dc);
            if (nb < 0 || dr * dr + dc * dc > rad * rad + int(rng_.Below(3))) continue;
            ground_[nb] = SAND;
            state_[nb] = FREE;
          }
        islets_.push_back({z, centre});
        ++made;
      }
    }
  }
  // Water all around `cell`, `margin` cells out.
  bool Clear(int cell, int margin) const {
    for (int dr = -margin; dr <= margin; ++dr)
      for (int dc = -margin; dc <= margin; ++dc) {
        const int nb = Nb(cell, dr, dc);
        if (nb < 0 || ground_[nb] != WATER) return false;
      }
    return true;
  }
  std::vector<int> zone_cells_of(int z) const {
    std::vector<int> out;
    for (int cell : cells_)
      if (zone_[cell] == z) out.push_back(cell);
    return out;
  }
  // A power diagram's offset (d^2 - power): a template's zones' shares of the map as their sizes
  // ask, the cells convex (the classic zones: 0).
  double Power(int z) const { return size_t(z) < power_.size() ? power_[size_t(z)] : 0.0; }
  bool Town(int z) const { return size_t(z) < town_.size() && town_[size_t(z)]; }
  bool SeaZone(int z) const { return !sea_zone_.empty() && sea_zone_[size_t(z)]; }

  // An islet's finds: a treasure, guarded, and maybe more.
  void FillIslet(int z, int centre) {
    static const Models kFinds = {kArtifactTreasure, kArtifactMajor, "Resources.Treasure Chest",
                                  "Resources.Mage's Chest", "power ups.Tree of Knowledge", "Power Ups.Learning Stone"};
    const int count = 1 + int(rng_.Below(2));
    for (int i = 0; i < count; ++i) {
      std::vector<int> entries;
      const auto model = Pick(kFinds);
      const long obj = PlaceNear(model, z, centre, 1, entries);
      if (obj < 0 || entries.empty()) continue;
      const int guard = entries[rng_.Below(uint32_t(entries.size()))];  // on its way in
      objects.push_back(MakeMonster(3 + int(rng_.Below(2)), guard));
      guards_.insert(guard);
      state_[guard] = OCC;
    }
  }

  // Normal water's sea: along one of the map's sides, its depth wandering.
  void MakeCoast() {
    const int side = int(rng_.Below(4));
    // the diamond's sides are lines r + c and c - r; along a side, the other one
    std::vector<double> depth(static_cast<size_t>(2 * size_ + 2));
    double d = size_ * rng_.Uniform(0.09, 0.15);
    for (auto& x : depth) {
      d = std::clamp(d + rng_.Uniform(-1.0, 1.0), size_ * 0.05, size_ * 0.2);
      x = d;
    }
    const int h = size_ / 2;
    for (int cell : cells_) {
      if (L(cell) != 0) continue;
      const int r = R(cell), c = C(cell);
      const int dist = side == 0 ? c + r - (h - 1)          // north-west
                       : side == 1 ? c - r + h              // south-west
                       : side == 2 ? h + r - c              // north-east
                                   : 3 * h - 1 - r - c;     // south-east
      const int along = std::clamp((side == 0 || side == 3 ? c - r + size_ : c + r), 0, 2 * size_ + 1);
      if (dist < depth[size_t(along)]) {
        ground_[cell] = WATER;
        state_[cell] = BLOCK;
      }
    }
    coast_ = true;
  }

  // A shipyard on the island's shore: water next to a cell it is visited from (the boat's).
  // A neutral town of a random faction in a cave, well guarded.
  void PlaceCaveTown(int z) {
    const int variant = int(rng_.Below(2));
    std::vector<int> entries;
    const long obj = PlaceNear(
        kRandomTown[variant], z, hubs_[z], 6,
        [&](int r, int c) { return MakeTown(-1, variant, 6, r, c); },
        [&](const std::vector<int>& e) { return !e.empty(); }, entries);
    if (obj >= 0) items_[z].push_back({std::move(entries), 4, size_t(obj)});
  }

  int Shore(int z) const {
    int n = 0;
    for (int cell : zone_cells_[z]) {
      if (state_[cell] != FREE) continue;
      for (const auto& d : kDir4) {
        const int nb = Nb(cell, d[0], d[1]);
        if (nb >= 0 && ground_[nb] == WATER && L(nb) == 0) {
          ++n;
          break;
        }
      }
    }
    return n;
  }
  void PlaceShipyard(int z, bool needed = true) {
    std::vector<int> shore;
    for (int cell : zone_cells_[z]) {
      if (state_[cell] != FREE) continue;
      for (const auto& d : kDir8) {
        const int nb = Nb(cell, d[0], d[1]);
        if (nb >= 0 && ground_[nb] == WATER) {
          shore.push_back(cell);
          break;
        }
      }
    }
    if (shore.empty()) {
      if (!needed) return;
      throw Failure{"zone " + std::to_string(z) + " has no shore"};
    }
    auto wet = [&](const std::vector<int>& entries) {
      for (int e : entries)
        for (const auto& d : kDir8) {
          const int nb = Nb(e, d[0], d[1]);
          if (nb >= 0 && ground_[nb] == WATER) return true;
        }
      return false;
    };
    for (int tries = 0; tries < 6; ++tries) {
      const std::string_view model = rng_.Below(2) ? "miscellaneous.Shipyard" : "miscellaneous.Shipyard R";
      const int target = rng_.Choice(shore);
      std::vector<int> entries;
      const long obj = PlaceNear(
          model, z, target, 2, [&](int r, int c) { return MakeObject(model, r, c, 0); }, wet, entries);
      if (obj >= 0) {
        items_[z].push_back({std::move(entries), 0, size_t(obj)});
        shipyards_.push_back(int(obj));
        return;
      }
    }
    if (needed) throw Failure{"no room for a shipyard in zone " + std::to_string(z)};
  }

  // A few finds on the sea, visited by boat: flotsam, chests, buoys.
  void FillSea() {
    std::vector<int> open;
    for (int cell : cells_) {
      if (ground_[cell] != WATER) continue;
      bool clear = true;
      for (int dr = -1; dr <= 2 && clear; ++dr)
        for (int dc = -1; dc <= 2 && clear; ++dc) {
          const int nb = Nb(cell, dr, dc);
          clear = nb >= 0 && ground_[nb] == WATER;
        }
      if (clear) open.push_back(cell);
    }
    rng_.Shuffle(open);
    const Models kFinds = {"water objects.flotsam", "water objects.flotsam", "water objects.Sea Chest",
                           "water objects.buoy", "water objects.Shipwreck Survivor", "water objects.Dolphin School",
                           "moral-luck boosters.Oyster", "creature bank.Derelict Ship", "creature bank.Pirate Islands",
                           "creature bank.Shipwreck", "creature bank.Shipwreck 2"};
    const size_t want = open.size() / 150;
    for (size_t i = 0, placed = 0; i < open.size() && placed < want; ++i) {
      const int cell = open[i];
      if (sea_used_.count(cell)) continue;
      const std::string_view model = Pick(kFinds);
      const Footprint& fp = Foot(model);
      bool fits = true;  // all on water, none near another
      for (int dr = -1; dr <= fp.w && fits; ++dr)
        for (int dc = -1; dc <= fp.h && fits; ++dc) {
          const int nb = Nb(cell, dr, dc);
          fits = nb >= 0 && ground_[nb] == WATER && !sea_used_.count(nb);
        }
      if (!fits) continue;
      for (int dr = -3; dr <= fp.w + 2; ++dr)
        for (int dc = -3; dc <= fp.h + 2; ++dc)
          if (const int nb = Nb(cell, dr, dc); nb >= 0) sea_used_.insert(nb);
      objects.push_back(MakeObject(model, R(cell), C(cell), 0));
      ++placed;
      // the better ones guarded: a sea creature's army beside it, on the water
      const bool rich = model == "water objects.Sea Chest" || model == "water objects.Shipwreck Survivor";
      if (rich || rng_.Real() < 0.25)
        for (const auto& d : kDir8) {
          const int nb = Nb(cell, d[0] * 2, d[1] * 2);
          if (nb >= 0 && ground_[nb] == WATER && !SeaTaken(nb)) {
            PlaceWaterArmy(nb, rich ? 3 : 2);
            break;
          }
        }
    }
    // whirlpools: a pair or two, far apart (a boat entering one leaves by another)
    const int pairs = size_ >= 228 ? 2 : size_ >= 152 ? 1 : 0;
    std::vector<int> pools;
    for (size_t i = 0; i < open.size() && int(pools.size()) < 2 * pairs; ++i) {
      const int cell = open[i];
      bool far = !SeaTaken(cell) && Clear(cell, 2);
      for (int p : pools) far = far && std::abs(R(p) - R(cell)) + std::abs(C(p) - C(cell)) > size_ / 3;
      if (!far) continue;
      for (int dr = -3; dr <= 4; ++dr)
        for (int dc = -3; dc <= 4; ++dc)
          if (const int nb = Nb(cell, dr, dc); nb >= 0) sea_used_.insert(nb);
      objects.push_back(MakeObject("movement modifiers.whirlpool", R(cell), C(cell), 0));
      pools.push_back(cell);
    }
    if (pools.size() % 2) {  // never one alone
      objects.pop_back();
    }
    // a few armies on their own
    for (size_t i = 0, n = open.size() / 900; i < open.size() && n; ++i)
      if (!SeaTaken(open[i])) {
        PlaceWaterArmy(open[i], 1 + int(rng_.Below(3)));
        --n;
      }
  }
  bool SeaTaken(int cell) const { return sea_used_.count(cell) > 0; }
  // The sea's creatures by a guard's strength: mermaids, pirates, water elementals, sea monsters.
  void PlaceWaterArmy(int cell, int level) {
    static const int kSea[5] = {0, 38, 50, 62, 52};
    Object o;
    o.r = R(cell);
    o.c = C(cell);
    o.lv = L(cell);
    o.army = true;
    o.body = CreatureArmy(kSea[std::clamp(level, 1, 4)]);
    objects.push_back(std::move(o));
    for (int dr = -1; dr <= 1; ++dr)
      for (int dc = -1; dc <= 1; ++dc)
        if (const int nb = Nb(cell, dr, dc); nb >= 0) sea_used_.insert(nb);
  }

  // A lighthouse on a shore (a zone with a shipyard): its owner's boats go further.
  void PlaceLighthouse() {
    std::vector<int> zs;
    for (int o : shipyards_) zs.push_back(zone_[At(objects[size_t(o)].r, objects[size_t(o)].c, 0)]);
    if (zs.empty()) return;
    const int z = rng_.Choice(zs);
    std::vector<int> shore;
    for (int cell : zone_cells_[z])
      if (state_[cell] == FREE)
        for (const auto& d : kDir4) {
          const int nb = Nb(cell, d[0], d[1]);
          if (nb >= 0 && ground_[nb] == WATER) {
            shore.push_back(cell);
            break;
          }
        }
    if (shore.empty()) return;
    std::vector<int> entries;
    const long obj = PlaceNear("movement modifiers.Lighthouse", z, rng_.Choice(shore), 2, entries);
    if (obj >= 0) items_[z].push_back({std::move(entries), 0, size_t(obj)});
  }

  // -- 4b. water: lakes on open ground, rivers between them and to the map's edge

  bool DeepWater(int cell) const {
    for (int dr = -2; dr <= 2; ++dr)
      for (int dc = -2; dc <= 2; ++dc) {
        const int nb = Nb(cell, dr, dc);
        if (nb >= 0 && ground_[nb] != WATER) return false;
      }
    return true;
  }

  // A lake: open cells only, a cell's ring of 8 open (or lake) too: the shore stays walkable, no
  // path or object touches the water.
  void MakeLakes() {
    auto clear = [&](int cell) {
      if (state_[cell] != FREE || L(cell) != 0) return false;
      for (const auto& d : kDir8) {
        const int nb = Nb(cell, d[0], d[1]);
        if (nb < 0 || (state_[nb] != FREE && state_[nb] != WALL && ground_[nb] != WATER)) return false;
      }
      return true;
    };
    for (int z = 0, again = 0; z < zones(); again = 0, ++z) {
    lake_again:
      if (ZoneLevel(z) != 0 || SeaZone(z) || rng_.Real() < (again ? 0.4 : start_[z] ? 0.4 : 0.1)) continue;
      std::vector<int> open;
      for (int cell : zone_cells_[z])
        if (clear(cell)) open.push_back(cell);
      if (open.size() < 60) continue;
      const size_t want = size_t(double(open.size()) * rng_.Uniform(start_[z] ? 0.07 : 0.12, start_[z] ? 0.14 : 0.32));
      const int seed = rng_.Choice(open);
      std::vector<int> lake{seed}, edge{seed};
      ground_[seed] = WATER;
      while (lake.size() < want && !edge.empty()) {  // grows round, a little ragged
        const size_t i = rng_.Below(uint32_t(edge.size()));
        const int from = edge[i];
        bool grew = false;
        for (const auto& d : kDir4) {
          const int nb = Nb(from, d[0], d[1]);
          if (nb < 0 || zone_[nb] != z || ground_[nb] == WATER || !clear(nb)) continue;
          if (rng_.Real() < 0.5) continue;
          ground_[nb] = WATER;
          lake.push_back(nb);
          edge.push_back(nb);
          grew = true;
        }
        if (!grew && rng_.Real() < 0.3) edge.erase(edge.begin() + long(i));
      }
      if (lake.size() < 12) {
        for (int cell : lake) ground_[cell] = kZoneGround;
        continue;
      }
      for (int cell : lake) state_[cell] = BLOCK;
      lakes_.push_back(std::move(lake));
      if (!again && open.size() > 900) {  // a big zone: maybe a second lake
        again = 1;
        goto lake_again;
      }
    }
  }

  // Rivers: from a lake (or the map's edge) to the edge, through open ground, paths (fords) and the
  // zones' borders; never under an object. Each its zone's kind: lava in volcanic land, ice in snow.
  void MakeRivers() {
    const int rivers = 2 * (size_ / 76) + int(rng_.Below(2));  // the shipped maps: ~2% of the map river
    const int reach = std::max(20, size_ / 5);  // a river's least length to a lake or the sea
    // a slow noise to wander by: random on a coarse grid, smoothed between its points
    struct Layer {
      int step, gw;
      std::vector<double> v;
    };
    std::vector<Layer> layers;
    for (int step : {12, 5, 2}) {
      Layer l{step, size_ / step + 2, {}};
      l.v.resize(size_t(l.gw * l.gw));
      for (auto& x : l.v) x = rng_.Real();
      layers.push_back(std::move(l));
    }
    auto noise = [&](int cell) {
      double sum = 0;
      for (const Layer& l : layers) {
        const double fr = double(R(cell)) / l.step, fc = double(C(cell)) / l.step;
        const int r0 = int(fr), c0 = int(fc);
        const double a = fr - r0, b = fc - c0;
        auto g = [&](int r, int c) { return l.v[size_t(r * l.gw + c)]; };
        sum += (g(r0, c0) * (1 - b) + g(r0, c0 + 1) * b) * (1 - a) +
               (g(r0 + 1, c0) * (1 - b) + g(r0 + 1, c0 + 1) * b) * a;
      }
      const double n = sum / double(layers.size());
      return n * n;
    };
    auto allowed = [&](int cell) {
      if (L(cell) != 0 || ground_[cell] == WATER) return false;
      return state_[cell] == FREE || state_[cell] == RES || state_[cell] == WALL;
    };
    std::vector<int> edge;  // the map's border cells
    for (int cell : cells_)
      if (L(cell) == 0 && allowed(cell))
        for (const auto& d : kDir4)
          if (Nb(cell, d[0], d[1]) < 0) {
            edge.push_back(cell);
            break;
          }
    std::vector<uint8_t> near_town(zone_.size());
    for (int g : town_gates_)
      for (int dr = -6; dr <= 6; ++dr)
        for (int dc = -6; dc <= 6; ++dc) {
          const int nb = Nb(g, dr, dc);
          if (nb >= 0) near_town[nb] = 1;
        }
    std::vector<uint8_t> lake_cell(zone_.size());
    for (const auto& lake : lakes_)
      for (int w : lake) lake_cell[w] = 1;
    for (int k = 0; k < rivers && !edge.empty(); ++k) {
      // from a lake's shore, or an edge cell, to the edge far from it
      int from = -1;
      if (!lakes_.empty() && rng_.Real() < 0.7) {
        const auto& lake = lakes_[rng_.Below(uint32_t(lakes_.size()))];
        for (int tries = 0; tries < 50 && from < 0; ++tries) {
          const int w = rng_.Choice(lake);
          for (const auto& d : kDir4) {
            const int nb = Nb(w, d[0], d[1]);
            if (nb >= 0 && allowed(nb) && !near_town[nb]) from = nb;
          }
        }
      }
      if (from < 0) from = rng_.Choice(edge);
      std::set<int> goal;
      for (int cell : edge)
        if (std::abs(R(cell) - R(from)) + std::abs(C(cell) - C(from)) > size_ / 2) goal.insert(cell);
      for (const auto& lake : lakes_)
        if (std::find(lake.begin(), lake.end(), from) == lake.end())
          for (int w : lake)
            for (const auto& d : kDir4) {
              const int nb = Nb(w, d[0], d[1]);
              if (nb >= 0 && allowed(nb) && std::abs(R(nb) - R(from)) + std::abs(C(nb) - C(from)) > reach) goal.insert(nb);
            }
      if (coast_)  // or the sea
        for (int cell : cells_)
          if (L(cell) == 0 && allowed(cell) && std::abs(R(cell) - R(from)) + std::abs(C(cell) - C(from)) > reach)
            for (const auto& d : kDir4) {
              const int nb = Nb(cell, d[0], d[1]);
              if (nb >= 0 && ground_[nb] == WATER && !lake_cell[nb]) {
                goal.insert(cell);
                break;
              }
            }
      if (goal.empty()) continue;
      // cheapest way, by the noise (low ground), around the towns
      std::map<int, double> dist;
      std::map<int, int> came;
      using Q = std::pair<double, int>;
      std::priority_queue<Q, std::vector<Q>, std::greater<Q>> open;
      dist[from] = 0;
      open.push({0, from});
      int end = -1;
      while (!open.empty()) {
        const auto [d0, cur] = open.top();
        open.pop();
        if (d0 > dist[cur]) continue;
        if (goal.count(cur)) {
          end = cur;
          break;
        }
        for (const auto& d : kDir4) {
          const int nb = Nb(cur, d[0], d[1]);
          if (nb < 0 || !allowed(nb) || near_town[nb]) continue;
          const double nd = d0 + 0.05 + noise(nb) * 8 + (state_[nb] == RES ? 2 : 0);
          auto it = dist.find(nb);
          if (it != dist.end() && nd >= it->second) continue;
          dist[nb] = nd;
          came[nb] = cur;
          open.push({nd, nb});
        }
      }
      if (end < 0) continue;
      std::vector<int> path;
      for (int cur = end;; cur = came[cur]) {
        path.push_back(cur);
        if (cur == from) break;
      }
      const int side = rng_.Below(2) ? 1 : -1;
      for (size_t i = 0; i < path.size(); ++i) {  // two wide: the cell and one beside it, across the flow
        const int cell = path[i];
        Paint(cell);
        const int next = path[i + 1 < path.size() ? i + 1 : i - 1];
        const int dr = R(next) - R(cell), dc = C(next) - C(cell);  // along: one of the 4 ways
        const int nb = Nb(cell, dc * side, dr * side);
        if (nb >= 0 && allowed(nb) && !near_town[nb]) Paint(nb);
      }
    }
  }
  void Paint(int cell) {
    const uint8_t t = terrain_[zone_[cell]];
    ground_[cell] = t == VOLCANIC ? LAVA_RIVER : t == SNOW ? ICE_RIVER : RIVER;
  }

  // The ground along the paths, kept clear: a cell to each side, and the two in front of it on the
  // screen (below it: a tree there stands up over the path and hides it). Not the zones' borders:
  // they stay closed (only the open ground's forests and obstacles keep off it).
  void MarkPathSides() {
    path_side_.assign(zone_.size(), 0);
    for (int cell : cells_)
      if (state_[cell] == RES)
        for (int dr = -1; dr <= 2; ++dr)
          for (int dc = -1; dc <= 2; ++dc)
            if (const int nb = Nb(cell, dr, dc); nb >= 0) path_side_[nb] = 1;
  }

  // forests: clumps on the open ground (as the caves' rock: random, then smoothed; 38% of it at
  // first: fewer long walls of trees), trees where the zone's land has them; none along the paths.
  void FillForests() {
    MarkShore();
    MarkPathSides();
    for (int z = 0; z < zones(); ++z) {
      if (ZoneLevel(z) != 0 || SeaZone(z)) continue;
      std::vector<int> free;
      for (int cell : zone_cells_[z])
        if (state_[cell] == FREE) free.push_back(cell);
      std::vector<uint8_t> want(free.size());
      for (auto& w : want) w = rng_.Real() < 0.38;
      std::map<int, size_t> index;
      for (size_t i = 0; i < free.size(); ++i) index[free[i]] = i;
      for (int pass = 0; pass < 4; ++pass) {
        std::vector<uint8_t> next(want.size());
        for (size_t i = 0; i < free.size(); ++i) {
          int solid = 0;
          for (const auto& d : kDir8) {
            const int nb = Nb(free[i], d[0], d[1]);
            const auto it = nb < 0 ? index.end() : index.find(nb);
            if (it != index.end()) solid += want[it->second];
            else solid += nb < 0 || state_[nb] == BLOCK || state_[nb] == WALL;
          }
          next[i] = solid >= 5 || (solid == 4 && want[i]);
        }
        want = std::move(next);
      }
      std::vector<int> forest;
      for (size_t i = 0; i < free.size(); ++i)
        if (want[i] && !shore_[free[i]] && !view_[free[i]] && !path_side_[free[i]]) {
          state_[free[i]] = WALL;
          forest.push_back(free[i]);
        }
      std::vector<const Footprint*> trees;
      for (const Footprint* fp : cat_.Obstacles(terrain_[z]))
        if (TreeLike(fp->model)) trees.push_back(fp);
      if (const uint8_t t = terrain_[z]; t == GRASS || t == DIRT || t == ROUGH || t == VOLCANIC)
        for (const Footprint* fp : cat_.Decor("trees.autumn."))
          if (rng_.Real() < 0.5) trees.push_back(fp);  // some autumn among the zone's own
      const auto& pool = trees.empty() ? cat_.Obstacles(terrain_[z]) : trees;
      rng_.Shuffle(forest);
      for (int cell : forest)
        if (state_[cell] == WALL && !PlaceObstacle(cell, WALL, pool, true)) PlaceObstacle(cell, WALL, pool, false);
      for (int cell : forest)
        if (state_[cell] == WALL) state_[cell] = FREE;
    }
  }

  // The ground in front of each town (on the screen: below it, where tall things would cover it)
  // and a cell round it: kept clear of trees.
  void MarkTownViews() {
    view_.assign(zone_.size(), 0);
    for (const Object& o : objects) {
      if (o.removed || (o.kind[0].second != "town" && o.kind[0].second != "random_town")) continue;
      const Footprint* fp = cat_.Find(o.model);
      const int w = fp ? fp->w : 5, h = fp ? fp->h : 5;
      for (int r = o.r - 1; r <= o.r + w + 3; ++r)
        for (int c = o.c - 1; c <= o.c + h + 3; ++c)
          if (const int cell = At(r, c, o.lv); cell >= 0) view_[cell] = 1;
    }
  }

  void PlaceLinkGuards() {
    for (const Link& l : links_) {
      const int a = l.zones[0], b = l.zones[1];
      if (l.strength == 0) continue;  // a template's open way
      const int level = l.strength > 0 ? std::min(4, l.strength)
                        : start_[a] && start_[b] ? 4
                                                 : std::min(4, 1 + std::max(tier_[a], tier_[b]));
      objects.push_back(MakeMonster(level, l.guard));
      guards_.insert(l.guard);
    }
  }

  // -- 5. obstacles

  // Covers `cell` with a fully blocking decoration whose footprint is all in `allowed` cells. In
  // front of a town (view_): nothing on open ground, and no tree on a zone's border (it would hide
  // the town), unless `spare_view` is false.
  bool PlaceObstacle(int cell, uint8_t allowed, const std::vector<const Footprint*>& pool, bool big,
                     bool spare_view = true) {
    std::vector<const Footprint*> models;
    if (big) models = pool;
    else
      for (const Footprint* f : pool)
        if (f->w * f->h == 1) models.push_back(f);
    for (int attempt = 0; attempt < (big ? 6 : 12); ++attempt) {
      if (models.empty()) return false;
      const Footprint& fp = *rng_.Choice(models);
      const int fx = int(rng_.Below(uint32_t(fp.w)));
      const int fy = int(rng_.Below(uint32_t(fp.h)));
      const int r0 = R(cell) - fx, c0 = C(cell) - fy, z = zone_[cell], lv = L(cell);
      bool fits = true;
      for (int y = 0; y < fp.h && fits; ++y)
        for (int x = 0; x < fp.w && fits; ++x) {
          const int nb = At(r0 + x, c0 + y, lv);
          fits = nb >= 0 && zone_[nb] == z && state_[nb] == allowed &&
                 !(allowed == FREE && !shore_.empty() && shore_[nb]) &&
                 !(allowed == FREE && !path_side_.empty() && path_side_[nb]) &&
                 !(spare_view && !view_.empty() && view_[nb] && (allowed == FREE || TreeLike(fp.model)));
        }
      if (!fits) continue;
      for (int y = 0; y < fp.h; ++y)
        for (int x = 0; x < fp.w; ++x) state_[At(r0 + x, c0 + y, lv)] = BLOCK;
      objects.push_back(MakeObject(fp.model, r0, c0, lv));
      return true;
    }
    return false;
  }

  void FillWalls() {
    std::vector<int> walls;
    for (int cell : cells_)
      if (state_[cell] == WALL) walls.push_back(cell);
    rng_.Shuffle(walls);
    if (islands_) MarkShore();
    for (int cell : walls) {
      if (state_[cell] != WALL) continue;
      if (islands_ && L(cell) == 0 && shore_[cell]) {  // islands: no land border, keep the shore open
        state_[cell] = FREE;
        continue;
      }
      if (L(cell) == 1) {  // underground: solid rock
        rock_[cell] = 1;
        state_[cell] = BLOCK;
        continue;
      }
      const auto& pool = cat_.Obstacles(terrain_[zone_[cell]]);
      if (!PlaceObstacle(cell, WALL, pool, true) && !PlaceObstacle(cell, WALL, pool, false) &&
          !PlaceObstacle(cell, WALL, cat_.Obstacles(GRASS), false))  // (the border closed above all)
        PlaceObstacle(cell, WALL, pool, false, false);
    }
    int left = 0;
    for (int cell : cells_) left += state_[cell] == WALL;
    if (left) log.push_back(std::to_string(left) + " wall cells left open");
  }

  // The land along the sea (not the lakes): open, two cells deep.
  void MarkShore() {
    shore_.assign(zone_.size(), 0);
    std::vector<uint8_t> lake(zone_.size());
    for (const auto& l : lakes_)
      for (int w : l) lake[w] = 1;
    for (int cell : cells_) {
      if (L(cell) != 0 || ground_[cell] == WATER) continue;
      for (int dr = -2; dr <= 2 && !shore_[cell]; ++dr)
        for (int dc = -2; dc <= 2; ++dc) {
          const int nb = Nb(cell, dr, dc);
          if (nb >= 0 && ground_[nb] == WATER && !lake[nb]) {
            shore_[cell] = 1;
            break;
          }
        }
    }
  }

  void FillInterior(double density = 0.14) {
    for (int z = 0; z < int(zone_cells_.size()); ++z) {
      if (SeaZone(z)) continue;
      std::vector<int> free;
      for (int cell : zone_cells_[z])
        if (state_[cell] == FREE) free.push_back(cell);
      if (ZoneLevel(z) == 1) FillRock(free, 0.5);  // a cave: mostly rock between its ways
      const int target = int(free.size() * (ZoneLevel(z) == 1 ? 0.06 : density));
      const std::vector<const Footprint*> pool = InteriorPool(terrain_[z]);
      int placed = 0, tries = 0;
      rng_.Shuffle(free);
      std::vector<int> seeds = free, recent;
      while (placed < target && tries < target * 6 && !seeds.empty()) {
        ++tries;
        int cell;
        if (!recent.empty() && rng_.Real() < 0.6) {
          const int from = rng_.Choice(recent);
          const auto& d = kDir8[rng_.Below(8)];
          cell = Nb(from, d[0], d[1]);
        } else {
          cell = seeds.back();
          seeds.pop_back();
        }
        if (cell < 0 || state_[cell] != FREE || zone_[cell] != z) continue;
        if (PlaceObstacle(cell, FREE, pool, true)) {
          ++placed;
          recent.push_back(cell);
          if (recent.size() > 6) recent.erase(recent.begin());
        }
      }
    }
  }

  // -- 5b. decorations, as the shipped maps have them (where and what: table in their survey)

  // The blocking things of a terrain for the open ground: its obstacles, and those of the
  // decoration groups the shipped maps put on it (dunes, lava's cracks, autumn trees, cave trees).
  std::vector<const Footprint*> InteriorPool(uint8_t t) const {
    std::vector<const Footprint*> pool = cat_.Obstacles(t);
    auto add = [&](const char* group) {
      for (const Footprint* fp : cat_.Decor(group))
        if (fp->blocked_count) pool.push_back(fp);
    };
    if (t == GRASS || t == DIRT || t == ROUGH || t == VOLCANIC) add("trees.autumn.");
    if (t == SAND) add("dunes.sand.");
    if (t == SNOW) add("dunes.snow.");
    if (t == VOLCANIC) add("cracks-holes.lava.");
    if (t == SUBTERRANEAN) add("trees.subterranean.");
    return pool;
  }

  bool NearWater(int cell, int radius) const {
    for (int dr = -radius; dr <= radius; ++dr)
      for (int dc = -radius; dc <= radius; ++dc) {
        const int nb = Nb(cell, dr, dc);
        if (nb >= 0 && (ground_[nb] == WATER || ground_[nb] == RIVER || ground_[nb] == LAVA_RIVER ||
                        ground_[nb] == ICE_RIVER))
          return true;
      }
    return false;
  }

  // Ruins: short lines of wall or fence on the open ground. (Not the worked-out mines: their
  // footprint is a ring round 7 x 7 cells, which it would shut in; no shipped map has one.)
  void FillRuins() {
    for (int z = 0; z < zones(); ++z) {
      if (SeaZone(z)) continue;
      const uint8_t t = terrain_[z];
      const char* group = t == VOLCANIC ? "walls.evil." : t == GRASS || t == DIRT ? "walls.fence."
                          : t == SNOW || t == SWAMP || t == ROUGH ? "walls.regular." : nullptr;
      if (!group || ZoneLevel(z) == 1) continue;
      const auto& pieces = cat_.Decor(group);
      if (pieces.empty()) continue;
      int open = 0;
      for (int cell : zone_cells_[z]) open += state_[cell] == FREE;
      for (int n = int(open / 1000.0 * 0.8 + rng_.Real()); n-- > 0;) {
        int cell = RandomSpot(z);
        if (cell < 0) break;
        const auto& d = kDir4[rng_.Below(4)];
        const int len = 2 + int(rng_.Below(3));
        std::vector<int> line;
        for (int i = 0; i < len && cell >= 0 && state_[cell] == FREE && zone_[cell] == z && !shore_[cell] &&
                         !path_side_[cell]; ++i) {
          line.push_back(cell);
          cell = Nb(cell, d[0], d[1]);
        }
        if (line.size() < 2) continue;
        for (int c : line) {
          state_[c] = BLOCK;
          objects.push_back(MakeObject(rng_.Choice(pieces)->model, R(c), C(c), L(c)));
        }
      }
    }
  }

  // A passable decoration over `cell` (any of its cells), on open ground of the zone not yet decorated.
  bool PlaceDecor(int cell, const std::vector<const Footprint*>& pool, bool water) {
    if (pool.empty()) return false;
    for (int attempt = 0; attempt < 4; ++attempt) {
      const Footprint& fp = *rng_.Choice(pool);
      const int r0 = R(cell) - int(rng_.Below(uint32_t(fp.w))), c0 = C(cell) - int(rng_.Below(uint32_t(fp.h)));
      const int lv = L(cell), z = zone_[cell];
      bool fits = true;
      for (int y = 0; y < fp.h && fits; ++y)
        for (int x = 0; x < fp.w && fits; ++x) {
          const int nb = At(r0 + x, c0 + y, lv);
          fits = nb >= 0 && !decor_[nb] &&
                 (water ? ground_[nb] == WATER && !SeaTaken(nb)
                        : zone_[nb] == z && state_[nb] == FREE && ground_[nb] == kZoneGround && !rock_[nb]);
        }
      if (!fits) continue;
      for (int y = 0; y < fp.h; ++y)
        for (int x = 0; x < fp.w; ++x) decor_[At(r0 + x, c0 + y, lv)] = 1;
      objects.push_back(MakeObject(fp.model, r0, c0, lv));
      return true;
    }
    return false;
  }

  // Grass, bushes, fields by the towns, reeds by the water, stumps: passable, in clumps.
  void FillDecor() {
    decor_.assign(zone_.size(), 0);
    auto group = [&](std::initializer_list<const char*> names) {
      std::vector<const Footprint*> out;
      for (const char* g : names)
        for (const Footprint* fp : cat_.Decor(g))
          if (!fp->blocked_count) out.push_back(fp);
      return out;
    };
    const auto reeds = group({"shrubs.reeds.", "shrubs.cattails."});
    for (int z = 0; z < zones(); ++z) {
      if (SeaZone(z)) continue;
      const uint8_t t = terrain_[z];
      std::vector<const Footprint*> pool;
      switch (t) {
        case GRASS: pool = group({"shrubs.grass.", "shrubs.rough.", "shrubs.snow tufts.", "shrubs.vines.", "stumps."}); break;
        case ROUGH: pool = group({"shrubs.brush.", "shrubs.rough."}); break;
        case SWAMP: pool = group({"shrubs.reeds.", "shrubs.cattails.", "shrubs.grass."}); break;
        case VOLCANIC: pool = group({"shrubs.brush.", "cracks-holes.lava."}); break;
        case SNOW: pool = group({"shrubs.snow.", "shrubs.snow tufts.", "dunes.snow.", "stumps."}); break;
        case SAND: pool = group({"shrubs.brush.", "dunes.sand."}); break;
        case DIRT: pool = group({"shrubs.brush.", "shrubs.grass.", "shrubs.snow.", "stumps."}); break;
        default: pool = group({"shrubs.vines."}); break;
      }
      std::vector<int> free;
      for (int cell : zone_cells_[z])
        if (state_[cell] == FREE && !rock_[cell]) free.push_back(cell);
      rng_.Shuffle(free);
      // fields round the zone's town
      const auto fields = group({t == SNOW ? "farms.snow." : "farms.green."});
      if (hubs_[z] >= 0 && start_[z] && t != SUBTERRANEAN && t != SAND)
        for (int i = 0, n = 2 + int(rng_.Below(3)); i < 40 && n > 0; ++i) {
          const int r = R(hubs_[z]) + int(rng_.Below(15)) - 7, c = C(hubs_[z]) + int(rng_.Below(15)) - 7;
          const int cell = At(r, c, L(hubs_[z]));
          if (cell >= 0 && zone_[cell] == z && PlaceDecor(cell, fields, false)) --n;
        }
      const int target = int(free.size() * 0.09);
      std::vector<int> recent;
      size_t next = 0;
      for (int placed = 0, tries = 0; placed < target && tries < target * 6 && next < free.size(); ++tries) {
        int cell;
        if (!recent.empty() && rng_.Real() < 0.55) {
          const auto& d = kDir8[rng_.Below(8)];
          cell = Nb(rng_.Choice(recent), d[0] * 2, d[1] * 2);
        } else {
          cell = free[next++];
        }
        if (cell < 0 || zone_[cell] != z || state_[cell] != FREE) continue;
        const bool wet = !reeds.empty() && L(cell) == 0 && NearWater(cell, 1) && rng_.Real() < 0.7;
        if (PlaceDecor(cell, wet ? reeds : pool, false)) {
          ++placed;
          recent.push_back(cell);
          if (recent.size() > 5) recent.erase(recent.begin());
        }
      }
    }
  }

  // On the water: lily pads and kelp along the shores (passable), rocks, ice floes and icebergs on
  // the open water (each with water all round it, away from the shipyards: the boats get by).
  void FillWaterDecor() {
    if (decor_.empty()) decor_.assign(zone_.size(), 0);
    auto group = [&](std::initializer_list<const char*> names, bool blocking) {
      std::vector<const Footprint*> out;
      for (const char* g : names)
        for (const Footprint* fp : cat_.Decor(g))
          if (bool(fp->blocked_count) == blocking) out.push_back(fp);
      return out;
    };
    const auto weeds = group({"water objects.lily pads.", "water objects.kelp."}, false);
    const auto cold_weeds = group({"water objects.kelp."}, false);
    const auto rocks = group({"rocks.water."}, true);
    const auto ice = group({"water objects.ice flows.", "water objects.iceberg."}, true);
    std::vector<int> water;
    for (int cell : cells_)
      if (L(cell) == 0 && ground_[cell] == WATER) water.push_back(cell);
    if (water.empty()) return;
    rng_.Shuffle(water);
    auto near_yard = [&](int cell) {
      for (int o : shipyards_)
        if (!objects[size_t(o)].removed &&
            std::abs(objects[size_t(o)].r - R(cell)) + std::abs(objects[size_t(o)].c - C(cell)) < 8)
          return true;
      return false;
    };
    auto cold = [&](int cell) { return zone_[cell] >= 0 && terrain_[zone_[cell]] == SNOW; };
    // the shores' weeds
    const int weeds_want = int(water.size() * 0.012);
    for (size_t i = 0, n = 0; i < water.size() && int(n) < weeds_want; ++i) {
      const int cell = water[i];
      bool shore = false;
      for (const auto& d : kDir8)
        if (const int nb = Nb(cell, d[0] * 2, d[1] * 2); nb >= 0 && ground_[nb] != WATER) shore = true;
      if (shore && PlaceDecor(cell, cold(cell) ? cold_weeds : weeds, true)) ++n;
    }
    // the open water's rocks and ice
    const int rocks_want = int(water.size() * 0.004);
    for (size_t i = 0, n = 0; i < water.size() && int(n) < rocks_want; ++i) {
      const int cell = water[i];
      if (SeaTaken(cell) || near_yard(cell)) continue;
      const auto& pool = cold(cell) && !ice.empty() ? ice : rocks;
      if (pool.empty()) continue;
      const Footprint& fp = *rng_.Choice(pool);
      const int r0 = R(cell), c0 = C(cell);
      bool fits = true;  // water two cells all round it, nothing there
      for (int x = -2; x < fp.w + 2 && fits; ++x)
        for (int y = -2; y < fp.h + 2 && fits; ++y) {
          const int nb = At(r0 + x, c0 + y, 0);
          fits = nb >= 0 && ground_[nb] == WATER && !SeaTaken(nb) && !decor_[nb];
        }
      if (!fits) continue;
      for (int x = -2; x < fp.w + 2; ++x)
        for (int y = -2; y < fp.h + 2; ++y) {
          const int nb = At(r0 + x, c0 + y, 0);
          sea_used_.insert(nb);
          if (x >= 0 && y >= 0 && x < fp.w && y < fp.h) state_[nb] = BLOCK;
        }
      objects.push_back(MakeObject(fp.model, r0, c0, 0));
      ++n;
    }
  }

  // Turns about `share` of the zone's free cells into solid rock, as caves: rock at random, then
  // smoothed (a cell is rock when most of its 8 neighbours are: a cellular automaton). Only free
  // cells change: the paths and what is along them stay open. `free` keeps those left free.
  void FillRock(std::vector<int>& free, double share) {
    std::vector<uint8_t> want(free.size());
    for (auto& w : want) w = rng_.Real() < share;
    std::map<int, size_t> index;
    for (size_t i = 0; i < free.size(); ++i) index[free[i]] = i;
    for (int pass = 0; pass < 4; ++pass) {
      std::vector<uint8_t> next(want.size());
      for (size_t i = 0; i < free.size(); ++i) {
        int solid = 0;
        for (const auto& d : kDir8) {
          const int nb = Nb(free[i], d[0], d[1]);
          if (nb < 0 || zone_[nb] != zone_[free[i]]) {
            ++solid;  // the zone's edge: rock
            continue;
          }
          const auto it = index.find(nb);
          if (it != index.end()) solid += want[it->second];
          else solid += state_[nb] == BLOCK || state_[nb] == WALL;
        }
        next[i] = solid >= 5 || (solid == 4 && want[i]);
      }
      want = std::move(next);
    }
    for (size_t i = 0; i < free.size(); ++i)
      if (want[i]) {
        rock_[free[i]] = 1;
        state_[free[i]] = BLOCK;
      }
    std::erase_if(free, [&](int cell) { return state_[cell] != FREE; });
  }

  // -- 6. tiles

  std::vector<Tile> BuildGrid() {
    std::vector<uint8_t> road(zone_.size());
    for (const auto& [z, path] : main_paths_) {
      const uint8_t kind = start_[z] ? kRoadCobble : kRoadDirt;
      for (int cell : path) road[cell] = std::max(road[cell], kind);
    }
    for (const Link& l : links_)
      for (int cell : l.corridor)
        if (!road[cell]) road[cell] = kRoadDirt;
    for (const auto& [z, path] : road_paths_) {
      const uint8_t kind = start_[z] ? kRoadCobble : kRoadDirt;
      for (int cell : path) road[cell] = std::max(road[cell], kind);
    }
    std::vector<Tile> tiles;
    tiles.reserve(cells_.size());
    for (int cell : cells_) {
      Tile t;
      t.terrain = rock_[cell] ? SUBTERRANEAN : ground_[cell] != kZoneGround ? ground_[cell] : terrain_[zone_[cell]];
      if (t.terrain == WATER) t.subtype = DeepWater(cell) ? 1 : 0;
      t.road = road[cell];
      t.rock = rock_[cell];
      tiles.push_back(std::move(t));
    }
    // Bridges where a road crosses a river two wide, straight across.
    auto river = [&](int cell) { return cell >= 0 && ground_[cell] >= RIVER && ground_[cell] <= ICE_RIVER; };
    auto road_at = [&](int cell) { return cell >= 0 && road[cell] && !river(cell); };
    std::set<int> bridged;
    for (int cell : cells_) {
      if (L(cell) != 0 || !river(cell) || !road[cell] || bridged.count(cell)) continue;
      const int down = Nb(cell, 1, 0), right = Nb(cell, 0, 1);
      if (river(down) && road[down] && !bridged.count(down) && road_at(Nb(cell, -1, 0)) && road_at(Nb(cell, 2, 0))) {
        objects.push_back(MakeObject("movement modifiers.Bridge.wood.L", R(cell), C(cell), 0));  // 2 x 1
        bridged.insert({cell, down});
      } else if (river(right) && road[right] && !bridged.count(right) && road_at(Nb(cell, 0, -1)) &&
                 road_at(Nb(cell, 0, 2))) {
        objects.push_back(MakeObject("movement modifiers.Bridge.wood.R", R(cell), C(cell), 0));  // 1 x 2
        bridged.insert({cell, right});
      }
    }
    // A cave's edge: its rock cells next to open ground, each with its wall.
    for (size_t i = 0; i < cells_.size(); ++i) {
      const int cell = cells_[i];
      if (!rock_[cell]) continue;
      unsigned bits = 0;
      for (int k = 0; k < 8; ++k) {
        const int nb = Nb(cell, kWallDirs[k][0], kWallDirs[k][1]);
        if (nb < 0 || rock_[nb]) bits |= 1u << k;
      }
      if (bits == 0xFF) continue;
      tiles[i].rock_transition = 1;
      const auto& variants = cat_.CaveWalls(kCaveWall[bits]);
      if (variants.empty()) throw Failure{"the game's data have no cave walls"};
      objects.push_back(MakeObject(rng_.Choice(variants)->model, R(cell), C(cell), L(cell)));
    }
    // Each level blended on its own (cells_: level by level).
    const size_t per = cells_.size() / size_t(levels_);
    for (int lv = 0; lv < levels_; ++lv)
      if (!BuildSurfaceTransitions(size_, seed_ + 1 + uint32_t(lv), std::span<Tile>(tiles).subspan(per * lv, per)))
        throw Failure{"the terrain could not be blended"};
    return tiles;
  }

  // Every town reaches the others (through the guards); no start zone can be left past them.
  std::vector<std::string> Check() const {
    auto reach = [&](int start, bool through_guards) {
      std::vector<uint8_t> seen(zone_.size());
      std::vector<int> stack{start};
      seen[start] = 1;
      while (!stack.empty()) {
        const int cur = stack.back();
        stack.pop_back();
        for (const auto& d : kDir8) {
          const int nb = Nb(cur, d[0], d[1]);
          if (nb < 0 || seen[nb]) continue;
          const bool sail = islands_ && ground_[nb] == WATER;  // by boat: every island has a shipyard
          if (!sail && !Walkable(state_[nb])) continue;
          if (!through_guards && guards_.count(nb)) continue;
          seen[nb] = 1;
          stack.push_back(nb);
        }
        const auto j = jumps_.find(cur);  // through a passage to the other level
        if (j != jumps_.end())
          for (const Jump& to : j->second) {
            if (seen[to.to] || (!through_guards && to.guarded)) continue;
            seen[to.to] = 1;
            stack.push_back(to.to);
          }
      }
      return seen;
    };
    std::vector<std::string> problems;
    if (town_gates_.size() != size_t(players_)) problems.push_back("a player has no town");
    if (town_gates_.empty()) return problems;
    const auto full = reach(town_gates_[0], true);
    for (size_t i = 0; i < town_gates_.size(); ++i)
      if (!full[town_gates_[i]]) problems.push_back("town " + std::to_string(i) + " cannot be reached");
    for (long o : shipyards_)
      if (objects[size_t(o)].removed) problems.push_back("a shipyard cannot be reached");
    if (islands_) return problems;  // a start island is left by boat, past no guard
    for (int g : town_gates_) {
      const auto sealed = reach(g, false);
      for (int cell : cells_)
        if (sealed[cell] && zone_[cell] != zone_[g]) {
          problems.push_back("start zone " + std::to_string(zone_[g]) + " can be left past its guards");
          break;
        }
    }
    return problems;
  }
};

}  // namespace

MapSetup ResolvePlayers(const MapSetup& in, int min_size, int max_size) {
  MapSetup s = in;
  auto draw = [&](uint32_t salt, int n) {
    uint32_t h = s.seed * 2654435761u ^ salt;
    h ^= h >> 15;
    h *= 2246822519u;
    h ^= h >> 13;
    return int(h % uint32_t(n));
  };
  if (s.random_size) {
    std::vector<int> sizes;
    for (int z : kMapSizes)
      if (z >= min_size && z <= max_size) sizes.push_back(z);
    if (!sizes.empty()) s.size = sizes[size_t(draw(0x73697A, int(sizes.size())))];
  }
  if (s.random_underground) s.underground = draw(0x756E64, 2) == 1;
  if (!s.random_humans && !s.random_computers) return s;
  std::vector<PlayerSetup> humans, computers;
  for (const PlayerSetup& p : s.players) (p.human ? humans : computers).push_back(p);
  int h = std::max(1, int(humans.size())), k = int(computers.size());
  if (s.random_humans && s.random_computers) {
    const int total = 2 + draw(0x746F74, 5);  // 2 to 6, one computer at least
    h = 1 + draw(0x68756D, total - 1);
    k = total - h;
  } else if (s.random_humans) {
    const int lo = std::max(1, 2 - k), hi = 6 - k;
    h = lo + draw(0x68756D, hi - lo + 1);
  } else {
    const int lo = std::max(0, 2 - h), hi = 6 - h;
    k = lo + draw(0x636F6D, hi - lo + 1);
  }
  humans.resize(size_t(h));
  PlayerSetup cpu;
  cpu.human = false;
  computers.resize(size_t(k), cpu);
  s.players = humans;
  s.players.insert(s.players.end(), computers.begin(), computers.end());
  for (PlayerSetup& p : s.players)
    if (p.team >= int(s.players.size())) p.team = -1;
  bool rivals = false;
  for (size_t i = 1; i < s.players.size(); ++i) rivals |= TeamOf(s.players, i) != TeamOf(s.players, 0);
  if (!rivals)
    for (PlayerSetup& p : s.players) p.team = -1;
  return s;
}

bool GenerateMap(const Catalog& catalog, const MapSetup& asked, GeneratedMap& out, std::string* error,
                 const std::atomic<bool>* cancel) {
  int min_size = 76, max_size = 304;  // a random size: one the template's variants are for
  if (asked.random_size && !asked.template_text.empty()) {
    std::vector<Template> ts;
    if (ParseTemplates(asked.template_text, ts, nullptr)) {
      min_size = 304, max_size = 76;
      for (const Template& t : ts)
        if (t.name == asked.template_name)
          min_size = std::min(min_size, t.min_size), max_size = std::max(max_size, t.max_size);
      if (min_size > max_size) min_size = 76, max_size = 304;
    }
  }
  const MapSetup setup = ResolvePlayers(asked, min_size, max_size);
  auto fail = [&](const std::string& e) {
    if (error) *error = e;
    return false;
  };
  if (std::find(std::begin(kMapSizes), std::end(kMapSizes), setup.size) == std::end(kMapSizes))
    return fail("unknown map size");
  if (setup.players.size() < 2 || setup.players.size() > 6) return fail("2 to 6 players");
  bool human = false;
  for (const PlayerSetup& p : setup.players) {
    human |= p.human;
    if (p.faction < -1 || p.faction >= kFactionCount) return fail("unknown faction");
  }
  if (!human) return fail("at least one player must be human");
  bool rivals = false;
  for (size_t i = 1; i < setup.players.size(); ++i) rivals |= TeamOf(setup.players, i) != TeamOf(setup.players, 0);
  if (!rivals) return fail("all players are in one team");
  const std::string title = setup.title.empty() ? "_Random_Map_" + std::to_string(setup.seed) : setup.title;
  Template tpl;
  const bool use_template = !setup.template_name.empty() && setup.template_name != kClassicTemplate;
  std::string terr;
  if (use_template &&
      !MakeTemplate(setup.template_name, setup.template_text, int(setup.players.size()), setup.size, tpl, &terr))
    return fail(terr);
  const std::string description = DescribeSetup(catalog, setup);
  out = {};
  out.title = title;
  for (int attempt = 0; attempt < 10; ++attempt) {
    const uint32_t seed = attempt ? setup.seed + uint32_t(attempt) * 7919u : setup.seed;
    Generator gen(catalog, setup, seed, cancel, use_template ? &tpl : nullptr);
    std::vector<std::string> problems;
    std::vector<uint8_t> file;
    try {
      problems = gen.Generate(file, title, description);
    } catch (const Failure& f) {
      if (f.what == "cancelled") return fail("cancelled");
      problems = {f.what};
    }
    out.attempts = attempt + 1;
    for (const std::string& l : gen.log) out.log.push_back(l);
    if (problems.empty()) {
      out.file = std::move(file);
      out.objects = gen.written();
      out.zones = gen.zones();
      return true;
    }
    std::string line = "attempt " + std::to_string(attempt + 1) + ":";
    for (const std::string& p : problems) line += " " + p + ";";
    out.log.push_back(line + " retrying");
  }
  return fail("no valid map in 10 attempts");
}

}  // namespace h4::random_map
