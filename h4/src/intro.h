// Skipping the intro movies (the setting in the apps' settings): the 3DO and NWC logos at every
// start and the intro the first time, as if they had played through (resolution.cpp ends every
// movie at its first tick until the main menu first shows).
#pragma once

#include <string>

namespace h4 {

// The movies shown from now on are skipped (or not).
void SkipIntroMovies(bool on);
bool SkipIntroMoviesNow();

// The setting, kept in `path` ("1" or "0"; MobileSetup: <data>/skip_intro), off at first:
// read at start (skipping from then until the main menu shows), and changed in the settings
// (from the next start). intro.cpp, the apps only.
void LoadSkipIntro(const std::string& path);
bool SkipIntro();
void SetSkipIntro(bool on);
const std::string& SkipIntroPath();

}  // namespace h4
