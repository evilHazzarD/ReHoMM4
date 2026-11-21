// The gamepad's layout (x86recomp's user32_window.cpp, touch::PadTick), shown from the settings
// ("Gamepad controls"; the Linux and Switch apps show it as ui/pad_help.argb, render_ui.swift;
// Android: HeroesActivity.GAMEPAD_*). Keep the four in step.
#pragma once

namespace h4 {

inline const char* GamepadHelpTitle(bool uk) { return uk ? "Керування геймпадом" : "Gamepad controls"; }

inline const char* GamepadHelp(bool uk) {
  return uk ? "Лівий стік — курсор\n"
              "Правий стік — прокрутка мапи\n"
              "A — клік, B — правий клік (інформація)\n"
              "X — магія (C)\n"
              "Y — чекати / розбудити героя (W)\n"
              "R — захист / копати (D)\n"
              "L — наступний герой (H)\n"
              "ZR — рух армії / OK (Enter)\n"
              "ZL — скасувати, закрити (Esc)\n"
              "Хрестовина ↑ / ↓ — наступне місто (T) / герой спить (Z)\n"
              "Хрестовина ← / → — Page Up / Page Down (списки, вкладки книги магії)\n"
              "+ — кінець ходу (E)\n"
              "− — зберегти гру (Alt+S)\n"
              "Натиск правого стіка — до героя (Home)\n\n"
              "Xbox / PlayStation: L/R — LB/RB (L1/R1), ZL/ZR — LT/RT (L2/R2), + — Menu (Options), "
              "− — View (Share)."
            : "Left stick: the pointer\n"
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
              "− View (Share).";
}

}  // namespace h4
