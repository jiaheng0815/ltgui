#pragma once
#include "animation.h"
#include "signal.hpp"
#include "widgets/textwidget.h"

namespace ltgui {

class Button : public TextWidget {
public:
  explicit Button(const std::string &text = "", Widget *parent = nullptr);

  // Emitted when the button is activated (click or Enter/Space).
  Signal<> onClicked;

  LTGUI_DECLARE_WIDGET_TYPE(Button)
  bool canAcceptFocus() const override { return true; }
  Size sizeHint() const override;

protected:
  void paintSelf(NativeCanvas *canvas) override;
  bool handleEvent(Event &event) override;

private:
  // Smoothly transitions the background between normal/hover/pressed.
  void animateBg();

  AnimatedColor bgAnim_;
};

} // namespace ltgui
