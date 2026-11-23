#include "template.h"

#include <algorithm>
#include <cctype>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <map>
#include <set>
#include <sstream>

namespace h4::random_map {

namespace {

std::string Trim(const std::string& s) {
  size_t a = 0, b = s.size();
  while (a < b && std::isspace(static_cast<unsigned char>(s[a]))) ++a;
  while (b > a && std::isspace(static_cast<unsigned char>(s[b - 1]))) --b;
  return s.substr(a, b - a);
}

std::string Lower(std::string s) {
  for (char& c : s) c = char(std::tolower(static_cast<unsigned char>(c)));
  return s;
}

// A map size from a word or a number; 0 if neither.
int SizeOf(const std::string& w) {
  const std::string l = Lower(w);
  if (l == "small" || l == "s" || l == "76") return 76;
  if (l == "medium" || l == "m" || l == "152") return 152;
  if (l == "large" || l == "l" || l == "228") return 228;
  if (l == "huge" || l == "xl" || l == "extra-large" || l == "extralarge" || l == "304") return 304;
  return 0;
}

bool Number(const std::string& s, double& out) {
  char* end = nullptr;
  out = std::strtod(s.c_str(), &end);
  if (!s.empty() && end && *end == '/' && end != s.c_str()) {  // a fraction, as turn=3/8
    char* end2 = nullptr;
    const double d = std::strtod(end + 1, &end2);
    if (!end2 || *end2 || d == 0) return false;
    out /= d;
    return true;
  }
  return !s.empty() && end && *end == 0;
}

int GuardOf(const std::string& w) {
  const std::string l = Lower(w);
  if (l == "open" || l == "none") return 0;
  if (l == "weak") return 1;
  if (l == "normal") return 2;
  if (l == "strong") return 3;
  if (l == "deadly") return 4;
  if (l == "auto") return -1;
  return -2;
}

}  // namespace

std::vector<TemplateInfo> BuiltInTemplates() {
  std::vector<TemplateInfo> out(1);
  out[0].name = kClassicTemplate;
  return out;
}

namespace {

// Checks a finished template; "" if it is good.
std::string Finish(Template& t, bool players_given) {
  if (t.name.empty()) return "no name";
  int starts = 0;
  for (const TemplateZone& z : t.zones) starts += z.start;
  if (starts < 2) return t.name + ": 2 start zones at least";
  if (starts > 6) return t.name + ": 6 start zones at most";
  if (!players_given) t.min_players = 2, t.max_players = starts;
  if (t.max_players > starts) return t.name + ": more players than start zones";
  std::vector<int> parent(t.zones.size());  // every zone reachable
  for (size_t i = 0; i < parent.size(); ++i) parent[i] = int(i);
  auto find = [&](int x) {
    while (parent[size_t(x)] != x) x = parent[size_t(x)] = parent[size_t(parent[size_t(x)])];
    return x;
  };
  for (const TemplateLink& l : t.links) parent[size_t(find(l.a))] = find(l.b);
  for (size_t i = 1; i < t.zones.size(); ++i)
    if (find(int(i)) != find(0)) return t.name + ": zone " + t.zones[i].id + " is not linked to the others";
  return "";
}

}  // namespace

bool ParseTemplates(const std::string& text, std::vector<Template>& out, std::string* error) {
  out.clear();
  Template cur;
  bool open = false, players_given = false;
  std::map<std::string, int> ids;
  std::set<std::pair<int, int>> joined;
  std::istringstream in(text);
  std::string raw;
  int line_no = 0, block_line = 0;
  auto fail = [&](const std::string& e) {
    if (error) *error = (line_no ? "line " + std::to_string(line_no) + ": " : std::string()) + e;
    return false;
  };
  auto close = [&]() {
    if (!open) return std::string();
    const std::string e = Finish(cur, players_given);
    if (!e.empty()) return "the template from line " + std::to_string(block_line) + ": " + e;
    out.push_back(std::move(cur));
    cur = {};
    open = players_given = false;
    ids.clear();
    joined.clear();
    return std::string();
  };
  while (std::getline(in, raw)) {
    ++line_no;
    if (const size_t hash = raw.find('#'); hash != std::string::npos) raw.resize(hash);
    std::string line = Trim(raw);
    if (line.empty()) continue;
    // "key = value" and "key value" alike; attributes as name=value
    std::vector<std::string> words;
    {
      std::istringstream ws(line);
      for (std::string w; ws >> w;)
        if (w != "=") words.push_back(w);
    }
    if (words.empty()) continue;
    std::string key = Lower(words[0]);
    if (!key.empty() && key.back() == '=') key.pop_back();
    if (!key.empty() && key.back() == ':') key.pop_back();
    if (key == "name") {  // a template begins (a file may have several: its variants)
      if (const std::string e = close(); !e.empty()) {
        line_no = 0;
        return fail(e);
      }
      std::string name = Trim(line.substr(words[0].size()));
      if (!name.empty() && (name[0] == '=' || name[0] == ':')) name = Trim(name.substr(1));
      std::string clean;  // the map's description keeps it between ", " and "."
      for (char c : name)
        if (c != ',' && c != '.' && c != '\r') clean += c;
      cur.name = Trim(clean);
      if (cur.name.empty()) return fail("an empty name");
      open = true;
      block_line = line_no;
      continue;
    }
    if (!open) return fail("a template starts with its name (name <its name>)");
    if (key == "players") {
      if (words.size() != 2) return fail("players: a number or a range, as 2-4");
      const std::string& v = words[1];
      const size_t dash = v.find('-');
      cur.min_players = std::atoi(v.substr(0, dash).c_str());
      cur.max_players = dash == std::string::npos ? cur.min_players : std::atoi(v.substr(dash + 1).c_str());
      if (cur.min_players < 2 || cur.max_players > 6 || cur.min_players > cur.max_players)
        return fail("players: 2 to 6");
      players_given = true;
    } else if (key == "sizes" || key == "size") {
      if (words.size() != 2) return fail("sizes: a size or a range, as small-large");
      const std::string& v = words[1];
      const size_t dash = v.find('-', 1);
      const std::string a = v.substr(0, dash), b = dash == std::string::npos ? a : v.substr(dash + 1);
      cur.min_size = SizeOf(a);
      cur.max_size = SizeOf(b);
      if (!cur.min_size || !cur.max_size || cur.min_size > cur.max_size)
        return fail("sizes: small, medium, large or huge, or a range of them");
    } else if (key == "zone") {
      if (words.size() < 3) return fail("zone <name> start|treasure [rich] [size=N] [town] [x=N y=N]");
      TemplateZone z;
      z.id = words[1];
      if (ids.count(z.id)) return fail("zone " + z.id + " twice");
      const std::string kind = Lower(words[2]);
      if (kind == "start") z.start = true, z.tier = 0;
      else if (kind != "treasure") return fail("a zone is start or treasure, not " + words[2]);
      bool has_x = false, has_y = false, has_r = false, has_turn = false;
      for (size_t i = 3; i < words.size(); ++i) {
        const std::string w = Lower(words[i]);
        const size_t eq = w.find('=');
        double v = 0;
        if (w == "rich") z.tier = z.start ? 0 : 2;
        else if (w == "normal") z.tier = z.start ? 0 : 1;
        else if (w == "town") z.town = !z.start;
        else if (eq != std::string::npos && Number(w.substr(eq + 1), v)) {
          const std::string k = w.substr(0, eq);
          if (k == "size") {
            if (v < 0.25 || v > 4) return fail("size: 0.25 to 4");
            z.size = v;
          } else if (k == "x" || k == "y") {
            if (v < -0.4 || v > 0.4) return fail(k + ": -0.4 to 0.4");
            (k == "x" ? z.x : z.y) = v;
            (k == "x" ? has_x : has_y) = true;
          } else if (k == "r") {
            if (v < 0 || v > 0.4) return fail("r: 0 to 0.4");
            z.r = v;
            has_r = true;
          } else if (k == "turn") {
            z.turn = v;
            has_turn = true;
          } else {
            return fail("unknown: " + words[i]);
          }
        } else {
          return fail("unknown: " + words[i]);
        }
      }
      if (has_x != has_y) return fail("x and y go together");
      if (has_r != has_turn) return fail("r and turn go together");
      if (has_x && has_r) return fail("x and y, or r and turn: not both");
      z.placed = has_x;
      z.polar = has_r;
      ids[z.id] = int(cur.zones.size());
      cur.zones.push_back(std::move(z));
      if (cur.zones.size() > 40) return fail("40 zones at most");
    } else if (key == "link") {
      if (words.size() < 3) return fail("link <zone> <zone> [open|weak|normal|strong|deadly]");
      const auto a = ids.find(words[1]), b = ids.find(words[2]);
      if (a == ids.end()) return fail("no zone " + words[1] + " (above this line)");
      if (b == ids.end()) return fail("no zone " + words[2] + " (above this line)");
      if (a->second == b->second) return fail("a zone linked to itself");
      const auto key2 = std::minmax(a->second, b->second);
      if (!joined.insert({key2.first, key2.second}).second) return fail("these zones are linked already");
      TemplateLink l{a->second, b->second, -1};
      for (size_t i = 3; i < words.size(); ++i) {
        std::string w = words[i];
        if (Lower(w).rfind("guard=", 0) == 0) w = w.substr(6);
        const int g = GuardOf(w);
        if (g == -2) return fail("unknown guard: " + words[i]);
        l.guard = g;
      }
      cur.links.push_back(l);
    } else {
      return fail("unknown: " + words[0]);
    }
  }
  line_no = 0;
  if (const std::string e = close(); !e.empty()) return fail(e);
  if (out.empty()) return fail("no template in it");
  return true;
}

std::string TemplateSum(const std::string& text) {
  uint32_t h = 2166136261u;
  for (char c : text) {
    if (c == '\r') continue;  // (the same file with Windows' line ends)
    h = (h ^ uint8_t(c)) * 16777619u;
  }
  char out[9];
  std::snprintf(out, sizeof out, "%08x", h);
  return out;
}

std::vector<TemplateInfo> TemplatesOf(const std::string& text) {
  std::vector<TemplateInfo> list;
  std::vector<Template> ts;
  if (!ParseTemplates(text, ts, nullptr)) return list;
  for (const Template& t : ts) {
    TemplateInfo* info = nullptr;
    for (TemplateInfo& i : list)
      if (i.name == t.name) info = &i;
    if (!info) {
      list.push_back({});
      info = &list.back();
      info->name = t.name;
      info->text = text;
    }
    info->variants.push_back({t.min_players, t.max_players, t.min_size, t.max_size});
  }
  return list;
}

bool MakeTemplate(const std::string& name, const std::string& text, int players, int size, Template& out,
                  std::string* error) {
  auto fail = [&](const std::string& e) {
    if (error) *error = e;
    return false;
  };
  std::vector<Template> ts;
  if (text.empty()) return fail("no template " + name + " (its file is gone?)");
  if (!ParseTemplates(text, ts, error)) return false;
  bool named = false;
  for (Template& t : ts) {  // the first variant for these players and size
    if (t.name != name) continue;
    named = true;
    if (players < t.min_players || players > t.max_players || size < t.min_size || size > t.max_size) continue;
    out = std::move(t);
    int n = 0;  // the starts past the players': treasure zones
    for (TemplateZone& z : out.zones)
      if (z.start && n++ >= players) z.start = false, z.tier = 1;
    return true;
  }
  if (!named) return fail("no template " + name);
  return fail("the template " + name + " is not for " + std::to_string(players) + " players on this map size");
}

const char* const kTemplateExample = R"(# A random map template for ReHoMM4's random map generator.
# Put .txt files like this one in this folder: the generator's "Template" setting lists them
# (the ones that fit the map size and the players' number). Lines starting with # are comments.
#
#   name <the template's name>
#   players <from>-<to>          (2 to 6; at most the start zones' number; default: 2 to it)
#   sizes <from>-<to>            (small, medium, large, huge; default: all)
#   zone <id> start              a player's start (in order: player 1, 2, ...); the starts past
#                                the players' number become treasure zones
#   zone <id> treasure [rich] [town] [size=N]
#                                rich: better treasures, stronger monsters; town: a neutral town;
#                                size: its area against the others' (0.25 to 4, default 1)
#   ... [x=N y=N]                where the zone is: -0.4 to 0.4, the map's middle 0 0 (the
#                                generator turns the whole by quarters)
#   ... [r=N turn=N]             or round the middle: r from it (0 to 0.4), turn a part of
#                                the circle (as 3/8; the generator turns the whole by any
#                                angle). Without either the generator lays
#                                the zones out by their links itself, but then zones linked
#                                in the file do not always border each other on the map (the
#                                generator then joins them through others): for a template
#                                that must keep all its links, give every zone x and y.
#   link <id> <id> [open|weak|normal|strong|deadly]
#                                a way between two zones and its guard (default: by the zones'
#                                riches; open: no guard)
#
# Every zone must be linked to the others. Water, the underground, monsters' strength and the
# obelisks come from the generator's settings.
#
# A file may have several templates (each from its "name" line), and several of one name: its
# variants, for other players' numbers or map sizes (the first that fits is used), as
# ring.txt, hub.txt and duel.txt here. A template's name is listed once (the first file's).

name Crossroads
players 2-4
sizes small-huge

zone p1 start x=-0.32 y=-0.32
zone p2 start x=0.32 y=-0.32
zone p3 start x=0.32 y=0.32
zone p4 start x=-0.32 y=0.32
zone a treasure x=-0.17 y=-0.17
zone b treasure x=0.17 y=-0.17
zone c treasure x=0.17 y=0.17
zone d treasure x=-0.17 y=0.17
zone middle treasure rich town size=1.5 x=0 y=0

link p1 a
link p2 b
link p3 c
link p4 d
link a b weak
link b c weak
link c d weak
link d a weak
link a middle strong
link b middle strong
link c middle strong
link d middle strong
)";

}  // namespace h4::random_map
