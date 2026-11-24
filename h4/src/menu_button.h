// The settings button (⚙︎) over the main menu, in every app but the plain desktop builds: shown
// while the main menu is the screen (resolution.cpp, every main loop pass), and the settings it
// opens, drawn into the game's frame in the game's dialogs' look (menu_button_drawn.cpp, app_ui/):
// the game's language (language.h), saved, then the app started again with it (or a message that
// it applies once the game is restarted), the interface's language, the scaling and more; the
// save sync (iOS, Android, the Mac: its ☁️ button left of ⚙︎, app_ui/native.h).
#pragma once

#include <functional>
#include <string>
#include <vector>

#include "side_panels.h"

#if defined(__APPLE__)
#include <TargetConditionals.h>
#endif

namespace h4 {

// Set by main.cpp for every app variant; only some variants draw native controls for it.
const std::string& GameFolder();
void SetGameFolder(const std::string& path);

#if defined(__ANDROID__) || (defined(__APPLE__) && (TARGET_OS_IOS || defined(H4_DESKTOP_APP))) || \
    defined(H4_LINUX_APP) || defined(H4_WINDOWS_APP) || defined(H4_SWITCH_APP)
// What the settings offer: the current language ("en", "uk") and the file the choice is saved
// to (main.cpp, at start). Without it no languages in them (Linux, the Switch: no button).
void SetLanguageChoice(const std::string& lang, const std::string& saved_path);
// Shows or hides it, its top-right corner at (x, y) in the game's frame (pixels; ☕ and ☁️ left
// of it); called on every main loop pass. (`left`: the menu's top-left corner's frame x, unused.)
void ShowLanguageButton(bool shown, int x, int y, int left = -1);
// The Mac, Linux and Android apps: the app started again as it was (a new language applied at its
// start).
void RestartApp();
// (x, y) in the frame -> points in a w x h view showing it (menu_button_*).
void FrameToView(int x, int y, float w, float h, float* vx, float* vy);
#if defined(__APPLE__) && (TARGET_OS_IOS || defined(H4_DESKTOP_APP))
// The settings' checkbox: save sync on (then the ☁️ button shows) or off (save_sync_apple.mm).
// SetSaveSync(true) on iOS opens the folder picker (on once one is picked); on the Mac it syncs
// with iCloud Drive (false when iCloud Drive is off). Returns whether it is on now.
bool ShowSyncButton();
bool SetSaveSync(bool on, void* source_view);
// iOS: true once, at the first main menu without a folder (sync is on by default: the app asks
// for the folder then; <data>/icloud.asked remembers it asked).
bool AskSaveSyncFolder();
// The saved games' folder here (<data>/data/h4/install/games), shown in the settings.
std::string SavesFolder();
// The folder the saves are synced with (iCloud Drive/ReHOMM4 or the one picked), "" when off.
std::string SyncedFolder();
// iOS and the Mac app: saved games kept in step with iCloud Drive/ReHOMM4
// (save_sync_apple.mm); a ☁️ button in the main menu's top-left corner sets it.
void StartSaveSync(const std::string& data_dir);
#endif
#else
// Elsewhere (the plain desktop builds) no button.
inline void SetLanguageChoice(const std::string&, const std::string&) {}
inline void ShowLanguageButton(bool, int, int, int = -1) {}
#endif

}  // namespace h4
