#pragma once
#include "signal.h"

namespace ltgui {

class Widget;

// Mixin for widgets with a checked/unchecked state (CheckBox, RadioButton).
// NOT a Widget subclass — combine via multiple inheritance:
//   class CheckBox : public TextWidget, public Checkable
// The host widget is stored so state changes can trigger a repaint.
class Checkable {
public:
    explicit Checkable(Widget* host) : host_(host) {}

    bool isChecked() const { return checked_; }
    virtual void setChecked(bool checked);

    // Emitted whenever the checked state changes.
    Signal<bool> onToggled;

protected:
    Widget* host_ = nullptr;
    bool checked_ = false;
};

} // namespace ltgui
