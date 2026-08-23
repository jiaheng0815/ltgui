#pragma once
#include "geometry.h"
#include <string>
#include <utility>  // std::to_underlying (C++23)

namespace ltgui {

enum class EventType {
  None,
  MouseDown,
  MouseUp,
  MouseMove,
  MouseWheel,
  KeyDown,
  KeyUp,
  Paint,
  Resize,
  Close,
  FocusIn,
  FocusOut,
  ImeComposition, // IME preedit/composition update (imeText, imeCursor)
  ThemeChanged,   // Theme was changed — widgets should refresh colors
  DragEnter,
  DragMove,
  DragLeave,
  DragDrop
};

enum class MouseButton { None = 0, Left = 1, Right = 2, Middle = 3 };

enum class KeyModifier {
  None = 0,
  Shift = 1,
  Control = 2,
  Alt = 4,
  Super = 8 // Windows key / macOS Command (Cmd) / X11 Super
};

inline KeyModifier operator|(KeyModifier a, KeyModifier b) {
  return static_cast<KeyModifier>(std::to_underlying(a) | std::to_underlying(b));
}
inline KeyModifier operator&(KeyModifier a, KeyModifier b) {
  return static_cast<KeyModifier>(std::to_underlying(a) & std::to_underlying(b));
}
// Mixed-type operators for use with Event::modifiers (int-typed)
inline int operator|(int a, KeyModifier b) { return a | std::to_underlying(b); }
inline int operator&(int a, KeyModifier b) { return a & std::to_underlying(b); }

inline bool hasModifier(int modifiers, KeyModifier mod) {
  return (modifiers & std::to_underlying(mod)) != 0;
}

enum class Key {
  Unknown,
  A,
  B,
  C,
  D,
  E,
  F,
  G,
  H,
  I,
  J,
  K,
  L,
  M,
  N,
  O,
  P,
  Q,
  R,
  S,
  T,
  U,
  V,
  W,
  X,
  Y,
  Z,
  Num0,
  Num1,
  Num2,
  Num3,
  Num4,
  Num5,
  Num6,
  Num7,
  Num8,
  Num9,
  F1,
  F2,
  F3,
  F4,
  F5,
  F6,
  F7,
  F8,
  F9,
  F10,
  F11,
  F12,
  Escape,
  Enter,
  Space,
  Backspace,
  Tab,
  Shift,
  Control,
  Alt,
  Left,
  Right,
  Up,
  Down,
  Home,
  End,
  PageUp,
  PageDown,
  Insert,
  Delete
};

struct Event {
  EventType type = EventType::None;
  Point pos;
  Point globalPos;
  MouseButton button = MouseButton::None;
  Key key = Key::Unknown;
  int modifiers = 0;
  int wheelDelta = 0;
  unsigned int charCode = 0; // Unicode char from WM_CHAR
  std::string imeText;       // Preedit/composition text from IME
  int imeCursor = 0;         // Cursor position within imeText
  int width = 0;
  int height = 0;
  bool accepted = false;
};

} // namespace ltgui
