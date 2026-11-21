# Changelog

Each release's notes on GitHub are its section here (tools/release/make_release.sh). Newest first.

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
