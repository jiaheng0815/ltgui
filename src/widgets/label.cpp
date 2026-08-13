#include "widgets/label.h"
#include "platform/native_canvas.h"
#include "theme.h"
#include "window.h"

namespace ltgui {

Label::Label(const std::string &text, Widget *parent)
    : TextWidget(text, parent) {
  style().bgColor = Color::Transparent;
  style().borderWidth = 0;
}

Size Label::sizeHint() const {
  if (!sizeHintDirty())
    return cachedSizeHint();
  setCachedSizeHint(
      textSizeHint({60, 20}, style().paddingHorz(), style().paddingVert()));
  return cachedSizeHint();
}

void Label::paintSelf(NativeCanvas *canvas) {
  Rect r = absoluteRect();
  ResolvedStyle st = resolvedStyle();
  canvas->setColor(st.fgColor);
  canvas->setFont(st.font);
  int flags = NativeCanvas::AlignLeft | NativeCanvas::AlignVCenter |
              NativeCanvas::SingleLine;
  canvas->drawText(text_, r, flags);
}

} // namespace ltgui
