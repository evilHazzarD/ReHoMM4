#pragma once

#include <string>

union SDL_Event;  // (SDL3/SDL_events.h)

namespace h4 {
namespace random_map {
class Font;
struct Image;
}

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

// The game's own font (font.prose_antique.<size>: 16, or 20 to 36 by 2), read with the
// generator's data on a thread of its own when first asked for: null until it is there (or for
// a size it has not).
const random_map::Font* GameFont(int size = 16);

// The game's dialog look for the app's own panels (app_ui): the parchment, the 800 x 600 dialog
// background its frame comes from, its big text button (released, pressed); each null where the
// game has none. Null until read (with the font).
struct GameDialogArt {
  const random_map::Image* parchment = nullptr;
  const random_map::Image* dialog = nullptr;
  const random_map::Image* button_up = nullptr;
  const random_map::Image* button_down = nullptr;
};
const GameDialogArt* DialogArt();
// The app's other controls over the game (the settings drawn into the frame: menu_button_drawn.cpp)
// on the one recomp::SetAppInputHook: asked first, each mouse and key event; true: taken (the game
// does not get it, and is woken to show what it changed).
void SetMenuInputHook(bool (*hook)(const SDL_Event&));
// The adventure screen's right dragon column as the game draws it (layers.adventure.1280's
// Right_Border, its 4-bit alpha kept), read with the font: null until it is there.
const random_map::Image* AdventureRightBorder();
// Its Top: the thin strip along the top (934 x 16; its end, under the dragon column, has the
// dragon's head).
const random_map::Image* AdventureTopStrip();

}  // namespace h4
