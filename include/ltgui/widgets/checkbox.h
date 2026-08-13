#pragma once
#include "widgets/checkable.h"
#include "widgets/textwidget.h"

namespace ltgui {

class CheckBox : public TextWidget, public Checkable {
public:
  explicit CheckBox(const std::string &text = "", Widget *parent = nullptr);

  LTGUI_DECLARE_WIDGET_TYPE(CheckBox)
  bool canAcceptFocus() const override { return true; }
  Size sizeHint() const override;

protected:
  void paintSelf(NativeCanvas *canvas) override;
  bool handleEvent(Event &event) override;
};

} // namespace ltgui
