// The language button over the main menu (iOS, Android, the Mac and Linux
// apps), shown while the main menu is the screen (resolution.cpp, every main
// loop pass): it shows the language, and a click switches to the other one
// (language.h), saved, with a message that it applies once the game is
// restarted. Native buttons on iOS, Android and the Mac; drawn into the frame
// on Linux (menu_button_linux.cpp).
#pragma once

#include <functional>
#include <string>
#include <vector>

#if defined(__APPLE__)
#include <TargetConditionals.h>
#endif

namespace h4 {

#if defined(__ANDROID__) || (defined(__APPLE__) && (TARGET_OS_IOS || defined(H4_DESKTOP_APP))) || \
    defined(H4_LINUX_APP) || defined(H4_SWITCH_APP)
// What the button offers: the current language ("en", "uk") and the file
// the choice is saved to (main.cpp, at start). Without it no button.
void SetLanguageChoice(const std::string& lang, const std::string& saved_path);
// Shows or hides it, its top centre at (x, y) in the game's frame (pixels;
// shown where the frame is, zoom included); called on every main loop pass.
// `left`: the frame x of the menu's top-left corner, where iOS and the Mac
// app put their ☁️ button (at the height y).
void ShowLanguageButton(bool shown, int x, int y, int left = -1);
// (x, y) in the frame -> points in a w x h view showing it (menu_button_*).
void FrameToView(int x, int y, float w, float h, float* vx, float* vy);
#if defined(__APPLE__) && (TARGET_OS_IOS || defined(H4_DESKTOP_APP))
// iOS and the Mac app: saved games kept in step with iCloud Drive/ReHOMM4
// (save_sync_apple.mm); a ☁️ button in the main menu's top-left corner sets it.
void StartSaveSync(const std::string& data_dir);
void ShowSaveSyncSheet(void* source_view, bool ukrainian);
#endif
#if defined(__APPLE__) && !TARGET_OS_IOS && defined(H4_DESKTOP_APP)
// The Mac app's dialogs (the language list, its note, the iCloud one): a panel drawn over the game
// in its own window, not a sheet, which takes the window's focus and leaves the system cursor
// showing next to the game's (menu_button_mac.mm). `done` gets the button's index
// (Esc: the last one, Return: the first).
void ShowMacChoice(void* any_view, const std::string& title, const std::string& text,
                   const std::vector<std::string>& buttons, std::function<void(int)> done);
#endif
#else
// Elsewhere (the plain desktop builds) no button.
inline void SetLanguageChoice(const std::string&, const std::string&) {}
inline void ShowLanguageButton(bool, int, int, int = -1) {}
#endif

}  // namespace h4
