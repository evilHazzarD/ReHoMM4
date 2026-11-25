#pragma once

#include <cstdint>
#include <string_view>
#include <vector>

namespace h4::random_map {

class Resources;

// One of the game's own fonts (font.<name>.<size>.h4d in its archives): u16 version 1, u8 first
// character, u8 height, u8 line height, u8 baseline, u8 count; per character u32 width, u32 height,
// i32 space before, i32 space after and width x height bytes: the high nibble the ink's alpha, the low one its shadow's.
// Characters are the game's bytes (cp1252, or what a translation's fonts hold), so its text and a
// translation's show as the game shows them.
class Font {
 public:
  bool Load(const Resources& res, std::string_view name);
  int Height() const { return line_; }
  int Width(std::string_view text) const;
  // Draws `text` with its top-left at (x, y) into an ARGB canvas of w x h; `shadow` false: without
  // the font's dark shadow (dark ink on parchment, as the game's own dialogs write).
  void Draw(std::vector<uint32_t>& canvas, int w, int h, int x, int y, std::string_view text,
            uint32_t ink, bool shadow = true) const;

 private:
  struct Glyph {
    int w = 0, h = 0, x = 0, y = 0;  // x, y: the space before and after it
    std::vector<uint8_t> px;
  };
  std::vector<Glyph> glyphs_;  // by character, 256
  int line_ = 0;
};

}  // namespace h4::random_map
