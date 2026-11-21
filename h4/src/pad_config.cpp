// pad_config.h
#include "pad_config.h"

#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <cstring>

#include <SDL3/SDL.h>

#include "runtime/hle/user32_window.h"

namespace h4 {

namespace {

struct Action {
  const char* id;
  const char* en;
  const char* uk;
  uint8_t mouse;
  SDL_Scancode key, mod;
};
constexpr SDL_Scancode kNone = SDL_SCANCODE_UNKNOWN;
const Action kActions[] = {
    {"none", "Nothing", "Нічого", 0, kNone, kNone},
    {"click", "Click (left button)", "Клік (ЛКМ)", SDL_BUTTON_LEFT, kNone, kNone},
    {"rclick", "Right-click (information)", "Правий клік (інформація)", SDL_BUTTON_RIGHT, kNone, kNone},
    {"esc", "Cancel, close (Esc)", "Скасувати, закрити (Esc)", 0, SDL_SCANCODE_ESCAPE, kNone},
    {"enter", "Move the army / OK (Enter)", "Рух армії / OK (Enter)", 0, SDL_SCANCODE_RETURN, kNone},
    {"spell", "Cast a spell (C)", "Магія (C)", 0, SDL_SCANCODE_C, kNone},
    {"wait", "Wait / wake the hero (W)", "Чекати / розбудити героя (W)", 0, SDL_SCANCODE_W, kNone},
    {"defend", "Defend / dig (D)", "Захист / копати (D)", 0, SDL_SCANCODE_D, kNone},
    {"hero", "Next hero (H)", "Наступний герой (H)", 0, SDL_SCANCODE_H, kNone},
    {"town", "Next town (T)", "Наступне місто (T)", 0, SDL_SCANCODE_T, kNone},
    {"sleep", "The hero sleeps (Z)", "Герой спить (Z)", 0, SDL_SCANCODE_Z, kNone},
    {"pgup", "Page Up (lists, spell tabs)", "Page Up (списки, вкладки магії)", 0, SDL_SCANCODE_PAGEUP, kNone},
    {"pgdn", "Page Down (lists, spell tabs)", "Page Down (списки, вкладки магії)", 0, SDL_SCANCODE_PAGEDOWN, kNone},
    {"endturn", "End the turn (E)", "Кінець ходу (E)", 0, SDL_SCANCODE_E, kNone},
    {"save", "Save the game (Alt+S)", "Зберегти гру (Alt+S)", 0, SDL_SCANCODE_S, SDL_SCANCODE_LALT},
    {"home", "Centre on the hero (Home)", "До героя (Home)", 0, SDL_SCANCODE_HOME, kNone},
    {"shift", "Shift (held)", "Shift (утримання)", 0, SDL_SCANCODE_LSHIFT, kNone},
    {"ctrl", "Ctrl (held)", "Ctrl (утримання)", 0, SDL_SCANCODE_LCTRL, kNone},
};
constexpr int kActionCount = int(sizeof kActions / sizeof kActions[0]);

struct Button {
  const char* id;  // in gamepad.conf
  const char* ps;  // its name on a PlayStation pad, an Xbox one, a Nintendo one (English, Ukrainian)
  const char* xbox;
  const char* sw_en;
  const char* sw_uk;
  const char* preset;  // the presets' action (the same by label on every pad)
};
// recomp::PadButton's order.
const Button kButtons[kPadButtons] = {
    {"A", "✕", "A", "A", "A", "click"},
    {"B", "○", "B", "B", "B", "esc"},
    {"X", "□", "X", "X", "X", "spell"},
    {"Y", "△", "Y", "Y", "Y", "wait"},
    {"L", "L1", "LB", "L", "L", "hero"},
    {"R", "R1", "RB", "R", "R", "defend"},
    {"ZL", "L2", "LT", "ZL", "ZL", "rclick"},
    {"ZR", "R2", "RT", "ZR", "ZR", "enter"},
    {"Up", nullptr, nullptr, "D-pad ↑", "Хрестовина ↑", "town"},
    {"Down", nullptr, nullptr, "D-pad ↓", "Хрестовина ↓", "sleep"},
    {"Left", nullptr, nullptr, "D-pad ←", "Хрестовина ←", "pgup"},
    {"Right", nullptr, nullptr, "D-pad →", "Хрестовина →", "pgdn"},
    {"Plus", "Options", "Menu", "+", "+", "endturn"},
    {"Minus", "Share", "View", "−", "−", "ctrl"},
    {"L3", "L3", "LS", "L3", "L3", "shift"},
    {"R3", "R3", "RS", "R3", "R3", "home"},
};

int ActionIndex(const char* id) {
  for (int i = 0; i < kActionCount; i++)
    if (!std::strcmp(kActions[i].id, id)) return i;
  return -1;
}

std::string g_path;
int g_style = -1;  // PadStyle; -1: not chosen (the pad's own)
float g_pointer = 1, g_scroll = 1;
bool g_on = true;
int g_binding[kPadButtons];

void Apply() {
  recomp::SetPadEnabled(g_on);
  recomp::SetPadPointerSpeed(g_pointer);
  recomp::SetPadScrollSpeed(g_scroll);
  for (int b = 0; b < kPadButtons; b++) {
    const Action& a = kActions[g_binding[b]];
    recomp::PadAction p;
    p.mouse = a.mouse;
    p.key = a.key;
    p.mod = a.mod;
    recomp::SetPadAction(recomp::PadButton(b), p);
  }
}

void Defaults() {
  g_style = -1;
  g_on = true;
  g_pointer = g_scroll = 1;
  for (int b = 0; b < kPadButtons; b++) g_binding[b] = std::max(0, ActionIndex(kButtons[b].preset));
}

const char* const kStyleIds[] = {"ps", "xbox", "switch"};

void Save() {
  FILE* f = std::fopen(g_path.c_str(), "w");
  if (!f) return;
  std::fprintf(f, "enabled=%d\n", g_on ? 1 : 0);
  if (g_style >= 0) std::fprintf(f, "style=%s\n", kStyleIds[g_style]);
  std::fprintf(f, "pointer=%.2f\nscroll=%.2f\n", g_pointer, g_scroll);
  for (int b = 0; b < kPadButtons; b++) std::fprintf(f, "%s=%s\n", kButtons[b].id, kActions[g_binding[b]].id);
  std::fclose(f);
}

}  // namespace

const char* PadButtonName(int button, bool uk) {
  if (button < 0 || button >= kPadButtons) return "";
  const Button& b = kButtons[button];
  const int style = PadStyle();
  if (style == kPadPlayStation && b.ps) return b.ps;
  if (style == kPadXbox && b.xbox) return b.xbox;
  return uk ? b.sw_uk : b.sw_en;
}

int PadStyle() {
  if (g_style >= 0) return g_style;
  // The pad connected now (the first one, as PadTick takes it).
  int style = kPadXbox, n = 0;
  if (SDL_JoystickID* ids = SDL_GetGamepads(&n)) {
    if (n > 0) switch (SDL_GetGamepadTypeForID(ids[0])) {
        case SDL_GAMEPAD_TYPE_PS3:
        case SDL_GAMEPAD_TYPE_PS4:
        case SDL_GAMEPAD_TYPE_PS5: style = kPadPlayStation; break;
        case SDL_GAMEPAD_TYPE_NINTENDO_SWITCH_PRO:
        case SDL_GAMEPAD_TYPE_NINTENDO_SWITCH_JOYCON_LEFT:
        case SDL_GAMEPAD_TYPE_NINTENDO_SWITCH_JOYCON_RIGHT:
        case SDL_GAMEPAD_TYPE_NINTENDO_SWITCH_JOYCON_PAIR: style = kPadSwitch; break;
        default: break;
      }
    SDL_free(ids);
  }
#if defined(__SWITCH__)
  style = kPadSwitch;
#endif
  return style;
}

const char* PadStyleName(int style) {
  return style == kPadPlayStation ? "PlayStation" : style == kPadXbox ? "Xbox" : "Switch";
}

void ResetPadConfig() {
  const int style = g_style;  // (the names stay)
  Defaults();
  g_style = style;
  Apply();
  Save();
  std::fprintf(stderr, "h4: gamepad reset\n");
}

void ApplyPadPreset(int style) {
  const float pointer = g_pointer, scroll = g_scroll;  // (the speeds stay)
  const bool on = g_on;
  Defaults();
  g_on = on;
  g_pointer = pointer;
  g_scroll = scroll;
  g_style = std::clamp(style, 0, kPadStyles - 1);
  Apply();
  Save();
  std::fprintf(stderr, "h4: gamepad preset %s\n", kStyleIds[g_style]);
}

int PadActionCount() { return kActionCount; }
const char* PadActionId(int action) { return action >= 0 && action < kActionCount ? kActions[action].id : ""; }
const char* PadButtonId(int button) { return button >= 0 && button < kPadButtons ? kButtons[button].id : ""; }
const char* PadStyleId(int style) { return style >= 0 && style < kPadStyles ? kStyleIds[style] : ""; }

const char* PadActionName(int action, bool uk) {
  if (action < 0 || action >= kActionCount) return "";
  return uk ? kActions[action].uk : kActions[action].en;
}

void LoadPadConfig(const std::string& path) {
  g_path = path;
  Defaults();
  if (FILE* f = std::fopen(path.c_str(), "r")) {
    char line[128];
    while (std::fgets(line, sizeof line, f)) {
      line[std::strcspn(line, "\r\n")] = 0;
      char* eq = std::strchr(line, '=');
      if (!eq) continue;
      *eq = 0;
      const char* value = eq + 1;
      if (!std::strcmp(line, "enabled")) {
        g_on = std::atoi(value) != 0;
      } else if (!std::strcmp(line, "style")) {
        for (int i = 0; i < kPadStyles; i++)
          if (!std::strcmp(value, kStyleIds[i])) g_style = i;
      } else if (!std::strcmp(line, "pointer")) {
        g_pointer = std::clamp(std::strtof(value, nullptr), kPointerMin, kPointerMax);
      } else if (!std::strcmp(line, "scroll")) {
        g_scroll = std::clamp(std::strtof(value, nullptr), kScrollMin, kScrollMax);
      } else {
        for (int b = 0; b < kPadButtons; b++)
          if (!std::strcmp(line, kButtons[b].id) && ActionIndex(value) >= 0) g_binding[b] = ActionIndex(value);
      }
    }
    std::fclose(f);
  }
  Apply();
}

bool PadEnabled() { return g_on; }
void SetPadEnabled(bool on) {
  g_on = on;
  Apply();
  Save();
  std::fprintf(stderr, "h4: gamepad %s\n", on ? "on" : "off");
}

float PadPointerSpeed() { return g_pointer; }
void SetPadPointerSpeed(float factor) {
  g_pointer = std::clamp(factor, kPointerMin, kPointerMax);
  Apply();
  Save();
}

float PadScrollSpeed() { return g_scroll; }
void SetPadScrollSpeed(float factor) {
  g_scroll = std::clamp(factor, kScrollMin, kScrollMax);
  Apply();
  Save();
}

int PadBinding(int button) { return button >= 0 && button < kPadButtons ? g_binding[button] : 0; }
void SetPadBinding(int button, int action) {
  if (button < 0 || button >= kPadButtons || action < 0 || action >= kActionCount) return;
  g_binding[button] = action;
  Apply();
  Save();
  std::fprintf(stderr, "h4: gamepad %s: %s\n", kButtons[button].id, kActions[action].id);
}


std::string PadHelp(bool uk) {
  std::string s = uk ? "Лівий стік — курсор\nПравий стік — прокрутка мапи\n"
                     : "Left stick: the pointer\nRight stick: scrolls the map\n";
  for (int b = 0; b < kPadButtons; b++) {
    if (g_binding[b] == 0) continue;
    s += std::string(PadButtonName(b, uk)) + (uk ? " — " : ": ") + PadActionName(g_binding[b], uk) + "\n";
  }
  s += uk ? "\nXbox / PlayStation: L/R — LB/RB (L1/R1), ZL/ZR — LT/RT (L2/R2), + — Menu (Options), "
            "− — View (Share)."
          : "\nXbox / PlayStation: L/R are LB/RB (L1/R1), ZL/ZR LT/RT (L2/R2), + Menu (Options), "
            "− View (Share).";
  return s;
}

}  // namespace h4
