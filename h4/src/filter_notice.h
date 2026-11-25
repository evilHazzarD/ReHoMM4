#pragma once

#include <string>
#include <vector>

namespace h4 {

// The scaling filter (how the frame is stretched to the screen; F10 picks the next one, the
// runtime's gfx::SetPresentFilter names): the one chosen last is kept in `path` and used again at
// the next start (H4_FILTER, unless the environment sets it), and each choice is shown for two
// seconds at the top of the frame, in the game's font and language ("Scaling 7/7: FSR").
// Before the graphics device is made (main.cpp's MobileSetup).
void LoadFilter(const std::string& path);

// For the settings' dropdown (no F10 on phones and tablets): the filters' ids in F10's order, the one in use, one's name in the
// language `lang` ("Sharp bilinear"), and choosing one (kept, shown, as F10).
std::vector<std::string> FilterIds();
std::string CurrentFilter();
std::string FilterName(const std::string& id, const std::string& lang);
void SetFilter(const std::string& id);

// Any notice the same way, for `seconds`: `english` in the game's language (ui_text.h).
void ShowNotice(const std::string& english, int seconds);

}  // namespace h4
