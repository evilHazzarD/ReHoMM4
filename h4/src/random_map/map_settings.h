#pragma once

#include <cstdint>
#include <string>
#include <vector>

#include "catalog.h"
#include "generator.h"

namespace h4::random_map {

// What a generated map was made with, kept in the map itself (no file beside it), so it can be
// edited again as homm4-random-map-generator's "My random maps" do: the size, players (human or
// computer) and factions are the scenario's rules; the seed and the starting heroes its
// description, which the game shows as well:
//   Random map, seed 1112, 2 players. Starting heroes: red Bakrus, blue random.
//   Random map, seed 7, 3 players, underground. Starting heroes: ...   (with the second level)
//   Random map, seed 7, 3 players, strong monsters. Starting heroes: ...   (weak or strong)
// Heroes by name (the game's bytes, as Catalog has them).
std::string DescribeSetup(const Catalog& catalog, const MapSetup& setup);

// From a generated map's file (.h4c): false if it is not one (no seed in its description).
// A hero not found (another language's names, an older map) is random.
bool ReadSetup(const Catalog& catalog, const std::vector<uint8_t>& file, MapSetup* setup);

}  // namespace h4::random_map
