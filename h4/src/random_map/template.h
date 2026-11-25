#pragma once
// Map templates: a graph of zones (the players' starts, treasure zones) and the links between
// them, as the Heroes 3 generator's. Classic is the generator's own layout (no template); every
// other one is a text file in the player's Templates folder (kTemplateExample: the format), ours
// (kShippedTemplates: written there the first time) or the player's. A file may have several
// templates, and several of one name: its variants for other players' numbers and map sizes (as
// a Heroes 3 HotA template pack).
#include <string>
#include <vector>

namespace h4::random_map {

struct TemplateZone {
  std::string id;      // the file's name for it
  bool start = false;  // a player's start: the starts in order are players 1, 2, ...
  int tier = 1;        // a treasure zone's riches: 1 normal, 2 rich
  double size = 1;     // its area against the others' (0.25 to 4)
  bool town = false;   // a neutral town in it, well guarded
  bool placed = false; // x, y given: where it is on the map, -0.4 to 0.4 (the map's middle 0, 0)
  double x = 0, y = 0;
  // Or at `r` from the middle, `turn` of a full circle round it (r= turn=; the generator's own
  // sine: the same maps on every platform).
  bool polar = false;
  double r = 0, turn = 0;
};

struct TemplateLink {
  int a = 0, b = 0;  // the zones' indices
  int guard = -1;    // -1: by the zones' riches; 0 none (an open way); 1 to 4 the monsters' level
};

struct Template {
  std::string name;
  int min_players = 2, max_players = 6;
  int min_size = 76, max_size = 304;  // the map sizes it is for
  std::vector<int> teams;  // the "teams" line: player i's team (0, 1, ...; -1 its own), empty none
  std::vector<TemplateZone> zones;
  std::vector<TemplateLink> links;
};

// A template the panel lists: Classic (text empty) or a file's (its text), with its variants.
struct TemplateInfo {
  std::string name;
  std::string text;
  struct Variant {
    int min_players, max_players, min_size, max_size;
    std::vector<int> teams;  // Template::teams: set in the panel when the template is chosen
  };
  std::vector<Variant> variants;  // none: any (Classic)
  // A variant for these players and size; players 0: any number, size 0: any size.
  bool Fits(int players, int size) const {
    if (variants.empty()) return true;
    for (const Variant& v : variants)
      if ((!players || (players >= v.min_players && players <= v.max_players)) &&
          (!size || (size >= v.min_size && size <= v.max_size)))
        return true;
    return false;
  }
};

constexpr const char* kClassicTemplate = "Classic";

// Classic.
std::vector<TemplateInfo> BuiltInTemplates();

// Reads a template file's text: its templates. false: `error` says what is wrong and on which line.
bool ParseTemplates(const std::string& text, std::vector<Template>& out, std::string* error);

// A short sum of a template file's text (8 hex digits): the shipped files' (whether the player has
// changed one).
std::string TemplateSum(const std::string& text);
// The same of what makes the maps (comments, blank lines and the teams line, which only the
// panel reads, left out), written with its name into a map: its editor can tell when the file has
// changed since. (Maps of 0.4 have TemplateSum's.)
std::string TemplateMapSum(const std::string& text);

// The templates a file's text has, for the panel (none if it does not read).
std::vector<TemplateInfo> TemplatesOf(const std::string& text);

// The template `name` in a file's `text` for these players and map size: its first variant for
// them (its starts past the players' number become treasure zones). false: `error`.
bool MakeTemplate(const std::string& name, const std::string& text, int players, int size, Template& out,
                  std::string* error);

// The templates folder's example file: the format, as comments, and a template that works.
extern const char* const kTemplateExample;
constexpr const char* kTemplateExampleName = "example.txt";

// Our templates' files (templates_shipped.cpp): written into the folder when not there.
struct ShippedTemplate {
  const char* file;
  const char* text;
};
extern const ShippedTemplate kShippedTemplates[];
extern const int kShippedTemplateCount;

}  // namespace h4::random_map
