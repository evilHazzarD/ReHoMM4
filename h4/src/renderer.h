// Who draws the frame on phones and tablets: the GPU (SDL_GPU, the runtime's default) or the CPU
// (the runtime's soft device, H4_GFX=soft). <player>/renderer: "gpu", "soft", or "auto" (none: the
// default). Auto starts on the GPU and watches how long the game's present waits for it; a GPU too
// slow for the game (a frame waiting 50 ms and more, again and again: Adreno 610 class) is noted in
// <player>/renderer_auto, said on screen, and the CPU draws from the next start on.
#pragma once

#include <string>
#include <vector>

namespace h4 {

// Before the graphics device is made (main.cpp's MobileSetup); an H4_GFX of the environment wins.
void LoadRenderer(const std::string& data_dir);

// The game's present took `ms` (resolution.cpp's present hook): auto's measurement.
void NotePresent(double ms);

// For the settings' dropdown: "auto", "gpu", "soft"; the one chosen; one's name in the language
// `lang` ("Auto (GPU)", with "(from the next start)" when it is chosen and is not what draws now);
// choosing one (kept; from the next start).
std::vector<std::string> RendererChoices();
std::string RendererChoice();
std::string RendererChoiceName(const std::string& choice, const std::string& lang);
void SetRenderer(const std::string& choice);

// The interface's layout (resolution.cpp's H4_LAYOUT), <player>/<file>: "1280" (1280x1024), "1024"
// (1024x768: all of it a quarter larger) or "800" (800x600, larger still), chosen in the game's
// Options dialog; none chosen, `fallback`: the device's ("desktop": 1280 where the screen has room,
// else 1024, resolution.cpp). The Switch keeps one for handheld and one for the TV (`file`).
void LoadLayout(const std::string& data_dir, const std::string& fallback, const std::string& file = "layout");
void SaveLayout(int width);
// The Switch put in its dock or taken out: from now on `file` keeps the choice; the layout to use,
// its choice or `fallback` (800, 1024 or 1280).
int UseLayoutFile(const std::string& file, int fallback);
int CurrentLayout();  // 800, 1024 or 1280

}  // namespace h4
