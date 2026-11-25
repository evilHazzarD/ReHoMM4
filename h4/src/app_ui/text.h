// How the app's own panels write (app_ui/ui.h): with the game's own font (prose_antique, read from
// its archives: random_map_ui.h's GameFont) in the game's code page, as its dialogs do, where the
// interface's language fits that code page; else, and for the symbols the game's fonts lack (the
// arrows of a gamepad's d-pad), with the app's own fonts (Noto Sans: h4/fonts, built into the app).
#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace h4::app_ui {

class Writer {
 public:
  // `size`: the game font's (16 for text, 20 a button's, 24 a title's); `lang`: the interface's
  // language (ui_text.h), whose words it writes. The game's language is H4_LANG's (main.cpp).
  Writer(int size, const std::string& lang, bool bold = false);
  int Height() const { return height_; }  // a line's
  int Width(const std::string& utf8) const;
  // `utf8` with its line's top-left at (x, y) into the ARGB canvas w x h, in `ink`.
  void Draw(std::vector<uint32_t>& canvas, int w, int h, int x, int y, const std::string& utf8,
            uint32_t ink) const;
  // Lines of at most `max_w` pixels: cut at '\n', and at spaces where a line is too long (a word
  // longer than that alone on its line).
  std::vector<std::string> Wrap(const std::string& utf8, int max_w) const;

 private:
  bool GameFontFor(const std::string& utf8) const;  // the game's font writes it
  const void* game_ = nullptr;  // random_map::Font
  int code_page_ = 1252;
  bool game_for_all_ = false;  // the interface's words fit the game's code page
  float em_ = 13;              // the app's font's size (pixels)
  bool bold_ = false;
  int height_ = 16, ascent_ = 12;
};

}  // namespace h4::app_ui
