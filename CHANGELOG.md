# Changelog

Each release's notes on GitHub are its section here (tools/release/make_release.sh). Newest first.

## 0.5

Settings drawn in the game's style on every device, "What's new" and update checks, more readable layouts on small screens, and more GOG editions.

| Device | File |
|---|---|
| Mac, macOS 13+ | `reHoMM4-v0.5-macos-arm64.dmg` (Apple Silicon) or `reHoMM4-v0.5-macos-x86_64.dmg` (Intel) |
| iPhone and iPad, iOS / iPadOS 17+ | `reHoMM4-v0.5-ios.ipa` |
| Android 10+, 64-bit | `reHoMM4-v0.5-android_arm64.apk` |
| Windows 10 or 11, 64-bit | `reHoMM4-v0.5-windows-x86_64.zip` |
| Linux and Steam Deck | `reHoMM4-v0.5-linux-x86_64.tar.gz` or `reHoMM4-v0.5-linux-arm64.tar.gz` |
| Switch (homebrew) | `reHoMM4-v0.5-switch.nro` |
| Switch 2 (homebrew, kaiHEN) | `reHoMM4-v0.5-switch2.nro` |
| Web browser | `reHoMM4-v0.5-web.zip` |

**New**

- **Settings in the game's style everywhere.** Mac, iPhone, iPad and Android now use the same ⚙ panel as Linux, Windows and the web, drawn with the game's frame and parchment. Above the main menu: ☁ (save sync, when on), ☕ and ⚙, each with a tooltip.
- **What's new.** ⚙ → **What's new?** shows the notes of the releases newer than yours (or this one's), page by page, with a link to the releases.
- **Update check.** ⚙ → **Check for updates** on Mac, Android, Linux and Windows (iPhone and iPad: the release page). The Mac app opens the downloaded .dmg; Android installs the APK.
- **Folders.** On Mac, iPhone, iPad and Android, ⚙ → **Folders** shows where the saves and the game are (Mac and Android: **Open**).
- **Screen layouts.** Choose 800×600, 1024×768 or 1280×1024 in the game's Options. Smaller layouts make text and controls larger, and the adventure map widens to fit the screen. Switch keeps separate handheld and docked choices.
- **Interface language.** The app's settings can follow the game's language or use a separate language. App text is available in English, Ukrainian, German, French, Italian, Spanish, Polish and Russian.
- **More GOG editions.** Supported English, German, French, Italian and Spanish editions install directly. The Polish edition requires the executable from the supported English edition. See the README for the supported GOG build.
- **Renderer setting** (Mac, iPhone, iPad, Android). Choose Auto, GPU or CPU. On Android, Auto can fall back to CPU rendering on the next launch when it detects a slow GPU.

**Changed**

- Web: the start page keeps only the language; **Skip the intro movies** and **Black side panels** are in ⚙ (your earlier choice is kept), and **Start** always opens full screen. The ☕ button opens Ko-fi in a new tab.
- Changing the game's language restarts the app on Linux, Mac and Android.

**Fixed**

- Web: copying the game into the browser's storage failed in Safari ("An operation that depends on state cached in an interface object…").
- The game's fullscreen option now controls the app window or the browser's fullscreen mode where available.
- Android keeps the game clear of display cutouts and rounded corners.
- The Windows launcher starts the game from its game directory.
- Switch 2 uses SDL's OpenGL ES renderer.

## 0.4.1

Windows, the scaling filter's name on screen, better random maps, and fixes.

| Device | File |
|---|---|
| Mac, macOS 13+ | `reHoMM4-v0.4.1-macos-arm64.dmg` (Apple chip, M1 or newer) or `reHoMM4-v0.4.1-macos-x86_64.dmg` (Intel) |
| iPhone, iPad, iOS / iPadOS 17+ | `reHoMM4-v0.4.1-ios.ipa` (install with AltStore or Sideloadly) |
| Android 10+, 64-bit | `reHoMM4-v0.4.1-android_arm64.apk` |
| Windows 10 or 11, 64-bit | `reHoMM4-v0.4.1-windows-x86_64.zip`: unzip, start `ReHoMM4\ReHoMM4.exe` |
| Linux, Steam Deck | `reHoMM4-v0.4.1-linux-x86_64.tar.gz` (x86-64) or `reHoMM4-v0.4.1-linux-arm64.tar.gz` (64-bit ARM) |
| Switch (homebrew) | `reHoMM4-v0.4.1-switch.nro` |
| Switch 2 (homebrew, kaiHEN) | `reHoMM4-v0.4.1-switch2.nro` |
| Web browser (computer, phone or tablet; 64-bit Chrome, Edge, Firefox, Safari) | `reHoMM4-v0.4.1-web.zip`: unzip, run `python3 serve.py` in it, open http://localhost:8080 (or put it on an HTTPS server) |

**New**
- **Windows**: a portable app for 64-bit Windows 10 and 11 (Vulkan drivers), with the side panels, ⚙ settings, save sync and fullscreen. It is not signed: if Windows says "Windows protected your PC", **More info** → **Run anyway** (README: Windows).
- **The picture scaling on screen**: F10 (or **⚙ → Scaling** on iPhone, iPad and Android) shows the filter chosen and its number for two seconds, "Scaling 7/7: FSR". The choice is remembered for the next start. README: Picture scaling, with the seven filters.
- **Random maps**:
  - the panel in the game's style: its dialog frame, parchment and big buttons, a bigger title, sharp button text;
  - the hero's card: the name, faction and class under the portrait, the skills beside it, the army the game really gives, the biography larger and scrolling;
  - a template made for some number of players sets it (Three on three: 6), and its teams (Allies, Three on three);
  - the random map button also in the New Scenario dialog opened from the adventure map;
  - the failures' reasons in Ukrainian, in red.

**Changed**
- Random maps (generator 3: a seed now makes another map than in 0.4):
  - paths are no longer covered by trees, and there are fewer walls of forest;
  - on large and extra large island maps the sea between the islands is wider, by the seed;
  - every player can be made human in the game's setup (computers were computer only);
  - no extra starting army on top of the game's own (a Life hero had 16 squires and 6 swordsmen more than the game gives).

**Fixed**
- Random maps: obelisks are on the map (they were written as another object and not shown).
- The adventure map's frame: the top right corner under the dragon on wide screens.
- Android: the gamepad settings (pointer and scroll speed, the buttons) were not applied.

**Known limits**
- The Mac and Windows apps are not signed: the first time, right-click the Mac app → **Open** → **Open**; on Windows see above.
- The Switch app has been tested only in the Astris emulator, not on a real console.
- The Switch 2 app has not been tested on a console.

## 0.4

Random maps, Switch 2, the game in a web browser, a new icon, and fixes.

| Device | File |
|---|---|
| Mac, macOS 13+ | `reHoMM4-v0.4-macos-arm64.dmg` (Apple chip, M1 or newer) or `reHoMM4-v0.4-macos-x86_64.dmg` (Intel) |
| iPhone, iPad, iOS / iPadOS 17+ | `reHoMM4-v0.4-ios.ipa` (install with AltStore or Sideloadly) |
| Android 10+, 64-bit | `reHoMM4-v0.4-android_arm64.apk` |
| Linux, Steam Deck | `reHoMM4-v0.4-linux-x86_64.tar.gz` (x86-64) or `reHoMM4-v0.4-linux-arm64.tar.gz` (64-bit ARM) |
| Switch (homebrew) | `reHoMM4-v0.4-switch.nro` |
| Switch 2 (homebrew, kaiHEN) | `reHoMM4-v0.4-switch2.nro` |
| Web browser (computer, phone or tablet; 64-bit Chrome, Edge, Firefox, Safari) | `reHoMM4-v0.4-web.zip`: unzip, run `python3 serve.py` in it, open http://localhost:8080 (or put it on an HTTPS server) |

**New**
- **Random maps**: New Game → Scenario → **Random map**. Choose the template, the map size, the number of players (each human or computer, with faction, team and starting hero), water, underground, monsters, obelisks, treasures and the seed; most can be **Random**. The hero's card shows the starting hero's skills, statistics, army and biography. A random map in the list can be edited and made again.
- **32 templates**: Classic and 31 more, our own layouts of the kinds popular in Heroes 3 (Ring, Spider, Mirror, Allies, Three on three, Labyrinth, Checkerboard, …). They are text files in `Maps/RandomMaps/Templates`: change them or add your own (`example.txt` explains the format).
- **In a web browser**: the game compiled to WebAssembly runs in a browser on a computer, phone or tablet. It installs from GOG's installer files or an installed game's folder into the browser's storage; no public site yet: `reHoMM4-v0.4-web.zip` is the site, to run on your computer or put on a server (README: In a browser). English only in it; a translation can be added (its README.txt).
- **A new icon** on every platform.
- **Switch 2**: its own build, `reHoMM4-v0.4-switch2.nro`, for kaiHEN's homebrew menu (README: Nintendo Switch 2).
- **Black side panels** can be chosen in the settings (from the next start).

**Changed**
- The Linux archives unpack into a `ReHoMM4` folder, and the Mac app is `ReHoMM4.app` (was `Heroes IV`). On the Mac, move the old **Heroes IV** from Applications to the Bin after installing; saves and settings are kept.
- The app's file is `ReHoMM4` everywhere: `ReHoMM4/ReHoMM4` on Linux and the Steam Deck (re-add it to Steam), `ReHoMM4.nro` on the Switch (in `switch/ReHoMM4/`), `ReHoMM4.app` on iPhone and iPad.
- The installer scrolls when it is taller than the screen (a phone on its side).

**Fixed**
- Android: a black screen after signing in to GOG (the download went on behind it).
- Android, iPhone, iPad: the random map buttons showed only after a touch.
- Android: the launcher's mask no longer cuts the icon's edge.

**Known limits**
- The Mac app is not signed: the first time, right-click it → **Open** → **Open**.
- The Switch app has been tested only in the Astris emulator, not on a real console.
- The Switch 2 app has not been tested on a console.

## 0.3

The Switch app runs, other-language GOG installers, smoother play on phones, and fixes.

| Device | File |
|---|---|
| Mac, macOS 13+ | `reHoMM4-v0.3-macos-arm64.dmg` (Apple chip, M1 or newer) or `reHoMM4-v0.3-macos-x86_64.dmg` (Intel) |
| iPhone, iPad, iOS / iPadOS 17+ | `reHoMM4-v0.3-ios.ipa` (install with AltStore or Sideloadly) |
| Android 10+, 64-bit | `reHoMM4-v0.3-android_arm64.apk` |
| Linux, Steam Deck | `reHoMM4-v0.3-linux-x86_64.tar.gz` (x86-64) or `reHoMM4-v0.3-linux-arm64.tar.gz` (64-bit ARM) |
| Switch (homebrew) | `reHoMM4-v0.3-switch.nro` |

**New**
- **GOG installers in any language**: put `heroes4.exe` from GOG's English version next to the installer files (or pick it with them), and the game plays in the installer's language. An installed game in another language works the same way, with the English `heroes4.exe` in its folder.
- **The game's language from GOG** shows in the settings when the game was installed in another language.
- **Switch**: the game is copied to the card as an installed folder (`switch/ReHoMM4/game`); the app no longer unpacks installers there, and says where to copy the game when it is missing.
- **60 frames a second at most** by default, on 120 Hz screens too: less battery and heat, the same motion.
- **Close button** (✕) in the installer on the Mac and Linux.
- **Linux on 64-bit ARM** (arm64): a build of its own, `reHoMM4-v0.3-linux-arm64.tar.gz`.
- The release is tagged `v0.3` and its files are named `reHoMM4-v0.3-…` (were `HeroesIV-0.2-…`); the update check in the apps finds both.

**Changed**
- The installer checks only `heroes4.exe`: other languages' installers have a different set of files.
- Gamepad support is always on: the settings no longer have a checkbox for it.
- The Mac settings show one saves folder: the synced one while sync is on, otherwise the saves' own.

**Fixed**
- The game could crash on its first start right after installing (Mac).
- Linux (X11): the game hung when fullscreen was turned off in the settings.
- Switch: crashes at the start and while installing, a black screen, and a picture too wide for the screen.
- Smoother frames on Android phones and on the Switch.

**Known limits**
- The Mac app is not signed: the first time, right-click it → **Open** → **Open**.
- The Switch app has been tested only in the Astris emulator, not on a real console. The Intel Mac app has been tested only under Rosetta. The Linux arm64 app has not been tested on an ARM machine yet.

How to install and run: [README](https://github.com/evilHazzarD/ReHoMM4#readme).

---

Програма для Switch запускається, інсталятори GOG іншими мовами, плавніша гра на телефонах і виправлення.

**Нове**
- **Інсталятори GOG будь-якою мовою**: покладіть `heroes4.exe` з англійської версії GOG поруч з файлами інсталятора (або оберіть його разом з ними) — і гра йде мовою інсталятора. Встановлена гра іншою мовою працює так само, з англійським `heroes4.exe` у її теці.
- **Мова гри з GOG** показується в налаштуваннях, коли гру встановлено іншою мовою.
- **Switch**: гра копіюється на картку встановленою текою (`switch/ReHoMM4/game`); інсталятори там більше не розпаковуються, а без гри програма пише, куди її скопіювати.
- **Не більше 60 кадрів на секунду** типово, і на екранах 120 Гц: менше витрат батареї й нагріву, рух той самий.
- **Кнопка закриття** (✕) в інсталяторі на Маці й Linux.
- **Linux на 64-бітному ARM** (arm64): окрема збірка, `reHoMM4-v0.3-linux-arm64.tar.gz`.
- Тег релізу — `v0.3`, файли звуться `reHoMM4-v0.3-…` (було `HeroesIV-0.2-…`); перевірка оновлень у програмах знаходить обидва формати.

**Змінено**
- Інсталятор перевіряє лише `heroes4.exe`: в інсталяторах іншими мовами інший набір файлів.
- Підтримка геймпада завжди увімкнена: прапорця для неї в налаштуваннях більше немає.
- Налаштування на Маці показують одну теку збережень: синхронізовану, коли синхронізація увімкнена, інакше — власну.

**Виправлено**
- Гра могла падати при першому запуску одразу після встановлення (Мак).
- Linux (X11): гра зависала, коли в налаштуваннях вимикали повноекранний режим.
- Switch: падіння на старті й під час встановлення, чорний екран і зображення, ширше за екран.
- Плавніші кадри на телефонах з Android і на Switch.

**Обмеження**
- Програма для Мака не підписана: першого разу — правою кнопкою → **«Відкрити»** → **«Відкрити»**.
- Програму для Switch перевірено лише в емуляторі Astris, на справжній консолі — ще ні. Програму для Мака з Intel перевірено лише через Rosetta. Програму для Linux arm64 ще не перевірено на ARM-комп'ютері.

Як встановити й запустити: [README](https://github.com/evilHazzarD/ReHoMM4#українською).

## 0.2

Settings on every device, gamepad controls you can change, and the Ukrainian translation's single scenarios.

| Device | File |
|---|---|
| Mac, macOS 13+ | `HeroesIV-0.2-macos-arm64.dmg` (Apple chip, M1 or newer) or `HeroesIV-0.2-macos-x86_64.dmg` (Intel) |
| iPhone, iPad, iOS / iPadOS 17+ | `HeroesIV-0.2-ios.ipa` (install with AltStore or Sideloadly) |
| Android 10+, 64-bit | `HeroesIV-0.2-android_arm64.apk` |
| Linux, Steam Deck | `HeroesIV-0.2-linux-x86_64.tar.gz` |
| Switch (homebrew) | `HeroesIV-0.2-switch.nro` |

**New**
- **Settings** (⚙ in the main menu): the version and the update check at the top, the game's language as a dropdown, fullscreen (Mac, Linux), skipping the intro movies, save sync, the gamepad.
- **A new language applies at once**: the game restarts with it (Android, Mac, Linux).
- **Gamepad**: turn it on or off, set the sticks' speeds and what each button does; presets named as on a PlayStation, Xbox or Nintendo pad (the Switch: always its own).
- **Folders** (Android, Mac): the saves' folder (the synced one while sync is on) and the game's folder, each with Open. On Android they also show in the Files app as "Heroes IV", so saves and game files can be copied in and out.
- **Save sync help**: a ? next to the sync setting (Android, Linux) tells how to set it up for that device: Syncthing, a cloud through FolderSync / Round Sync / rclone, Nextcloud.
- **Translations**: translated single scenarios (maps) are used in place of the English ones; the Ukrainian translation 0.2 has them all.
- The apps' files are named with their version (`HeroesIV-<version>-…`).

**Known limits**
- The Mac app is not signed: the first time, right-click it → **Open** → **Open**.
- The Switch app has not been tested (no Switch with custom firmware). The Intel Mac app has been tested only under Rosetta.

How to install and run: [README](https://github.com/evilHazzarD/ReHoMM4#readme).

---

Налаштування на всіх пристроях, керування геймпадом, яке можна змінити, і окремі сценарії в українському перекладі.

**Нове**
- **Налаштування** (⚙ у головному меню): версія й перевірка оновлень угорі, мова гри — випадний список, повноекранний режим (Мак, Linux), пропуск вступних відео, синхронізація збережень, геймпад.
- **Нова мова застосовується одразу**: гра перезапускається з нею (Android, Мак, Linux).
- **Геймпад**: увімкнути чи вимкнути, швидкість стіків і що робить кожна кнопка; пресети з назвами кнопок PlayStation, Xbox чи Nintendo (на Switch — завжди свої).
- **Теки** (Android, Мак): тека збережень (під час синхронізації — синхронізована) і тека гри, кожна з кнопкою «Відкрити». На Android вони є й у застосунку «Файли» як «Heroes IV»: збереження та файли гри можна копіювати туди й звідти.
- **Підказка про синхронізацію**: «?» біля синхронізації (Android, Linux) пояснює, як її налаштувати на цьому пристрої: Syncthing, хмара через FolderSync / Round Sync / rclone, Nextcloud.
- **Переклади**: перекладені окремі сценарії (карти) замінюють англійські; український переклад 0.2 має їх усі.
- Файли програм названо з номером версії (`HeroesIV-<версія>-…`).

**Обмеження**
- Програма для Мака не підписана: першого разу — правою кнопкою → **«Відкрити»** → **«Відкрити»**.
- Програму для Switch не перевірено (немає Switch з кастомною прошивкою). Програму для Мака з Intel перевірено лише через Rosetta.

Як встановити й запустити: [README](https://github.com/evilHazzarD/ReHoMM4#українською).

## 0.1

First release of ReHoMM4: Heroes of Might and Magic IV: Complete as a native app for the Mac, iPhone/iPad, Android, Linux/Steam Deck and the Switch.

The game itself is not included. The app installs it from your GOG account, or from the **English** GOG offline installer for Windows (both parts: the `.exe` and the `.bin`).

| Device | File |
|---|---|
| Mac, macOS 13+ | `HeroesIV-macos-arm64.dmg` (Apple chip, M1 or newer) or `HeroesIV-macos-x86_64.dmg` (Intel) |
| iPhone, iPad, iOS / iPadOS 17+ | `HeroesIV-ios.ipa` (install with AltStore or Sideloadly) |
| Android 10+, 64-bit | `HeroesIV-android_arm64.apk` |
| Linux, Steam Deck | `HeroesIV-linux-x86_64.tar.gz` |
| Switch (homebrew) | `HeroesIV.nro` |

`HOMM4-extra-assets.zip` is only for building from source (the side panels; the apps already have them).

**What is in it**
- Wide screens: the interface centred, with side panels.
- Touch controls on phones and tablets.
- Language list (🌐 in the main menu). Ukrainian is available ([ReHoMM4-translations](https://github.com/evilHazzarD/ReHoMM4-translations)).
- Saves can follow you between devices (all but the Switch) through iCloud Drive, Syncthing, rclone or other folder sync.

**Known limits**
- The Mac app is not signed: the first time, right-click it → **Open** → **Open**.
- The Switch app has not been tested (no Switch with custom firmware). The Intel Mac app has been tested only under Rosetta.

How to install and run: [README](https://github.com/evilHazzarD/ReHoMM4#readme).

---

Перший реліз ReHoMM4: Heroes of Might and Magic IV: Complete як програма для Мака, iPhone/iPad, Android, Linux/Steam Deck і Switch.

Самої гри тут немає. Програма встановлює її з вашого облікового запису GOG або з **англійського** офлайн-інсталятора GOG для Windows (обидві частини: `.exe` і `.bin`).

**Що є**
- Широкі екрани: інтерфейс по центру, з бічними панелями.
- Сенсорне керування на телефонах і планшетах.
- Список мов (🌐 у головному меню). Є українська ([ReHoMM4-translations](https://github.com/evilHazzarD/ReHoMM4-translations)).
- Збереження можуть переходити між пристроями (крім Switch) через iCloud Drive, Syncthing, rclone чи іншу синхронізацію папок.

**Обмеження**
- Програма для Мака не підписана: першого разу — правою кнопкою → **«Відкрити»** → **«Відкрити»**.
- Програму для Switch не перевірено (немає Switch з кастомною прошивкою). Програму для Мака з Intel перевірено лише через Rosetta.

`HOMM4-extra-assets.zip` потрібен лише для збирання з коду (бічні панелі; у програмах вони вже є).

Як встановити й запустити: [README](https://github.com/evilHazzarD/ReHoMM4#українською).
