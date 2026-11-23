#pragma once

#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace h4::random_map {

// The generator's own words (its panel, buttons and messages) in the game's language. The game's
// fonts hold a Windows code page's bytes, not UTF-8: a translation's words are kept in UTF-8
// (translations.cpp) and given as the code page of its language.
//
// A new language: a block in translations.cpp (its code as the settings give it, H4_LANG, and
// the English words it has); a word left out shows in English.
struct Translation {
  const char* code;  // "uk"
  std::vector<std::pair<const char*, const char*>> words;  // English -> UTF-8
};
const std::vector<Translation>& Translations();

// The code page of the game's fonts in a language ("uk": 1251, "pl": 1250; else 1252).
int CodePage(std::string_view code);
// UTF-8 -> that code page (1250, 1251 or 1252); '?' for a character it has not.
std::string Encode(std::string_view utf8, int code_page);

class Locale {
 public:
  // The game's language: a translation's code ("uk"), a GOG install's ("de"), "" or "en".
  void Set(std::string_view code);
  const std::string& code() const { return code_; }
  // `english` in the language, as the game's bytes; English when it is not translated. A word
  // with two meanings has its context after '|' ("Normal|water"), left out in English.
  std::string Text(const char* english) const;

 private:
  std::string code_ = "en";
  int code_page_ = 1252;
  const Translation* translation_ = nullptr;
};

}  // namespace h4::random_map
