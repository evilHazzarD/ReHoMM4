#pragma once

#include <string>

namespace h4 {

// The scaling filter (how the frame is stretched to the screen; F10 picks the next one, the
// runtime's gfx::SetPresentFilter names): the one chosen last is kept in `path` and used again at
// the next start (H4_FILTER, unless the environment sets it), and each choice is shown for two
// seconds at the top of the frame, in the game's font and language ("Scaling 7/7: FSR").
// Before the graphics device is made (main.cpp's MobileSetup).
void LoadFilter(const std::string& path);

// For the settings (iOS, Android: no F10 there): "Scaling: FSR" ("Масштабування: FSR" when `uk`),
// UTF-8, the filter in use; NextFilter() picks the next one as F10 does (kept, shown).
std::string FilterLabel(bool uk);
void NextFilter();

}  // namespace h4
