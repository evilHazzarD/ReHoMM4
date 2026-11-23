#pragma once

#include <cstdint>
#include <span>
#include <vector>

namespace h4::random_map {

// Wraps a serialized scenario body in the version 22 .h4c container.
// Returns false on compression failure and leaves output unchanged.
bool PackScenario(std::span<const uint8_t> body, std::vector<uint8_t>& output);

}  // namespace h4::random_map
