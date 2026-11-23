#pragma once

#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

#include "catalog.h"
#include "font.h"
#include "generator.h"
#include "hero_card.h"
#include "locale.h"
#include "template.h"

namespace h4::random_map {

// The random map's setup, drawn into an ARGB picture (the app shows it over the game's frame:
// gfx::SetPresentBadge) and driven by clicks on it (a mouse, a touch, the gamepad's pointer) and
// keys: the map's size, 2 to 6 players (human or computer, faction, starting hero) and the seed.
// No SDL and nothing of the game's state here (random_map_ui.cpp does that), so a host tool can
// draw it too.
class Panel {
 public:
  enum Action { kNone, kCreate, kCancel, kClose, kNewSeedAction };
  enum class Mode { kEdit, kWorking, kDone };

  // `font`: the game's (Font::Load); `language`: the game's (locale.h), for the app's own words.
  void Init(const Font* font, std::string_view language, uint32_t seed);
  // The heroes to choose from (until then: random ones only).
  void SetCatalog(const Catalog* catalog);
  // The hero card's pictures and texts (until then: names only).
  void SetHeroArt(const HeroArt* art);
  // The templates to choose from: the built-in ones and the player's files (template.h).
  void SetTemplates(std::vector<TemplateInfo> templates);
  void SetMode(Mode m, const std::string& message);  // message: the game's bytes
  Mode mode() const { return mode_; }
  const MapSetup& setup() const { return setup_; }
  void SetSeed(uint32_t seed);  // a rolled one (the player's own: typed, stepped)
  bool rolled() const { return rolled_; }

  // (x, y) in the picture; right: the right button (a list's previous item).
  Action Click(int x, int y, bool right);
  // A key: '0'-'9', '\b', '\r' (create; in the seed's field: done), 27 (Esc: close or cancel; in
  // the field: done).
  Action Key(int key);
  // The seed's field has the keys (clicked; until a click elsewhere, Enter or Esc): the on-screen
  // keyboard is wanted.
  bool typing() const { return seed_focus_; }
  // The template's list is open (Esc closes it).
  bool choosing() const { return templates_open_; }

  // The picture, drawn again when something changed (changed: since the last call).
  const std::vector<uint32_t>& Picture(int* w, int* h, bool* changed);
  // The button that opens it ("Random map") and, right of it for the random map selected in the
  // list, the ones that edit and remove it: remove 0 none (no random map selected), 1 shown,
  // 2 asking to click "Remove" again.
  const std::vector<uint32_t>& ButtonPicture(int* w, int* h, int remove = 0);
  // Which of those buttons (x, y) in that picture is on: 0 none, else a kOpenButton... one.
  enum { kOpenButton = 1, kRemoveButton = 2, kEditButton = 3 };
  int ButtonAt(int x, int y) const;

  // A map made before, to change and make again in its place (its seed kept): the panel then
  // offers "Update map" (kCreate) for it. NewMap: back to a new map, with this seed.
  void EditMap(const MapSetup& setup, const std::string& title);
  void NewMap(uint32_t seed);
  bool editing() const { return !editing_.empty(); }

  // The app's words for the message line (the game's bytes).
  std::string Text(const char* english) const;

 private:
  struct Hit {
    int x0, y0, x1, y1, id;
  };
  const Font* font_ = nullptr;
  const Catalog* catalog_ = nullptr;
  const HeroArt* art_ = nullptr;
  int card_player_ = 0;  // whose hero the card shows: the row last changed
  Locale locale_;
  Mode mode_ = Mode::kEdit;
  std::string message_;
  MapSetup setup_;
  int size_index_ = 0;
  std::vector<uint32_t> pic_, button_;
  int button_remove_ = -1;            // what button_ shows (ButtonPicture's remove)
  int button_w_ = 0, button_h_ = 0;   // its size
  std::vector<std::pair<int, int>> button_spans_;  // each button's x0, x1 in it
  std::string editing_;  // the title of the map edited ("" a new one)
  std::vector<Hit> hits_;
  bool dirty_ = true;
  bool seed_typed_ = false;  // keys typed since the seed was last set
  bool seed_focus_ = false;  // the seed's field has the keys (typing())
  bool rolled_ = true;
  bool templates_open_ = false;  // the template's list, open over the panel

  void Redraw();
  void DrawCard(std::vector<uint32_t>& px, int x0, int x1);
  void Step(int id, int dir);
  // With a random number of humans or computers: the rows kept as the draw uses them (the most
  // it can draw of the other kind's rows; both random: one of each).
  void FitRandomRows();
  void SetPlayers(int want);  // 2 to 6, not random: computers added or taken off
  void ChooseTemplate(const TemplateInfo& t);  // and the players and size set for it if they do not fit
  // The rows' least and most numbers of humans and computers the setup can give.
  void Counts(int* min_h, int* max_h, int* min_k, int* max_k) const;
  std::vector<TemplateInfo> templates_;
};

}  // namespace h4::random_map
