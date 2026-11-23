#pragma once

#include "tiles.h"

namespace h4::random_map {

// Computes terrain and road overlays in diamond row order.
bool BuildSurfaceTransitions(int size, uint32_t seed, std::span<Tile> tiles);

}  // namespace h4::random_map
