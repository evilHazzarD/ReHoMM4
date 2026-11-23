#pragma once

#include <cstdint>
#include <span>
#include <vector>

namespace h4::random_map {

struct TerrainTransition {
  uint8_t terrain;
  uint8_t subtype;
  uint16_t mask;
  uint8_t mask_set;
};

struct RoadTransition {
  uint8_t road;
  uint16_t mask;
  uint8_t mask_set;
};

struct Tile {
  uint8_t terrain = 1;
  uint8_t subtype = 0;
  uint8_t road = 0;
  uint8_t rock = 0;
  uint8_t rock_transition = 0;
  uint32_t visibility = 0x80000;
  std::vector<TerrainTransition> terrain_transitions;
  std::vector<RoadTransition> road_transitions;
};

// Appends tiles and vertex heights in diamond row order: every level's tiles (the surface's
// first), then every level's heights. Returns false on invalid data and leaves the destination
// unchanged.
bool AppendTiles(uint16_t size, int levels, std::span<const Tile> tiles,
                 std::span<const uint8_t> heights, std::vector<uint8_t>& bytes);

}  // namespace h4::random_map
