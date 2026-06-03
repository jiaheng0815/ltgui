#pragma once
#include "event.h"
#include <functional>
#include <vector>
#include <string>

namespace ltgui {

// Keyboard shortcut with optional modifiers.
// Register on a Window; the Window dispatches matching KeyDown events.
//
// Usage:
//   window->registerShortcut(Shortcut(Key::S, Modifier::Ctrl), []{ save(); });

class Window;

class Shortcut {
public:
    using Callback = std::function<void()>;

    Key key = Key::Unknown;
    KeyModifier modifiers = KeyModifier::None;

    Shortcut() = default;
    Shortcut(Key k, KeyModifier m = KeyModifier::None) : key(k), modifiers(m) {}

    bool matches(Key pressedKey, KeyModifier pressedMods) const {
        if (key == Key::Unknown) return false;
        return pressedKey == key && pressedMods == modifiers;
    }

    bool operator==(const Shortcut& o) const {
        return key == o.key && modifiers == o.modifiers;
    }
};

} // namespace ltgui
