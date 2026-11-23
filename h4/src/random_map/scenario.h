#pragma once

#include <array>
#include <cstdint>
#include <span>
#include <string_view>
#include <vector>

#include "objects.h"
#include "tiles.h"

namespace h4::random_map {

struct PlayerSlot {
  bool human = true;
  uint8_t faction_mask = 0x3F;  // Life, Order, Death, Chaos, Nature, Might.
};

struct ScenarioRules {
  uint16_t size = 76;
  uint8_t levels = 1;  // 2: with the underground
  uint32_t terrain_seed = 0;
  std::span<const PlayerSlot> players;
  // Scenario strings are bytes in the game's Windows-1252 encoding.
  std::string_view title;
  std::string_view description;
  // The teams: each a bit mask of players (bit i: the i-th); empty: each player alone.
  std::vector<uint8_t> teams;
  // The obelisks' rewards by colour (their subtype: 0 gold, 4 black, 10 silver, 11 bronze, ...):
  // artifacts dug up where they lead. None: the game's own (the Grail).
  std::array<std::vector<uint16_t>, 12> obelisk_artifacts;
};

// Serializes the rules for a format-29 scenario (one or two levels) with one
// team per player and the editor's default events and restrictions.
bool EncodeScenarioRules(const ScenarioRules& rules, std::vector<uint8_t>& output);

bool PackGeneratedScenario(const ScenarioRules& rules, std::span<const MapObject> objects,
                           std::span<const Tile> tiles, std::span<const uint8_t> heights,
                           std::vector<uint8_t>& output);

}  // namespace h4::random_map
