// The in-app installer (iOS, Android): shown when the app has no game files.
// The player signs in to GOG.com and the app downloads the game from their
// library, or picks GOG's offline installer files and the app unpacks them;
// either way every file is checked against the manifest. Copying a "game"
// folder in by hand (Finder, adb) still works and skips this.
#pragma once

#include <functional>
#include <string>

#include "task.h"

namespace h4::install {

// Runs the installer screen until the game is in `files_dir`/game; returns
// that folder, or "" if the player left. Main thread; SDL video is shut
// down again before it returns (the game sets it up its own way).
// `autostart` ("files" or "gog", for testing on a device without touching
// it) presses that button at once.
std::string RunInstaller(const std::string& files_dir, const std::string& autostart = "");

// Downloads `url` (`size` bytes, SHA-256 `sha256` in hex) to `dest` on a
// progress screen headed `title`, with Cancel; an unfinished download
// (<dest>.part) is continued. "" when done, else why not ("cancelled").
std::string DownloadWithProgress(const std::string& title, const std::string& url, uint64_t size,
                                 const std::string& sha256, const std::string& dest);

// `work` on its own thread, with the same progress screen (its Task: phase, bytes, cancel): ""
// when it returned true, else its error ("failed" without one).
std::string RunWithProgress(const std::string& title, const std::function<bool(Task*, std::string*)>& work);

}  // namespace h4::install
