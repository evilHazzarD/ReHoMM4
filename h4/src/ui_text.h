// The app's own words (its settings on every platform, its notices, the random map panel...) in
// the game's languages. The code has them in English; each language is one file,
// h4/ui_text/<code>.json, built into the app (h4/cmake/ui_text.cmake):
//
//   {"name": "Deutsch", "words": {"Settings": "Einstellungen", "Folder: {1}": "Ordner: {1}", ...}}
//
// A new language: a new file there, its code the game's ("de": a GOG install's, "uk": a
// translation's; language.h), with the English words it has: a word left out shows in English.
// A word with two meanings has its context after '|' ("Normal|water"), not shown in English.
// {1}, {2}... are filled in by the calls that take arguments.
#pragma once

#include <initializer_list>
#include <map>
#include <string>
#include <vector>

namespace h4 {

// `english` in the interface's language, UTF-8: the one chosen (UiLanguageChoice), else `lang`, the
// game's ("en", "" or one without a file: English). The text stays valid while the app runs.
const char* TrIn(const std::string& lang, const char* english);
// The same with {1}, {2}... replaced by `args`.
std::string TrIn(const std::string& lang, const char* english, std::initializer_list<std::string> args);
std::string TrIn(const std::string& lang, const char* english, const std::vector<std::string>& args);

// The interface's own language, chosen in the settings apart from the game's: "" (the default:
// the game's, the `lang` the calls give), or a code ("en", "de"...), which TrIn then uses whatever
// `lang` it is given. Kept in <data_dir>/ui_language. The words drawn in the game's font (the
// random map panel, the notices: random_map/locale.h) stay in the game's language, its code page.
void LoadUiLanguageChoice(const std::string& data_dir);
std::string UiLanguageChoice();
void SetUiLanguageChoice(const std::string& code);
// The choices in order: "" (as the game), "en", then the files' languages by their names.
std::vector<std::string> UiLanguageChoices();
// A language's name in it ("English", "Deutsch"; the code when it has no file).
std::string UiLanguageName(const std::string& code);
// The choice's name in the interface's language: "As the game (Deutsch)" for "" when the game's
// language is `game_lang`, else UiLanguageName.
std::string UiLanguageChoiceName(const std::string& choice, const std::string& game_lang);

// A language's words (English -> UTF-8), or null when there is no file for it.
const std::map<std::string, std::string>* UiWords(const std::string& lang);
// The languages with a file ("de", "uk"...).
std::vector<std::string> UiLanguages();

}  // namespace h4
