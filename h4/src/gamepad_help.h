// The gamepad's layout (x86recomp's user32_window.cpp, touch::PadTick), shown from the settings
// ("Gamepad controls"; Android: HeroesActivity.GAMEPAD_*). Keep them in step.
#pragma once

#include <string>

#include "ui_text.h"

namespace h4 {

inline const char* GamepadHelpTitle(const std::string& lang) { return TrIn(lang, "Gamepad controls"); }

inline const char* GamepadHelp(const std::string& lang) {
  return TrIn(lang, "Left stick: the pointer\n"
              "Right stick: scrolls the map\n"
              "A: click, B: right-click (information)\n"
              "X: cast a spell (C)\n"
              "Y: wait / wake the hero (W)\n"
              "R: defend / dig (D)\n"
              "L: next hero (H)\n"
              "ZR: move the army / OK (Enter)\n"
              "ZL: cancel, close (Esc)\n"
              "D-pad ↑ / ↓: next town (T) / the hero sleeps (Z)\n"
              "D-pad ← / →: Page Up / Page Down (lists, spell book tabs)\n"
              "+: end the turn (E)\n"
              "−: save the game (Alt+S)\n"
              "Right stick click: centre on the hero (Home)\n\n"
              "Xbox / PlayStation: L/R are LB/RB (L1/R1), ZL/ZR LT/RT (L2/R2), + Menu (Options), "
              "− View (Share).");
}

}  // namespace h4
