// The random map generator's regression check: a set of maps (seeds, sizes, player counts, the
// settings) made from the player's game data, each map's scenario (unpacked: zlib's packing may
// differ between builds) hashed. run.sh builds and runs it:
//   check <game dir> > hashes.txt          the hashes now
//   check <game dir> hashes.txt            compares; exit 1 on a difference
// The maps of the classic generator must stay the same (an edited map is made again from its
// seed): its lines in classic.txt never change.
#include <chrono>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <map>
#include <sstream>
#include <string>
#include <vector>

#include "random_map/catalog.h"
#include "random_map/generator.h"
#include "random_map/map_settings.h"
#include "random_map/resources.h"
#include "random_map/template.h"

using namespace h4::random_map;

namespace {

uint64_t Fnv1a(const std::vector<uint8_t>& d) {
  uint64_t h = 1469598103934665603ull;
  for (uint8_t b : d) h = (h ^ b) * 1099511628211ull;
  return h;
}

struct Case {
  std::string name;
  MapSetup setup;
};

std::vector<Case> Cases() {
  std::vector<Case> out;
  for (uint32_t seed : {1u, 1112u, 77777u})
    for (int size : kMapSizes)
      for (int players : {2, 4, 6}) {
        Case c;
        c.setup.seed = seed;
        c.setup.size = size;
        for (int i = 0; i < players; ++i) {
          PlayerSetup p;
          p.human = i == 0;
          p.faction = (seed + uint32_t(i)) % 7 == 6 ? -1 : int((seed + uint32_t(i)) % 7);
          p.hero = p.faction >= 0 && i % 2 == 0 ? int(seed % 5) : -1;
          c.setup.players.push_back(p);
        }
        c.name = "classic seed=" + std::to_string(seed) + " size=" + std::to_string(size) +
                 " players=" + std::to_string(players);
        out.push_back(c);
        c.setup.underground = true;  // and with the underground
        c.name += " underground";
        out.push_back(c);
        if (size != 152) {  // islands on the other sizes too, without the underground
          Case v = c;
          v.setup.underground = false;
          v.setup.water = 2;
          v.name += " islands";
          out.push_back(std::move(v));
        }
        if (size == 152) {  // weak and strong monsters, with the underground
          for (int w : {0, 2, 3}) {  // no water, islands, random
            Case v = c;
            v.setup.water = w;
            v.name += w == 0 ? " dry" : w == 2 ? " islands" : " random water";
            out.push_back(std::move(v));
          }
          Case o = c;  // no obelisks; obelisks with treasures
          o.setup.obelisks = false;
          o.name += " no obelisks";
          out.push_back(std::move(o));
          Case t = c;
          t.setup.treasures = 3;
          t.name += " treasures";
          out.push_back(std::move(t));
          if (players >= 4) {  // two teams: 0+2, 1+3...
            Case a = c;
            for (size_t i = 0; i < a.setup.players.size(); ++i) a.setup.players[i].team = int(i % 2);
            a.name += " teams";
            out.push_back(std::move(a));
          }
          Case q = c;  // random numbers of humans and computers
          q.setup.random_humans = q.setup.random_computers = true;
          q.name += " random players";
          out.push_back(std::move(q));
          Case z = c;  // the size and the underground drawn by the seed
          z.setup.random_size = z.setup.random_underground = true;
          z.name += " random size";
          out.push_back(std::move(z));
          Case r = c;  // random monsters
          r.setup.monsters = 3;
          r.name += " random monsters";
          out.push_back(std::move(r));
          for (int m : {0, 2}) {
            Case w = c;
            w.setup.monsters = m;
            w.name += m ? " strong" : " weak";
            out.push_back(std::move(w));
          }
        }
      }
  // the templates: the built-in ones and the example file's, every size, a few player numbers
  std::vector<TemplateInfo> templates = TemplatesOf(kTemplateExample);
  for (int i = 0; i < kShippedTemplateCount; ++i)
    for (TemplateInfo& t : TemplatesOf(kShippedTemplates[i].text)) templates.push_back(std::move(t));
  for (const TemplateInfo& t : templates)
    for (int size : kMapSizes)
      for (int players : {2, 3, 4, 6}) {
        if (!t.Fits(players, size)) continue;
        const char* name = t.name.c_str();
        Case c;
        c.setup.seed = 4242u + uint32_t(size) + uint32_t(players);
        c.setup.size = size;
        c.setup.template_name = name;
        c.setup.template_text = t.text;
        for (int i = 0; i < players; ++i) {
          PlayerSetup p;
          p.human = i == 0;
          p.faction = int((c.setup.seed + uint32_t(i)) % 6);
          c.setup.players.push_back(p);
        }
        c.name = std::string("template ") + name + " size=" + std::to_string(size) + " players=" + std::to_string(players);
        out.push_back(c);
        if (size == 152) {
          Case u = c;
          u.setup.underground = true;
          u.name += " underground";
          out.push_back(std::move(u));
          Case w = c;
          w.setup.water = 2;
          w.name += " islands";
          out.push_back(std::move(w));
        }
      }
  return out;
}

}  // namespace

int main(int argc, char** argv) {
  if (argc < 2) {
    std::fprintf(stderr, "check <game dir> [hashes.txt]\n");
    return 2;
  }
  Catalog cat;
  std::string err;
  if (!cat.Load(argv[1], &err)) {
    std::fprintf(stderr, "the game's data: %s\n", err.c_str());
    return 2;
  }
  std::map<std::string, std::string> want;
  if (argc > 2) {
    std::ifstream f(argv[2]);
    for (std::string line; std::getline(f, line);) {
      const size_t at = line.rfind(' ');
      if (at != std::string::npos && line[0] != '#') want[line.substr(0, at)] = line.substr(at + 1);
    }
  }
  int bad = 0;
  const char* only = std::getenv("CASE");  // only the cases whose names have this in them
  for (const Case& c : Cases()) {
    if (only && c.name.find(only) == std::string::npos) continue;
    const auto t0 = std::chrono::steady_clock::now();
    GeneratedMap m;
    std::string line;
    if (!GenerateMap(cat, c.setup, m, &err)) {
      line = "failed";
      std::fprintf(stderr, "  %s: %s\n", c.name.c_str(), err.c_str());
      for (size_t i = 0; i < m.log.size() && i < (only ? 400u : 12u); ++i) std::fprintf(stderr, "    %s\n", m.log[i].c_str());
    } else {
      std::vector<uint8_t> body;
      if (m.file.size() < 15 || !Gunzip(m.file.data() + 15, m.file.size() - 15, body)) return 2;
      char h[17];
      std::snprintf(h, sizeof h, "%016llx", (unsigned long long)Fnv1a(body));
      line = h;
      // what the panel's Edit reads back: the same settings
      MapSetup back;
      const MapSetup resolved = ResolvePlayers(c.setup);
      bool same = ReadSetup(cat, m.file, &back) && back.seed == c.setup.seed && back.size == resolved.size &&
                  back.underground == resolved.underground && back.monsters == c.setup.monsters &&
                  back.water == c.setup.water && back.obelisks == c.setup.obelisks &&
                  back.treasures == c.setup.treasures &&
                  back.random_humans == c.setup.random_humans && back.random_computers == c.setup.random_computers &&
                  back.template_name == c.setup.template_name && back.random_size == c.setup.random_size &&
                  back.random_underground == c.setup.random_underground &&
                  back.generator_version == kGeneratorVersion &&
                  back.template_sum == (c.setup.template_text.empty() ? "" : TemplateSum(c.setup.template_text)) &&
                  back.players.size() == resolved.players.size();
      for (size_t i = 0; same && i < back.players.size(); ++i)
        same = TeamOf(back.players, i) == TeamOf(back.players, 0) ==
                   (TeamOf(resolved.players, i) == TeamOf(resolved.players, 0)) &&
               back.players[i].human == resolved.players[i].human &&
               back.players[i].faction == resolved.players[i].faction && back.players[i].hero == resolved.players[i].hero;
      if (!same) line += " (read back differently)";
      if (only)
        for (const std::string& l : m.log)
          if (l.rfind("template", 0) == 0) std::fprintf(stderr, "  %s: %s\n", c.name.c_str(), l.c_str());
    }
    const long long ms =
        std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - t0).count();
    if (argc > 2) {
      const auto it = want.find(c.name);
      const bool same = it != want.end() && it->second == line;
      bad += !same;
      std::fprintf(stderr, "%s %s (%lld ms)%s\n", same ? "ok  " : "DIFF", c.name.c_str(), ms,
                   same ? "" : (" got " + line + ", want " + (it == want.end() ? "none" : it->second)).c_str());
    } else {
      std::printf("%s %s\n", c.name.c_str(), line.c_str());
      std::fflush(stdout);
      std::fprintf(stderr, "%s (%lld ms)\n", c.name.c_str(), ms);
    }
  }
  return bad ? 1 : 0;
}
