#pragma once
#include "geometry.h"

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
    FocusOut
};

enum class MouseButton {
    None = 0,
    Left = 1,
    Right = 2,
    Middle = 3
};

enum class KeyModifier {
    None = 0,
    Shift = 1,
    Control = 2,
    Alt = 4
};

inline KeyModifier operator|(KeyModifier a, KeyModifier b) {
    return static_cast<KeyModifier>(static_cast<int>(a) | static_cast<int>(b));
}
inline int operator&(KeyModifier a, KeyModifier b) {
    return static_cast<int>(a) & static_cast<int>(b);
}

enum class Key {
    Unknown,
    A, B, C, D, E, F, G, H, I, J, K, L, M,
    N, O, P, Q, R, S, T, U, V, W, X, Y, Z,
    Num0, Num1, Num2, Num3, Num4, Num5, Num6, Num7, Num8, Num9,
    F1, F2, F3, F4, F5, F6, F7, F8, F9, F10, F11, F12,
    Escape, Enter, Space, Backspace, Tab,
    Shift, Control, Alt,
    Left, Right, Up, Down,
    Home, End, PageUp, PageDown,
    Insert, Delete
};

struct Event {
    EventType type = EventType::None;
    Point pos;
    Point globalPos;
    MouseButton button = MouseButton::None;
    Key key = Key::Unknown;
    int modifiers = 0;
    int wheelDelta = 0;
    unsigned int charCode = 0;  // Unicode char from WM_CHAR
    int width = 0;
    int height = 0;
    bool accepted = false;
};

} // namespace ltgui
