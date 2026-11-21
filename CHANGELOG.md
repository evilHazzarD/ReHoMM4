# Changelog

Each release's notes on GitHub are its section here (tools/release/make_release.sh). Newest first.

## 0.1

First release of ReHoMM4: Heroes of Might and Magic IV: Complete as a native app for the Mac, iPhone/iPad, Android, Linux/Steam Deck and the Switch.

The game itself is not included. The app installs it from your GOG account, or from the **English** GOG offline installer for Windows (both parts: the `.exe` and the `.bin`).

| Device | File |
|---|---|
| Mac, Apple chip (M1 or newer), macOS 13+ | `HeroesIV-macos-arm64.dmg` |
| Mac, Intel, macOS 13+ | `HeroesIV-macos-x86_64.dmg` |
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
