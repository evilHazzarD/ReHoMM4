#include "transitions.h"

#include <algorithm>
#include <array>
#include <random>

namespace h4::random_map {
namespace {

// Mask order and blend rules adapted from Tihomir Ivanov's
// homm4-random-map-generator, transitions.py and transition_codes.json.
constexpr int kCodes[] = {
    33, 201, 30, 3333, 1233, 300, 3000, 1010, 212, 103, 1203, 1030, 3300, 3102, 3023,
    2310, 3, 312, 1003, 2, 2000, 21, 330, 3003, 20, 200, 121, 3103, 3123, 2020, 31,
    210, 2101, 2121, 1000, 2010, 2021, 230, 1002, 1, 310, 302, 1200, 120, 12, 2301,
    3330, 233, 1210, 3020, 23, 1012, 10, 3303, 3310, 1202, 1230, 2031, 1212, 3120,
    3012, 3033, 2001, 1033, 102, 333, 3030, 2100, 100, 3302, 2300, 2030, 202, 3100,
    3002, 123, 301, 203, 2120, 1020, 101, 2331, 331, 303, 1023, 2330, 231, 3010,
    3312, 400, 4000, 40, 4};
constexpr auto kIndex = [] {
  std::array<int, 4445> index{};
  index.fill(-1);
  for (size_t i = 0; i < std::size(kCodes); ++i) index[kCodes[i]] = static_cast<int>(i);
  return index;
}();
struct Terrain { uint8_t type, subtype; };
constexpr Terrain kPriority[] = {
    {0,1}, {0,0}, {11,0}, {9,0}, {10,0}, {8,1}, {8,0}, {3,1}, {3,0}, {2,1},
    {2,0}, {7,1}, {7,0}, {6,1}, {6,0}, {1,1}, {1,0}, {5,1}, {5,0}, {4,1},
    {4,0}, {12,0}, {13,0}, {14,0}, {15,0}, {16,0}, {17,0}, {18,0}};
constexpr auto kRank = [] {
  std::array<std::array<int, 2>, 19> rank{};
  for (auto& row : rank) row.fill(-1);
  for (size_t i = 0; i < std::size(kPriority); ++i)
    rank[kPriority[i].type][kPriority[i].subtype] = static_cast<int>(i);
  return rank;
}();

// Search the at most 15 partitions of four corners, in upstream order.
struct Masks {
  std::array<int, 4> codes{};
  int count = 5;
};

void Partition(const std::array<int, 4>& corners, int i, int groups,
               std::array<int, 4>& parts, Masks& best) {
  if (groups >= best.count) return;
  if (i == 4) {
    for (int j = 0; j < groups; ++j)
      if (kIndex[parts[j]] < 0) return;
    best = {parts, groups};
    return;
  }
  if (!corners[i]) return Partition(corners, i + 1, groups, parts, best);
  for (int j = 0; j <= groups; ++j) {
    parts[j] += corners[i];
    Partition(corners, i + 1, groups + (j == groups), parts, best);
    parts[j] -= corners[i];
  }
}

// Each entry is weighted by its decimal position: E, S, W, N.
template <typename Digit>
Masks CornerMasks(Digit digit) {
  std::array<int, 4> corners{};
  for (int i = 0, weight = 1000; i < 4; ++i, weight /= 10)
    corners[i] = weight * digit(2 * i + 1, (2 * i + 3) % 8, (2 * i + 2) % 8);
  std::array<int, 4> parts{};
  Masks masks;
  Partition(corners, 0, 0, parts, masks);
  std::sort(masks.codes.begin(), masks.codes.begin() + masks.count);
  return masks;
}

int Edges(bool before, bool after) { return int(before) + 2 * int(after); }
bool Waterlike(int terrain) { return terrain == 0 || (terrain >= 9 && terrain <= 11); }

}  // namespace

bool BuildSurfaceTransitions(int size, uint32_t seed, std::span<Tile> tiles) {
  if (size != 76 && size != 152 && size != 228 && size != 304) return false;
  if (tiles.size() != static_cast<size_t>(size * (size / 2 + 1))) return false;
  for (const Tile& tile : tiles)
    if (tile.terrain >= kRank.size() || tile.subtype >= 2 ||
        kRank[tile.terrain][tile.subtype] < 0 || tile.road > 3) return false;

  struct Row { int first, width, offset; };
  std::vector<Row> rows;
  for (int r = 0, offset = 0; r < size; ++r) {
    const int d = std::min(r, size - 1 - r);
    rows.push_back({size / 2 - 1 - d, 2 + 2 * d, offset});
    offset += rows.back().width;
  }
  auto at = [&](int r, int c) -> Tile* {
    if (r < 0 || r >= size || c < rows[r].first || c >= rows[r].first + rows[r].width)
      return nullptr;
    return &tiles[rows[r].offset + c - rows[r].first];
  };
  constexpr int dr[] = {-1, -1, -1, 0, 1, 1, 1, 0};
  constexpr int dc[] = {-1, 0, 1, 1, 1, 0, -1, -1};
  std::mt19937 rng(seed);
  for (int r = 0; r < size; ++r) {
    for (int c = rows[r].first; c < rows[r].first + rows[r].width; ++c) {
      Tile& tile = *at(r, c);
      const int own = kRank[tile.terrain][tile.subtype];
      std::array<int, 8> rank, road;
      std::array<int, 4> road_count{};
      for (int i = 0; i < 8; ++i) {
        const Tile* next = at(r + dr[i], c + dc[i]);
        rank[i] = next ? kRank[next->terrain][next->subtype] : own;
        road[i] = next ? next->road : -1;
        if (road[i] > 0) ++road_count[road[i]];
      }
      tile.terrain_transitions.clear();
      tile.road_transitions.clear();
      for (int p = own + 1; p < static_cast<int>(std::size(kPriority)); ++p) {
        if (std::find(rank.begin(), rank.end(), p) == rank.end()) continue;
        const Masks masks = CornerMasks([&](int a, int b, int d) {
          const int edges = Edges(rank[a] >= p, rank[b] >= p);
          return edges ? edges : (rank[d] >= p ? 4 : 0);
        });
        const auto [terrain, subtype] = kPriority[p];
        const int family = Waterlike(tile.terrain) ? (terrain == 0 ? 0 : 5) :
                           Waterlike(terrain) ? 0 : 15;
        const auto mask_set = static_cast<uint8_t>(family + rng() % 5);
        for (int i = 0; i < masks.count; ++i)
          tile.terrain_transitions.push_back({terrain, subtype,
              static_cast<uint16_t>(kIndex[masks.codes[i]]), mask_set});
      }
      const int kind = tile.road ? 0 : static_cast<int>(
          std::max_element(road_count.begin(), road_count.end()) - road_count.begin());
      if (!tile.road && !kind) continue;
      const Masks masks = CornerMasks([&](int a, int b, int d) {
        if (tile.road) return road[d] == 0 ? Edges(road[a] == 0, road[b] == 0) : 0;
        return (road[a] == kind || road[a] == -1) && (road[b] == kind || road[b] == -1) ? 3 : 0;
      });
      for (int i = 0; i < masks.count; ++i)
        tile.road_transitions.push_back({static_cast<uint8_t>(kind),
            static_cast<uint16_t>(kIndex[masks.codes[i]]), static_cast<uint8_t>(10 + rng() % 5)});
    }
  }
  return true;
}

}  // namespace h4::random_map
