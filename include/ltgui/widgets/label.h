#pragma once
#include "widgets/textwidget.h"

namespace ltgui {

class Label : public TextWidget {
public:
  explicit Label(const std::string &text = "", Widget *parent = nullptr);

  LTGUI_DECLARE_WIDGET_TYPE(Label)
  bool canAcceptFocus() const override { return false; }
  Size sizeHint() const override;

protected:
  void paintSelf(NativeCanvas *canvas) override;
};

} // namespace ltgui
