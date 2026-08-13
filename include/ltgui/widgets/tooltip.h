#pragma once
#include "widgets/textwidget.h"

namespace ltgui {

class Tooltip : public TextWidget {
public:
  explicit Tooltip(Widget *parent = nullptr);

  void showAt(const Point &screenPos);
  void dismiss();

  static void show(Widget *target, const std::string &text);

  LTGUI_DECLARE_WIDGET_TYPE(Tooltip)
  Size sizeHint() const override;

protected:
  void paintSelf(NativeCanvas *canvas) override;

private:
  Point position_;
};

} // namespace ltgui
