# ReHoMM4: Heroes of Might and Magic IV, native

Native port of Heroes of Might and Magic IV (including both expansions) for macOS, iOS, iPadOS, Android, Linux, Steam Deck, and Nintendo Switch. Runs directly on hardware—no Wine, emulators, or VMs.

[Українською — нижче](#українською)

- English, the original, or a translation: Ukrainian today (interface, campaigns, campaign
  voice-over; The Gathering Storm stays in English), and every language added to
  [ReHoMM4-translations](https://github.com/evilHazzarD/ReHoMM4-translations).
- Wide screen and a sharp picture on high-resolution displays.
- Smooth animation on the adventure map and in combat.
- Touch controls on phones and tablets; saves can follow you between devices.

**What you need:**

- A purchased copy of **Heroes of Might and Magic IV: Complete Edition** from
  [GOG](https://www.gog.com/game/heroes_of_might_and_magic_4_complete). The game itself is not
  here: the app takes it from your GOG account.
- One of:
  - a Mac with macOS 13 or later (an Apple chip or Intel);
  - an iPhone or iPad;
  - an Android phone or tablet (64-bit, Android 10 or later);
  - a Linux PC (x86-64) or a Steam Deck;
  - a Nintendo Switch that runs homebrew.
- An internet connection and about 3 GB of free space.

## Install

1. Open the [Releases page](https://github.com/evilHazzarD/ReHoMM4/releases) and download the file
   for your device:

   | Device | File | How to install |
   |---|---|---|
   | Mac | `HeroesIV-macos-arm64.dmg` (Apple chip: M1 or newer), `HeroesIV-macos-x86_64.dmg` (Intel) | open it, drag **Heroes IV** to **Applications**; the first time, right-click it → **Open** → **Open** |
   | iPhone, iPad | `HeroesIV-ios.ipa` | install it with [AltStore](https://altstore.io) or [Sideloadly](https://sideloadly.io) (they sign it with your Apple ID); then Settings → General → VPN & Device Management → trust your Apple ID |
   | Android | `HeroesIV-android_arm64.apk` | open it on the phone and allow installing apps from this source |
   | Linux, Steam Deck | `HeroesIV-linux-x86_64.tar.gz` | unpack it and run `heroes4`; on the Deck, in Desktop Mode, add `heroes4` to Steam with **Add a Non-Steam Game** |
   | Switch | `HeroesIV.nro` | copy it to `sdmc:/switch/ReHoMM4/` on the SD card, together with the GOG installer files (step 2); start it from the homebrew menu in title takeover (hold **R** while starting a game) |

2. **Open the app.** The first time, it installs the game files. Choose one:
   - **Sign in to GOG.** GOG's own sign-in page opens; the app checks that your account owns
     Heroes of Might and Magic IV: Complete and downloads the game (1 GB). Signing in with
     Google, Discord, Steam or Xbox works too. The sign-in is kept only until the download is done.
   - **GOG installer files.** On [gog.com](https://www.gog.com/game/heroes_of_might_and_magic_4_complete)
     download the **Windows offline installer in English** (`setup_heroes_of_might_and_magic_4_complete_….exe`
     and its `.bin` file; GOG offers it in several languages, and only the English one works) and
     pick them in the app. On the Mac and Linux, files in **Downloads**
     are found by themselves; on the Switch, put them next to the `.nro`.
     On the game's page in your GOG library: **Language: English**, then under
     **Download offline backup game installers** both parts, **Part 1 of 2** (1 MB, the `.exe`) and
     **Part 2 of 2** (1.1 GB, the `.bin`). The patches below them are not needed.

     ![The GOG library page: Language English, and the two installer parts to download](docs/gog_files.jpg)

   Every game file is checked, then the game starts. An interrupted download continues where it
   stopped. What happens to the installer files afterwards: see below.
3. **Language:** the 🌐 button in the top right corner of the main menu opens the list of
   languages. Pick one and restart the game; the translation is downloaded when it is first chosen.

### The installer files after installing

The installer (`setup_….exe` and its `.bin`, about 1 GB) is needed only once. Whether the app
deletes it depends on where it was:

| Where the installer was | After the game is installed |
|---|---|
| **In the app's own folder**: iPhone/iPad: Files → On My iPhone → Heroes IV; Android: `Android/data/games.hazzard.heroes4/files/` (copied with `adb push`); Switch: `sdmc:/switch/ReHoMM4/`; Mac: `~/Library/Application Support/ReHoMM4/`; Linux: `~/.local/share/ReHoMM4/` | **deleted**: it is a copy made for the app, and it takes space for nothing |
| **Picked in the app** from anywhere else (Files, Downloads, a USB drive, iCloud Drive…) | **kept**: these are your files, the app only reads them |
| **In Downloads** on the Mac or Linux (found by itself) | **kept** |
| Not used: the game came from your **GOG account** | nothing to delete: the app downloads the game files themselves, not the installer |

If the installation stops with an error or is cancelled, nothing is deleted: try again with the same files.

The licenses of the third-party libraries are in `THIRD_PARTY_NOTICES.txt` inside every app.

## If something goes wrong

**The Mac says "Heroes IV cannot be opened because the developer cannot be verified".**
Right-click the app → **Open** → **Open**.

**The installer says the files are not the right ones.** The port works only with the current GOG
version of Heroes of Might and Magic IV: Complete (the English Windows installer, version 3.0).
Installers in other languages are not supported: download the English one (translations come
from the 🌐 button).

**The installation was interrupted.** Open the app again: it continues where it stopped.

**A translation does not download.** It needs the internet the first time it is chosen; try
again online.

**The game crashed or behaves oddly.** Open an [issue](https://github.com/evilHazzarD/ReHoMM4/issues),
say which device, and attach the log `h4.log` and the newest file of `crashes/`. They are in the
game's player folder:

| Device | Player folder |
|---|---|
| Mac | `~/Library/Application Support/ReHoMM4/player` (in Finder: **Cmd ⌘ + Shift + G**, paste the path, **Enter**); the log in `logs/`, crashes in `data/h4/crashes/` |
| Linux, Steam Deck | `~/.local/share/ReHoMM4/player` |
| Android | `Android/data/games.hazzard.heroes4/files/player` on the phone's storage |
| iPhone, iPad | the app's folder in the Files app or Finder: **Heroes IV** → `player` |
| Switch | `sdmc:/switch/ReHoMM4/player` |

**To remove the game:** delete the app; on the Mac also the folder
`~/Library/Application Support/ReHoMM4`, on Linux `~/.local/share/ReHoMM4` (the game files and
saves).

## Controls

As in the original game; on touch screens a tap is a click and a long press a right-click. In
addition, **F10** switches the picture smoothing filter. All the touch gestures:
[`platforms/README.md`](platforms/README.md#controls).

## Saves on several devices

Saves can follow you between devices (all but the Switch) through iCloud Drive, Syncthing, rclone,
Round Sync, FolderSync or Nextcloud: the game keeps them in a `ReHOMM4` folder. How to turn it on
for each app: [Syncing saves between devices](platforms/README.md#syncing-saves-between-devices).

## Tested devices

The iPhone/iPad, Android, Mac and Linux apps have been tested on real devices; the Intel Mac app so far
only under Rosetta on an Apple Silicon Mac. The Switch app has
not been tested: the author has no Switch with custom firmware to run homebrew on. Reports from
Switch owners are welcome in the [issues](https://github.com/evilHazzarD/ReHoMM4/issues).

## Translations

The translations live in their own repository,
[ReHoMM4-translations](https://github.com/evilHazzarD/ReHoMM4-translations), and reach the apps
only through its releases: the apps check the latest one at every start and download a language
when it is chosen. A new language or a corrected one needs no new app. How to add your language:
[TRANSLATING.md](https://github.com/evilHazzarD/ReHoMM4-translations/blob/main/TRANSLATING.md).

## How it is made

The port is made by static recompilation with [x86recomp](https://github.com/JTM-rootstorm/x86recomp):
the code of `heroes4.exe` is translated ahead of time into native code, while graphics, sound and
the window run on SDL3 (Metal on Apple devices, Vulkan on Android and Linux). The repository holds
no game code or game files.

| Path | What |
|---|---|
| `h4/` | the port: `lifter.manifest`, replaced game functions (`src/`) |
| `patches/` | changes to x86recomp, applied on a pinned commit |
| `symbols/` | function and jump-table addresses of `heroes4.exe` (from Ghidra) |
| `platforms/` | the app builds: iOS, Android, Linux, Mac and Switch (`build.sh`, a folder per platform); a Mac DMG that builds the game on the player's Mac (`macos-dmg/`) |
| `libs/innoextract/` | the GOG installer unpacker every app uses |
| `assets/icon/` | the app icons (the shield with "IV") |
| `licenses/` | license texts of the third-party parts the apps carry |
| `tools/` | preparing the source build (`build.sh`), pre-publish checks (`release/`), Ghidra scripts and art tools (`re/`) |

### Building from source

On a Mac with Homebrew and your GOG installer: `tools/build.sh [path to setup_….exe]` unpacks the
game and prepares its code, then for each platform `platforms/deps.sh <platform>` and:

| Command | Result |
|---|---|
| `platforms/build.sh ios [TEAM_ID]` | `HeroesIV-ios.ipa` |
| `platforms/build.sh android` | `HeroesIV-android_arm64.apk` |
| `platforms/build.sh linux` | `HeroesIV-linux-x86_64.tar.gz` |
| `platforms/build.sh macos-arm64` | `Heroes IV.app` and `HeroesIV-macos-arm64.dmg` |
| `platforms/build.sh macos-x86_64` | `Heroes IV.app` and `HeroesIV-macos-x86_64.dmg` (Intel) |
| `platforms/build.sh switch` | `HeroesIV.nro` |

Details: [`platforms/README.md`](platforms/README.md).

## Credits

ReHoMM4 started from [HoMM4Deadware](https://github.com/LeoDeadware/HoMM4Deadware) by
LeoDeadware (MIT License): the port's first version and the Ukrainian translation come from it.
The files taken from it say so at their top.

## License

The port's own code, tools and icon: MIT (see `LICENSE`), with the parts from HoMM4Deadware under
LeoDeadware's copyright. x86recomp: MIT, by its authors; ffmpeg: LGPL 2.1+. Every app carries
`THIRD_PARTY_NOTICES.txt` with the license of everything in it (SDL3, ffmpeg, innoextract, Boost,
XZ, glslang, SPIRV-Cross, and on the Switch libnx, curl, zlib, Mesa and more), made by
`platforms/notices.py` from the build's link line. The license does not cover the game itself: it
stays yours, from GOG.

Heroes of Might and Magic is a trademark of Ubisoft. This is a fan project, unaffiliated with
Ubisoft or GOG.

---

## Українською

«Герої меча і магії IV» з обома доповненнями як рідна програма для Мака, iPhone та iPad, Android,
Linux і Steam Deck, і Nintendo Switch. Без Wine, емуляторів і віртуальних машин: гра працює на
пристрої напряму.

- Англійська (оригінал) або переклад: зараз українська (інтерфейс, кампанії, озвучка кампаній;
  доповнення The Gathering Storm лишається англійською) і кожна мова, додана до
  [ReHoMM4-translations](https://github.com/evilHazzarD/ReHoMM4-translations).
- Широкий екран і чітка картинка на дисплеях високої роздільності.
- Плавна анімація на карті й у бою.
- Керування дотиком на телефонах і планшетах; збереження можуть переходити між пристроями.

**Що потрібно:**

- Куплена гра **Heroes of Might and Magic IV: Complete Edition** на
  [GOG](https://www.gog.com/game/heroes_of_might_and_magic_4_complete). Самої гри тут немає:
  програма бере її з вашого облікового запису GOG.
- Один з пристроїв:
  - Мак з macOS 13 або новішою (процесор Apple або Intel);
  - iPhone або iPad;
  - телефон чи планшет на Android (64-бітний, Android 10 або новіший);
  - ПК з Linux (x86-64) або Steam Deck;
  - Nintendo Switch, на якому запускається homebrew.
- Інтернет і близько 3 ГБ вільного місця.

### Встановлення

1. Відкрийте [сторінку Releases](https://github.com/evilHazzarD/ReHoMM4/releases) і завантажте
   файл для свого пристрою:

   | Пристрій | Файл | Як встановити |
   |---|---|---|
   | Мак | `HeroesIV-macos-arm64.dmg` (процесор Apple: M1 або новіший), `HeroesIV-macos-x86_64.dmg` (Intel) | відкрийте його, перетягніть **Heroes IV** у **Applications**; першого разу — правою кнопкою → **«Відкрити»** → **«Відкрити»** |
   | iPhone, iPad | `HeroesIV-ios.ipa` | встановіть через [AltStore](https://altstore.io) або [Sideloadly](https://sideloadly.io) (вони підписують її вашим Apple ID); далі Параметри → Загальні → Керування VPN і пристроями → довіряти своєму Apple ID |
   | Android | `HeroesIV-android_arm64.apk` | відкрийте його на телефоні й дозвольте встановлення програм з цього джерела |
   | Linux, Steam Deck | `HeroesIV-linux-x86_64.tar.gz` | розпакуйте й запустіть `heroes4`; на Deck — у режимі робочого столу додайте `heroes4` у Steam через **Add a Non-Steam Game** |
   | Switch | `HeroesIV.nro` | скопіюйте в `sdmc:/switch/ReHoMM4/` на SD-картці разом з файлами інсталятора GOG (крок 2); запускайте з homebrew-меню в режимі title takeover (тримайте **R**, запускаючи гру) |

2. **Відкрийте програму.** Першого разу вона встановлює файли гри. Оберіть одне:
   - **Увійти в GOG.** Відкривається сторінка входу GOG; програма перевіряє, що на вашому
     обліковому записі є Heroes of Might and Magic IV: Complete, і завантажує гру (1 ГБ). Вхід через
     Google, Discord, Steam чи Xbox теж працює. Вхід зберігається лише до кінця завантаження.
   - **Файли інсталятора GOG.** На [gog.com](https://www.gog.com/game/heroes_of_might_and_magic_4_complete)
     завантажте **англійський офлайн-інсталятор для Windows** (`setup_heroes_of_might_and_magic_4_complete_….exe`
     і його файл `.bin`; GOG пропонує його кількома мовами, а підходить лише англійський) і
     оберіть їх у програмі. На Маці й Linux файли з теки **«Завантаження»**
     знаходяться самі; на Switch покладіть їх поруч із `.nro`.
     На сторінці гри у вашій бібліотеці GOG: **Language: English**, далі в розділі
     **Download offline backup game installers** обидві частини — **Part 1 of 2** (1 МБ, `.exe`) і
     **Part 2 of 2** (1,1 ГБ, `.bin`). Патчі під ними не потрібні.

     ![Сторінка бібліотеки GOG: Language English і дві частини інсталятора](docs/gog_files.jpg)

   Кожен файл гри перевіряється, далі гра запускається. Перерване завантаження продовжується з
   того місця, де зупинилося. Що буває з файлами інсталятора потім — див. нижче.
3. **Мова:** кнопка 🌐 у правому верхньому куті головного меню відкриває список мов. Оберіть мову
   й перезапустіть гру; переклад завантажується, коли його обрано вперше.

#### Файли інсталятора після встановлення

Інсталятор (`setup_….exe` і його `.bin`, приблизно 1 ГБ) потрібен лише раз. Чи видалить його
програма, залежить від того, де він лежав:

| Де лежав інсталятор | Після встановлення гри |
|---|---|
| **У папці самої програми**: iPhone/iPad: «Файли» → «На iPhone» → Heroes IV; Android: `Android/data/games.hazzard.heroes4/files/` (скопійовано через `adb push`); Switch: `sdmc:/switch/ReHoMM4/`; Мак: `~/Library/Application Support/ReHoMM4/`; Linux: `~/.local/share/ReHoMM4/` | **видаляється**: це копія для програми, і вона даремно займає місце |
| **Обрано в програмі** з будь-якого іншого місця («Файли», «Завантаження», флешка, iCloud Drive…) | **лишається**: це ваші файли, програма їх лише читає |
| **У «Завантаженнях»** на Маці чи Linux (знайдено самостійно) | **лишається** |
| Не потрібен: гру встановлено з **облікового запису GOG** | видаляти нічого: програма завантажує самі файли гри, а не інсталятор |

Якщо встановлення зупинилося з помилкою чи його скасовано, нічого не видаляється: спробуйте ще
раз з тими самими файлами.

Ліцензії сторонніх бібліотек — у `THIRD_PARTY_NOTICES.txt` у кожній програмі.

### Якщо щось пішло не так

**Мак пише «Heroes IV не можна відкрити, бо розробника не перевірено».** Клацніть по програмі
правою кнопкою → **«Відкрити»** → **«Відкрити»**.

**Інсталятор каже, що файли не ті.** Порт працює тільки з актуальною версією Heroes of Might and
Magic IV: Complete з GOG (англійський інсталятор для Windows, версія 3.0). Інсталятори іншими
мовами не підходять: завантажте англійський (переклади — через кнопку 🌐).

**Встановлення перервалося.** Відкрийте програму ще раз: вона продовжить з того місця, де
зупинилася.

**Переклад не завантажується.** Першого разу, коли мову обрано, потрібен інтернет; спробуйте ще
раз онлайн.

**Гра вилетіла або поводиться дивно.** Створіть [issue](https://github.com/evilHazzarD/ReHoMM4/issues),
напишіть, на якому пристрої, і додайте журнал `h4.log` та найновіший файл з `crashes/`. Вони в теці
гравця:

| Пристрій | Тека гравця |
|---|---|
| Мак | `~/Library/Application Support/ReHoMM4/player` (у Finder: **Cmd ⌘ + Shift + G**, вставте шлях, **Enter**); журнал у `logs/`, вильоти в `data/h4/crashes/` |
| Linux, Steam Deck | `~/.local/share/ReHoMM4/player` |
| Android | `Android/data/games.hazzard.heroes4/files/player` у пам'яті телефона |
| iPhone, iPad | тека програми в «Файлах» або у Finder: **Heroes IV** → `player` |
| Switch | `sdmc:/switch/ReHoMM4/player` |

**Видалити гру:** видаліть програму; на Маці також теку `~/Library/Application Support/ReHoMM4`, на
Linux — `~/.local/share/ReHoMM4` (файли гри й збереження).

### Керування

Як в оригінальній грі; на сенсорних екранах дотик — це клік, довге натискання — правий клік.
Додатково: **F10** перемикає фільтр згладжування картинки. Усі жести дотиком:
[`platforms/README.md`](platforms/README.md#controls).

### Збереження на кількох пристроях

Збереження можна синхронізувати між пристроями (крім Switch) через iCloud Drive, Syncthing, rclone,
Round Sync, FolderSync чи Nextcloud: гра тримає їх у папці `ReHOMM4`. Як увімкнути для кожної
програми: [Syncing saves between devices](platforms/README.md#syncing-saves-between-devices).

### Перевірені пристрої

Програми для iPhone/iPad, Android, Мака й Linux перевірено на справжніх пристроях; програму для Мака
з Intel поки що — лише через Rosetta на Маці з процесором Apple. Програма для
Switch не перевірена: автор не має Switch з кастомною прошивкою, на якій запускається homebrew.
Відгуки власників Switch — в [issues](https://github.com/evilHazzarD/ReHoMM4/issues).

### Переклади

Переклади живуть в окремому репозиторії,
[ReHoMM4-translations](https://github.com/evilHazzarD/ReHoMM4-translations), і потрапляють у
програми тільки через його релізи: програми перевіряють найновіший при кожному запуску й
завантажують мову, коли її обрано. Нова чи виправлена мова не потребує нової програми. Як додати
свою мову: [TRANSLATING.md](https://github.com/evilHazzarD/ReHoMM4-translations/blob/main/TRANSLATING.md).

### Як це зроблено

Порт зроблено статичною рекомпіляцією за допомогою [x86recomp](https://github.com/JTM-rootstorm/x86recomp):
код `heroes4.exe` заздалегідь перекладається в нативний код, а графіка, звук і вікно працюють через
SDL3 (Metal на пристроях Apple, Vulkan на Android і Linux). У репозиторії немає ні коду, ні файлів
гри.

| Шлях | Що |
|---|---|
| `h4/` | порт: `lifter.manifest`, замінені функції гри (`src/`) |
| `patches/` | зміни до x86recomp, що накладаються на зафіксований коміт |
| `symbols/` | адреси функцій і таблиць переходів `heroes4.exe` (з Ghidra) |
| `platforms/` | збирання програм: iOS, Android, Linux, Мак і Switch (`build.sh`, по теці на платформу); DMG для Мака, який збирає гру на Маці гравця (`macos-dmg/`) |
| `libs/innoextract/` | розпаковувач інсталятора GOG, яким користуються всі програми |
| `assets/icon/` | іконки програм (щит із «IV») |
| `licenses/` | тексти ліцензій сторонніх частин, що входять у програми |
| `tools/` | підготовка збирання з коду (`build.sh`), перевірки перед публікацією (`release/`), скрипти Ghidra та інструменти для арту (`re/`) |

#### Збирання з коду

На Маці з Homebrew і вашим інсталятором GOG: `tools/build.sh [шлях до setup_….exe]` розпаковує гру
й готує її код, далі для кожної платформи `platforms/deps.sh <платформа>` і:

| Команда | Результат |
|---|---|
| `platforms/build.sh ios [TEAM_ID]` | `HeroesIV-ios.ipa` |
| `platforms/build.sh android` | `HeroesIV-android_arm64.apk` |
| `platforms/build.sh linux` | `HeroesIV-linux-x86_64.tar.gz` |
| `platforms/build.sh macos-arm64` | `Heroes IV.app` і `HeroesIV-macos-arm64.dmg` |
| `platforms/build.sh macos-x86_64` | `Heroes IV.app` і `HeroesIV-macos-x86_64.dmg` (Intel) |
| `platforms/build.sh switch` | `HeroesIV.nro` |

Подробиці: [`platforms/README.md`](platforms/README.md).

### Подяки

ReHoMM4 почався з [HoMM4Deadware](https://github.com/LeoDeadware/HoMM4Deadware) від LeoDeadware
(ліцензія MIT): звідти перша версія порту й український переклад. Файли, узяті звідти, кажуть про
це на початку.

### Ліцензія

Власний код порту, інструменти та іконка — MIT (див. `LICENSE`), частини з HoMM4Deadware — під
авторським правом LeoDeadware. x86recomp — MIT від його авторів; ffmpeg — LGPL 2.1+. Кожна
програма містить `THIRD_PARTY_NOTICES.txt` з ліцензіями всього, що в ній (SDL3, ffmpeg,
innoextract, Boost, XZ, glslang, SPIRV-Cross, а на Switch — libnx, curl, zlib, Mesa тощо); його
складає `platforms/notices.py` з рядка лінкування збірки. На саму гру ліцензія не поширюється: вона
ваша, з GOG.

Heroes of Might and Magic — торгова марка Ubisoft. Це фанатський проєкт, не пов'язаний з Ubisoft
чи GOG.
