#include "container.h"

#include <array>
#include <limits>
#include <utility>

#include <zlib.h>

namespace h4::random_map {

bool PackScenario(std::span<const uint8_t> body, std::vector<uint8_t>& output) {
  if (body.size() > std::numeric_limits<uInt>::max()) return false;

  // H4CAMPAIGN, little-endian container version, reserved byte, gzip stream.
  std::vector<uint8_t> packed{'H', '4', 'C', 'A', 'M', 'P', 'A', 'I', 'G', 'N',
                              22, 0, 0, 0, 0};
  z_stream stream{};
  if (deflateInit2(&stream, Z_BEST_COMPRESSION, Z_DEFLATED, MAX_WBITS + 16, 8,
                   Z_DEFAULT_STRATEGY) != Z_OK)
    return false;
  struct EndStream {
    z_stream& stream;
    ~EndStream() { deflateEnd(&stream); }
  } end{stream};

  stream.next_in = const_cast<Bytef*>(body.data());
  stream.avail_in = static_cast<uInt>(body.size());
  std::array<uint8_t, 16 * 1024> chunk;
  for (;;) {
    stream.next_out = chunk.data();
    stream.avail_out = static_cast<uInt>(chunk.size());
    const int result = deflate(&stream, Z_FINISH);
    if (result != Z_OK && result != Z_STREAM_END) return false;
    packed.insert(packed.end(), chunk.begin(), chunk.end() - stream.avail_out);
    if (result == Z_STREAM_END) break;
  }

  output = std::move(packed);
  return true;
}

}  // namespace h4::random_map
