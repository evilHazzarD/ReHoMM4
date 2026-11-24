#include "map_settings.h"
#include "template.h"

#include <algorithm>
#include <cstdlib>
#include <cstring>

#include "resources.h"

namespace h4::random_map {
namespace {

// The players' colours in the game's order (the description's words).
const char* const kColours[6] = {"red", "blue", "green", "orange", "purple", "teal"};
constexpr const char* kHeroes = " Starting heroes: ";

// The computers' places (", computers 2,3"; none: ""): the map lets each player be a human's
// (scenario: every slot human), the setup's choice kept for its editor.
std::string Computers(const MapSetup& setup) {
  std::string s;
  for (size_t i = 0; i < setup.players.size(); ++i)
    if (!setup.players[i].human) s += (s.empty() ? ", computers " : ",") + std::to_string(i + 1);
  return s;
}

}  // namespace

std::string DescribeSetup(const Catalog& catalog, const MapSetup& setup) {
  std::string d = "Random map, seed " + std::to_string(setup.seed) + ", " +
                  std::to_string(setup.players.size()) + " players" +
                  (setup.underground ? ", underground" : "") +
                  (setup.monsters == 0   ? ", weak monsters"
                   : setup.monsters == 2 ? ", strong monsters"
                   : setup.monsters == 3 ? ", random monsters"
                                         : "") +
                  (setup.water == 0 ? ", no water" : setup.water == 2 ? ", islands" : setup.water == 3 ? ", random water" : "") +
                  (setup.obelisks ? "" : ", no obelisks") + (setup.treasures ? ", treasures " + std::to_string(setup.treasures) : "") +
                  (setup.random_humans ? ", random humans" : "") + (setup.random_computers ? ", random computers" : "") +
                  (setup.random_size ? ", random size" : "") + (setup.random_underground ? ", random underground" : "") +
                  Computers(setup) +
                  (setup.template_name.empty() || setup.template_name == "Classic"
                       ? ""
                       : ", template " + setup.template_name +
                             (setup.template_text.empty() ? "" : " #" + TemplateMapSum(setup.template_text))) +
                  ", generator " + std::to_string(kGeneratorVersion) +
                  "." +
                  kHeroes;
  for (size_t i = 0; i < setup.players.size() && i < 6; ++i) {
    const PlayerSetup& p = setup.players[i];
    std::string hero = "random";
    if (p.faction >= 0 && p.hero >= 0 && size_t(p.hero) < catalog.Heroes(p.faction).size())
      hero = catalog.Heroes(p.faction)[size_t(p.hero)].name;
    d += std::string(i ? ", " : "") + kColours[i] + " " + hero;
  }
  return d + ".";
}

bool ReadSetup(const Catalog& catalog, const std::vector<uint8_t>& file, MapSetup* setup) {
  // H4CAMPAIGN, the container's version (u16), a reserved byte, then the gzipped scenario.
  constexpr size_t kHead = 15;
  if (file.size() <= kHead || std::memcmp(file.data(), "H4CAMPAIGN", 10) != 0) return false;
  std::vector<uint8_t> b;
  if (!Gunzip(file.data() + kHead, file.size() - kHead, b, 1 << 16) || b.size() < 12) return false;
  // The rules (scenario.cpp's EncodeScenarioRules): format, expansion, size (u16), levels,
  // terrain seed, the players (number, human, 0, 0, faction mask), title, difficulty, description.
  size_t p = 4;
  auto u16 = [&](size_t at) { return uint16_t(b[at] | b[at + 1] << 8); };
  MapSetup s;
  s.size = u16(p);
  s.underground = b[6] == 2;  // the levels
  p = 11;
  const int count = b[p++];
  if (count < 2 || count > 6 || p + size_t(count) * 5 > b.size()) return false;
  for (int i = 0; i < count; ++i, p += 5) {
    PlayerSetup pl;
    pl.human = b[p + 1] != 0;
    const uint8_t mask = b[p + 4] & 0x3F;
    if (mask && !(mask & (mask - 1)))
      for (int f = 0; f < kFactionCount; ++f)
        if (mask == 1u << f) pl.faction = f;
    s.players.push_back(pl);
  }
  auto text = [&](std::string* out) {
    if (p + 2 > b.size()) return false;
    const size_t n = u16(p);
    if (p + 2 + n > b.size()) return false;
    out->assign(reinterpret_cast<const char*>(&b[p + 2]), n);
    p += 2 + n;
    return true;
  };
  std::string title, description;
  if (!text(&title) || p + 1 > b.size()) return false;
  s.title = title;
  ++p;  // difficulty
  if (!text(&description)) return false;
  // the teams: version (u16), number, a player mask each
  if (p + 3 <= b.size()) {
    const int teams = b[p + 2];
    for (int k = 0; k < teams && p + 3 + size_t(k) < b.size(); ++k)
      for (size_t i = 0; i < s.players.size(); ++i)
        if (b[p + 3 + size_t(k)] >> i & 1) s.players[i].team = k;
  }

  const size_t at = description.find("seed ");
  if (at == std::string::npos) return false;
  uint64_t seed = 0;
  size_t q = at + 5;
  if (q >= description.size() || description[q] < '0' || description[q] > '9') return false;
  for (; q < description.size() && description[q] >= '0' && description[q] <= '9' && seed < 1000000000; ++q)
    seed = seed * 10 + uint64_t(description[q] - '0');
  s.seed = uint32_t(seed);
  const std::string head = description.substr(0, description.find(kHeroes));
  if (head.find(", weak monsters") != std::string::npos) s.monsters = 0;
  if (head.find(", strong monsters") != std::string::npos) s.monsters = 2;
  if (head.find(", random monsters") != std::string::npos) s.monsters = 3;
  if (head.find(", no water") != std::string::npos) s.water = 0;
  if (head.find(", islands") != std::string::npos) s.water = 2;
  if (head.find(", random water") != std::string::npos) s.water = 3;
  s.obelisks = head.find(", no obelisks") == std::string::npos;
  s.random_humans = head.find(", random humans") != std::string::npos;
  s.random_computers = head.find(", random computers") != std::string::npos;
  s.random_size = head.find(", random size") != std::string::npos;
  s.random_underground = head.find(", random underground") != std::string::npos;
  if (const size_t at = head.find(", computers "); at != std::string::npos) {
    for (PlayerSetup& pl : s.players) pl.human = true;  // (the file's: every one may be)
    for (size_t q = at + 12; q < head.size() && head[q] >= '0' && head[q] <= '9';) {
      const int n = head[q] - '0';
      if (n >= 1 && size_t(n) <= s.players.size()) s.players[size_t(n - 1)].human = false;
      q += head[q + 1] == ',' ? 2 : 1;
    }
  } else if (head.find(", generator ") != std::string::npos &&
             std::atoi(head.c_str() + head.find(", generator ") + 12) >= 3) {
    for (PlayerSetup& pl : s.players) pl.human = true;  // generator 3 on: no computers listed, none
  }
  if (const size_t at = head.find(", template "); at != std::string::npos) {
    const size_t from = at + 11, to = head.find_first_of(",.", from);
    std::string t = head.substr(from, to == std::string::npos ? std::string::npos : to - from);
    if (const size_t sum = t.find(" #"); sum != std::string::npos) {
      s.template_sum = t.substr(sum + 2);
      t.resize(sum);
    }
    s.template_name = t;
  }
  s.generator_version = 0;
  if (const size_t at = head.find(", generator "); at != std::string::npos)
    s.generator_version = std::atoi(head.c_str() + at + 12);
  if (const size_t at = head.find(", treasures "); at != std::string::npos)
    s.treasures = std::clamp(std::atoi(head.c_str() + at + 12), 0, 4);

  // The heroes: "<colour> <name>" each, ", " between them, "." after the last.
  const size_t h = description.find(kHeroes);
  if (h != std::string::npos) {
    std::string list = description.substr(h + std::strlen(kHeroes));
    if (!list.empty() && list.back() == '.') list.pop_back();
    size_t from = 0;
    for (;;) {
      const size_t to = list.find(", ", from);
      const std::string item = list.substr(from, to == std::string::npos ? std::string::npos : to - from);
      for (int i = 0; i < count; ++i) {
        const std::string c = std::string(kColours[i]) + " ";
        if (item.compare(0, c.size(), c) != 0) continue;
        const std::string name = item.substr(c.size());
        PlayerSetup& pl = s.players[size_t(i)];
        if (pl.faction < 0) break;
        const auto& heroes = catalog.Heroes(pl.faction);
        for (size_t k = 0; k < heroes.size(); ++k)
          if (heroes[k].name == name) pl.hero = int(k);
        break;
      }
      if (to == std::string::npos) break;
      from = to + 2;
    }
  }
  *setup = std::move(s);
  return true;
}

}  // namespace h4::random_map
