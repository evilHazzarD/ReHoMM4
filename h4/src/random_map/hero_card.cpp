#include "hero_card.h"

#include <algorithm>
#include <cctype>
#include <cstdlib>
#include <cstring>
#include <iterator>

#include "catalog.h"
#include "resources.h"

namespace h4::random_map {
namespace {

std::string Lower(std::string s) {
  for (char& ch : s) ch = char(std::tolower(static_cast<unsigned char>(ch)));
  return s;
}

// Skill numbers: the 9 primary skills, then their secondary ones group by group; the icon sheet
// of each group (layers.icons.skills.<sheet>.52.h4d).
const char* const kPrimary[9] = {"tactics", "toughness", "scouting", "nobility", "life",
                                 "order",   "death",     "chaos",    "nature"};
const char* const kSheet[9] = {"tactics", "combat", "scouting", "nobility", "life",
                               "order",   "death",  "chaos",    "nature"};
const char* const kSecondary[9][3] = {
    {"offense", "defense", "leadership"},     {"combat", "archery", "resistance"},
    {"pathfinding", "seamanship", "stealth"}, {"estates", "mining", "diplomacy"},
    {"healing", "spirit", "resurrection"},    {"enchantment", "mind", "charm"},
    {"black", "demonology", "necromancy"},    {"conjuration", "pyromancy", "sorcery"},
    {"herbalism", "meditation", "summoning"}};

std::string SkillKeyword(int n) {
  if (n < 9) return kPrimary[n];
  return kSecondary[(n - 9) / 3][(n - 9) % 3];
}
int SkillGroup(const std::string& skill) {
  for (int g = 0; g < 9; ++g) {
    if (skill == kPrimary[g]) return g;
    for (const char* s : kSecondary[g])
      if (skill == s) return g;
  }
  return -1;
}

// heroes4.exe's class table, in Catalog's class order: the starting skill numbers.
struct ClassData {
  const char* title;
  std::vector<int> skills;
};
const ClassData kClassData[] = {
    {"Knight", {0, 10}},      {"Priest", {4, 21}},       {"Lord", {3, 18}},
    {"Mage", {5, 24}},        {"Death Knight", {0, 9}},  {"Necromancer", {6, 27}},
    {"Thief", {2, 17}},       {"Sorcerer", {7, 30}},     {"Archer", {1, 13}},
    {"Druid", {8, 33}},       {"Barbarian", {1, 12, 14}}};

int Int(const std::string& s) { return std::atoi(s.c_str()); }

uint16_t U16(const std::vector<uint8_t>& d, size_t p) { return uint16_t(d[p] | d[p + 1] << 8); }
uint32_t U32(const std::vector<uint8_t>& d, size_t p) {
  return uint32_t(d[p]) | uint32_t(d[p + 1]) << 8 | uint32_t(d[p + 2]) << 16 | uint32_t(d[p + 3]) << 24;
}

}  // namespace

std::map<std::string, Image> ReadLayers(const std::vector<uint8_t>& d) {
  std::map<std::string, Image> out;
  auto have = [&](size_t p, size_t n) { return p <= d.size() && n <= d.size() - p; };
  if (!have(0, 2)) return out;
  const int count = U16(d, 0);
  size_t p = 2;
  for (int layer = 0; layer < count; ++layer) {
    if (!have(p, 4)) break;
    const int npal = U16(d, p), block = U16(d, p + 2);
    p += 4;
    if (!have(p, size_t(npal) * 3 + 2)) break;
    const size_t pal = p;
    p += size_t(npal) * 3;
    const size_t k = U16(d, p);
    if (!have(p + 2, k + 17)) break;
    std::string name(reinterpret_cast<const char*>(&d[p + 2]), k);
    p += 2 + k;
    const int depth = d[p];
    const int left = int32_t(U32(d, p + 1)), top = int32_t(U32(d, p + 5)), right = int32_t(U32(d, p + 9)),
              bottom = int32_t(U32(d, p + 13));
    p += 17;
    const int w = std::max(0, right - left), h = std::max(0, bottom - top);
    if (w > 4096 || h > 4096 || !have(p, size_t(h) * 8)) break;
    const size_t rows = p;
    p += size_t(h) * 8;
    size_t n = 0;
    if (h) {
      const size_t last = rows + size_t(h - 1) * 8;
      n = U32(d, last + 4) + U16(d, last + 2) - U16(d, last);
    }
    if (!have(p, n)) break;
    const size_t pix = p;
    p += n;
    size_t alpha = 0;
    if (depth >= 4 && n) {
      alpha = p;
      p += (n + 1) / 2;
      if (block) p += (n + 131) / 128;
      if (p > d.size()) break;
    }
    Image img;
    img.w = w;
    img.h = h;
    img.argb.assign(size_t(w) * h, 0);
    for (int y = 0; y < h; ++y) {
      const size_t r = rows + size_t(y) * 8;
      const int x0 = U16(d, r), x1 = U16(d, r + 2);
      const size_t off = U32(d, r + 4);
      for (int i = 0; i < x1 - x0 && x0 + i < w; ++i) {
        const size_t at = off + size_t(i);
        if (at >= n) break;
        const int idx = d[pix + at];
        uint32_t a;
        if (depth >= 4) {
          const uint8_t b = d[alpha + (at >> 1)];
          a = uint32_t((at & 1 ? b >> 4 : b & 15) * 17);
        } else {
          a = idx ? 255 : 0;
        }
        if (idx >= npal) continue;
        const uint8_t* c = &d[pal + size_t(idx) * 3];
        img.argb[size_t(y) * w + x0 + i] = a << 24 | uint32_t(c[2]) << 16 | uint32_t(c[1]) << 8 | c[0];
      }
    }
    out.emplace(Lower(name), std::move(img));
  }
  return out;
}

bool HeroArt::Load(const Resources& res) {
  res_ = &res;
  std::vector<uint8_t> d;
  if (res.Read("table.skills.h4d", d))
    for (const auto& row : ReadTable(d))
      if (row.size() > 1 && !row[0].empty()) skill_names_[Lower(row[0])] = row[1];
  if (res.Read("table.creatures.h4d", d)) {
    const auto rows = ReadTable(d);
    if (rows.size() > 2) {
      const auto& head = rows[1];
      auto column = [&](const char* name) {
        for (size_t i = 0; i < head.size(); ++i)
          if (head[i] == name) return int(i);
        return -1;
      };
      const int c_plural = column("Plural Name"), c_level = column("Level"), c_attack = column("Attack"),
                c_defense = column("Defense"), c_hp = column("Hit Points"), c_low = column("Low"),
                c_high = column("High"), c_speed = column("Speed"), c_move = column("Move");
      for (size_t r = 2; r < rows.size(); ++r) {
        const auto& row = rows[r];
        if (row.empty() || row[0].empty()) continue;
        auto cell = [&](int c) { return c >= 0 && size_t(c) < row.size() ? row[size_t(c)] : std::string(); };
        Creature c;
        c.plural = cell(c_plural);
        c.level = Int(cell(c_level));
        c.attack = Int(cell(c_attack));
        c.defense = Int(cell(c_defense));
        c.hit_points = Int(cell(c_hp));
        c.low = Int(cell(c_low));
        c.high = Int(cell(c_high));
        c.speed = Int(cell(c_speed));
        c.move = Int(cell(c_move));
        creatures_.emplace(Lower(row[0]), std::move(c));
      }
    }
  }
  return !skill_names_.empty() && !creatures_.empty();
}

std::vector<std::string> HeroArt::ClassSkills(int hero_class) {
  std::vector<std::string> out;
  if (hero_class >= 0 && hero_class < int(std::size(kClassData)))
    for (int n : kClassData[hero_class].skills) out.push_back(SkillKeyword(n));
  return out;
}

const char* HeroArt::ClassTitle(int hero_class) {
  return hero_class >= 0 && hero_class < int(std::size(kClassData)) ? kClassData[hero_class].title : "";
}

std::vector<std::pair<std::string, int>> HeroArt::Army(int faction) {
  switch (faction) {
    case kLife: return {{"squire", 16}, {"crossbowman", 6}};
    case kOrder: return {{"dwarf", 14}, {"halfling", 9}};
    case kDeath: return {{"skeleton", 19}, {"imp", 11}};
    case kChaos: return {{"orc", 12}, {"bandit", 10}};
    case kNature: return {{"wolf", 11}, {"sprite", 9}};
    case kMight: return {{"berserker", 14}, {"centaur", 4}};
    default: return {};
  }
}

const char* const HeroArt::kStats[kStatCount] = {"melee_attack",   "ranged_attack", "spell_points",
                                                 "melee_defense",  "ranged_defense", "hit_points",
                                                 "damage",         "speed",          "movement"};
const char* const HeroArt::kStatTitles[kStatCount] = {"Melee Attack", "Ranged Attack", "Spell Points",
                                                      "Melee Defense", "Ranged Defense", "Hit Points",
                                                      "Damage", "Speed", "Movement"};

std::vector<int> HeroArt::Stats(int hero_class) {
  // heroes4.exe's t_hero (level L = 1, the skills basic): attack 10 x the Melee (skill 12) factor
  // (none 1, basic 1.5: 0x98918C), ranged 10 x Archery's (skill 13: none and basic 1, 0x9891A4),
  // defense 10 x Combat's (skill 1: 1, 1.5: 0x989174) for both; hit points 100 + 10 (L - 1)
  // (0x72A1F0), damage 16 + 1.6 (L - 1) to 24 + 2.4 (L - 1) (0x72A090, 0x72A130), speed 6
  // (0x72A270), movement 24 (2400: 0x72A260), spell points 10 + 10 a basic Healing, Enchantment,
  // Occultism, Conjuration or Herbalism (0x729FF0).
  std::vector<int> st(kStatCount, -1);
  const std::vector<std::string> skills = ClassSkills(hero_class);
  auto has = [&](const char* s) { return std::find(skills.begin(), skills.end(), s) != skills.end(); };
  st[0] = has("combat") ? 15 : 10;
  st[1] = 10;
  int sp = 10;
  for (const char* s : {"healing", "enchantment", "black", "conjuration", "herbalism"}) sp += has(s) ? 10 : 0;
  st[2] = sp;
  st[3] = st[4] = has("toughness") ? 15 : 10;
  st[5] = 100;
  st[6] = 16;
  st[7] = 6;
  st[8] = 24;
  return st;
}

std::string HeroArt::SkillName(const std::string& skill) const {
  const auto it = skill_names_.find(skill + "_basic");
  if (it != skill_names_.end()) return it->second;
  std::string s = skill;
  if (!s.empty()) s[0] = char(std::toupper(static_cast<unsigned char>(s[0])));
  return s;
}

const HeroArt::Creature* HeroArt::FindCreature(const std::string& keyword) const {
  const auto it = creatures_.find(Lower(keyword));
  return it == creatures_.end() ? nullptr : &it->second;
}

const Image* HeroArt::Find(const std::string& sheet, const std::string& name) const {
  auto it = sheets_.find(sheet);
  if (it == sheets_.end()) {
    std::vector<uint8_t> d;
    std::map<std::string, Image> layers;
    if (res_ && res_->Read(sheet, d)) layers = ReadLayers(d);
    it = sheets_.emplace(sheet, std::move(layers)).first;
  }
  const auto img = it->second.find(Lower(name));
  return img == it->second.end() || img->second.argb.empty() ? nullptr : &img->second;
}

const Image* HeroArt::Portrait(int faction, const std::string& keyword, const std::string& name) const {
  if (faction < 0 || faction >= kFactionCount) return nullptr;
  const std::string sheet = std::string("layers.portraits.") + kFactionNames[faction] + ".82.h4d";
  const Image* img = Find(sheet, keyword);
  return img ? img : Find(sheet, name);
}

const Image* HeroArt::SkillIcon(const std::string& skill) const {
  const int g = SkillGroup(skill);
  if (g < 0) return nullptr;
  return Find(std::string("layers.icons.skills.") + kSheet[g] + ".52.h4d", skill);
}

const Image* HeroArt::CreatureIcon(const std::string& keyword) const {
  return Find("layers.icons.creatures.52.h4d", keyword);
}

const Image* HeroArt::StatIcon(const char* stat) const {
  return Find("layers.icons.stats.27.h4d", stat);
}

const Image* HeroArt::MoodIcon(const char* what) const {
  return Find("layers.icons.morale.34.h4d", std::string("0 ") + what);
}

}  // namespace h4::random_map
