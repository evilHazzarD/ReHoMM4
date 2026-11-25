#pragma once

#include <map>
#include <string>
#include <string_view>

namespace h4::random_map {

// The generator's own words (its panel, buttons and messages) in the game's language: the app's
// words (ui_text.h, h4/ui_text/<code>.json). The game's fonts hold a Windows code page's bytes, not
// UTF-8: they are given as the code page of the language.

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
  const std::map<std::string, std::string>* words_ = nullptr;  // UiWords(code_)
};

}  // namespace h4::random_map
