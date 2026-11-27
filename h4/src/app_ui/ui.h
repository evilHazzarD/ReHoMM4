// The app's own panels in the game's dialogs' look (parchment in the game's frame, its fonts, its
// big text buttons: random_map/art.h), drawn into an ARGB picture the app shows over the frame
// (gfx::SetPresentBadge) where it has no toolkit of its own: the Linux, Windows, Switch and
// browser apps' settings (menu_button_drawn.cpp). One panel for every such app, so a setting is
// added once.
//
// A panel is built by a function that lays it out top to bottom with Ui's parts (a title, rows
// with a dropdown, a checkbox or a button, text, the game's buttons...), each given what a click
// on it does. It is run to measure the panel, then to draw it; the clicks then go to the parts.
#pragma once

#include <cstdint>
#include <functional>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "text.h"

namespace h4::app_ui {

struct Picture {
  int w = 0, h = 0;
  std::vector<uint32_t> argb;
};

class Ui;
using Build = std::function<void(Ui&)>;

class Ui {
 public:
  // The panel's own words in the interface's language (ui_text.h).
  const char* T(const char* english) const;
  const std::string& lang() const { return lang_; }
  int width() const { return w_; }
  int left() const { return kBorder + kPad; }
  int right() const { return w_ - kBorder - kPad; }
  int y = 0;  // where the next part goes

  // A title, centred (the game's dialogs' big font).
  void Title(const std::string& text);
  // Text, wrapped to the panel's width (left-aligned; `centred`: each line centred).
  void Paragraph(const std::string& text, bool centred = false, uint32_t ink = 0);
  // A row: `label` at the left, a dropdown of `names` at the right (`selected` shown); `id` tells
  // the dropdowns apart (which one is open). `pick` with a new choice's index.
  void Dropdown(const std::string& id, const std::string& label, const std::vector<std::string>& names, int selected,
                std::function<void(int)> pick);
  // A row: `label` at the left, `value` at the right (nothing to choose).
  void Value(const std::string& label, const std::string& value);
  // A checkbox row; `help`: a small square "?" button right after its text.
  void Check(const std::string& label, bool on, std::function<void()> toggle, std::function<void()> help = {});
  // A row: `label` at the left, a small button at the right.
  void LabelButton(const std::string& label, const std::string& button, std::function<void()> click);
  // A row: `label` (dim) at the left, a small button at the right ("ReHoMM4 v0.4", "Check for updates").
  void InfoButton(const std::string& info, const std::string& button, std::function<void()> click);
  // The same with buttons side by side at the right (no `info`: centred); `marked`: that one's
  // text in another colour on a lit plate ("Update available").
  void InfoButtons(const std::string& info, const std::vector<std::pair<std::string, std::function<void()>>>& buttons,
                   int marked = -1);
  // The game's big text buttons side by side, centred.
  void Buttons(const std::vector<std::pair<std::string, std::function<void()>>>& buttons);
  // A row: `label`, a slider from lo to hi at `value` (its % right of it); `set` as it is dragged.
  void Slider(const std::string& label, float value, float lo, float hi, std::function<void(float)> set);
  // A click anywhere on the panel that no part takes (a message's "click to close").
  void Anywhere(std::function<void()> click);
  // ✕ in the top right corner.
  void Close(std::function<void()> close);
  void Gap(int pixels) { y += pixels; }

  // Parts of one's own: a box at (x0, y0)-(x1, y1) with `text`, a ▾ at its right when `arrow`;
  // `click` with the button (right: the right one).
  void Box(int x0, int y0, int x1, int y1, const std::string& text, bool arrow, std::function<void(bool right)> click);
  void Text(int x, int y, const std::string& text, uint32_t ink, int size = 16) const;
  int TextWidth(const std::string& text, int size = 16) const;
  int LineHeight(int size = 16) const;
  bool drawing() const { return px_ != nullptr; }

  static constexpr int kBorder = 18, kPad = 14;

 private:
  friend class Panel;
  struct Hit {
    int x0, y0, x1, y1;
    std::function<void(int x, bool right)> fn;
    bool drag = false;  // a slider: its moves too
    bool popup = false;
  };
  Ui(int w, int h, std::vector<uint32_t>* px, const std::string& lang, std::string* open);
  const Writer& W(int size) const;
  void Fill(int x0, int y0, int x1, int y1, uint32_t c) const;
  void Frame(int x0, int y0, int x1, int y1, uint32_t c) const;
  void Plate(int x0, int y0, int x1, int y1, bool lit) const;
  void Arrow(int cx, int cy, uint32_t c) const;  // ▾
  int w_, h_;
  std::vector<uint32_t>* px_;
  std::string lang_;
  std::string* open_;  // the open dropdown's id ("" none)
  std::vector<Hit> hits_;
  std::vector<std::function<void()>> popups_;  // drawn last, over the rest
  int bottom_ = 0;                             // the lowest any part reaches (the open list too)
  mutable std::vector<std::pair<int, std::unique_ptr<Writer>>> writers_;
};

// A panel: its picture, drawn again when it changed, and its clicks.
class Panel {
 public:
  Panel() = default;
  Panel(int width, const std::string& lang, Build build) { Set(width, lang, std::move(build)); }
  void Set(int width, const std::string& lang, Build build);
  // Drawn again at the next Draw (the settings changed under it).
  void Invalidate() { dirty_ = true; }
  const Picture& Draw();
  bool changed() const { return dirty_; }
  uint64_t version() const { return version_; }  // one more at each drawing
  // Its height without an open list (which may make the picture taller): where it is placed by,
  // so it stays put while a list opens.
  int base_height() const { return base_h_; }
  // A click at (x, y) of the picture (right: the right button): what is there done. False when it
  // was beside the panel (or nothing is there and no list was open).
  bool Click(int x, int y, bool right);
  // The left button down at (x, y): a slider there is held (Drag moves it) until Release.
  void Press(int x, int y);
  void Drag(int x);
  void Release() { held_ = -1; }
  bool holding() const { return held_ >= 0; }
  // Esc: an open list closed (true), else nothing to close here (false).
  bool Back();

 private:
  int w_ = 0;
  std::string lang_;
  Build build_;
  std::string open_;
  Picture pic_;
  std::vector<Ui::Hit> hits_;
  int held_ = -1;
  bool dirty_ = true;
  uint64_t version_ = 1;
  int base_h_ = 0;
};

// The buttons over the main menu (☁️ the save sync, ☕ the author's Ko-fi page, ⚙ the settings):
// each glyph on a plate in the game's frame, `side` square, `gap` pixels apart, left to right.
// `tip` (not empty): a tooltip under the icon `under`, its right edge at that icon's (the picture
// then wider to the left when it is longer than the row: the row at `*row_x`).
Picture IconRow(const std::vector<std::string>& glyphs, int side = 40, int gap = 8, const std::string& tip = "",
                int under = -1, const std::string& lang = "", int* row_x = nullptr);

}  // namespace h4::app_ui
