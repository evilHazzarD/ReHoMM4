// The settings button (⚙︎) over the main menu (iOS, Android, the Mac, Linux and the Switch),
// shown while the main menu is the screen (resolution.cpp, every main loop pass). The settings:
// the game's language (language.h), saved, with a message that it applies once the game is
// restarted; and (iOS, Android, the Mac) a checkbox that shows the ☁️ save sync button in the
// main menu's top-left corner (off at first). Native buttons on iOS, Android and the Mac; drawn
// into the frame on Linux and the Switch (menu_button_linux.cpp: the languages only).
#pragma once

#include <functional>
#include <string>
#include <vector>

#include "side_panels.h"

#if defined(__APPLE__)
#include <TargetConditionals.h>
#endif

namespace h4 {

#if defined(__ANDROID__) || (defined(__APPLE__) && (TARGET_OS_IOS || defined(H4_DESKTOP_APP))) || \
    defined(H4_LINUX_APP) || defined(H4_SWITCH_APP)
// What the settings offer: the current language ("en", "uk") and the file the choice is saved
// to (main.cpp, at start). Without it no languages in them (Linux, the Switch: no button).
void SetLanguageChoice(const std::string& lang, const std::string& saved_path);
// Shows or hides it, its top-right corner at (x, y) in the game's frame (pixels;
// shown where the frame is, zoom included), the ☁️ button and "ReHoMM4 <version>" left of it
// (iOS, Android, the Mac); called on every main loop pass.
// `left`: the frame x of the menu's top-left corner, where iOS, Android and the Mac
// app put their ☁️ button (at the height y), when ShowSyncButton().
void ShowLanguageButton(bool shown, int x, int y, int left = -1);
// The game's folder as the app found it (main.cpp; "" before), shown in the settings.
const std::string& GameFolder();
void SetGameFolder(const std::string& path);
// Mac and Linux apps: the app started again as it was (a new language applied at its start).
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
void ShowSaveSyncSheet(void* source_view, bool ukrainian);
#endif
#if defined(__APPLE__) && !TARGET_OS_IOS && defined(H4_DESKTOP_APP)
// The Mac app's dialogs (the language list, its note, the iCloud one): a panel drawn over the game
// in its own window, not a sheet, which takes the window's focus and leaves the system cursor
// showing next to the game's (menu_button_mac.mm). `done` gets the button's index
// (Esc: the last one, Return: the first). `checks`: checkboxes, each above its button; the panel
// stays up while they are clicked (`changed` gets the state).
struct MacCheck {
  std::string label;
  bool on = false;
  std::function<bool(bool)> changed;  // -> the box's state now
  int before = -1;  // above this button (-1: the last one)
};
void ShowMacChoice(void* any_view, const std::string& title, const std::string& text,
                   const std::vector<std::string>& buttons, std::function<void(int)> done,
                   const std::vector<MacCheck>& checks = {});
#endif
#else
// Elsewhere (the plain desktop builds) no button.
inline void SetLanguageChoice(const std::string&, const std::string&) {}
inline void ShowLanguageButton(bool, int, int, int = -1) {}
#endif

}  // namespace h4
