// A small JSON reader for GOG's API answers (install/gog.cpp): objects,
// arrays, strings, numbers (kept as text too: build ids do not fit a
// double), true/false/null. Read-only; no writer.
#pragma once

#include <map>
#include <memory>
#include <string>
#include <vector>

namespace h4::install {

struct Json {
  enum Type { Null, Bool, Number, String, Array, Object } type = Null;
  bool b = false;
  double num = 0;
  std::string str;  // String, or a Number's text
  std::vector<Json> arr;
  std::map<std::string, Json> obj;

  // Member or element, or a Null value when there is none.
  const Json& operator[](const std::string& key) const;
  const Json& operator[](size_t i) const;
  size_t size() const { return type == Array ? arr.size() : type == Object ? obj.size() : 0; }
  bool has(const std::string& key) const { return type == Object && obj.count(key); }
  // A String's text or a Number's text; "" otherwise.
  const std::string& text() const { return str; }
  long long integer() const;

  // Parses `s`; false (and *error) if it is not JSON.
  static bool Parse(const std::string& s, Json* out, std::string* error = nullptr);
};

}  // namespace h4::install
