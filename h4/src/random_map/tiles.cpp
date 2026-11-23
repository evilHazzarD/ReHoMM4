#include "tiles.h"

#include <algorithm>

namespace h4::random_map {

// Adapts the tile encoding from Tihomir Ivanov's
// homm4-random-map-generator, tools/h4tiles.py.
bool AppendTiles(uint16_t size, int levels, std::span<const Tile> tiles,
                 std::span<const uint8_t> heights, std::vector<uint8_t>& bytes) {
  if ((size != 76 && size != 152 && size != 228 && size != 304) || levels < 1 || levels > 2) return false;
  size_t tile_count = 0;
  size_t vertex_count = 0;
  int previous_width = 0;
  for (int row = 0; row < size; ++row) {
    const int width = 2 * (1 + std::min(row, size - 1 - row));
    tile_count += width;
    vertex_count += std::max(width, previous_width) + 1;
    previous_width = width;
  }
  vertex_count += previous_width + 1;
  if (tiles.size() != tile_count * size_t(levels) || heights.size() != vertex_count * size_t(levels))
    return false;

  size_t encoded_size = heights.size();
  for (const Tile& tile : tiles) {
    if (tile.terrain >= 19 || tile.subtype >= (tile.terrain < 9 ? 2 : 1) ||
        tile.road > 3 || tile.rock > 2 || tile.rock_transition > 1 ||
        (tile.visibility & ~0x3FFFFu) != 0x80000 ||
        tile.terrain_transitions.size() > 12 || tile.road_transitions.size() > 12)
      return false;
    encoded_size += 11 + 5 * tile.terrain_transitions.size() + 4 * tile.road_transitions.size();
  }

  bytes.reserve(bytes.size() + encoded_size);
  for (const Tile& tile : tiles) {
    bytes.insert(bytes.end(), {tile.terrain, tile.subtype, tile.road, tile.rock,
                               tile.rock_transition});
    for (unsigned shift = 0; shift < 32; shift += 8)
      bytes.push_back(static_cast<uint8_t>(tile.visibility >> shift));
    bytes.push_back(static_cast<uint8_t>(tile.terrain_transitions.size()));
    for (const TerrainTransition& t : tile.terrain_transitions)
      bytes.insert(bytes.end(), {t.terrain, t.subtype, static_cast<uint8_t>(t.mask),
                                 static_cast<uint8_t>(t.mask >> 8), t.mask_set});
    bytes.push_back(static_cast<uint8_t>(tile.road_transitions.size()));
    for (const RoadTransition& t : tile.road_transitions)
      bytes.insert(bytes.end(), {t.road, static_cast<uint8_t>(t.mask),
                                 static_cast<uint8_t>(t.mask >> 8), t.mask_set});
  }
  bytes.insert(bytes.end(), heights.begin(), heights.end());
  return true;
}

}  // namespace h4::random_map
