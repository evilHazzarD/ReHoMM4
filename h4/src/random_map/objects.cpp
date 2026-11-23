#include "objects.h"

#include <limits>

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

// Adapts the object record writer from Tihomir Ivanov's
// homm4-random-map-generator, tools/h4objects.py.
bool AppendObjects(uint16_t size, int levels, std::span<const MapObject> objects,
                   std::vector<uint8_t>& bytes) {
  constexpr auto max_text = std::numeric_limits<uint16_t>::max();
  if (objects.size() > std::numeric_limits<uint32_t>::max()) return false;
  for (const MapObject& object : objects) {
    if (object.row < 0 || object.row >= size || object.column < 0 ||
        object.column >= size || object.level < 0 || object.level >= levels ||
        object.body.empty()) return false;
    if (!object.army) {
      if (object.model.empty() || object.model.size() > max_text) return false;
      for (const ObjectKind& kind : object.kind)
        if (kind.name.size() > max_text) return false;
    }
  }

  AppendLE(bytes, static_cast<uint32_t>(objects.size()), 4);
  for (const MapObject& object : objects) {
    AppendLE(bytes, static_cast<uint32_t>(object.row), 4);
    AppendLE(bytes, static_cast<uint32_t>(object.column), 4);
    AppendLE(bytes, static_cast<uint32_t>(object.level), 4);
    bytes.push_back(object.army);
    if (!object.army) {
      AppendString(bytes, object.model);
      AppendLE(bytes, 0, 2);  // object type record version
      for (const ObjectKind& kind : object.kind) {
        AppendLE(bytes, kind.id, 2);
        AppendString(bytes, kind.name);
      }
    }
    bytes.insert(bytes.end(), object.body.begin(), object.body.end());
  }
  return true;
}

}  // namespace h4::random_map
