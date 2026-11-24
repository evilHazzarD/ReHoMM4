#pragma once

#include <cstdint>
#include <vector>

#include "hero_card.h"  // Image

namespace h4::random_map {

// How the app's own panels look like the game's dialogs (the random map panel, the settings of the
// apps that draw them into the frame: app_ui): parchment (the game's texture where it is read),
// gold-brown frames, dark brown ink, headings in a deep red-brown, the chosen option gold.
namespace art {
constexpr uint32_t kBack = 0xFFE6D3A8, kEdge = 0xFFA07020, kPlate = 0xFFF4E8C4, kPlateDim = 0xFFDCCBA4,
                   kInk = 0xFF3C2410, kGold = 0xFF7A2E0A, kDim = 0xFF9A8464, kLit = 0xFFD9A84A,
                   kWarn = 0xFFA01808, kField = 0xFFFFF8E2, kFieldTyping = 0xFFFFFFF0, kMarked = 0xFFE8C878,
                   kList = 0xFFF8EED2;
}  // namespace art

// The game's parchment over all of w x h (mirrored at its edges: no seams), paler so text reads;
// plain kBack without it.
void DrawParchment(std::vector<uint32_t>& px, int w, int h, const Image* parchment);
// The game's dialog frame round w x h, `border` wide, from its 800 x 600 dialog background (the
// corners as they are, the sides repeated from a plain stretch of them); a drawn gold one without it.
void DrawDialogFrame(std::vector<uint32_t>& px, int w, int h, const Image* dialog, int border);
// The game's text button `art` stretched to `w` into `out` (`ow` wide) at x0, its height kept: its
// ends as they are, its middle repeated. y0: its top in `out` (0: its pixels as they are, else the
// art's transparent ones leave what is there).
void DrawButtonArt(std::vector<uint32_t>& out, int ow, int x0, int w, const Image& art, int y0 = 0);
// `src` scaled by `k` (bilinear, the colours weighted by alpha so the edges keep theirs).
Image ScaleImage(const Image& src, float k);

}  // namespace h4::random_map
