#pragma once

#include <string>

namespace h4 {

// The random map generator in the scenario list (New Game > Scenario): a "Random map" button
// beside the game's dialog opens a panel over it (gfx::SetPresentBadge, so the game's windows are
// left alone) that makes a map (random_map/generator.h) on a worker thread and writes it to the
// folder the list shows. Called on every main loop pass (resolution.cpp's KeepWide) after the
// settings button (it uses the same picture, free while the dialog is open):
//   shown: the dialog is up and nothing over it; l, t, r, b: its rectangle in the frame;
//   folder: the guest folder the list shows ("C:\Heroes4\maps\").
//   selected: the file name of the map selected in the list ("" none); for a random map one
//   (rm_<date>_<time>_<seed>.h4c, or an older _random_map_<seed>.h4c) a "Remove random map"
//   button shows right of the first.
// true: a map was just written or removed; the caller has the list read again, from `open` when
// that is set (a new map goes into the maps' RandomMaps folder: the list shows it there).
bool RandomMapPass(bool shown, int l, int t, int r, int b, const std::string& folder,
                   const std::string& selected, std::string* open = nullptr);

// Whether a guest folder is the generated maps' (…\RandomMaps\): its list newest first.
bool IsRandomMapsFolder(const std::string& folder);

// The panel's seed field is being typed in: the on-screen keyboard is wanted (resolution.cpp's
// KeepScreenKeyboard; the browser's page keyboard is asked for here).
bool RandomMapTyping();

}  // namespace h4
