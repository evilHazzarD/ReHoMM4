#include "panel.h"

#include <algorithm>
#include <cmath>
#include <cctype>
#include <cstring>
#include <map>

namespace h4::random_map {
namespace {

// The setup on the left, the hero card right of it.
constexpr int kSetupW = 600, kCardW = 600, kW = kSetupW + kCardW, kH = 640;
constexpr int kBorder = 18;
constexpr int kCardHead = 200;  // the hero card's portrait column (its name under it)  // the frame round it (the game's: 14, and its shadow)
// The game's own dialogs' colours: parchment (its texture when given: SetBackground), gold-brown
// frames, dark brown ink, headings in a deep red-brown, the chosen option gold.
constexpr uint32_t kBack = 0xFFE6D3A8, kEdge = 0xFFA07020, kPlate = 0xFFF4E8C4, kPlateDim = 0xFFDCCBA4,
                   kInk = 0xFF3C2410, kGold = 0xFF7A2E0A, kDim = 0xFF9A8464, kLit = 0xFFD9A84A,
                   kWarn = 0xFFA01808, kField = 0xFFFFF8E2, kFieldTyping = 0xFFFFFFF0, kMarked = 0xFFE8C878,
                   kList = 0xFFF8EED2;
// The players' colours, in the game's order.
constexpr uint32_t kPlayerColour[6] = {0xFFD02020, 0xFF2050E0, 0xFF20A030, 0xFFE08020, 0xFF9030C0, 0xFF20B0B0};
const char* const kSizeNames[4] = {"Small", "Medium", "Large", "Extra large"};
const char* const kFactionWords[kFactionCount] = {"Life", "Order", "Death", "Chaos", "Nature", "Might"};
const char* const kMonsterWords[4] = {"Weak", "Normal", "Strong", "Random"};
const char* const kWaterWords[4] = {"None|water", "Normal|water", "Islands", "Random|water"};

enum Id { kSize = 1, kPlayers, kSeed, kNewSeed, kCreateId, kCloseId, kUnderground, kMonsters, kWater, kWho = 10,
           kComputers = 40, kObelisks, kTreasures, kTemplate, kBioUp, kBioDown, kTeam = 50, kChoice = 100,
           kFaction = 20, kHero = 30 };


struct Canvas {
  std::vector<uint32_t>& px;
  int w, h;
  void Fill(int x0, int y0, int x1, int y1, uint32_t c) {
    for (int y = std::max(0, y0); y < std::min(h, y1); ++y)
      for (int x = std::max(0, x0); x < std::min(w, x1); ++x) px[size_t(y) * w + x] = c;
  }
  void Frame(int x0, int y0, int x1, int y1, uint32_t c) {
    Fill(x0, y0, x1, y0 + 1, c);
    Fill(x0, y1 - 1, x1, y1, c);
    Fill(x0, y0, x0 + 1, y1, c);
    Fill(x1 - 1, y0, x1, y1, c);
  }
  // c blended over the pixel at (x, y) by a (0..255).
  void Blend(int x, int y, uint32_t c, uint32_t a) {
    if (!a || x < 0 || x >= w || y < 0 || y >= h) return;
    uint32_t& d = px[size_t(y) * w + x];
    uint32_t out = 0xFF000000;
    for (int sh = 0; sh < 24; sh += 8) {
      const uint32_t sc = c >> sh & 255, dc = d >> sh & 255;
      out |= ((sc * a + dc * (255 - a)) / 255) << sh;
    }
    d = out;
  }
  // How much of the pixel at (x, y) is within the ring r0..r1 around (cx, cy): 0..255, smooth edges.
  static uint32_t Cover(float x, float y, float cx, float cy, float r0, float r1) {
    const float d = std::sqrt((x + 0.5f - cx) * (x + 0.5f - cx) + (y + 0.5f - cy) * (y + 0.5f - cy));
    const float k = std::clamp(r1 - d + 0.5f, 0.0f, 1.0f) * std::clamp(d - r0 + 0.5f, 0.0f, 1.0f);
    return uint32_t(k * 255 + 0.5f);
  }
  // A ring (r0 0: a disc) around (cx, cy).
  void Ring(float cx, float cy, float r0, float r1, uint32_t c) {
    for (int y = int(cy - r1) - 1; y <= int(cy + r1) + 1; ++y)
      for (int x = int(cx - r1) - 1; x <= int(cx + r1) + 1; ++x) Blend(x, y, c, Cover(x, y, cx, cy, r0, r1));
  }
  // A picture blended over what is there; with r > 0 only its part within r of (cx, cy).
  void Blit(const Image& img, int x0, int y0, float cx = 0, float cy = 0, float r = 0) {
    for (int y = 0; y < img.h; ++y)
      for (int x = 0; x < img.w; ++x) {
        const uint32_t s = img.argb[size_t(y) * img.w + x];
        uint32_t a = s >> 24;
        if (r > 0) a = a * Cover(x0 + x, y0 + y, cx, cy, 0, r) / 255;
        Blend(x0 + x, y0 + y, s, a);
      }
  }
};

}  // namespace

std::string Panel::Text(const char* english) const { return locale_.Text(english); }

std::string Panel::Reason(const std::string& e) const {
  // "the template <name> is not for <n> players on this map size"
  const std::string a = "the template ", b = " is not for ", c = " players on this map size";
  if (e.rfind(a, 0) == 0 && e.size() > a.size() + b.size() + c.size() && e.find(b) != std::string::npos &&
      e.compare(e.size() - c.size(), c.size(), c) == 0) {
    const size_t at = e.find(b, a.size());
    const std::string name = e.substr(a.size(), at - a.size()), n = e.substr(at + b.size(), e.size() - c.size() - at - b.size());
    return Text("the template") + " " + Text(name.c_str()) + " " + Text("is not for") + " " + n + " " +
           Text("players on this map size");
  }
  // "no template <name> (its file is gone?)", "no template <name>"
  const std::string d = "no template ", f = " (its file is gone?)";
  if (e.rfind(d, 0) == 0) {
    const bool gone = e.size() > d.size() + f.size() && e.compare(e.size() - f.size(), f.size(), f) == 0;
    const std::string name = e.substr(d.size(), e.size() - d.size() - (gone ? f.size() : 0));
    return Text("no template") + " " + Text(name.c_str()) + (gone ? " (" + Text("its file is gone?") + ")" : "");
  }
  return Text(e.c_str());
}

void Panel::Init(const Font* font, std::string_view language, uint32_t seed) {
  font_ = font;
  locale_.Set(language);
  setup_ = {};
  setup_.seed = seed;
  rolled_ = true;
  seed_typed_ = seed_focus_ = false;
  size_index_ = 0;
  setup_.size = kMapSizes[0];
  setup_.players.assign(2, {});
  setup_.players[1].human = false;
  mode_ = Mode::kEdit;
  message_.clear();
  dirty_ = true;
  button_.clear();
}

void Panel::SetCatalog(const Catalog* catalog) {
  catalog_ = catalog;
  dirty_ = true;
}

void Panel::SetHeroArt(const HeroArt* art) {
  art_ = art;
  dirty_ = true;
}

void Panel::SetTemplates(std::vector<TemplateInfo> templates) {
  templates_ = std::move(templates);
  for (const TemplateInfo& t : templates_)  // a file's text again (it may have changed)
    if (t.name == setup_.template_name) setup_.template_text = t.text;
  dirty_ = true;
}

void Panel::Counts(int* min_h, int* max_h, int* min_k, int* max_k) const {
  int h = 0, k = 0;
  for (const PlayerSetup& p : setup_.players) (p.human ? h : k) += 1;
  *min_h = *max_h = h;
  *min_k = *max_k = k;
  if (setup_.random_humans && setup_.random_computers) {
    *min_h = *min_k = 1;
    *max_h = *max_k = 5;
  } else if (setup_.random_humans) {
    *min_h = std::max(1, 2 - k);
    *max_h = 6 - k;
  } else if (setup_.random_computers) {
    *min_k = std::max(0, 2 - h);
    *max_k = 6 - h;
  }
}

void Panel::FitRandomRows() {
  if (!setup_.random_humans && !setup_.random_computers) return;
  std::vector<PlayerSetup> humans, computers;
  for (const PlayerSetup& p : setup_.players) (p.human ? humans : computers).push_back(p);
  PlayerSetup cpu;
  cpu.human = false;
  if (humans.empty()) humans.push_back({});
  if (setup_.random_humans && setup_.random_computers) {
    humans.resize(1);
    computers.resize(1, cpu);
  } else if (setup_.random_humans) {
    computers.resize(std::min<size_t>(computers.size(), 5), cpu);
    humans.resize(6 - computers.size(), PlayerSetup{});
  } else {
    computers.resize(6 - humans.size(), cpu);
  }
  setup_.players = humans;
  setup_.players.insert(setup_.players.end(), computers.begin(), computers.end());
  for (PlayerSetup& p : setup_.players)
    if (p.team >= int(setup_.players.size())) p.team = -1;
  card_player_ = std::min(card_player_, int(setup_.players.size()) - 1);
}

void Panel::SetMode(Mode m, const std::string& message, bool error) {
  mode_ = m;
  message_ = message;
  message_error_ = error && !message.empty();
  dirty_ = true;
}

void Panel::SetSeed(uint32_t seed) {
  setup_.seed = seed;
  seed_typed_ = false;
  rolled_ = true;
  dirty_ = true;
}

void Panel::Step(int id, int dir) {
  auto wrap = [](int v, int n) { return ((v % n) + n) % n; };
  if (id == kSize) {
    size_index_ = wrap(size_index_ + dir, 4);
    setup_.size = kMapSizes[size_index_];
  } else if (id == kPlayers || id == kComputers) {
    // the humans (one at least) and the computers, 2 to 6 in all: the humans first
    std::vector<PlayerSetup> humans, computers;
    for (const PlayerSetup& p : setup_.players) (p.human ? humans : computers).push_back(p);
    int h = int(humans.size()), k = int(computers.size());
    if (id == kPlayers) {
      h = std::clamp(h + dir, 1, 6);
      if (h + k > 6) k = 6 - h;
      if (h + k < 2) k = 2 - h;
    } else {
      k = std::clamp(k + dir, 0, 5);
      if (h + k > 6) h = 6 - k;
      if (h + k < 2) h = 2 - k;
    }
    humans.resize(size_t(h), PlayerSetup{});
    PlayerSetup cpu;
    cpu.human = false;
    computers.resize(size_t(k), cpu);
    setup_.players = humans;
    setup_.players.insert(setup_.players.end(), computers.begin(), computers.end());
    for (PlayerSetup& p : setup_.players)  // a team past the players' number: none
      if (p.team >= int(setup_.players.size())) p.team = -1;
    for (int k = 0; k < 6; ++k) {  // and never all in one team
      bool rivals = false;
      for (size_t j = 1; j < setup_.players.size(); ++j)
        rivals |= TeamOf(setup_.players, j) != TeamOf(setup_.players, 0);
      if (rivals) break;
      setup_.players.back().team = -1;
      if (TeamOf(setup_.players, setup_.players.size() - 1) == TeamOf(setup_.players, 0))
        setup_.players[0].team = -1;
    }
  } else if (id == kTemplate) {
    // the next one that fits the size and the players (a random number of them: any)
    const int n = int(templates_.size());
    if (!n) return;
    int at = 0;
    for (int i = 0; i < n; ++i)
      if (templates_[size_t(i)].name == (setup_.template_name.empty() ? kClassicTemplate : setup_.template_name)) at = i;
    for (int k = 0; k < n; ++k) {
      at = wrap(at + dir, n);
      const bool random = setup_.random_humans || setup_.random_computers;
      if (templates_[size_t(at)].Fits(random ? 0 : int(setup_.players.size()), setup_.random_size ? 0 : setup_.size))
        break;
    }
    setup_.template_name = templates_[size_t(at)].name == kClassicTemplate ? "" : templates_[size_t(at)].name;
    setup_.template_text = templates_[size_t(at)].text;
    ApplyTemplateTeams(templates_[size_t(at)]);
  } else if (id == kObelisks) {
    setup_.obelisks = !setup_.obelisks;
  } else if (id == kTreasures) {
    setup_.treasures = wrap(setup_.treasures + dir, 5);  // none, 1 to 4 colours
  } else if (id == kUnderground) {
    setup_.underground = !setup_.underground;
  } else if (id == kWater) {
    setup_.water = wrap(setup_.water + dir, 4);
  } else if (id == kMonsters) {
    setup_.monsters = wrap(setup_.monsters + dir, 4);
  } else if (id == kSeed) {
    setup_.seed = uint32_t(std::clamp<int64_t>(int64_t(setup_.seed) + dir, 0, 999999999));
    seed_typed_ = false;
    rolled_ = false;
  } else if (id >= kWho && id < kWho + 6) {
    card_player_ = id - kWho;
    auto& p = setup_.players[size_t(id - kWho)];
    int humans = 0;
    for (const PlayerSetup& q : setup_.players) humans += q.human;
    if (p.human && humans == 1) return;  // one human at least
    p.human = !p.human;
    FitRandomRows();
  } else if (id >= kFaction && id < kFaction + 6) {
    card_player_ = id - kFaction;
    auto& p = setup_.players[size_t(id - kFaction)];
    p.faction = wrap(p.faction + 1 + dir, kFactionCount + 1) - 1;  // -1 (random), 0..5
    p.hero = -1;
  } else if (id >= kTeam && id < kTeam + 6) {
    const size_t i = size_t(id - kTeam);
    card_player_ = int(i);
    const int n = int(setup_.players.size());
    for (int k = 0; k < n; ++k) {  // the next number that leaves a rival
      setup_.players[i].team = wrap(TeamOf(setup_.players, i) + dir, n);
      bool rivals = false;
      for (size_t j = 1; j < setup_.players.size(); ++j)
        rivals |= TeamOf(setup_.players, j) != TeamOf(setup_.players, 0);
      if (rivals) break;
    }
  } else if (id >= kHero && id < kHero + 6) {
    card_player_ = id - kHero;
    auto& p = setup_.players[size_t(id - kHero)];
    if (p.faction < 0 || !catalog_) return;
    const int n = int(catalog_->Heroes(p.faction).size());
    if (n) p.hero = wrap(p.hero + 1 + dir, n + 1) - 1;
  }
  dirty_ = true;
}

void Panel::SetPlayers(int want) {
  // computers added or taken off the end (a human kept)
  setup_.random_humans = setup_.random_computers = false;
  int humans = 0;
  for (const PlayerSetup& p : setup_.players) humans += p.human;
  while (int(setup_.players.size()) > want) {
    size_t at = setup_.players.size() - 1;  // the last computer, else the last human
    for (size_t i = setup_.players.size(); i-- > 0;)
      if (!setup_.players[i].human) { at = i; break; }
    if (setup_.players[at].human && humans == 1) break;
    humans -= setup_.players[at].human;
    setup_.players.erase(setup_.players.begin() + long(at));
  }
  PlayerSetup cpu;
  cpu.human = false;
  while (int(setup_.players.size()) < want) setup_.players.push_back(cpu);
  Step(kPlayers, 0);  // (the teams kept sound)
}

void Panel::ApplyTemplateTeams(const TemplateInfo& t) {
  const int players = int(setup_.players.size());
  for (const TemplateInfo::Variant& v : t.variants)
    if (players >= v.min_players && players <= v.max_players &&
        (setup_.random_size || (setup_.size >= v.min_size && setup_.size <= v.max_size))) {
      if (v.teams.empty()) return;  // (the first that fits is the one the generator uses)
      for (int i = 0; i < players; ++i) setup_.players[size_t(i)].team = i < int(v.teams.size()) ? v.teams[size_t(i)] : -1;
      return;
    }
}

void Panel::ChooseTemplate(const TemplateInfo& t) {
  setup_.template_name = t.name == kClassicTemplate ? "" : t.name;
  setup_.template_text = t.text;
  struct Teams {  // the template's teams set once the players and the size are settled, below
    Panel* p;
    const TemplateInfo& t;
    ~Teams() { p->ApplyTemplateTeams(t); }
  } teams{this, t};
  const bool random = setup_.random_humans || setup_.random_computers;
  const int size_now = setup_.random_size ? 0 : setup_.size;
  // A random number of players stays only if the template is for every number it can give
  // (2 to 6); one for some numbers (Three on three: 6) gets a fixed one of them.
  bool fits = true;
  if (random)
    for (int n = 2; n <= 6 && fits; ++n) fits = t.Fits(n, size_now);
  else
    fits = t.Fits(int(setup_.players.size()), size_now);
  if (fits) return;
  // Not for these players or this size: the nearest that it is for (its variant nearest them)
  const int players = int(setup_.players.size());
  const TemplateInfo::Variant* best = nullptr;
  int best_cost = 1 << 30;
  for (const TemplateInfo::Variant& v : t.variants) {
    const int dp = std::max({0, v.min_players - players, players - v.max_players});
    const int ds = setup_.random_size ? 0 : std::max({0, v.min_size - setup_.size, setup_.size - v.max_size});
    if (dp * 1000 + ds < best_cost) best_cost = dp * 1000 + ds, best = &v;
  }
  if (!best) return;
  if (random || players < best->min_players || players > best->max_players) {
    SetPlayers(std::clamp(players, best->min_players, best->max_players));
  }
  if (!setup_.random_size && (setup_.size < best->min_size || setup_.size > best->max_size)) {
    int pick = -1;  // the size in it nearest the one set
    for (int i = 0; i < 4; ++i)
      if (kMapSizes[i] >= best->min_size && kMapSizes[i] <= best->max_size &&
          (pick < 0 || std::abs(kMapSizes[i] - setup_.size) < std::abs(kMapSizes[pick] - setup_.size)))
        pick = i;
    if (pick >= 0) size_index_ = pick, setup_.size = kMapSizes[pick];
  }
}

Panel::Action Panel::Click(int x, int y, bool right) {
  // A click anywhere ends typing in the seed's field; on its number, starts it (what is typed
  // then replaces the number).
  if (seed_focus_) {
    seed_focus_ = false;
    dirty_ = true;
  }
  if (templates_open_) {  // the template's list: a click on one chooses it, anywhere else closes it
    templates_open_ = false;
    dirty_ = true;
    for (const Hit& h : hits_)
      if (h.id < 0 && x >= h.x0 && x < h.x1 && y >= h.y0 && y < h.y1) ChooseTemplate(templates_[size_t(-h.id - 1)]);
    return kNone;
  }
  for (const Hit& h : hits_) {
    if (x < h.x0 || x >= h.x1 || y < h.y0 || y >= h.y1) continue;
    if (h.id == kTemplate * 4 + 3) {
      if (mode_ != Mode::kEdit) return kNone;
      templates_open_ = true;
      dirty_ = true;
      return kNone;
    }
    if (h.id == kSeed * 4 + 3) {
      if (mode_ != Mode::kEdit) return kNone;
      seed_focus_ = true;
      seed_typed_ = false;
      dirty_ = true;
      return kNone;
    }
    const int id = h.id / 4, dir = h.id % 4 == 1 ? -1 : 1;
    if (id >= kChoice) {  // a row's option: kChoice + row * 8 + option
      if (mode_ != Mode::kEdit) return kNone;
      const int row = (id - kChoice) / 8, opt = (id - kChoice) % 8;
      switch (row) {
        case 0:
          setup_.random_size = opt == 4;
          if (opt < 4) size_index_ = opt, setup_.size = kMapSizes[opt];
          break;
        case 1:
          SetPlayers(opt + 2);
          break;
        case 9:
          setup_.random_humans = false;
          setup_.random_computers = true;
          break;
        case 3:
          setup_.random_underground = opt == 2;
          if (opt < 2) setup_.underground = opt == 1;
          break;
        case 4: setup_.water = opt; break;
        case 5: setup_.monsters = opt; break;
        case 6: setup_.obelisks = opt == 1; break;
        case 7: setup_.treasures = opt; break;
      }
      if (row == 1 || row == 9) FitRandomRows();
      dirty_ = true;
      return kNone;
    }
    if (id == kBioUp || id == kBioDown) {  // the biography's arrows (in any mode: only reading)
      Wheel(bio_x0_, bio_y0_, id == kBioUp ? 1 : -1);
      return kNone;
    }
    if (id == kCreateId) return mode_ == Mode::kEdit ? kCreate : kClose;
    if (id == kCloseId) return mode_ == Mode::kWorking ? kCancel : kClose;
    if (mode_ != Mode::kEdit) return kNone;
    if (id == kNewSeed) return kNewSeedAction;
    Step(id, right ? -dir : dir);
    return kNone;
  }
  return kNone;
}

bool Panel::Wheel(int x, int y, int dy) {
  if (x < bio_x0_ || x >= bio_x1_ || y < bio_y0_ || y >= bio_y1_ || !bio_max_ || !dy) return false;
  const int now = std::clamp(bio_scroll_ - dy, 0, bio_max_);
  if (now != bio_scroll_) bio_scroll_ = now, dirty_ = true;
  return true;
}

Panel::Action Panel::Key(int key) {
  if (seed_focus_ && (key == 27 || key == '\r')) {  // done typing
    seed_focus_ = false;
    dirty_ = true;
    return kNone;
  }
  if (templates_open_ && key == 27) {
    templates_open_ = false;
    dirty_ = true;
    return kNone;
  }
  if (key == 27) return mode_ == Mode::kWorking ? kCancel : kClose;
  if (key == '\r') return mode_ == Mode::kEdit ? kCreate : mode_ == Mode::kDone ? kClose : kNone;
  if (mode_ != Mode::kEdit || !editing_.empty()) return kNone;
  if (key >= '0' && key <= '9') {
    const uint64_t base = seed_typed_ ? setup_.seed : 0;
    const uint64_t v = base * 10 + uint64_t(key - '0');
    if (v <= 999999999) setup_.seed = uint32_t(v);
    seed_typed_ = true;
    rolled_ = false;
    dirty_ = true;
  } else if (key == '\b') {
    setup_.seed /= 10;
    seed_typed_ = true;
    rolled_ = false;
    dirty_ = true;
  }
  return kNone;
}

const std::vector<uint32_t>& Panel::Picture(int* w, int* h, bool* changed) {
  *w = kW;
  *h = kH;
  *changed = dirty_;
  if (dirty_) Redraw();
  dirty_ = false;
  return pic_;
}

void Panel::SetButtonArt(std::vector<const Font*> fonts, const Image* released, const Image* pressed) {
  button_fonts_ = std::move(fonts);
  button_up_ = released;
  button_down_ = pressed;
  button_.clear();
}

namespace {
// `art` stretched to w x h's width into `out` (its height kept) at x0: its ends as they are, its
// middle repeated (the frame's top and bottom and the parchment have no seams to speak of).
// (y0: its top in `out`; the art's transparent pixels leave what is there.)
void DrawButtonArt(std::vector<uint32_t>& out, int ow, int x0, int w, const Image& art, int y0 = 0) {
  const int cap = std::min(16 * art.h / 40, art.w / 3), mid = art.w - 2 * cap;
  for (int y = 0; y < art.h; ++y)
    for (int x = 0; x < w; ++x) {
      const int sx = x < cap ? x : x >= w - cap ? art.w - (w - x) : cap + (x - cap) % mid;
      const uint32_t p = art.argb[size_t(y) * size_t(art.w) + size_t(sx)];
      uint32_t& d = out[size_t(y0 + y) * size_t(ow) + size_t(x0 + x)];
      if (y0 == 0 || (p >> 24) >= 0x80) d = y0 == 0 ? p : p | 0xFF000000u;
    }
}

// `src` scaled by `k` (bilinear, the colours weighted by alpha so the edges keep theirs).
Image ScaleImage(const Image& src, float k) {
  Image out;
  out.w = std::max(1, int(float(src.w) * k + 0.5f));
  out.h = std::max(1, int(float(src.h) * k + 0.5f));
  out.argb.assign(size_t(out.w) * size_t(out.h), 0);
  for (int y = 0; y < out.h; ++y)
    for (int x = 0; x < out.w; ++x) {
      const float fx = std::clamp((float(x) + 0.5f) / k - 0.5f, 0.0f, float(src.w - 1));
      const float fy = std::clamp((float(y) + 0.5f) / k - 0.5f, 0.0f, float(src.h - 1));
      const int x0 = int(fx), y0 = int(fy), x1 = std::min(x0 + 1, src.w - 1), y1 = std::min(y0 + 1, src.h - 1);
      const float ax = fx - float(x0), ay = fy - float(y0);
      float acc[4] = {};
      const int xs[2] = {x0, x1}, ys[2] = {y0, y1};
      for (int j = 0; j < 2; ++j)
        for (int i = 0; i < 2; ++i) {
          const float wgt = (i ? ax : 1 - ax) * (j ? ay : 1 - ay);
          const uint32_t p = src.argb[size_t(ys[j]) * size_t(src.w) + size_t(xs[i])];
          const float a = float(p >> 24) * wgt;
          acc[0] += a;
          acc[1] += float((p >> 16) & 0xFF) * a;
          acc[2] += float((p >> 8) & 0xFF) * a;
          acc[3] += float(p & 0xFF) * a;
        }
      if (acc[0] <= 0) continue;
      auto ch = [](float v) { return uint32_t(std::clamp(v + 0.5f, 0.0f, 255.0f)); };
      out.argb[size_t(y) * size_t(out.w) + size_t(x)] = ch(acc[0]) << 24 | ch(acc[1] / acc[0]) << 16 |
                                                        ch(acc[2] / acc[0]) << 8 | ch(acc[3] / acc[0]);
    }
  return out;
}
}  // namespace

const std::vector<uint32_t>& Panel::ButtonPicture(int* w, int* h, int remove, int pressed, float scale,
                                                  float* density) {
  const bool art = !button_fonts_.empty() && button_fonts_[0] && button_up_ && button_down_ &&
                   button_up_->w > 0 && button_up_->h == button_down_->h && button_up_->w == button_down_->w;
  if (art) {
    // The biggest font the screen shows at no more than its own pixels (20 in the frame).
    size_t fi = 0;
    for (size_t i = 1; i < button_fonts_.size(); ++i)
      if (button_fonts_[i] && 20.0f + 2.0f * float(i) <= 20.0f * scale + 1.0f) fi = i;
    const Font& font = *button_fonts_[fi];
    const float d = (20.0f + 2.0f * float(fi)) / 20.0f;
    if (d != button_density_) {
      button_up_scaled_ = d == 1.0f ? *button_up_ : ScaleImage(*button_up_, d);
      button_down_scaled_ = d == 1.0f ? *button_down_ : ScaleImage(*button_down_, d);
      button_.clear();
    }
    if (button_remove_ != remove || button_pressed_ != pressed || button_density_ != d || button_.empty()) {
      // The game's text buttons: dark ink on the parchment, a red warning.
      static constexpr uint32_t kArtInk = 0xFF3C2410, kArtWarn = 0xFFA01808;
      std::vector<std::pair<std::string, uint32_t>> labels = {{Text("Random map"), kArtInk}};
      if (remove) {
        labels.push_back({Text("Edit random map"), kArtInk});
        labels.push_back(remove == 2 ? std::pair{Text("Remove? Click again"), kArtWarn}
                                     : std::pair{Text("Remove random map"), kArtInk});
      }
      static constexpr int kWhich[3] = {kOpenButton, kEditButton, kRemoveButton};
      // Widths in the frame's pixels (whole ones: clicks are tested there), drawn d times finer.
      auto width = [&](const std::string& s) {
        return std::max(button_up_->w, int(float(font.Width(s)) / d + 0.999f) + 44);
      };
      constexpr int gap = 6;
      int fw = 0;
      for (const auto& l : labels) fw += width(l.first) + (fw ? gap : 0);
      const int tw = int(float(fw) * d + 0.5f), th = button_up_scaled_.h;
      button_.assign(size_t(tw) * size_t(th), 0);  // transparent between the buttons
      button_spans_.clear();
      int x0 = 0;
      for (size_t i = 0; i < labels.size(); ++i) {
        const auto& [label, ink] = labels[i];
        const int bwi = width(label);
        const int tx0 = int(float(x0) * d + 0.5f), tx1 = std::min(tw, int(float(x0 + bwi) * d + 0.5f));
        const bool down = pressed == kWhich[i];
        DrawButtonArt(button_, tw, tx0, tx1 - tx0, down ? button_down_scaled_ : button_up_scaled_);
        const int shift = down ? int(d + 0.5f) : 0;  // the label sinks with the button
        font.Draw(button_, tw, th, tx0 + (tx1 - tx0 - font.Width(label)) / 2 + shift,
                  (th - font.Height()) / 2 + shift, label, ink, false);
        button_spans_.push_back({x0, x0 + bwi});
        x0 += bwi + gap;
      }
      button_remove_ = remove;
      button_pressed_ = pressed;
      button_density_ = d;
      button_tw_ = tw;
      button_th_ = th;
      button_w_ = fw;
      button_h_ = button_up_->h;
    }
    *w = button_tw_;
    *h = button_th_;
    if (density) *density = button_density_;
    return button_;
  }
  if (density) *density = 1.0f;
  if (!art && (button_remove_ != remove || button_.empty())) {
    // Random map, and for a random map selected: Edit, Remove (or asking to click it again).
    std::vector<std::pair<std::string, uint32_t>> labels = {{Text("Random map"), kGold}};
    if (remove) {
      labels.push_back({Text("Edit random map"), kGold});
      labels.push_back(remove == 2 ? std::pair{Text("Remove? Click again"), kWarn}
                                   : std::pair{Text("Remove random map"), kGold});
    }
    auto width = [&](const std::string& s) { return (font_ ? font_->Width(s) : 8 * int(s.size())) + 28; };
    constexpr int gap = 8;
    int bw = 0;
    for (const auto& l : labels) bw += width(l.first) + (bw ? gap : 0);
    const int bh = (font_ ? font_->Height() : 16) + 12;
    button_.assign(size_t(bw) * bh, 0);  // transparent between the buttons
    Canvas c{button_, bw, bh};
    button_spans_.clear();
    int x0 = 0;
    for (const auto& [label, ink] : labels) {
      const int x1 = x0 + width(label);
      c.Fill(x0, 0, x1, bh, kPlate);
      c.Frame(x0, 0, x1, bh, kEdge);
      c.Frame(x0 + 1, 1, x1 - 1, bh - 1, 0xFF000000);
      if (font_) font_->Draw(button_, bw, bh, x0 + 14, 6, label, ink);
      button_spans_.push_back({x0, x1});
      x0 = x1 + gap;
    }
    button_remove_ = remove;
    button_w_ = bw;
    button_h_ = bh;
  }
  *w = button_w_;
  *h = button_h_;
  return button_;
}

int Panel::ButtonAt(int x, int y) const {
  if (button_.empty() || y < 0 || y >= button_h_) return 0;
  static constexpr int kWhich[3] = {kOpenButton, kEditButton, kRemoveButton};
  for (size_t i = 0; i < button_spans_.size() && i < 3; ++i)
    if (x >= button_spans_[i].first && x < button_spans_[i].second) return kWhich[i];
  return 0;
}

void Panel::EditMap(const MapSetup& setup, const std::string& title) {
  setup_ = setup;
  for (int i = 0; i < 4; ++i)
    if (kMapSizes[i] == setup_.size) size_index_ = i;
  setup_.size = kMapSizes[size_index_];
  for (const TemplateInfo& t : templates_)
    if (t.name == setup_.template_name) setup_.template_text = t.text;
  FitRandomRows();
  editing_ = title;
  rolled_ = false;
  seed_typed_ = seed_focus_ = false;
  card_player_ = 0;
  mode_ = Mode::kEdit;
  message_.clear();
  dirty_ = true;
}

void Panel::NewMap(uint32_t seed) {
  if (editing_.empty()) return;
  editing_.clear();
  SetSeed(seed);
  mode_ = Mode::kEdit;
  message_.clear();
}

void Panel::Redraw() {
  pic_.assign(size_t(kW) * kH, kBack);
  if (back_ && back_->w > 0 && back_->h > 0)  // the parchment, mirrored at its edges (no seams)
    for (int y = 0; y < kH; ++y) {
      const int py = y / back_->h % 2 ? back_->h - 1 - y % back_->h : y % back_->h;
      for (int x = 0; x < kW; ++x) {
        const int px = x / back_->w % 2 ? back_->w - 1 - x % back_->w : x % back_->w;
        const uint32_t p = back_->argb[size_t(py) * size_t(back_->w) + size_t(px)];
        auto lift = [&](int s, uint32_t to) { return ((((p >> s) & 255u) * 9 + to * 11) / 20) << s; };  // paler: text reads
        pic_[size_t(y) * kW + size_t(x)] = 0xFF000000u | lift(16, 0xF4) | lift(8, 0xE8) | lift(0, 0xC8);
      }
    }
  hits_.clear();
  Canvas c{pic_, kW, kH};
  if (frame_ && frame_->w == 800 && frame_->h == 600) {
    // The game's frame, from its dialog: the corners as they are, the sides repeated from a plain
    // stretch of them; between the halves its middle bar (x 440 to 462, the shadows with it), its
    // ends where it meets the frame.
    auto sx = [&](int x) { return x < kBorder ? x : x >= kW - kBorder ? 800 - (kW - x) : 30 + (x - kBorder) % 380; };
    auto sy = [&](int y) { return y < kBorder ? y : y >= kH - kBorder ? 600 - (kH - y) : 100 + (y - kBorder) % 380; };
    for (int y = 0; y < kH; ++y)
      for (int x = 0; x < kW; ++x) {
        int fx;
        if (x >= kSetupW - 11 && x < kSetupW + 11) fx = 440 + x - (kSetupW - 11);
        else if (x < kBorder || x >= kW - kBorder || y < kBorder || y >= kH - kBorder) fx = sx(x);
        else continue;
        pic_[size_t(y) * kW + size_t(x)] = 0xFF000000u | frame_->argb[size_t(sy(y)) * 800 + size_t(fx)];
      }
  } else {  // a drawn one: gold between dark lines
    c.Frame(0, 0, kW, kH, 0xFF3A2208);
    for (int i = 1; i < 5; ++i) c.Frame(i, i, kW - i, kH - i, i == 2 || i == 3 ? 0xFFD8A840 : 0xFFA07020);
    c.Frame(5, 5, kW - 5, kH - 5, 0xFF3A2208);
    c.Fill(kSetupW, 10, kSetupW + 1, kH - 10, kDim);
  }
  if (!font_) return;
  const int fh = font_->Height();
  auto text = [&](int x, int y, const std::string& s, uint32_t ink) { font_->Draw(pic_, kW, kH, x, y, s, ink, false); };
  auto centred = [&](int x0, int x1, int y, const std::string& s, uint32_t ink) {
    text(x0 + (x1 - x0 - font_->Width(s)) / 2, y, s, ink);
  };
  const bool edit = mode_ == Mode::kEdit;
  auto button = [&](int x0, int y0, int x1, const std::string& s, int id, bool on) {
    const int y1 = y0 + fh + 8;
    c.Fill(x0, y0, x1, y1, on ? kPlate : kPlateDim);
    c.Frame(x0, y0, x1, y1, on ? kEdge : kDim);
    centred(x0, x1, y0 + 4, s, on ? kInk : kDim);
    if (on) hits_.push_back({x0, y0, x1, y1, id * 4 + 2});
  };
  // A list's value between ◀ and ▶ (drawn as < >): the arrows step it, a click on it steps on.
  // < value >: the arrows step it; a click on the value steps it on, or (field) starts typing in it.
  auto spinner = [&](int x0, int y0, int x1, const std::string& s, int id, bool on, bool field = false) {
    const int y1 = y0 + fh + 8, a = 22;
    const bool typing = field && seed_focus_ && on;
    c.Fill(x0, y0, x1, y1, on ? kPlate : kPlateDim);
    if (field && on) c.Fill(x0 + a + 1, y0 + 1, x1 - a - 1, y1 - 1, typing ? kFieldTyping : kField);
    c.Frame(x0, y0, x1, y1, on ? kEdge : kDim);
    c.Frame(x0 + a, y0, x1 - a, y1, typing ? kGold : on ? kEdge : kDim);
    const uint32_t ink = on ? kInk : kDim;
    centred(x0, x0 + a, y0 + 4, "<", ink);
    centred(x1 - a, x1, y0 + 4, ">", ink);
    std::string v = s;
    while (v.size() > 1 && font_->Width(v) > x1 - x0 - 2 * a - 8) v.pop_back();
    if (typing) {  // the number, then the caret; not typed in yet: marked, as what a key replaces
      const int tw = font_->Width(v), tx = x0 + a + (x1 - x0 - 2 * a - tw) / 2;
      if (!seed_typed_) c.Fill(tx - 2, y0 + 3, tx + tw + 2, y1 - 3, kMarked);
      text(tx, y0 + 4, v, kGold);
      c.Fill(tx + tw + 2, y0 + 4, tx + tw + 4, y1 - 4, kGold);
    } else {
      centred(x0 + a, x1 - a, y0 + 4, v, ink);
    }
    if (on) {
      hits_.push_back({x0, y0, x0 + a, y1, id * 4 + 1});
      hits_.push_back({x1 - a, y0, x1, y1, id * 4 + 2});
      hits_.push_back({x0 + a, y0, x1 - a, y1, id * 4 + (field ? 3 : 2)});
    }
  };

  // A row of options, side by side between x0 and x1: the chosen one lit.
  auto choices = [&](int x0, int x1, int y0, const std::vector<std::string>& words, int chosen, int row) {
    const int n = int(words.size()), y1 = y0 + fh + 8;
    for (int i = 0; i < n; ++i) {
      const int a = x0 + (x1 - x0) * i / n, b = x0 + (x1 - x0) * (i + 1) / n - 3;
      const bool lit = i == chosen;
      c.Fill(a, y0, b, y1, lit ? kLit : edit ? kPlate : kPlateDim);
      c.Frame(a, y0, b, y1, lit ? kGold : edit ? kEdge : kDim);
      std::string v = words[size_t(i)];
      while (v.size() > 1 && font_->Width(v) > b - a - 6) v.pop_back();
      centred(a, b, y0 + 4, v, lit ? kGold : edit ? kInk : kDim);
      if (edit) hits_.push_back({a, y0, b, y1, (kChoice + row * 8 + i) * 4});
    }
  };
  auto words = [&](std::initializer_list<const char*> list) {
    std::vector<std::string> out;
    for (const char* w : list) out.push_back(Text(w));
    return out;
  };

  // the title in a bigger font of the game's (24, as its dialogs' titles)
  const Font* title_font = button_fonts_.size() > 2 && button_fonts_[2] ? button_fonts_[2] : font_;
  {
    std::string s = editing_.empty() ? Text("Random map") : Text("Edit") + " " + editing_;
    while (s.size() > 1 && title_font->Width(s) > kSetupW - 60) s.pop_back();
    title_font->Draw(pic_, kW, kH, (kSetupW - title_font->Width(s)) / 2, kBorder + 6, s, kGold, false);
  }
  const int row_h = fh + 16;  // a row of options and the gap under it
  int y = kBorder + 6 + title_font->Height() + 6;
  const int lx = 24, vx = 150, vw = 260, vr = kSetupW - 20;
  // The template: a drop-down (its list drawn last, over the rest)
  const int tbox_y = y;
  const bool any_templates = edit && templates_.size() > 1;
  {
    const std::string name = setup_.template_name.empty() ? std::string(kClassicTemplate) : setup_.template_name;
    text(lx, y + 4, Text("Template"), kInk);
    const int y1 = y + fh + 8, a = 22;
    const bool lit = templates_open_;
    c.Fill(vx, y, vr, y1, lit ? kLit : any_templates ? kPlate : kPlateDim);
    c.Frame(vx, y, vr, y1, lit ? kGold : any_templates ? kEdge : kDim);
    c.Frame(vr - a, y, vr, y1, lit ? kGold : any_templates ? kEdge : kDim);
    const uint32_t ink = lit ? kGold : any_templates ? kInk : kDim;
    std::string v = Text(name.c_str());
    while (v.size() > 1 && font_->Width(v) > vr - vx - a - 16) v.pop_back();
    text(vx + 10, y + 4, v, ink);
    // the arrow: a small triangle pointing down
    for (int k = 0; k < 5; ++k) c.Fill(vr - a / 2 - 5 + k, y + 8 + k, vr - a / 2 + 6 - k, y + 9 + k, ink);
    if (any_templates) hits_.push_back({vx, y, vr, y1, kTemplate * 4 + 3});
  }
  y += row_h;
  text(lx, y + 4, Text("Map size"), kInk);
  choices(vx, vr, y, words({kSizeNames[0], kSizeNames[1], kSizeNames[2], kSizeNames[3], "Random"}),
          setup_.random_size ? 4 : size_index_, 0);
  y += row_h;
  int humans = 0;
  for (const PlayerSetup& p : setup_.players) humans += p.human;
  const int mid = 350, right = 450;  // a row of two: the second's label, options
  const int rx = 446;  // a count's "Random"
  // The players' number (who is human: each row's button); random: the computers' number drawn
  const bool random_players = setup_.random_humans || setup_.random_computers;
  text(lx, y + 4, Text("Players"), kInk);
  choices(vx, rx - 4, y, {"2", "3", "4", "5", "6"}, random_players ? -1 : int(setup_.players.size()) - 2, 1);
  choices(rx, vr, y, words({"Random"}), random_players ? 0 : -1, 9);
  (void)humans;
  y += row_h;
  text(lx, y + 4, Text("Underground"), kInk);
  choices(vx, vr, y, words({"No", "Yes", "Random"}), setup_.random_underground ? 2 : setup_.underground, 3);
  y += row_h;
  text(lx, y + 4, Text("Water"), kInk);
  choices(vx, vr, y, words({kWaterWords[0], kWaterWords[1], kWaterWords[2], kWaterWords[3]}), setup_.water, 4);
  y += row_h;
  text(lx, y + 4, Text("Monsters"), kInk);
  choices(vx, vr, y, words({kMonsterWords[0], kMonsterWords[1], kMonsterWords[2], kMonsterWords[3]}), setup_.monsters, 5);
  y += row_h;
  text(lx, y + 4, Text("Obelisks"), kInk);
  choices(vx, vx + 2 * (vr - vx) / 5, y, words({"No", "Yes"}), setup_.obelisks, 6);  // (as the sizes' cells)
  text(mid, y + 4, Text("Treasures"), kInk);
  std::vector<std::string> counts = words({"No"});
  for (const char* d : {"1", "2", "3", "4"}) counts.push_back(d);
  choices(right, vr, y, counts, setup_.treasures, 7);
  y += row_h;
  text(lx, y + 4, Text("Seed"), kInk);
  // An edited map keeps its seed (its file and title are named by it).
  spinner(vx, y, vx + vw, std::to_string(setup_.seed), kSeed, edit && editing_.empty(), true);
  button(vx + vw + 10, y, vr - 3, Text("New"), kNewSeed, edit && editing_.empty());  // (to the rows' right edge)
  y += fh + 18;

  const int cw0 = 44, cw1 = 164, cf0 = 172, cf1 = 322, ct0 = 330, ct1 = 410, ch0 = 418, ch1 = kSetupW - 20;
  text(cw0, y, Text("Player"), kGold);
  text(cf0 + 22, y, Text("Faction"), kGold);
  text(ct0 + 22, y, Text("Team"), kGold);
  text(ch0 + 22, y, Text("Starting hero"), kGold);
  y += fh + 5;
  // A random number of humans or computers: the rows past the least it gives are dim, with "?"
  // (in the game if the seed draws that many: their faction, team and hero then as set here);
  // both random: one of each, and the rest of the six "random" rows.
  int min_h, max_h, min_k, max_k;
  Counts(&min_h, &max_h, &min_k, &max_k);
  const bool both = setup_.random_humans && setup_.random_computers;
  int seen_h = 0, seen_k = 0;
  bool maybe_rows = false;
  for (int i = 0; i < 6; ++i) {
    const bool used = i < int(setup_.players.size());
    if (used) {
      const PlayerSetup& p = setup_.players[size_t(i)];
      const bool maybe = p.human ? seen_h++ >= min_h : seen_k++ >= min_k;
      maybe_rows |= maybe;
      // its colour known: its place in the list is (the computers' follow a random number of humans)
      const bool placed = !maybe && (p.human || !setup_.random_humans);
      {  // a square in the row's middle
        const int side = 16, sy = y + (fh + 8 - side) / 2;
        c.Fill(22, sy, 22 + side, sy + side, placed ? kPlayerColour[i] : kPlateDim);
        c.Frame(22, sy, 22 + side, sy + side, placed ? kEdge : kDim);
      }
      // (who is human: by the counts above, while either is random)
      const bool counted = setup_.random_humans;  // (old maps': the humans' number drawn)
      button(cw0, y, cw1, Text(p.human ? (maybe ? "Human?" : "Human") : (maybe ? "Computer?" : "Computer")), kWho + i,
             edit && !counted && !(setup_.random_computers && humans >= 5 && !p.human));
      spinner(cf0, y, cf1, Text(p.faction < 0 ? "Random" : kFactionWords[p.faction]), kFaction + i, edit);
      spinner(ct0, y, ct1, std::to_string(TeamOf(setup_.players, size_t(i)) + 1), kTeam + i, edit);
      std::string hero = Text("Random");
      const bool heroes = catalog_ && p.faction >= 0 && !catalog_->Heroes(p.faction).empty();
      if (heroes && p.hero >= 0 && size_t(p.hero) < catalog_->Heroes(p.faction).size())
        hero = catalog_->Heroes(p.faction)[size_t(p.hero)].name;
      spinner(ch0, y, ch1, hero, kHero + i, edit && heroes);
    } else if (both) {  // a player the seed may add: human or computer, all random
      c.Fill(20, y + 3, 36, y + fh + 5, kPlateDim);
      c.Frame(20, y + 3, 36, y + fh + 5, kDim);
      button(cw0, y, cw1, Text("Maybe"), kWho + i, false);
      button(cf0, y, cf1, Text("Random"), kFaction + i, false);
      button(ct0, y, ct1, "-", kTeam + i, false);
      button(ch0, y, ch1, Text("Random"), kHero + i, false);
      maybe_rows = true;
    }
    y += fh + 11;
  }
  // The message line (two at most), then the buttons.
  y += 2;
  std::string line = message_.empty() && maybe_rows
                         ? Text("Rows with \"?\": in the game if the seed gives that many players.")
                         : message_,
              rest;
  while (font_->Width(line) > kSetupW - 40) {
    const size_t cut = line.rfind(' ');
    if (cut == std::string::npos) break;
    rest = line.substr(cut) + rest;
    line.resize(cut);
  }
  const uint32_t ink = message_error_ && !message_.empty() ? kWarn : kInk;
  centred(0, kSetupW, y, line, ink);
  if (!rest.empty()) centred(0, kSetupW, y + fh + 2, rest.substr(1), ink);
  // The panel's own buttons: the game's big text button (as the scenario dialog's Begin and
  // Back), plain plates without its art.
  const bool art = !button_fonts_.empty() && button_fonts_[0] && button_up_ && button_up_->w > 0;
  const int by = art ? kH - kBorder - button_up_->h - 12 : kH - kBorder - fh - 22;
  auto big = [&](int x0, int x1, const std::string& s, int id) {
    if (!art) return button(x0, by, x1, s, id, true);
    DrawButtonArt(pic_, kW, x0, x1 - x0, *button_up_, by);
    const Font& f = *button_fonts_[0];
    f.Draw(pic_, kW, kH, x0 + (x1 - x0 - f.Width(s)) / 2, by + (button_up_->h - f.Height()) / 2, s, 0xFF3C2410,
           false);
    hits_.push_back({x0, by, x1, by + button_up_->h, id * 4 + 2});
  };
  if (mode_ == Mode::kEdit) {
    big(kSetupW / 2 - 200, kSetupW / 2 - 8, editing_.empty() ? Text("Create map") : Text("Update map"), kCreateId);
    big(kSetupW / 2 + 8, kSetupW / 2 + 200, Text("Close"), kCloseId);
  } else if (mode_ == Mode::kWorking) {
    big(kSetupW / 2 - 96, kSetupW / 2 + 96, Text("Cancel"), kCloseId);
  } else {
    big(kSetupW / 2 - 96, kSetupW / 2 + 96, Text("OK"), kCreateId);
  }
  DrawCard(pic_, kSetupW + 11, kW - kBorder);

  // The template's list, open: under its box, in columns; the ones without a variant for these
  // players and this size dim (chosen, it sets them). Only its entries take clicks then (one elsewhere closes it).
  if (templates_open_ && !templates_.empty()) {
    hits_.clear();
    const int n = int(templates_.size()), cols = n > 24 ? 3 : n > 10 ? 2 : 1;
    const int rows = (n + cols - 1) / cols, ih = fh + 6;
    const int x0 = 14, x1 = kSetupW - 14, y0 = tbox_y + fh + 9;
    const int y1 = std::min(kH - kBorder - 4, y0 + rows * ih + 10);
    c.Fill(x0, y0, x1, y1, kList);
    c.Frame(x0, y0, x1, y1, kEdge);
    const std::string now = setup_.template_name.empty() ? std::string(kClassicTemplate) : setup_.template_name;
    const bool random = setup_.random_humans || setup_.random_computers;
    const int cw = (x1 - x0 - 10) / cols;
    for (int i = 0; i < n; ++i) {
      const TemplateInfo& t = templates_[size_t(i)];
      const int ix = x0 + 5 + (i / rows) * cw, iy = y0 + 5 + (i % rows) * ih;
      if (iy + ih > y1 - 4) continue;
      const bool fits = t.Fits(random ? 0 : int(setup_.players.size()), setup_.random_size ? 0 : setup_.size);
      const bool chosen = t.name == now;
      if (chosen) c.Fill(ix, iy, ix + cw - 4, iy + ih - 1, kLit), c.Frame(ix, iy, ix + cw - 4, iy + ih - 1, kGold);
      std::string v = Text(t.name.c_str());
      while (v.size() > 1 && font_->Width(v) > cw - 16) v.pop_back();
      text(ix + 8, iy + 3, v, chosen ? kGold : fits ? kInk : kDim);
      hits_.push_back({ix, iy, ix + cw - 4, iy + ih, -(i + 1)});  // (dim: chosen, the players and size set for it)
    }
  }
}

// The hero card: whose hero the setup's last changed row has (after homm4-random-map-generator's
// setup window): portrait, name, class, faction, morale and luck, skills, army and biography.
void Panel::DrawCard(std::vector<uint32_t>& px, int x0, int x1) {
  Canvas c{px, kW, kH};
  const int fh = font_->Height();
  auto text = [&](int x, int y, const std::string& s, uint32_t ink) { font_->Draw(px, kW, kH, x, y, s, ink, false); };
  auto fit = [&](std::string s, int width) {
    while (s.size() > 1 && font_->Width(s) > width) s.pop_back();
    return s;
  };
  // Lines of at most `width` (cut at spaces), `max` of them at most.
  auto wrap = [&](const std::string& s, int width, size_t max, const Font* f = nullptr) {
    if (!f) f = font_;
    std::vector<std::string> lines;
    std::string line;
    size_t i = 0;
    while (i < s.size() && lines.size() < max) {
      size_t j = s.find(' ', i);
      if (j == std::string::npos) j = s.size();
      const std::string word = s.substr(i, j - i);
      const std::string next = line.empty() ? word : line + " " + word;
      if (!line.empty() && f->Width(next) > width) {
        lines.push_back(line);
        line = word;
      } else {
        line = next;
      }
      i = j + 1;
    }
    if (!line.empty() && lines.size() < max) lines.push_back(line);
    return lines;
  };
  // A section's title in a bigger font of the game's (20), a line after it to the right edge.
  const Font* hfont = button_fonts_.size() > 1 && button_fonts_[1] ? button_fonts_[1] : font_;  // (22)
  const Font* bfont = !button_fonts_.empty() && button_fonts_[0] ? button_fonts_[0] : font_;     // the biography's (20)
  const int bh = bfont->Height();
  const int hh = hfont->Height();
  // in the middle of its section (x to the card's right edge), a line each side
  auto heading_at = [&](int x, int y, const char* s) {
    const std::string t = Text(s);
    const int r = x1 - 14, tw = hfont->Width(t), tx = x + (r - x - tw) / 2;
    hfont->Draw(px, kW, kH, tx, y, t, kGold, false);
    c.Fill(x, y + hh / 2, tx - 8, y + hh / 2 + 1, kDim);
    c.Fill(tx + tw + 8, y + hh / 2, r, y + hh / 2 + 1, kDim);
  };
  auto heading = [&](int y, const char* s) { heading_at(x0 + 14, y, s); };

  if (setup_.players.empty()) return;
  card_player_ = std::clamp(card_player_, 0, int(setup_.players.size()) - 1);
  const PlayerSetup& p = setup_.players[size_t(card_player_)];
  const HeroChoice* hero = nullptr;
  if (catalog_ && p.faction >= 0 && p.hero >= 0 && size_t(p.hero) < catalog_->Heroes(p.faction).size())
    hero = &catalog_->Heroes(p.faction)[size_t(p.hero)];
  const int faction = p.faction;

  // The header: the portrait in the player's colour, its name and who under it; the skills right of it.
  const int ps = 82, px0 = x0 + 14 + (kCardHead - 8 - ps) / 2 - 3, py0 = kBorder + 10;
  {  // round, as the game's: the picture in a ring of the player's colour
    const float cx = px0 + 3 + ps / 2.0f, cy = py0 + 3 + ps / 2.0f, r = ps / 2.0f;
    const Image* portrait = hero && art_ ? art_->Portrait(faction, hero->keyword, hero->name) : nullptr;
    const uint32_t portrait_back = portrait ? 0xFF1A120A : kPlateDim;  // none: a big dark "?" on parchment
    c.Ring(cx, cy, 0, r, portrait_back);
    if (portrait)
      c.Blit(*portrait, px0 + 3 + (ps - portrait->w) / 2, py0 + 3 + (ps - portrait->h) / 2, cx, cy, r);
    else
    {
      const Font* big = !button_fonts_.empty() && button_fonts_.back() ? button_fonts_.back() : font_;
      big->Draw(px, kW, kH, px0 + 3 + (ps - big->Width("?")) / 2, py0 + 3 + (ps - big->Height()) / 2, "?", kInk,
                false);
    }
    c.Ring(cx, cy, r - 1, r + 3, kPlayerColour[card_player_]);
  }
  // Under the portrait: the name, then "faction, class, sex" (the ring's colour says whose).
  const int col = px0 + 3 + ps / 2, col_w = kCardHead - 8;
  auto under = [&](int y, const std::string& s, uint32_t ink) {
    const std::string v = fit(s, col_w);
    text(col - font_->Width(v) / 2, y, v, ink);
  };
  under(py0 + ps + 12, hero ? hero->name : Text("Random hero"), kGold);
  std::string who = faction >= 0 ? Text(kFactionWords[faction]) : Text("Random faction");
  if (hero && hero->sex <= 1) who += ", " + Text(hero->sex ? "Male" : "Female");
  under(py0 + ps + 14 + fh, who, kInk);
  under(py0 + ps + 16 + 2 * fh,
        Text("Level 1") + ", " + (hero ? Text(HeroArt::ClassTitle(hero->hero_class)) : std::string("?????")),
        kInk);

  // Skills: the class's, with the game's icons and names, right of the portrait.
  {
    const int sx0 = x0 + 14 + kCardHead, sy = py0 + 4;
    heading_at(sx0, sy, "Skills");
    const std::vector<std::string> skills = hero ? HeroArt::ClassSkills(hero->hero_class) : std::vector<std::string>();
    // Each skill its icon over its name; the first one's left edge at the section's left, the
    // last one's right edge at its right, the space shared between them (one: in the middle);
    // up and down in the middle of the room under the title.
    const int sr = x1 - 14, top = sy + hh + 4, bottom = py0 + ps + 20 + 3 * fh, item_h = 54 + fh;
    const int iy = top + std::max(0, (bottom - top - item_h) / 2);
    if (skills.empty()) {
      int ly = top + 6;
      for (const std::string& line :
           wrap(Text("Skills come with the hero's class: choose a hero to see them."), x1 - 14 - sx0, 3)) {
        text(sx0, ly, line, kDim);
        ly += fh;
      }
    }
    const size_t n = std::min<size_t>(skills.size(), 3);
    std::vector<std::string> names;
    std::vector<const Image*> icons;
    std::vector<int> widths;
    int total = 0;
    for (size_t k = 0; k < n; ++k) {
      icons.push_back(art_ ? art_->SkillIcon(skills[k]) : nullptr);
      names.push_back(fit(art_ ? art_->SkillName(skills[k]) : skills[k], (sr - sx0) / int(n) - 6));
      widths.push_back(std::max(icons[k] ? icons[k]->w : 0, font_->Width(names[k])));
      total += widths[k];
    }
    const int gap = n > 1 ? (sr - sx0 - total) / int(n - 1) : 0;
    int ix = n == 1 ? sx0 + (sr - sx0 - total) / 2 : sx0;
    for (size_t k = 0; k < n; ++k) {
      if (n > 1 && k + 1 == n) ix = sr - widths[k];  // (exactly at the right edge)
      if (icons[k]) c.Blit(*icons[k], ix + (widths[k] - icons[k]->w) / 2, iy);
      text(ix + (widths[k] - font_->Width(names[k])) / 2, iy + 54, names[k], kInk);
      ix += widths[k] + gap;
    }
  }
  int y = py0 + ps + 20 + 3 * fh + 8;

  // Statistics: a level 1 hero's, as the game works them out (HeroArt::Stats).
  heading(y, "Statistics");
  y += hh + 6;
  {
    const std::vector<int> st = hero ? HeroArt::Stats(hero->hero_class) : std::vector<int>(HeroArt::kStatCount, -1);
    const int cell = (x1 - x0 - 28) / 3, row = 40;  // (the icons 30 high: a gap between the rows)
    for (int k = 0; k < HeroArt::kStatCount; ++k) {
      const int cx = x0 + 14 + (k % 3) * cell, cy = y + (k / 3) * row;
      if (const Image* icon = art_ ? art_->StatIcon(HeroArt::kStats[k]) : nullptr) c.Blit(*icon, cx, cy);
      text(cx + 38, cy + 5, fit(Text(HeroArt::kStatTitles[k]), cell - 38 - 40), kInk);
      std::string v = st[size_t(k)] < 0 ? std::string("-") : std::to_string(st[size_t(k)]);
      if (k == 6 && st[size_t(k)] >= 0) v += "-" + std::to_string(HeroArt::kDamageHigh);  // damage: low-high
      text(cx + cell - 14 - font_->Width(v), cy + 5, v, st[size_t(k)] < 0 ? kDim : kGold);
    }
    y += 3 * row + 4;
  }

  // The starting army: what the game gives (a week's growth of the town's level 1 creatures).
  heading(y, "Starting army");
  y += hh + 6;
  const auto army = HeroArt::Army(faction);
  if (army.empty()) {
    for (const std::string& line :
         wrap(Text("The game gives the hero a week's growth of its town's level 1 creatures."), x1 - x0 - 28, 2)) {
      text(x0 + 14, y, line, kDim);
      y += fh;
    }
    y += 4 * fh + 6 - 2 * fh;
  } else {
    const int half = (x1 - x0 - 28) / 2;
    for (size_t k = 0; k < army.size() && k < 2; ++k) {
      const std::string& keyword = army[k];
      const HeroArt::Creature* cr = art_ ? art_->FindCreature(keyword) : nullptr;
      const int count = cr ? cr->growth : 0;
      const int ix = x0 + 14 + int(k) * half;
      {  // round, as the portrait
        const float cx = ix + 27, cy = y + 27.0f;
        if (const Image* icon = art_ ? art_->CreatureIcon(keyword) : nullptr)
          c.Blit(*icon, ix + 27 - icon->w / 2, y + 27 - icon->h / 2, cx, cy, 26);
        c.Ring(cx, cy, 26, 28, kEdge);
      }
      const int lx = ix + 64, lw = half - 70;
      std::string plural = cr && !cr->plural.empty() ? cr->plural : keyword;
      for (size_t i = 0; i < plural.size(); ++i)  // Each Word Capitalised, as the game's army lists
        if (i == 0 || plural[i - 1] == ' ') plural[i] = char(std::toupper(static_cast<unsigned char>(plural[i])));
      text(lx, y, fit(std::to_string(count) + " " + plural, lw), kGold);
      if (cr) {
        const std::string n = " ";
        text(lx, y + fh + 2,
             fit(Text("Level") + n + std::to_string(cr->level) + ", " + Text("HP") + n + std::to_string(cr->hit_points), lw),
             kDim);
        text(lx, y + 2 * fh + 4,
             fit(Text("Attack") + n + std::to_string(cr->attack) + ", " + Text("Defense") + n +
                     std::to_string(cr->defense), lw),
             kDim);
        text(lx, y + 3 * fh + 6,
             fit(Text("Damage") + n + std::to_string(cr->low) + "-" + std::to_string(cr->high) + ", " +
                     Text("Speed") + n + std::to_string(cr->speed) + ", " + Text("Move") + n +
                     std::to_string(cr->move), lw),
             kDim);
      }
    }
    y += 4 * fh + 6;
  }

  // The biography: what fits, the rest by the wheel or the arrows right of it.
  y += 4;
  heading(y, "Biography");
  y += hh + 4;
  const std::string bio = hero ? (hero->biography.empty() ? Text("No biography.") : hero->biography)
                               : Text("The game picks a hero when the map starts.");
  std::string flat = bio;
  for (char& ch : flat)
    if (static_cast<unsigned char>(ch) < 0x20 || static_cast<unsigned char>(ch) == 0xA0) ch = ' ';  // breaks, no-break spaces
  const int visible = std::max(1, (kH - kBorder - 12 - y) / bh), bar = 14;
  std::vector<std::string> lines = wrap(flat, x1 - x0 - 28, size_t(-1), bfont);
  if (int(lines.size()) > visible) lines = wrap(flat, x1 - x0 - 28 - bar - 6, size_t(-1), bfont);  // room for the bar
  if (bio != bio_text_) bio_text_ = bio, bio_scroll_ = 0;
  bio_max_ = std::max(0, int(lines.size()) - visible);
  bio_scroll_ = std::clamp(bio_scroll_, 0, bio_max_);
  bio_x0_ = x0, bio_y0_ = y, bio_x1_ = x1, bio_y1_ = y + visible * bh;
  for (int i = 0; i < visible && bio_scroll_ + i < int(lines.size()); ++i)
    bfont->Draw(px, kW, kH, x0 + 14, y + i * bh, lines[size_t(bio_scroll_ + i)], kInk, false);
  if (bio_max_) {  // the bar: ▲ at the top, ▼ at the bottom, the part shown between
    const int bx0 = x1 - 14 - bar, bx1 = x1 - 14, by0 = y, by1 = y + visible * bh;
    c.Fill(bx0, by0, bx1, by1, kPlateDim);
    c.Frame(bx0, by0, bx1, by1, kEdge);
    auto arrow = [&](int top, bool up) {  // a triangle in a bar-wide square
      for (int i = 0; i < 6; ++i) {
        const int row = up ? top + 4 + i : top + bar - 5 - i, half = i;
        c.Fill(bx0 + bar / 2 - half - 1, row, bx0 + bar / 2 + half + 1, row + 1, kInk);
      }
    };
    arrow(by0, true);
    arrow(by1 - bar, false);
    const int track0 = by0 + bar, track1 = by1 - bar, track = std::max(1, track1 - track0);
    const int t0 = track0 + track * bio_scroll_ / int(lines.size()),
              t1 = track0 + track * std::min(int(lines.size()), bio_scroll_ + visible) / int(lines.size());
    c.Fill(bx0 + 3, t0, bx1 - 3, std::max(t0 + 4, t1), kLit);
    hits_.push_back({bx0, by0, bx1, by0 + (by1 - by0) / 2, kBioUp * 4});
    hits_.push_back({bx0, by0 + (by1 - by0) / 2, bx1, by1, kBioDown * 4});
  }
}

}  // namespace h4::random_map
