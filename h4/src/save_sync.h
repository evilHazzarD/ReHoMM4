// Save sync with a folder (Syncthing) and an rclone remote, for the desktop
// apps (save_sync.cpp); the Mac's iCloud part is save_sync_apple.mm.
#pragma once

#include <string>

namespace h4 {

// <data>/sync.conf: `folder=<path>` (Linux; default ~/ReHOMM4), `rclone=<remote:path>`,
// `off`; H4_SYNC_FOLDER / H4_SYNC_RCLONE override.
struct SyncConfig {
  std::string folder, rclone;
  bool off = false;
};
SyncConfig ReadSyncConfig(const std::string& data_dir);

// One pass between the saved games under <data_dir> and `folder` (made when missing).
// Returns the status line ("HH:mm: N save(s) in ReHOMM4, X sent, Y received").
std::string SyncFolderPass(const std::string& data_dir, const std::string& folder, int* up = nullptr,
                           int* down = nullptr);

// `rclone copy --update` remote -> folder (down) or folder -> remote; "" when done (or no
// remote), else what failed.
std::string RcloneAround(const std::string& remote, const std::string& folder, bool down);

#if defined(H4_DESKTOP_APP) && !defined(__APPLE__)
// Linux: syncing from now on (a thread; a last pass at exit). Apple: menu_button.h.
void StartSaveSync(const std::string& data_dir);
#endif

}  // namespace h4
