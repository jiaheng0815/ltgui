#include "widgets/textwidget.h"
#include "platform/native_canvas.h"
#include "window.h"

namespace ltgui {

Size TextWidget::textSizeHint(Size fallback, int extraW, int extraH) const {
  if (auto *win = window()) {
    if (auto *c = win->canvas()) {
      c->setFont(style().font);
      Size textSize = c->measureText(text_);
      return {textSize.width + extraW, textSize.height + extraH};
    }
  }
  return fallback;
}

} // namespace ltgui
