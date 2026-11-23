#include "scenario.h"

#include "container.h"

#include <limits>
#include <utility>

namespace h4::random_map {
namespace {

void AppendLE(std::vector<uint8_t>& bytes, uint32_t value, unsigned size) {
  for (unsigned i = 0; i < size; ++i) {
    bytes.push_back(static_cast<uint8_t>(value));
    value >>= 8;
  }
}

void AppendString(std::vector<uint8_t>& bytes, std::string_view text) {
  AppendLE(bytes, static_cast<uint32_t>(text.size()), 2);
  bytes.insert(bytes.end(), text.begin(), text.end());
}

}  // namespace

// Adapts the generated-map defaults from Tihomir Ivanov's
// homm4-random-map-generator, tools/h4map.py.
bool EncodeScenarioRules(const ScenarioRules& rules, std::vector<uint8_t>& output) {
  const size_t count = rules.players.size();
  constexpr auto max_text = std::numeric_limits<uint16_t>::max();
  if ((rules.size != 76 && rules.size != 152 && rules.size != 228 && rules.size != 304) ||
      rules.levels < 1 || rules.levels > 2 || count < 2 || count > 6 || rules.title.size() > max_text ||
      rules.description.size() > max_text) return false;

  std::vector<uint8_t> bytes{29, 0, 2, 0};  // format and expansion
  AppendLE(bytes, rules.size, 2);
  bytes.push_back(rules.levels);
  AppendLE(bytes, rules.terrain_seed, 4);
  bytes.push_back(static_cast<uint8_t>(count));
  for (uint8_t i = 0; i < count; ++i) {
    const PlayerSlot& player = rules.players[i];
    if (player.faction_mask == 0 || player.faction_mask > 0x3F) return false;
    bytes.insert(bytes.end(), {i, static_cast<uint8_t>(player.human), 0, 0,
                               player.faction_mask});
  }
  AppendString(bytes, rules.title);
  bytes.push_back(1);  // default difficulty
  AppendString(bytes, rules.description);
  AppendLE(bytes, 0, 2);  // team section version
  if (rules.teams.empty()) {
    bytes.push_back(static_cast<uint8_t>(count));
    for (uint8_t i = 0; i < count; ++i) bytes.push_back(static_cast<uint8_t>(1u << i));
  } else {
    bytes.push_back(static_cast<uint8_t>(rules.teams.size()));
    for (uint8_t mask : rules.teams) bytes.push_back(mask);
  }

  // No custom loss or victory text, cutscenes, or editor notes.
  bytes.insert(bytes.end(), {0, 0, 1, 0, 0, 0, 0, 0});
  bytes.insert(bytes.end(), 8, 0);  // four empty global event lists
  for (const auto& rewards : rules.obelisk_artifacts) {  // v0, rewards, 0xFFFF as the editor
    bytes.insert(bytes.end(), {0, 0, uint8_t(!rewards.empty()), 0xFF, 0xFF});
    if (rewards.empty()) continue;
    AppendLE(bytes, rewards.size(), 2);
    for (uint16_t id : rewards) {
      AppendLE(bytes, 0, 2);  // reward version
      AppendString(bytes, "artifact");
      AppendLE(bytes, id, 2);
      AppendLE(bytes, 1, 4);  // one of it
    }
  }
  for (uint8_t i = 0; i < count; ++i) bytes.insert(bytes.end(), {i, 0, 0});
  bytes.insert(bytes.end(), 4, 0);  // default artifact, skill and spell rules
  constexpr uint16_t artifact_bits = 248;
  for (uint8_t fill : {uint8_t{0xFF}, uint8_t{0}}) {
    AppendLE(bytes, 0, 2);  // bitset version
    AppendLE(bytes, artifact_bits, 2);
    bytes.insert(bytes.end(), artifact_bits / 8, fill);
  }
  bytes.insert(bytes.end(), 12, 0);  // three empty AI importance tables
  output = std::move(bytes);
  return true;
}

bool PackGeneratedScenario(const ScenarioRules& rules, std::span<const MapObject> objects,
                           std::span<const Tile> tiles, std::span<const uint8_t> heights,
                           std::vector<uint8_t>& output) {
  std::vector<uint8_t> body;
  if (!EncodeScenarioRules(rules, body)) return false;
  if (!AppendObjects(rules.size, rules.levels, objects, body)) return false;
  if (!AppendTiles(rules.size, rules.levels, tiles, heights, body)) return false;
  return PackScenario(body, output);
}

}  // namespace h4::random_map
