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
  // The parchment drawn under it (a piece of one of the game's dialogs); without it plain.
  void SetBackground(const Image* parchment) { back_ = parchment; dirty_ = true; }
  // The game's dialog frame round it and between its halves: taken from the new game options
  // dialog's background (layers.dialog.new_game_options "background", 800 x 600); without it a
  // plain drawn one.
  void SetFrame(const Image* dialog) { frame_ = dialog; dirty_ = true; }
  // The buttons drawn as the game's own text buttons (layers.button.large: "released", "pressed")
  // with a bigger font of its; without them, plain plates in the panel's font.
  // `fonts`: prose_antique by size, from 20 up (20, 22, ... 36; null where missing): the buttons
  // are drawn at the screen's own resolution with the size that fits it (ButtonPicture's scale).
  void SetButtonArt(std::vector<const Font*> fonts, const Image* released, const Image* pressed);
  // The templates to choose from: the built-in ones and the player's files (template.h).
  void SetTemplates(std::vector<TemplateInfo> templates);
  // message: the game's bytes; error: drawn red (what went wrong, not what was done)
  void SetMode(Mode m, const std::string& message, bool error = false);
  Mode mode() const { return mode_; }
  const MapSetup& setup() const { return setup_; }
  void SetSeed(uint32_t seed);  // a rolled one (the player's own: typed, stepped)
  bool rolled() const { return rolled_; }

  // (x, y) in the picture; right: the right button (a list's previous item).
  Action Click(int x, int y, bool right);
  // A key: '0'-'9', '\b', '\r' (create; in the seed's field: done), 27 (Esc: close or cancel; in
  // the field: done).
  Action Key(int key);
  // The mouse wheel at (x, y) in the picture (`dy` > 0 up): scrolls the hero's biography when
  // over it (true then).
  bool Wheel(int x, int y, int dy);
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
  // `pressed`: the one held down (a kOpenButton... one, 0 none), drawn pressed. `scale`: screen
  // pixels per frame pixel; the picture is drawn finer by *density (its w x h covers
  // w / density x h / density of the frame; 1 without the game's button art).
  const std::vector<uint32_t>& ButtonPicture(int* w, int* h, int remove = 0, int pressed = 0,
                                             float scale = 1.0f, float* density = nullptr);
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
  // A reason the generator gave (GenerateMap's error) in the app's words where it has them (a
  // template file's own errors, about its English keywords, and the system's stay as they are).
  std::string Reason(const std::string& english) const;

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
  bool message_error_ = false;
  MapSetup setup_;
  int size_index_ = 0;
  std::vector<uint32_t> pic_, button_;
  int button_remove_ = -1;            // what button_ shows (ButtonPicture's remove)
  int button_pressed_ = -1;           // and its pressed
  float button_density_ = 0;          // and its density
  int button_tw_ = 0, button_th_ = 0;  // its size (button_w_, button_h_: in the frame)
  std::vector<const Font*> button_fonts_;
  Image button_up_scaled_, button_down_scaled_;  // the art at button_density_
  const Image* button_up_ = nullptr;
  const Image* back_ = nullptr;
  const Image* frame_ = nullptr;
  const Image* button_down_ = nullptr;
  int button_w_ = 0, button_h_ = 0;   // its size
  std::vector<std::pair<int, int>> button_spans_;  // each button's x0, x1 in it
  std::string editing_;  // the title of the map edited ("" a new one)
  std::vector<Hit> hits_;
  std::string bio_text_;   // the biography shown, and how many lines it is scrolled down
  int bio_scroll_ = 0, bio_max_ = 0;
  int bio_x0_ = 0, bio_y0_ = 0, bio_x1_ = 0, bio_y1_ = 0;  // where it is (the wheel's place)
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
  void ApplyTemplateTeams(const TemplateInfo& t);  // its variant's teams line, if it has one
  // The rows' least and most numbers of humans and computers the setup can give.
  void Counts(int* min_h, int* max_h, int* min_k, int* max_k) const;
  std::vector<TemplateInfo> templates_;
};

}  // namespace h4::random_map
