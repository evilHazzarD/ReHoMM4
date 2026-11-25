// What the drawn settings (menu_button_drawn.cpp) ask of the system they run on: the few things only
// it can do (a browser, its files app, its folder picker). Each app implements its own: the Mac's in
// menu_button_mac.mm, iOS's in menu_button_apple.mm, Android's in menu_button_android.cpp (through
// HeroesActivity), Linux's and Windows' in menu_button_drawn.cpp; none on the Switch and in the
// browser (false).
#pragma once

#include <functional>
#include <string>
#include <utility>
#include <vector>

namespace h4::native {

// The browser at a web address (the author's Ko-fi page, a release's page); a local file's path
// opens that file as the desktop would (a downloaded .dmg mounted). False: nothing to open it with.
bool OpenUrl(const std::string& url);
// The files app (Finder, Explorer, Android's Files) at the folder `path`. False: none.
bool RevealFolder(const std::string& path);

// The save sync where the system keeps it (iOS, the Mac: iCloud Drive or a picked folder,
// save_sync_apple.mm; Android: a picked folder, SaveSync.java). Elsewhere (Linux, Windows) it is
// set up in sync.conf (save_sync.h), and the settings only say how.
bool HasSyncChoice();
bool SyncOn();
// On: a folder picked (the system's picker; on once one is), or (the Mac) iCloud Drive; off: stops.
void SetSync(bool on);
// The ☁️ button's panel: what the sync is doing and what can be done (its buttons' actions may open
// the system's folder picker).
struct SyncPanel {
  std::string title, text;
  std::vector<std::pair<std::string, std::function<void()>>> buttons;
};
SyncPanel SyncPanelFor(const std::string& lang);
// The saves' folders to show (the settings' Folders panel): "Saves (synced)", "Saves", "Game
// folder" and their paths (empty: not shown).
std::string SavesFolder();
std::string SyncedFolder();

// Android: the update's .apk downloaded by the system (its notification installs it).
void DownloadApk(const std::string& url, const std::string& tag);

}  // namespace h4::native
