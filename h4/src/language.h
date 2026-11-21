// The game's language: English, or a translation from the ReHoMM4-translations repository,
// carried as a language pack (<code>.h4p, its tools/make_pack.py): a delta
// applied to the player's own game folder in place. Its data is appended to five archives once,
// and the language is then which directory entries are written (a few hundred 12-byte writes,
// both ways). No second copy of the game. One translation's data is in the archives at a time:
// switching to another one takes the first back out.
#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace h4 {

// A language the app offers. The translations are released from the ReHoMM4-translations
// repository (its tools/release.sh): its latest release's languages.json lists them with their
// packs. The app is built with the list of the release current then (platforms/languages.py),
// checks for a newer one at every start (RefreshLanguages) and keeps it in <data>/languages.json,
// so a new release reaches the players without a new app: a changed pack is downloaded at the
// next start once that language is chosen.
struct LanguageInfo {
  std::string code;      // "uk"
  std::string name;      // in the language: "Українська"
  std::string english;   // "Ukrainian"
  std::string note;      // "restart the game to apply" in the language
  std::string url;       // where its pack is downloaded from, "" if it is in the app
  std::string sha256;    // of the pack
  uint64_t size = 0;
};

// The list: the app's own, or the latest release's seen (<data>/languages.json). At start.
void LoadLanguages(const std::string& data_dir);
// English first, then the translations.
std::vector<LanguageInfo> Languages();
// The entry of `code`, English if there is none.
LanguageInfo FindLanguage(const std::string& code);
// Looks for a newer release in the background (the address the app was built with): saves its
// languages.json and updates the list.
void RefreshLanguages(const std::string& data_dir);

// The pack of `code` in the app ("" if it is downloaded instead): a path SDL_IOFromFile opens
// (on Android an asset's).
std::string BundledPack(const std::string& code);

// Sets `game_dir` (the folder with Data/) to `code` ("en": English) with `pack`, that
// language's pack ("" for English). `others` are the other packs at hand (in the app, or
// downloaded, current or older): the one whose data is in the archives is taken back out first.
// Checks everything first and changes nothing if the archives are not the ones the pack was made
// from; false and `error` then.
bool SetGameLanguage(const std::string& game_dir, const std::string& code, const std::string& pack,
                     const std::vector<std::string>& others, std::string* error);

// The translation's files outside the archives (single scenarios' maps/...), from `pack` into
// `dir` (again only when the pack changed): `dir`, or "" when the pack has none (`dir` removed;
// so too for an empty `pack`). The game reads them in place of the install's
// (recomp::SetTranslationFiles): the game folder itself is never changed.
std::string TranslationFiles(const std::string& pack, const std::string& dir);

}  // namespace h4
