// ui_text.h: the words of h4/ui_text/*.json, read once from the table built into the app.
#include "ui_text.h"

#include <algorithm>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <mutex>

namespace h4 {
namespace {

struct UiTextFile {
  const char* code;
  const char* json;
  size_t size;
};
#include "ui_text_files.inc"  // kUiTextFiles (h4/cmake/ui_text.cmake)

// A JSON reader for these files: objects and strings (with their escapes) only.
class Reader {
 public:
  Reader(const char* s, size_t n) : s_(s), end_(s + n) {}

  // The top object's "words" (an object of strings) into `out`, its "name" into `name`; false if
  // the file is not that.
  bool Words(std::map<std::string, std::string>* out, std::string* name) {
    if (!Peek('{')) return false;
    for (bool first = true;; first = false) {
      if (Peek('}')) return true;
      if (!first && !Peek(',')) return false;
      std::string key;
      if (!String(&key) || !Peek(':')) return false;
      if (key == "words") {
        if (!Object(out)) return false;
      } else {
        std::string value;
        if (!String(&value)) return false;
        if (key == "name") *name = value;
      }
    }
  }

 private:
  void Space() {
    while (s_ < end_ && (*s_ == ' ' || *s_ == '\n' || *s_ == '\r' || *s_ == '\t')) s_++;
  }
  bool Peek(char c) {
    Space();
    if (s_ < end_ && *s_ == c) {
      s_++;
      return true;
    }
    return false;
  }
  bool Object(std::map<std::string, std::string>* out) {
    if (!Peek('{')) return false;
    for (bool first = true;; first = false) {
      if (Peek('}')) return true;
      if (!first && !Peek(',')) return false;
      std::string k, v;
      if (!String(&k) || !Peek(':') || !String(&v)) return false;
      (*out)[k] = v;
    }
  }
  static void Put(uint32_t cp, std::string* out) {
    if (cp < 0x80) {
      *out += char(cp);
    } else if (cp < 0x800) {
      *out += char(0xC0 | (cp >> 6));
      *out += char(0x80 | (cp & 0x3F));
    } else if (cp < 0x10000) {
      *out += char(0xE0 | (cp >> 12));
      *out += char(0x80 | ((cp >> 6) & 0x3F));
      *out += char(0x80 | (cp & 0x3F));
    } else {
      *out += char(0xF0 | (cp >> 18));
      *out += char(0x80 | ((cp >> 12) & 0x3F));
      *out += char(0x80 | ((cp >> 6) & 0x3F));
      *out += char(0x80 | (cp & 0x3F));
    }
  }
  bool Hex4(uint32_t* v) {
    if (end_ - s_ < 4) return false;
    *v = 0;
    for (int i = 0; i < 4; i++) {
      const char c = *s_++;
      *v = *v * 16 + uint32_t(c >= '0' && c <= '9' ? c - '0' : c >= 'a' && c <= 'f' ? c - 'a' + 10
                                                        : c >= 'A' && c <= 'F' ? c - 'A' + 10 : 99);
      if (*v > 0xFFFF) return false;
    }
    return true;
  }
  bool String(std::string* out) {
    if (!Peek('"')) return false;
    while (s_ < end_ && *s_ != '"') {
      if (*s_ != '\\') {
        *out += *s_++;
        continue;
      }
      if (++s_ >= end_) return false;
      const char e = *s_++;
      switch (e) {
        case 'n': *out += '\n'; break;
        case 't': *out += '\t'; break;
        case 'r': *out += '\r'; break;
        case 'b': *out += '\b'; break;
        case 'f': *out += '\f'; break;
        case 'u': {
          uint32_t cp = 0, lo = 0;
          if (!Hex4(&cp)) return false;
          if (cp >= 0xD800 && cp < 0xDC00 && end_ - s_ >= 6 && s_[0] == '\\' && s_[1] == 'u') {
            s_ += 2;
            if (!Hex4(&lo)) return false;
            cp = 0x10000 + ((cp - 0xD800) << 10) + (lo - 0xDC00);
          }
          Put(cp, out);
          break;
        }
        default: *out += e;  // \" \\ \/
      }
    }
    return Peek('"');
  }

  const char* s_;
  const char* end_;
};

struct Store {
  std::map<std::string, std::map<std::string, std::string>> words;  // by language
  std::map<std::string, std::string> names;                          // by language: "Deutsch"
  std::map<std::string, std::string> shown;  // English words with a context: the part shown
  std::mutex mu;                             // (for `shown`)
};

Store& Words() {
  static Store* s = [] {
    auto* st = new Store;
    for (const UiTextFile& f : kUiTextFiles) {
      std::map<std::string, std::string> w;
      std::string name;
      if (Reader(f.json, f.size).Words(&w, &name)) {
        st->words[f.code] = std::move(w);
        st->names[f.code] = name.empty() ? f.code : name;
      }
      else std::fprintf(stderr, "h4: h4/ui_text/%s.json is not a word list\n", f.code);
    }
    return st;
  }();
  return *s;
}

// "de-DE", "DE" -> "de".
std::string Code(const std::string& lang) {
  std::string c = lang.substr(0, lang.find_first_of("-_"));
  for (char& ch : c) ch = char(ch >= 'A' && ch <= 'Z' ? ch - 'A' + 'a' : ch);
  return c;
}

std::string Fill(std::string s, const std::string* args, size_t n) {
  for (size_t i = 0; i < n; i++) {
    const std::string key = "{" + std::to_string(i + 1) + "}";
    for (size_t at = 0; (at = s.find(key, at)) != std::string::npos; at += args[i].size()) s.replace(at, key.size(), args[i]);
  }
  return s;
}

std::mutex g_choice_mu;
std::string g_choice;      // UiLanguageChoice
std::string g_choice_dir;  // where it is kept

std::string Choice() {
  std::lock_guard<std::mutex> l(g_choice_mu);
  return g_choice;
}

}  // namespace

void LoadUiLanguageChoice(const std::string& data_dir) {
  std::string c;
  if (FILE* f = std::fopen((data_dir + "/ui_language").c_str(), "r")) {
    char buf[16] = {};
    if (std::fgets(buf, sizeof buf, f)) c.assign(buf, std::strcspn(buf, " \r\n"));
    std::fclose(f);
  }
  if (c != "en" && !UiWords(c)) c.clear();  // (a language no longer there: as the game)
  std::lock_guard<std::mutex> l(g_choice_mu);
  g_choice_dir = data_dir;
  g_choice = c;
  if (!c.empty()) std::printf("h4: interface language %s\n", c.c_str());
}

std::string UiLanguageChoice() { return Choice(); }

void SetUiLanguageChoice(const std::string& code) {
  std::lock_guard<std::mutex> l(g_choice_mu);
  g_choice = code;
  if (g_choice_dir.empty()) return;
  const std::string path = g_choice_dir + "/ui_language";
  if (code.empty()) {
    std::remove(path.c_str());
  } else if (FILE* f = std::fopen(path.c_str(), "w")) {
    std::fputs(code.c_str(), f);
    std::fclose(f);
  }
}

std::vector<std::string> UiLanguageChoices() {
  std::vector<std::pair<std::string, std::string>> by_name;
  for (const auto& [code, name] : Words().names) by_name.push_back({name, code});
  std::sort(by_name.begin(), by_name.end());
  std::vector<std::string> out = {"", "en"};
  for (const auto& [name, code] : by_name) out.push_back(code);
  return out;
}

std::string UiLanguageName(const std::string& code) {
  const std::string c = Code(code);
  if (c.empty() || c == "en") return "English";
  const auto it = Words().names.find(c);
  return it == Words().names.end() ? code : it->second;
}

std::string UiLanguageChoiceName(const std::string& choice, const std::string& game_lang) {
  if (!choice.empty()) return UiLanguageName(choice);
  return TrIn(game_lang, "As the game ({1})", {UiLanguageName(game_lang)});
}

const std::map<std::string, std::string>* UiWords(const std::string& lang) {
  Store& s = Words();
  const auto it = s.words.find(Code(lang));
  return it == s.words.end() ? nullptr : &it->second;
}

std::vector<std::string> UiLanguages() {
  std::vector<std::string> out;
  for (const auto& [code, w] : Words().words) out.push_back(code);
  return out;
}

const char* TrIn(const std::string& game_lang, const char* english) {
  const std::string chosen = Choice();
  const std::string& lang = chosen.empty() ? game_lang : chosen;
  if (const auto* w = UiWords(lang)) {
    const auto it = w->find(english);
    if (it != w->end()) return it->second.c_str();
  }
  const char* bar = std::strchr(english, '|');  // "Normal|water": the context is not shown
  if (!bar) return english;
  Store& s = Words();
  std::lock_guard<std::mutex> l(s.mu);
  auto it = s.shown.find(english);
  if (it == s.shown.end()) it = s.shown.emplace(english, std::string(english, size_t(bar - english))).first;
  return it->second.c_str();
}

std::string TrIn(const std::string& lang, const char* english, std::initializer_list<std::string> args) {
  return Fill(TrIn(lang, english), args.begin(), args.size());
}

std::string TrIn(const std::string& lang, const char* english, const std::vector<std::string>& args) {
  return Fill(TrIn(lang, english), args.data(), args.size());
}

}  // namespace h4
