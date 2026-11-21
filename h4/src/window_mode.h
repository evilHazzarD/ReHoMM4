// The desktop apps' (Mac, Linux) fullscreen setting: <data>/windowed ("1": a window; none or
// "0": fullscreen, the default). Read at start (before the game's window is made); the
// settings' checkbox switches it at once (recomp::SetWindowed).
#pragma once

#include <string>

namespace h4 {

void LoadWindowMode(const std::string& path);
bool Fullscreen();
void SetFullscreen(bool on);  // on the thread that owns the SDL windows (the main one)

// Testing: H4_TEST_WINDOWED=<s> switches to a window and back to fullscreen every <s> seconds (not
// saved), logging the SDL window's state a second after each; called on every main loop pass.
void TestWindowMode();

}  // namespace h4
