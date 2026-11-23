#pragma once

#include <array>
#include <cstdint>
#include <span>
#include <string_view>
#include <vector>

namespace h4::random_map {

struct ObjectKind {
  uint16_t id = 0;
  std::string_view name;
};

struct MapObject {
  int32_t row = 0;
  int32_t column = 0;
  int32_t level = 0;  // 0 the surface, 1 underground
  bool army = false;
  // CP1252 strings. Army records contain only their serialized body.
  std::string_view model;
  std::array<ObjectKind, 3> kind;
  std::vector<uint8_t> body;
};

// Appends the count and object records of a scenario with `levels` levels (1 or 2).
bool AppendObjects(uint16_t size, int levels, std::span<const MapObject> objects,
                   std::vector<uint8_t>& bytes);

}  // namespace h4::random_map
