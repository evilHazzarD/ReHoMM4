// The gamepad's settings (the apps' settings, "Gamepad"): the left stick's pointer speed, the right
// stick's scrolling speed and what each button does. Kept in <data>/gamepad.conf (lines
// "key=value": enabled=1, style=ps, pointer=1.5, scroll=0.5, A=click, ZL=esc...; what is not there: the default), read
// at start and changed in the settings at once (recomp::SetPad*).
#pragma once

#include <string>

namespace h4 {

// The pointer's speed: a factor of the default (1100 points a second at full tilt on a 1280-wide
// window); the scrolling's: the share of the game's own speed.
inline constexpr float kPointerMin = 0.25f, kPointerMax = 3.0f;
inline constexpr float kScrollMin = 0.1f, kScrollMax = 1.0f;

// The buttons, in recomp::PadButton's order.
inline constexpr int kPadButtons = 16;
// Its name as the pad's maker has it (PadStyle): "✕" / "A", "L1" / "LB" / "L", "Хрестовина ↑"...
const char* PadButtonName(int button, bool uk);

// The presets: the same layout (the buttons by their labels: A confirms on every pad), named as on
// a PlayStation, an Xbox or a Nintendo pad. Until one is picked: the connected pad's kind.
enum { kPadPlayStation, kPadXbox, kPadSwitch, kPadStyles };
int PadStyle();
const char* PadStyleName(int style);  // "PlayStation"...
void ApplyPadPreset(int style);  // its layout and names (saved; the speeds stay)
void ResetPadConfig();  // the speeds back to 1 and the preset's layout (the names stay; saved)

// What a button can do (index into the list; 0: nothing).
int PadActionCount();
const char* PadActionName(int action, bool uk);  // "Клік (ЛКМ)", "Магія (C)"...
// Their ids in gamepad.conf (and the Linux app's pictures: gp_act_<id>, gp_b_<style>_<button>).
const char* PadActionId(int action);   // "click"...
const char* PadButtonId(int button);   // "A", "Up", "L3"...
const char* PadStyleId(int style);     // "ps", "xbox", "switch"

void LoadPadConfig(const std::string& path);
bool PadEnabled();  // the gamepad read at all (on at first)
void SetPadEnabled(bool on);
float PadPointerSpeed();
void SetPadPointerSpeed(float factor);
float PadScrollSpeed();
void SetPadScrollSpeed(float factor);
int PadBinding(int button);
void SetPadBinding(int button, int action);

// The layout as it is set now, a line per button (and the sticks).
std::string PadHelp(bool uk);

}  // namespace h4
