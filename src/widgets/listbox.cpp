#include "widgets/listbox.h"
#include "platform/native_canvas.h"
#include "theme.h"
#include "window.h"
#include <algorithm>

namespace ltgui {

ListBox::ListBox(Widget *parent) : Widget(parent), ListItems(this) {
  style().borderRadius = 4;
}

Size ListBox::sizeHint() const {
  if (!sizeHintDirty())
    return cachedSizeHint();
  setCachedSizeHint(dpiScaleSize(160, 140));
  return cachedSizeHint();
}

int ListBox::visibleItems() const {
  return std::max(1, (height() - 2) / itemHeight_);
}

void ListBox::paintSelf(NativeCanvas *canvas) {
  Rect r = absoluteRect();
  ResolvedStyle st = resolvedStyle();

  paintBackground(canvas);

  canvas->setFont(st.font);
  int visible = visibleItems();
  int maxOffset = std::max(0, static_cast<int>(items_.size()) - visible);
  int scrollOffset = currentScrollOffset();
  if (scrollOffset > maxOffset)
    scrollOffset = maxOffset;

  for (int i = scrollOffset;
       i < std::min(static_cast<int>(items_.size()), scrollOffset + visible);
       i++) {
    int itemY = r.y + 1 + (i - scrollOffset) * itemHeight_;
    Rect itemRect(r.x + 1, itemY, r.width - 2, itemHeight_);

    if (i == selected_) {
      canvas->setColor(st.accent);
      canvas->fillRoundedRect(itemRect.adjusted(2, 1, -2, -1), 3);
      canvas->setColor(Color::White);
    } else {
      canvas->setColor(st.fgColor);
    }

    canvas->drawText(items_[i], itemRect.adjusted(6, 0, -4, 0),
                     NativeCanvas::AlignLeft | NativeCanvas::AlignVCenter |
                         NativeCanvas::SingleLine);
  }
}

bool ListBox::handleEvent(Event &event) {
  if (!isEnabled())
    return false;

  if (event.type == EventType::MouseDown && event.button == MouseButton::Left) {
    int localY = event.pos.y - y() - 1;
    int scrollOffset = currentScrollOffset();
    int index = scrollOffset + localY / itemHeight_;
    if (index >= 0 && index < static_cast<int>(items_.size())) {
      setCurrentIndex(index);
    }
    event.accepted = true;
    return true;
  }

  if (event.type == EventType::MouseWheel) {
    int maxOffset =
        std::max(0, static_cast<int>(items_.size()) - visibleItems());
    handleWheel(event.wheelDelta, maxOffset, 1);
    update();
    event.accepted = true;
    return true;
  }

  if (event.type == EventType::KeyDown) {
    if (event.key == Key::Down) {
      if (selected_ < static_cast<int>(items_.size()) - 1) {
        setCurrentIndex(selected_ + 1);
        // Ensure selection is visible
        int visible = visibleItems();
        int maxOffset = std::max(0, static_cast<int>(items_.size()) - visible);
        if (selected_ >= currentScrollOffset() + visible) {
          setScrollTarget(selected_ - visible + 1, maxOffset);
        }
      }
      event.accepted = true;
      return true;
    }
    if (event.key == Key::Up) {
      if (selected_ > 0) {
        setCurrentIndex(selected_ - 1);
        if (selected_ < currentScrollOffset()) {
          setScrollTarget(
              selected_,
              std::max(0, static_cast<int>(items_.size()) - visibleItems()));
        }
      }
      event.accepted = true;
      return true;
    }
  }

  return false;
}

} // namespace ltgui
