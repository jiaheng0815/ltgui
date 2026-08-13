#include "widgets/radiobutton.h"
#include "platform/native_canvas.h"
#include "theme.h"
#include "window.h"

namespace ltgui {

RadioButton::RadioButton(const std::string &text, Widget *parent)
    : TextWidget(text, parent), Checkable(this) {
  style().bgColor = Color::Transparent;
  style().borderWidth = 0;
}

void RadioButton::setChecked(bool checked) {
  // Radio buttons in a group cannot be unchecked by direct user action.
  // Only allow unchecking programmatically or when another button becomes
  // checked.
  if (checked_ == checked)
    return;

  if (checked) {
    // Uncheck all sibling radio buttons
    if (parent()) {
      for (auto &child : parent()->children()) {
        if (child.get() != this &&
            child->widgetType() == WidgetType::RadioButton) {
          auto *rb = static_cast<RadioButton *>(child.get());
          if (rb->isChecked()) {
            rb->checked_ = false;
            rb->update();
            rb->onToggled.emit(false);
          }
        }
      }
    }
  }
  // If trying to uncheck the only checked button in group, ignore
  if (!checked && checked_) {
    return;
  }

  checked_ = checked;
  update();
  onToggled.emit(checked_);
}

Size RadioButton::sizeHint() const {
  if (!sizeHintDirty())
    return cachedSizeHint();
  Size s = textSizeHint({100, 22});
  setCachedSizeHint({s.width + 24 + style().paddingHorz(),
                     std::max(s.height, 16 + style().paddingVert())});
  return cachedSizeHint();
}

void RadioButton::paintSelf(NativeCanvas *canvas) {
  Rect r = absoluteRect();
  ResolvedStyle st = resolvedStyle();

  int circleSize = 14;
  int circleY = r.y + (r.height - circleSize) / 2;
  Rect circleRect(r.x + 2, circleY, circleSize, circleSize);

  canvas->setColor(st.bgColor);
  canvas->fillEllipse(circleRect);
  canvas->setColor(checked_ ? st.accent : st.borderColor);
  canvas->strokeEllipse(circleRect, checked_ ? 2 : 1);

  if (checked_) {
    Rect dotRect(r.x + 5, circleY + 3, circleSize - 6, circleSize - 6);
    canvas->setColor(st.accent);
    canvas->fillEllipse(dotRect);
  }

  canvas->setColor(st.fgColor);
  canvas->setFont(st.font);
  Rect textRect(r.x + circleSize + 6, r.y, r.width - circleSize - 6, r.height);
  canvas->drawText(text_, textRect,
                   NativeCanvas::AlignLeft | NativeCanvas::AlignVCenter |
                       NativeCanvas::SingleLine);
}

bool RadioButton::handleEvent(Event &event) {
  if (!isEnabled())
    return false;

  if (event.type == EventType::MouseDown && event.button == MouseButton::Left) {
    setChecked(true);
    event.accepted = true;
    return true;
  }
  return false;
}

} // namespace ltgui
