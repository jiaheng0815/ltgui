#pragma once
#include "widgets/checkable.h"
#include "widgets/textwidget.h"

namespace ltgui {

class RadioButton : public TextWidget, public Checkable {
public:
  explicit RadioButton(const std::string &text = "", Widget *parent = nullptr);

  // Radio buttons in a group cannot be unchecked by direct user action;
  // checking one unchecks all sibling radio buttons.
  void setChecked(bool checked) override;

  LTGUI_DECLARE_WIDGET_TYPE(RadioButton)
  bool canAcceptFocus() const override { return true; }
  Size sizeHint() const override;

protected:
  void paintSelf(NativeCanvas *canvas) override;
  bool handleEvent(Event &event) override;
};

} // namespace ltgui
