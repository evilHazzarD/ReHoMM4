// Who draws the frame on phones and tablets: the GPU (SDL_GPU, the runtime's default) or the CPU
// (the runtime's soft device, H4_GFX=soft). <player>/renderer: "gpu", "soft", or "auto" (none: the
// default). Auto starts on the GPU and watches how long the game's present waits for it; a GPU too
// slow for the game (a frame waiting 50 ms and more, again and again: Adreno 610 class) is noted in
// <player>/renderer_auto, said on screen, and the CPU draws from the next start on.
#pragma once

#include <string>

namespace h4 {

// Before the graphics device is made (main.cpp's MobileSetup); an H4_GFX of the environment wins.
void LoadRenderer(const std::string& data_dir);

// The game's present took `ms` (resolution.cpp's present hook): auto's measurement.
void NotePresent(double ms);

// For the settings: "Renderer: Auto (GPU)" ("Рендер: ..." when `uk`), UTF-8, with "(from the next
// start)" when the choice differs from what draws now; NextRenderer() picks the next choice (kept).
std::string RendererLabel(bool uk);
void NextRenderer();

}  // namespace h4
