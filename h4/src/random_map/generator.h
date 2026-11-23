#pragma once

#include <atomic>
#include <cstdint>
#include <string>
#include <vector>

#include "catalog.h"

namespace h4::random_map {

// A random scenario, after Tihomir Ivanov's homm4-random-map-generator (tools/generator.py,
// github.com/tishonator/homm4-random-map-generator), ported to C++ with his permission. The same
// seed and settings give the same map on every platform (its own random sequence and math; not
// the Python program's maps).

// The generator's version, written into each map's description: raised whenever the same
// settings and seed give another map than before (tests/random_map's hashes change), so that an
// older map's editor can tell it will not be made again the same.
constexpr int kGeneratorVersion = 2;

struct PlayerSetup {
  bool human = true;
  int faction = -1;  // Faction, -1: any (the town is a random one)
  int hero = -1;     // index in Catalog::Heroes(faction) (a faction is needed), -1: random
  int team = -1;     // 0 to 5: players of one number are allies; -1: a team of its own (its index)
};

// A player's team number: its own, or its index when it has none.
inline int TeamOf(const std::vector<PlayerSetup>& players, size_t i) {
  return players[i].team >= 0 ? players[i].team : int(i);
}

struct MapSetup {
  uint32_t seed = 0;
  int size = 76;     // 76 small, 152 medium, 228 large, 304 extra large
  std::vector<PlayerSetup> players;  // 2 to 6
  bool underground = false;          // a second level, caves under the surface
  // The random monsters' strength, as Heroes 3's: 0 weak, 1 normal (the game's own), 2 strong,
  // 3 one of them by the seed.
  int monsters = 1;
  // 0 none, 1 normal (lakes and rivers), 2 islands (each zone of the surface its own, a shipyard on
  // each), 3 one of them by the seed.
  int water = 1;
  bool obelisks = true;        // gold obelisks: a puzzle map to the Grail
  // The humans' and computers' numbers drawn by the seed (2 to 6 in all; a human at least): the
  // players' rows as many as drawn, humans first (ResolvePlayers).
  bool random_humans = false, random_computers = false;
  // The map's size and the underground drawn by the seed (ResolvePlayers).
  bool random_size = false, random_underground = false;
  int treasures = 0;  // 0 to 4 more colours of obelisks, each to its own treasure (artifacts)
  // The scenario's title (the game's list sorts by it); empty: _Random_Map_<seed>. ReadSetup gives it.
  std::string title;
  // The zones' template (template.h): empty or "Classic" the generator's own layout; a template
  // file's: its text (template_text), the name kept in the map's description.
  std::string template_name;
  std::string template_text;
  // Read back from a map (ReadSetup): the generator's version that made it (0: before the
  // versions), its template file's text's sum (TemplateSum; "" none).
  int generator_version = kGeneratorVersion;
  std::string template_sum;
};

struct GeneratedMap {
  std::vector<uint8_t> file;  // the .h4c
  std::string title;          // _Random_Map_<seed>
  int objects = 0, zones = 0, attempts = 0;
  std::vector<std::string> log;  // what was left out, the retries
};

constexpr int kMapSizes[] = {76, 152, 228, 304};

// The setup with its random numbers of humans and computers drawn (the same for a seed): the
// human rows, then the computer ones, cut or added to (any faction and hero); teams past the
// players' number left out. And a random size (one of min_size to max_size: a template's) and
// underground drawn.
MapSetup ResolvePlayers(const MapSetup& setup, int min_size = 76, int max_size = 304);

// Builds the map; retries other layouts (a fixed sequence of seeds) when a check fails.
// false and `error` if none works, or `cancel` was set.
bool GenerateMap(const Catalog& catalog, const MapSetup& setup, GeneratedMap& out, std::string* error,
                 const std::atomic<bool>* cancel = nullptr);

}  // namespace h4::random_map
