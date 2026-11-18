# Heroes IV apps: iPhone, iPad, Android, Linux, Mac and Switch

The app builds of this port (`platforms/`): the same static recompilation of `heroes4.exe`, built for
arm64 iOS (Metal), arm64 Android (Vulkan), Linux x86-64, the Mac and the Switch. No game files come
with the apps: each one installs them from the player's GOG account or GOG installer. The ready apps
are on the Releases page (how to install them: the main README); this file is about building them
yourself. There is no App Store or Play Store version.

Status (2026-11-17): boots, plays the intro videos and reaches the adventure map on an iPhone 12
(iOS, Metal, free Apple ID) and a Xiaomi 2412DPC0AG (Android 16, Mali-G720, Vulkan). Not yet played
through; see *Known issues*.

## What you need

- An Apple Silicon Mac with Xcode (iOS SDK) and/or the Android SDK + NDK (Android Studio's SDK
  Manager: NDK 29), Homebrew, OpenJDK 17 for Android (`brew install openjdk@17`).
- Heroes of Might and Magic IV: Complete from GOG (the Windows installer), as for the Mac build.
- About 10 GB free disk.
- iOS: an Apple ID signed in to Xcode (Xcode > Settings > Accounts). A free account works: the app
  then has to be re-installed every 7 days (Apple's limit for free provisioning).
- Android: a 64-bit device with Vulkan, Android 10 (API 29) or later, USB debugging on.

## Build

```sh
tools/build.sh                     # once: game/, x86recomp + patches/, the lifted game (h4/generated)
platforms/deps.sh ios                 # SDL3, glslang, SPIRV-Cross, LGPL ffmpeg for iOS   (~5 min)
platforms/build.sh ios <TEAM_ID>      # -> .cache/outputs/ios/dist/HeroesIV-ios.ipa (+ the .app in build/)
platforms/deps.sh android             # the same for Android
platforms/build.sh android            # -> .cache/outputs/android/dist/HeroesIV-android_arm64.apk
```

`tools/build.sh` also builds the Mac DMG; for the mobile builds only its first steps are needed
(stop it after "Recompiling the game" if you like). It applies every `patches/x86recomp-*.patch`,
including `x86recomp-mobile.patch`.

Your team ID: Xcode > Settings > Accounts > your Apple ID > the team's ID (ten characters), or leave
it out to only compile. You can also open `.cache/outputs/ios/build/h4_recomp.xcodeproj`, choose the
`h4` scheme and your device, set the team under Signing, and press Run.

Each one carries `THIRD_PARTY_NOTICES.txt` (the iOS/macOS bundle's Resources, the APK's assets,
beside the Linux binary, in the Switch romfs and beside the .nro): the project's MIT license and
the license of every library linked in, made by `platforms/notices.py` from the build's own link line
(the license texts that no source tree carries are in `licenses/`). The build stops if a
library without a known license is linked.

The app icon is the project's own: a gold-rimmed shield with "IV", drawn at build time by
`assets/icon/icon.swift` (nothing taken from the game), in each platform's form by
`assets/icon/icon.sh` (iOS asset catalog, Android adaptive icon, macOS .icns, Linux PNG, Switch
JPEG). Some launchers keep an old icon until they are restarted.

## Install the app, then the game

**iOS**

```sh
xcrun devicectl list devices
xcrun devicectl device install app --device <id> ".cache/outputs/ios/build/Release-iphoneos/Heroes IV.app"
```

First start: Settings > General > VPN & Device Management > trust your Apple ID.

`HeroesIV-ios.ipa` (`Payload/Heroes IV.app`, unsigned without a team ID) is the same app for
sideloading tools that sign it with your own Apple ID (AltStore, Sideloadly).

**Android**

```sh
adb install -r .cache/outputs/android/dist/HeroesIV-android_arm64.apk
```

**The game files.** Without them the app opens its installer (English Heroes IV Complete, the GOG
version this port is made for):

- **Sign in to GOG.com and download.** GOG's own login page opens in the app; the app checks that
  your account owns *Heroes of Might and Magic 4: Complete* and downloads its files (1.0 GB) from
  GOG. Signing in with Google, Discord, Steam or Xbox works too (their popup opens over GOG's
  page). Cancelled or interrupted, it continues where it stopped. The sign-in is kept only until the
  download is done (iOS Keychain; Android: the app's private storage).
- **Install from GOG installer files.** GOG's offline installer,
  `setup_heroes_of_might_and_magic_4_complete_3.0_win11_gog_0.2_(76223).exe` and its `...-1.bin`:
  pick both (or a folder holding them), or put them in the app's folder first (iOS: Finder or the
  Files app > On My iPhone > Heroes IV; Android: `adb push` to
  `/sdcard/Android/data/games.hazzard.heroes4/files/`) and they are found by themselves. The app
  unpacks them with innoextract built in (`libs/innoextract`).

Every file is checked against `h4/src/install/manifest_en.inc` (size and SHA-256 of the 144 game
files: no game content), then the game starts. Copying a ready `game` folder (the one
`tools/build.sh` unpacks) into the app's folder still works and skips the installer.

**Saves in iCloud (iPhone, iPad, Mac app).** The saved games are kept in step with
`iCloud Drive/ReHOMM4` on every device signed in to the same Apple ID: copied both ways, file by
file, the newer copy winning, at start, every 20 s, when the app goes to the background (iOS) or
loses the focus or quits (Mac), and on "Sync now". Nothing is deleted on the other side. The Mac
app does it by itself (the folder is made the first time). On iPhone and iPad, once: the ☁️
button in the main menu's top-left corner, "Choose a folder…", open iCloud Drive and tap Open:
the app makes `ReHOMM4` there (or uses the folder picked, when it is called ReHOMM4). The same
button shows the folder and the last sync, "Sync now", another folder and "Stop syncing". (A
picked folder on iOS because Apple gives an app its own iCloud container only on a paid developer
account; `save_sync_apple.mm`.)

**Syncthing, rclone and other sync apps (every platform but the Switch).** The same sync works
with any folder a sync app keeps in step; see [Syncing saves between devices](#syncing-saves-between-devices)
for how to set up each one.

**Language.** The button in the main menu's top right corner (🌐 English, 🌐 Українська, ...)
opens the list of languages; the app then restarts in the one picked (Android relaunches itself;
iOS cannot, so the app closes and you open it again). On Linux and the Switch the list is drawn
into the frame.

The translations are a separate repository, [ReHoMM4-translations](https://github.com/evilHazzarD/ReHoMM4-translations),
managed and released on their own; this repository does not include it (its `TRANSLATING.md`
explains how to add a language). They reach players **only through a release** of it (its
`tools/release.sh`). Its
`languages.json` lists each language with its pack (`<code>.h4p`: the difference between the
game's archives and the translated ones, ~34 MB for Ukrainian). On Linux and the Switch the
button and the list are put together from generic pictures in the app ("🌐 UK": the language's
code), so a new language needs nothing more there.

- **The app** reads the latest release's `languages.json` at every start, in the background, and
  keeps it in `<player files>/languages.json`. A new language shows in the list at once. A
  language's pack is downloaded when it is chosen, and again when a release changes it (at the
  next start); its size and SHA-256 are checked. Packs are kept in `<player files>/packs`.
- **The build** (`platforms/languages.py`) only takes the latest release's list, so a first start
  offline already offers the languages. The address is
  `https://github.com/evilHazzarD/ReHoMM4-translations/releases/latest/download/languages.json`
  (`H4_TRANSLATIONS_REPO=owner/name` for another repository).
  `H4_TRANSLATIONS_URL` points at another `languages.json` (a test release served by
  `H4_RELEASE_BASE=... tools/release.sh` there). `H4_TRANSLATIONS_DIR=<a checkout of it>` builds
  every language there from your `game/` into the app, to test translations that are not released yet.
- **The Mac DMG** installer downloads each language's pack while it installs the game
  (`platforms/macos-dmg/apply_pack.py` applies it; offline, that language plays English).

On the device the pack is applied to the game folder in place: its data is appended to five
archives once, and the language is which of their directory entries are written (a few hundred
bytes, both ways), so there is no second 1 GB copy. One translation's data is in the archives at a
time; choosing another takes the first one out. A `game-<code>` folder copied in by hand is still
used when that language is chosen. `H4_LANG=<code>` (Android: `--es args '--lang=uk'`) overrides
the saved choice.

Other languages GOG sells (de, fr, es, it, pl) are not offered yet: each has its own
`heroes4.exe`, and this port is recompiled from the English one.

The installer screen turns with the device (portrait or landscape); the game itself is landscape
only.

The side panels beside the centred screens (main menu, towns, combat) are the Mac build's wings,
as JPEGs (quality 90; `H4_WINGS_QUALITY`):
`platforms/build.sh` takes them from `art/combat_wings` (or `heroes4-mac-assets.zip` from Releases in
Downloads, as `tools/build.sh`) through `platforms/wings.sh` and puts them in the app (iOS bundle;
Android assets, copied into private storage at first start). A `wings` folder of your own (JPEGs:
`paradise.jpg`, `hell.jpg`, `<faction>_light.jpg`, `<faction>_dark.jpg`) next to `game` overrides them. Without the art the sides stay black.

Testing the installer next to an existing install: start the app with `H4_FILES_DIR=<subfolder>`
(e.g. `xcrun devicectl device process launch ... --environment-variables '{"H4_FILES_DIR":"t"}'`;
Android: `adb shell "am start -n games.hazzard.heroes4/games.hazzard.heroes4.HeroesActivity --es args '--files-dir=t'"`),
plus `H4_INSTALL_AUTO` / `--install-auto=files|gog` to press a button at start;
`platforms/tools/install_test/run.sh` runs the installer's core on the Mac.

## Controls

| Touch | Mouse / keyboard |
|---|---|
| Tap | left click |
| Double tap (within ½ s, the second tap near the first) | double click, on the first tap's spot |
| Touch and hold (½ s) | right button, held until you lift the finger (the game's info pop-ups) |
| Touch, then drag | left-button drag |
| Pinch | zooms the picture (up to 3x) around your fingers; pinch out to see everything again |
| Two-finger drag | zoomed: moves the zoomed view, and scrolls the adventure map past its edge; not zoomed: arrow keys, scrolls the adventure map |
| Three-finger tap | shows or hides the on-screen keyboard (hot keys, save names) |

A mouse, trackpad or keyboard connected to the device works as on the Mac.

## Syncing saves between devices

The game copies its saves to and from a folder called **`ReHOMM4`** (at start, every 20 s, when
it goes to the background or quits, and on "Sync now"): file by file, the newer copy wins, nothing
is ever deleted. Another app moves that folder between your devices. Set up one of the options
below on every device you play on, then check it: save on one device, wait a minute, and the save
is in the other device's load list.

Where the `ReHOMM4` folder is on each device:

| Device | `ReHOMM4` folder | How the game finds it |
|---|---|---|
| Mac app | `iCloud Drive/ReHOMM4` (`~/Library/Mobile Documents/com~apple~CloudDocs/ReHOMM4`) | by itself; ☁️ → "Choose another folder…" for another one |
| Linux / Steam Deck | `~/ReHOMM4` | by itself (made at start); `folder=` in `sync.conf` for another one |
| Android | `ReHOMM4` inside the folder you pick, e.g. `Documents/ReHOMM4` | ☁️ → "Choose a folder…" once |
| iPhone / iPad | `ReHOMM4` inside the folder you pick in Files | ☁️ → "Choose a folder…" once |

A picked folder that is itself called `ReHOMM4` is used as it is (nothing made inside it).
`sync.conf` (Mac and Linux) is a text file in the app's player folder, made by you:
Mac `~/Library/Application Support/ReHoMM4/player/sync.conf`, Linux and Steam Deck
`~/.local/share/ReHoMM4/player/sync.conf`.

### iCloud Drive (Mac, iPhone, iPad)

Nothing to install; iCloud Drive must be on (System Settings / Settings → your name → iCloud →
iCloud Drive) with the same Apple ID on every device, and **with free iCloud storage**: when it is
full, iCloud keeps every file on its own device and nothing reaches the others (the ☁️ sheet then
says "not in iCloud yet: iCloud storage is full").

1. **Mac:** start the app once: it makes `iCloud Drive/ReHOMM4` and syncs it.
2. **iPhone and iPad:** ☁️ → "Choose a folder…" → in the picker open **iCloud Drive** (Browse →
   iCloud Drive) and tap **Open**: the game makes `ReHOMM4` there. (Or open the `ReHOMM4` folder
   the Mac made and tap Open.)

### Syncthing (every device, no cloud account)

Syncthing copies the folder straight between your devices, over your Wi-Fi or the internet; two
devices sync while both are on. Install it:

- **Android:** *Syncthing-Fork* (F-Droid, or Google Play). Give it the storage permission it asks
  for ("All files access"), or it cannot reach `Documents/ReHOMM4`.
- **Mac:** `brew install --cask syncthing` (or the app from syncthing.net); it runs in the menu bar.
- **Linux:** your distribution's `syncthing` package (`sudo apt install syncthing`, then
  `systemctl --user enable --now syncthing`); its settings are at http://127.0.0.1:8384.
- **Steam Deck:** in Desktop Mode, Discover → *SyncThingy* (Syncthing as a Flatpak), or the
  Syncthing plugin of Decky Loader.
- **iPhone / iPad:** *Möbius Sync* (App Store).

Then, once:

1. **Make the folder on one device.** Android: in the game, ☁️ → "Choose a folder…" → open
   **Documents** → "Use this folder" → Allow; the game makes `Documents/ReHOMM4`. Linux: start
   the game once (it makes `~/ReHOMM4`). Mac: start the app once (`iCloud Drive/ReHOMM4`).
2. **Pair the devices.** In Syncthing on one device: *Add Remote Device* and enter the other's
   *Device ID* (Actions → Show ID; on Android scan its QR code). Accept on the other device.
3. **Share the folder.** On the device from step 1: *Add Folder*, *Folder Label* `ReHOMM4`,
   *Folder Path* the `ReHOMM4` folder from the table above (Android:
   `/storage/emulated/0/Documents/ReHOMM4`), and under *Sharing* tick the other devices.
4. **Accept it on each other device** (a "wants to share folder ReHOMM4" notice → Add), and set
   its *Folder Path* to that device's `ReHOMM4` folder from the table. On iPhone/iPad accept it in
   Möbius Sync, then in the game ☁️ → "Choose a folder…" → Browse → **Möbius Sync** → open the
   `ReHOMM4` folder → Open.
5. Recommended: the folder's *File Versioning* → *Simple*: Syncthing then keeps the copies it
   replaces (the game keeps none).

The Mac can share `iCloud Drive/ReHOMM4` itself: the Mac, iPhone and iPad then meet through
iCloud, and Android and Linux through Syncthing, all in one folder. Without iCloud on the Mac, ☁️ →
"Choose another folder…" → pick (or make) a folder called `ReHOMM4`, e.g. in your home folder, and
share that one.

### rclone (Mac and Linux: Google Drive, Dropbox, OneDrive, WebDAV and 70 more)

The app runs rclone itself and keeps a cloud folder `ReHOMM4` in step with the local one (about
every minute, right after a save, and at quit; `rclone copy --update` both ways, never deleting).

1. **Install rclone.** Mac: `brew install rclone`. Linux: `sudo apt install rclone` (or the
   distribution's package; for the newest, the zip from rclone.org/downloads as for the Steam Deck).
   Steam Deck (its system is read-only): download the Linux amd64 zip from rclone.org/downloads,
   put `rclone` in `~/.local/bin` and `chmod +x ~/.local/bin/rclone`. The app also looks in
   `/opt/homebrew/bin`, `/usr/local/bin` and `/usr/bin`.
2. **Connect your cloud:** `rclone config` → `n` (new remote) → a name, e.g. `gdrive` → the
   storage (Google Drive, Dropbox, OneDrive...) → the defaults, and sign in in the browser it opens.
   Check it with `rclone lsd gdrive:`.
3. **Tell the app:** add to `sync.conf` (see the table above for where it is):
   ```
   rclone=gdrive:ReHOMM4
   ```
4. **Restart the game.** Its log (`player/logs/h4.log`) says
   `save sync with ... and rclone gdrive:ReHOMM4`; a problem shows as
   `h4: save sync: rclone failed (...)` or `rclone is not installed`.

On another Mac or Linux device use the same remote and folder (`gdrive:ReHOMM4`). On the Mac the
cloud folder is kept in step with `iCloud Drive/ReHOMM4`, so iPhone and iPad get the saves through
iCloud. For Android use Round Sync or FolderSync below with the same cloud folder.

### Round Sync (Android: rclone for Android)

1. Install *Round Sync* (Google Play, F-Droid or its GitHub page).
2. **Remotes** → add your cloud (the same one as on your other devices), sign in.
3. In the game, ☁️ → "Choose a folder…" → **Documents** → "Use this folder" → Allow (the game makes
   `Documents/ReHOMM4`).
4. In Round Sync, **Tasks** → new task: the remote's `ReHOMM4` folder and the local
   `Documents/ReHOMM4`, direction **both ways** (bisync; the name differs between versions), and
   give it a schedule (**Triggers**, e.g. every 15 minutes) so it runs without you.

### FolderSync (Android: Google Drive, Dropbox, OneDrive, Nextcloud, WebDAV...)

1. Install *FolderSync* (Google Play). **Accounts** → add your cloud, sign in.
2. In the game, ☁️ → "Choose a folder…" → **Documents** → "Use this folder" → Allow.
3. In FolderSync, **Folderpairs** → add: the account, remote folder `ReHOMM4` (make it), local
   folder `Documents/ReHOMM4`, sync type **Two-way**, and a schedule (and "Sync on change" for the
   local folder, if your version has it).

### Nextcloud

- **Mac and Linux:** the Nextcloud desktop client syncs a folder (`~/Nextcloud` by default). Make
  `ReHOMM4` in it, then point the game at it: Linux `folder=~/Nextcloud/ReHOMM4` in `sync.conf`;
  Mac ☁️ → "Choose another folder…" → that `ReHOMM4` folder.
- **Android:** install the *Nextcloud* app and sign in; in the game ☁️ → "Choose a folder…" → in the
  picker's menu choose **Nextcloud** → open (or make) `ReHOMM4` → "Use this folder". The game then
  writes into Nextcloud directly (it needs the network for each sync). FolderSync with a Nextcloud
  account (above) works too, and also offline.
- **iPhone / iPad:** install the *Nextcloud* app; in the game ☁️ → "Choose a folder…" → Browse →
  **Nextcloud** → the `ReHOMM4` folder → Open.

### Good to know

- Pick one way and the same `ReHOMM4` on every device; two ways at once (say Syncthing and rclone
  into the same cloud folder) work but bring nothing more.
- Google Drive's and Dropbox's own Android apps do not let other apps pick a folder in them: use
  Round Sync or FolderSync for those clouds.
- A save deleted in the game stays on the other devices (nothing is deleted). A save changed on two
  devices between syncs keeps the newer one.
- Linux and Steam Deck: `off` in `sync.conf` stops syncing; `H4_SYNC_FOLDER` and `H4_SYNC_RCLONE`
  (environment) override the file. Its passes are logged as `h4: save sync ...` in `h4.log`.
- The Switch has no sync app, so its saves stay on the SD card
  (`sdmc:/switch/ReHoMM4/player`): copy them by hand.

## Saves and logs

- iOS: in the app's container, `Library/Application Support/player` (`logs/h4.log`, `data/h4/...`).
  Xcode > Window > Devices and Simulators > the app > Download Container.
- Android: next to the game, `/sdcard/Android/data/games.hazzard.heroes4/files/player`
  (`adb pull /sdcard/Android/data/games.hazzard.heroes4/files/player/logs/h4.log`).

## How it differs from the Mac build

All in `patches/x86recomp-mobile.patch`, `h4/CMakeLists.txt`, `h4/src/main.cpp` and `platforms/`:

- **Guest arena.** iOS refuses a 4 GiB reservation without the extended-virtual-addressing
  entitlement (paid accounts only). `heroes4.exe` is not large-address-aware, so everything it
  and the runtime use is below 2 GiB: the runtime falls back to a 2 GiB arena, and refuses to run a
  large-address-aware EXE in one.
- **Static, one binary.** The runtime and HLE libraries are static on mobile (linked whole: their
  handlers register from static initialisers), with SDL3 (static on iOS), ffmpeg, glslang and
  SPIRV-Cross. The Mac keeps its shared libraries.
- **Platform layer.** An arm64 stack switch replaces `makecontext` (missing in bionic, deprecated
  on iOS); Android reads faults from its `sigcontext` and resumes guest exceptions with the same
  call redirect as macOS; backtraces use the unwinder where bionic lacks `execinfo` (before API 33).
- **Touch** (runtime `user32_window.cpp`, `touch::`): fingers become the mouse; Mac trackpads are
  not affected (only direct touch screens). **Background**: iOS pauses the game while the app is
  in the background (SDL does it on Android).
- **No launcher.** `main.cpp` does the launcher's job: data and log folders, the widescreen/filter
  defaults, finding `game`.
- **In-app installer** (`h4/src/install/`): a screen drawn with SDL, the GOG Galaxy download
  (`gog.cpp`), innoextract as a library (`inno.cpp`, `libs/innoextract/`, from HommaRe, MIT) and a
  small platform layer (`platform_apple.mm`, `platform_android.cpp` + `HeroesActivity.java`).
- Not built for mobile: the lifter and dev tools (they run on the Mac), `recomp_dbghelp` (needs
  LLVM; Heroes IV does not import dbghelp).

## Known issues

- Lightly tested: the guest fault path (Android) and the touch thresholds have not been exercised
  much yet. The iPhone 12 gave the full 4 GiB arena on a free account; the 2 GiB fallback is there
  for devices that refuse it.
- Occasional frames just over 40 ms while maps load. Profiling (Android, simpleperf: the APK is
  `profileable`) found most of the game thread's time in per-pixel blit copies; with fast paths for
  unscaled blits and 16-bit colour conversion, the game thread uses about a third less CPU and the
  stutter while units walk is gone.
- Android: user clip planes are off (Mali has no shaderClipDistance); Heroes IV does not use them.
- The interface is 1280x1024 scaled to the screen: small on phones, fine on tablets.
- iPhone: the game is kept inside the screen's safe area (clear of the notch and rounded corners on
  both sides, so its edge buttons can be pressed); the black margins are on purpose. iPads use the whole screen.
  `H4_SAFE_AREA=0` (Xcode scheme > Environment) uses the whole width.
- The combat "wings" art (`heroes4-mac-assets.zip`) is optional: a `wings` folder of JPEGs
  (`paradise.jpg`, `hell.jpg`, `<faction>_light.jpg`, `<faction>_dark.jpg`; `platforms/wings.sh` makes
  them) next to `game` is used over the app's.
- Android TLS: the runtime's thread-locals use the default model there (bionic refuses
  initial-exec in a dlopen'ed library); see the note in x86recomp's CMakeLists.txt.

## Linux and the Steam Deck

`platforms/build.sh linux` cross-builds it on the Mac for Linux x86-64 (glibc 2.36+: SteamOS 3,
Debian 12, Ubuntu 22.10 and later): `.cache/outputs/linux/dist/HeroesIV-linux-x86_64.tar.gz`, a
folder with `heroes4`, `lib/libSDL3.so.0`, the wings and (when built in) the translation. It is
the mobile app's way of working: the in-app installer (GOG sign-in in the browser: the address
it ends on is copied or pasted back; or the GOG installer files from `~/Downloads`, or picked with
the desktop's dialog: kdialog/zenity), the game and saves in `~/.local/share/ReHoMM4/`, the
language button drawn into the frame (pictures in `ui/`, made by `platforms/linux/render_ui.swift`),
the translation downloaded when first chosen (or built in). Video: SDL3 on X11 (Game Mode's gamescope
and the desktop's Xwayland) with Vulkan; audio through PipeWire, PulseAudio or ALSA, all loaded at
run time, so the binary needs only libc, libm and zlib.

The toolchain: Homebrew's clang aimed at a Debian 12 sysroot that `platforms/linux/sysroot.py`
assembles from Debian's packages (`.cache/outputs/linux/sysroot`, ~100 MB), linked by the Android
NDK's `ld.lld`; `platforms/deps.sh linux` builds SDL3, glslang, ffmpeg and innoextract for it; the
runtime is linked in statically (`RECOMP_STATIC_APP`). On the Deck: Desktop Mode, unpack, then in
Steam *Add a Non-Steam Game* > `heroes4`; Launch options `--lang=uk` for Ukrainian (any language code the app has).

## Mac app (Apple Silicon and Intel)

`platforms/build.sh macos-arm64` makes the same app for the Mac (Apple Silicon, macOS 13+):
`.cache/outputs/macos-arm64/dist/Heroes IV.app` and `HeroesIV-macos-arm64.dmg`. `platforms/build.sh
macos-x86_64` makes it for Intel Macs (cross-built on an Apple Silicon Mac, which runs it under Rosetta for
testing): `.cache/outputs/macos-x86_64/dist/`, `HeroesIV-macos-x86_64.dmg`. One binary (runtime and
SDL3 static, Metal), the in-app installer (GOG sign-in in a web view over the game, or installer
files from `~/Downloads` or picked in the standard file dialog), the language button, the game and saves in
`~/Library/Application Support/ReHoMM4/`, the wings and translation as on the phones
(`--lang=uk`). Ad-hoc signed: on another Mac, right-click > Open the first time.

The language button is the same everywhere (iOS, Android, Mac, Linux, Switch): it shows the
language chosen, a tap opens the list of languages, and a message says the game must be restarted
to apply the one picked.

## Nintendo Switch (homebrew)

`platforms/build.sh switch` makes `.cache/outputs/switch/dist/HeroesIV.nro`, for a Switch running
Atmosphère's homebrew menu. It needs devkitPro (`/opt/devkitpro`: the installer from
github.com/devkitPro/pacman, then `dkp-pacman -S switch-dev switch-mesa switch-libdrm_nouveau
switch-zlib switch-bzip2 switch-liblzma switch-curl switch-mbedtls`); `platforms/deps.sh switch`
builds devkitPro's SDL 3.4 Switch branch, the LGPL ffmpeg and innoextract for it.

How it differs: no SDL_GPU on the Switch, so the frames are shown by x86recomp's soft gfx device
(2D on the CPU, SDL_Renderer on OpenGL ES; `RECOMP_GPU=OFF`); the guest's 2 GiB arena is one heap
block (no mmap), so the game must run in title takeover (hold R while starting a game, then pick
it in the homebrew menu; the album applet has too little memory); no signals (no guest SEH for
faults, no watchdog); file views are copies. Input: the touch screen as on the phones, the Joy-Cons
as the mouse (left stick the pointer, A click, B right-click, right stick / D-pad the arrow keys,
+ Escape, X Enter).

On the SD card: `sdmc:/switch/ReHoMM4/HeroesIV.nro` and, beside it, the GOG installer files
(`setup_heroes_of_might_and_magic_4_complete_...exe` and `-1.bin`): the installer finds them there
(no file picker or GOG sign-in on the Switch); the game goes to `.../ReHoMM4/game`, saves to
`.../player`. The wings, the language button's pictures and (when built in) the translation are
in the .nro's romfs.

