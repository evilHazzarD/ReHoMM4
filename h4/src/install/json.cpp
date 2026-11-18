#include "install/json.h"

#include <cstdlib>

namespace h4::install {

namespace {

const Json kNull;

struct Parser {
  const std::string& s;
  size_t i = 0;
  std::string err;

  void Ws() {
    while (i < s.size() && (s[i] == ' ' || s[i] == '\t' || s[i] == '\n' || s[i] == '\r')) i++;
  }
  bool Fail(const char* what) {
    if (err.empty()) err = std::string(what) + " at " + std::to_string(i);
    return false;
  }
  static void PutUtf8(std::string* o, unsigned cp) {
    if (cp < 0x80) {
      o->push_back(char(cp));
    } else if (cp < 0x800) {
      o->push_back(char(0xC0 | (cp >> 6)));
      o->push_back(char(0x80 | (cp & 0x3F)));
    } else if (cp < 0x10000) {
      o->push_back(char(0xE0 | (cp >> 12)));
      o->push_back(char(0x80 | ((cp >> 6) & 0x3F)));
      o->push_back(char(0x80 | (cp & 0x3F)));
    } else {
      o->push_back(char(0xF0 | (cp >> 18)));
      o->push_back(char(0x80 | ((cp >> 12) & 0x3F)));
      o->push_back(char(0x80 | ((cp >> 6) & 0x3F)));
      o->push_back(char(0x80 | (cp & 0x3F)));
    }
  }
  bool Hex4(unsigned* v) {
    if (i + 4 > s.size()) return Fail("short \\u escape");
    *v = 0;
    for (int k = 0; k < 4; k++) {
      char c = s[i++];
      *v <<= 4;
      if (c >= '0' && c <= '9') *v |= unsigned(c - '0');
      else if (c >= 'a' && c <= 'f') *v |= unsigned(c - 'a' + 10);
      else if (c >= 'A' && c <= 'F') *v |= unsigned(c - 'A' + 10);
      else return Fail("bad \\u escape");
    }
    return true;
  }
  bool Str(std::string* o) {
    if (s[i] != '"') return Fail("expected string");
    i++;
    while (i < s.size() && s[i] != '"') {
      char c = s[i++];
      if (c != '\\') {
        o->push_back(c);
        continue;
      }
      if (i >= s.size()) return Fail("bad escape");
      char e = s[i++];
      switch (e) {
        case '"': o->push_back('"'); break;
        case '\\': o->push_back('\\'); break;
        case '/': o->push_back('/'); break;
        case 'b': o->push_back('\b'); break;
        case 'f': o->push_back('\f'); break;
        case 'n': o->push_back('\n'); break;
        case 'r': o->push_back('\r'); break;
        case 't': o->push_back('\t'); break;
        case 'u': {
          unsigned cp;
          if (!Hex4(&cp)) return false;
          if (cp >= 0xD800 && cp < 0xDC00 && i + 6 <= s.size() && s[i] == '\\' && s[i + 1] == 'u') {
            i += 2;
            unsigned lo;
            if (!Hex4(&lo)) return false;
            cp = 0x10000 + ((cp - 0xD800) << 10) + (lo - 0xDC00);
          }
          PutUtf8(o, cp);
          break;
        }
        default: return Fail("bad escape");
      }
    }
    if (i >= s.size()) return Fail("unterminated string");
    i++;
    return true;
  }
  bool Value(Json* v, int depth) {
    if (depth > 64) return Fail("too deep");
    Ws();
    if (i >= s.size()) return Fail("unexpected end");
    char c = s[i];
    if (c == '{') {
      v->type = Json::Object;
      i++;
      Ws();
      if (i < s.size() && s[i] == '}') return i++, true;
      for (;;) {
        Ws();
        std::string key;
        if (i >= s.size() || !Str(&key)) return Fail("expected key");
        Ws();
        if (i >= s.size() || s[i] != ':') return Fail("expected ':'");
        i++;
        if (!Value(&v->obj[key], depth + 1)) return false;
        Ws();
        if (i < s.size() && s[i] == ',') { i++; continue; }
        if (i < s.size() && s[i] == '}') return i++, true;
        return Fail("expected ',' or '}'");
      }
    }
    if (c == '[') {
      v->type = Json::Array;
      i++;
      Ws();
      if (i < s.size() && s[i] == ']') return i++, true;
      for (;;) {
        v->arr.emplace_back();
        if (!Value(&v->arr.back(), depth + 1)) return false;
        Ws();
        if (i < s.size() && s[i] == ',') { i++; continue; }
        if (i < s.size() && s[i] == ']') return i++, true;
        return Fail("expected ',' or ']'");
      }
    }
    if (c == '"') {
      v->type = Json::String;
      return Str(&v->str);
    }
    if (!s.compare(i, 4, "true")) { v->type = Json::Bool; v->b = true; i += 4; return true; }
    if (!s.compare(i, 5, "false")) { v->type = Json::Bool; i += 5; return true; }
    if (!s.compare(i, 4, "null")) { i += 4; return true; }
    size_t start = i;
    if (s[i] == '-') i++;
    while (i < s.size() && (isdigit((unsigned char)s[i]) || s[i] == '.' || s[i] == 'e' ||
                            s[i] == 'E' || s[i] == '+' || s[i] == '-'))
      i++;
    if (i == start) return Fail("unexpected character");
    v->type = Json::Number;
    v->str = s.substr(start, i - start);
    v->num = std::strtod(v->str.c_str(), nullptr);
    return true;
  }
};

}  // namespace

const Json& Json::operator[](const std::string& key) const {
  if (type != Object) return kNull;
  auto it = obj.find(key);
  return it == obj.end() ? kNull : it->second;
}

const Json& Json::operator[](size_t i) const {
  return type == Array && i < arr.size() ? arr[i] : kNull;
}

long long Json::integer() const {
  return type == Number || type == String ? std::strtoll(str.c_str(), nullptr, 10) : 0;
}

bool Json::Parse(const std::string& s, Json* out, std::string* error) {
  Parser p{s};
  *out = Json{};
  bool ok = p.Value(out, 0);
  if (ok) {
    p.Ws();
    if (p.i != s.size()) ok = p.Fail("trailing data");
  }
  if (!ok && error) *error = p.err;
  return ok;
}

}  // namespace h4::install
