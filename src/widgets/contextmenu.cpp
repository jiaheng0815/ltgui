#include "widgets/contextmenu.h"
#include "platform/native_canvas.h"
#include "theme.h"
#include "window.h"
#include <algorithm>

namespace ltgui {

ContextMenu::ContextMenu(Widget *parent) : Widget(parent) {
  style().borderRadius = 6;
  style().setPadding(4, 2);
  setVisible(false);
}

int ContextMenu::addItem(const std::string &text, ItemCallback cb) {
  items_.push_back({text, std::move(cb), false});
  invalidateSizeHint();
  return static_cast<int>(items_.size()) - 1;
}

void ContextMenu::addSeparator() { items_.push_back({"", nullptr, true}); }

void ContextMenu::clear() {
  items_.clear();
  hovered_ = -1;
  invalidateSizeHint();
}

int ContextMenu::count() const { return static_cast<int>(items_.size()); }

std::string ContextMenu::itemText(int index) const {
  if (index >= 0 && index < static_cast<int>(items_.size()))
    return items_[index].text;
  return {};
}

void ContextMenu::popup(const Point &screenPos) {
  int w = bestWidth();
  int h =
      static_cast<int>(items_.size()) * itemHeight_ + style().paddingVert() + 4;
  setGeometry(Rect(screenPos.x, screenPos.y, w, h));
  hovered_ = -1;
  setVisible(true);
  raiseToTop();
  // Register with the window so clicks outside the menu dismiss it, and so
  // the window routes input to us while we're open.
  if (auto *win = window())
    win->setOpenContextMenu(this);
  claimFocus();
  update();
}

void ContextMenu::dismiss() {
  if (auto *win = window())
    win->setOpenContextMenu(nullptr);
  setVisible(false);
}

Size ContextMenu::sizeHint() const {
  if (!sizeHintDirty())
    return cachedSizeHint();
  setCachedSizeHint({bestWidth(), dpiScaleSize(1, 100).height});
  return cachedSizeHint();
}

int ContextMenu::bestWidth() const {
  int w = 120;
  if (auto *win = window()) {
    if (auto *c = win->canvas()) {
      c->setFont(style().font);
      for (auto &item : items_) {
        if (item.separator)
          continue;
        int tw = c->measureText(item.text).width + 28;
        w = std::max(w, tw);
      }
    }
  }
  return w;
}

void ContextMenu::paintSelf(NativeCanvas *canvas) {
  Rect r = absoluteRect();
  ResolvedStyle st = resolvedStyle();
  const Theme &t = currentTheme();

  paintBackground(canvas);

  canvas->setFont(st.font);
  int pad = st.paddingTop;
  int y = r.y + pad;

  for (int i = 0; i < static_cast<int>(items_.size()); i++) {
    if (items_[i].separator) {
      canvas->setColor(st.borderColor);
      canvas->drawLine({r.x + 8, y + 3}, {r.right() - 8, y + 3}, 1);
      y += 8;
      continue;
    }

    Rect itemRect(r.x + 2, y, r.width - 4, itemHeight_);

    if (i == hovered_) {
      canvas->setColor(t.menuItemSelected);
      canvas->fillRoundedRect(itemRect.adjusted(2, 1, -2, -1), 3);
      canvas->setColor(Color::White);
    } else {
      canvas->setColor(st.fgColor);
    }

    canvas->drawText(items_[i].text, itemRect.adjusted(12, 0, -8, 0),
                     NativeCanvas::AlignLeft | NativeCanvas::AlignVCenter |
                         NativeCanvas::SingleLine);
    y += itemHeight_;
  }
}

bool ContextMenu::handleEvent(Event &event) {
  if (!isEnabled() || !isVisible())
    return false;

  // Compute visual Y offset accounting for separator rows (which are 8px, not
  // itemHeight_)
  int localY = event.pos.y - y();
  int yOff = style().paddingTop;
  int itemIdx = -1;
  for (int i = 0; i < static_cast<int>(items_.size()); i++) {
    int rowH = items_[i].separator ? 8 : itemHeight_;
    if (localY >= yOff && localY < yOff + rowH) {
      itemIdx = i;
      break;
    }
    yOff += rowH;
  }

  if (event.type == EventType::MouseMove) {
    if (itemIdx >= 0 && !items_[itemIdx].separator) {
      hovered_ = itemIdx;
    } else {
      hovered_ = -1;
    }
    update();
    event.accepted = true;
    return true;
  }

  if (event.type == EventType::MouseDown) {
    // Snapshot the item's action BEFORE dismissing: the callback (or any
    // re-entrant handler) may mutate or destroy this menu while it runs,
    // so it must not run while we still hold a live reference into the
    // state that dismissed the menu.
    bool hasAction = itemIdx >= 0 && !items_[itemIdx].separator;
    ItemCallback callback;
    if (hasAction)
      callback = items_[itemIdx].callback;
    // Any click on the menu (item hit or not, any button) dismisses it.
    dismiss();
    if (hasAction && callback)
      callback();
    event.accepted = true;
    return true;
  }

  if (event.type == EventType::KeyDown && event.key == Key::Escape) {
    dismiss();
    event.accepted = true;
    return true;
  }

  return false;
}

} // namespace ltgui
