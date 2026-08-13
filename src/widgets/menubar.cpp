#include "widgets/menubar.h"
#include "platform/native_canvas.h"
#include "theme.h"
#include "window.h"
#include <algorithm>

namespace ltgui {

MenuBar::MenuBar(Widget *parent) : Widget(parent) {}

int MenuBar::addMenu(const std::string &label) {
  menus_.push_back({label, {}});
  invalidateSizeHint();
  update();
  return static_cast<int>(menus_.size()) - 1;
}

int MenuBar::addItem(int menuIdx, const std::string &text, ItemCallback cb) {
  if (menuIdx < 0 || menuIdx >= static_cast<int>(menus_.size()))
    return -1;
  menus_[menuIdx].items.push_back({text, std::move(cb), false});
  return static_cast<int>(menus_[menuIdx].items.size()) - 1;
}

void MenuBar::addSeparator(int menuIdx) {
  if (menuIdx < 0 || menuIdx >= static_cast<int>(menus_.size()))
    return;
  menus_[menuIdx].items.push_back({"", nullptr, true});
}

void MenuBar::setItemShortcut(int menuIdx, int itemIdx, const std::string &s) {
  if (menuIdx < 0 || menuIdx >= (int)menus_.size())
    return;
  if (itemIdx < 0 || itemIdx >= (int)menus_[menuIdx].items.size())
    return;
  menus_[menuIdx].items[itemIdx].shortcut = s;
}

void MenuBar::setItemCheckable(int menuIdx, int itemIdx, bool c) {
  if (menuIdx < 0 || menuIdx >= (int)menus_.size())
    return;
  if (itemIdx < 0 || itemIdx >= (int)menus_[menuIdx].items.size())
    return;
  menus_[menuIdx].items[itemIdx].checkable = c;
}

void MenuBar::setItemChecked(int menuIdx, int itemIdx, bool c) {
  if (menuIdx < 0 || menuIdx >= (int)menus_.size())
    return;
  if (itemIdx < 0 || itemIdx >= (int)menus_[menuIdx].items.size())
    return;
  auto &item = menus_[menuIdx].items[itemIdx];
  item.checked = c;
  if (item.radio && c) {
    // Uncheck others in same radio group
    for (auto &mi : menus_[menuIdx].items) {
      if (mi.radio && mi.radioGroup == item.radioGroup && &mi != &item)
        mi.checked = false;
    }
  }
  update();
}

bool MenuBar::isItemChecked(int menuIdx, int itemIdx) const {
  if (menuIdx < 0 || menuIdx >= (int)menus_.size())
    return false;
  if (itemIdx < 0 || itemIdx >= (int)menus_[menuIdx].items.size())
    return false;
  return menus_[menuIdx].items[itemIdx].checked;
}

void MenuBar::setItemRadio(int menuIdx, int itemIdx, int radioGroup) {
  if (menuIdx < 0 || menuIdx >= (int)menus_.size())
    return;
  if (itemIdx < 0 || itemIdx >= (int)menus_[menuIdx].items.size())
    return;
  menus_[menuIdx].items[itemIdx].radio = true;
  menus_[menuIdx].items[itemIdx].radioGroup = radioGroup;
}

int MenuBar::addSubmenu(int menuIdx, int itemIdx, const std::string &label) {
  if (menuIdx < 0 || menuIdx >= (int)menus_.size())
    return -1;
  if (itemIdx < 0 || itemIdx >= (int)menus_[menuIdx].items.size())
    return -1;
  menus_[menuIdx].items[itemIdx].submenu.push_back({label, nullptr, false});
  return static_cast<int>(menus_[menuIdx].items[itemIdx].submenu.size()) - 1;
}

int MenuBar::addSubItem(int menuIdx, int itemIdx, int subIdx,
                        const std::string &text, ItemCallback cb) {
  if (menuIdx < 0 || menuIdx >= (int)menus_.size())
    return -1;
  auto &items = menus_[menuIdx].items;
  if (itemIdx < 0 || itemIdx >= (int)items.size())
    return -1;
  auto &sub = items[itemIdx].submenu;
  if (subIdx < 0 || subIdx >= (int)sub.size())
    return -1;
  sub[subIdx].text = text;
  sub[subIdx].callback = std::move(cb);
  return subIdx;
}

void MenuBar::addSubSeparator(int menuIdx, int itemIdx, int subIdx) {
  if (menuIdx < 0 || menuIdx >= (int)menus_.size())
    return;
  auto &items = menus_[menuIdx].items;
  if (itemIdx < 0 || itemIdx >= (int)items.size())
    return;
  auto &sub = items[itemIdx].submenu;
  if (subIdx < 0 || subIdx >= (int)sub.size())
    return;
  sub[subIdx].separator = true;
}

Size MenuBar::sizeHint() const {
  if (!sizeHintDirty())
    return cachedSizeHint();
  setCachedSizeHint(dpiScaleSize(400, menuBarHeight_));
  return cachedSizeHint();
}

int MenuBar::menuX(int idx) const {
  int x = 4;
  for (int i = 0; i < idx; i++)
    x += menuWidth(i) + 2;
  return x;
}

int MenuBar::menuWidth(int idx) const {
  if (idx < 0 || idx >= static_cast<int>(menus_.size()))
    return 0;
  return static_cast<int>(menus_[idx].label.size()) * 10 + 16;
}

int MenuBar::hitTestMenu(int localX, int localY) const {
  if (localY < 0 || localY > height())
    return -1;
  int xPos = 4;
  for (int i = 0; i < static_cast<int>(menus_.size()); i++) {
    if (localX >= xPos && localX < xPos + menuWidth(i))
      return i;
    xPos += menuWidth(i) + 2;
  }
  return -1;
}

int MenuBar::hitTestItem(int localY) const {
  if (openMenu_ < 0 || openMenu_ >= (int)menus_.size())
    return -1;
  int relY = localY - height() - 2;
  int idx = relY / itemHeight_;
  if (idx < 0 || idx >= (int)menus_[openMenu_].items.size())
    return -1;
  return idx;
}

int MenuBar::dropWidth(const std::vector<MenuItem> &items, int depth) const {
  int maxW = 160;
  for (auto &item : items) {
    int w = static_cast<int>(item.text.size()) * 8 + 24;
    if (!item.shortcut.empty())
      w += static_cast<int>(item.shortcut.size()) * 8 + 20;
    if (!item.submenu.empty())
      w += 20; // space for arrow
    if (item.checkable || item.radio)
      w += 20; // space for checkmark
    maxW = std::max(maxW, w);
    if (!item.submenu.empty()) {
      int subW = dropWidth(item.submenu, depth + 1);
      maxW = std::max(maxW, subW);
    }
  }
  return maxW;
}

// --- Paint ---

static void drawCheck(NativeCanvas *canvas, const Rect &r, const Color &c) {
  canvas->setColor(c);
  int cy = r.y + r.height / 2;
  canvas->drawLine({r.x + 3, cy}, {r.x + 6, cy + 3}, 2);
  canvas->drawLine({r.x + 6, cy + 3}, {r.x + 11, cy - 3}, 2);
}

static void drawRadio(NativeCanvas *canvas, const Rect &r, const Color &c,
                      bool filled) {
  int cx = r.x + 7, cy = r.y + r.height / 2;
  canvas->setColor(c);
  canvas->strokeEllipse(Rect(cx - 5, cy - 5, 10, 10));
  if (filled)
    canvas->fillEllipse(Rect(cx - 3, cy - 3, 6, 6));
}

static void drawHighlight(NativeCanvas *canvas, const Rect &r,
                          const Color &accent) {
  canvas->setColor(accent);
  canvas->fillRoundedRect(r.adjusted(1, 2, -1, -2), 3);
  canvas->setColor(Color::White);
}

void MenuBar::paintItem(NativeCanvas *canvas, const Rect &r,
                        const MenuItem &item, bool hovered, int depth) {
  ResolvedStyle st = resolvedStyle();
  const Theme &t = currentTheme();
  int indent = depth * 12;

  if (hovered && !item.separator)
    drawHighlight(canvas, r, t.menuItemSelected);
  else
    canvas->setColor(hovered ? Color::White : st.fgColor);

  if (item.separator) {
    canvas->setColor(st.borderColor);
    canvas->drawLine({r.x + 8, r.y + r.height / 2},
                     {r.x + r.width - 8, r.y + r.height / 2}, 1);
    return;
  }

  int checkWidth = 0;
  if (item.checkable) {
    checkWidth = 16;
    drawCheck(canvas, Rect(r.x, r.y, 16, r.height),
              hovered ? Color::White : t.menuItemSelected);
  }
  if (item.radio) {
    checkWidth = 16;
    drawRadio(canvas, Rect(r.x, r.y, 16, r.height),
              hovered ? Color::White : t.menuItemSelected, item.checked);
  }

  if (item.checked && !item.checkable && !item.radio) {
    checkWidth = 16;
    // Show no check for non-checkable items
  }

  Rect textRect(r.x + indent + checkWidth, r.y,
                r.width - indent - checkWidth - 20, r.height);
  canvas->drawText(item.text, textRect,
                   NativeCanvas::AlignLeft | NativeCanvas::AlignVCenter);

  if (!item.shortcut.empty()) {
    canvas->drawText(item.shortcut, Rect(r.x, r.y, r.width - 28, r.height),
                     NativeCanvas::AlignRight | NativeCanvas::AlignVCenter);
  }

  if (!item.submenu.empty()) {
    canvas->drawText(">", Rect(r.x + r.width - 16, r.y, 16, r.height),
                     NativeCanvas::AlignRight | NativeCanvas::AlignVCenter);
  }
}

void MenuBar::paintSelf(NativeCanvas *canvas) {
  Rect r = absoluteRect();
  ResolvedStyle st = resolvedStyle();
  const Theme &t = currentTheme();
  // Menu bar uses the theme's dedicated menuBarBg color.
  canvas->setColor(t.menuBarBg);
  canvas->fillRect(r);
  canvas->setFont(Font(st.font.family, 13, st.font.weight, st.font.style));

  // Top-level menus
  int x = r.x + 4;
  for (int i = 0; i < static_cast<int>(menus_.size()); i++) {
    int w = menuWidth(i);
    Rect itemRect(x, r.y, w, r.height);
    if (i == hoveredMenu_ || i == openMenu_)
      drawHighlight(canvas, itemRect, t.menuItemSelected);
    else
      canvas->setColor(st.fgColor);
    canvas->drawText(menus_[i].label, itemRect.adjusted(8, 0, -8, 0),
                     NativeCanvas::AlignLeft | NativeCanvas::AlignVCenter);
    x += w + 2;
  }

  if (openMenu_ < 0 || openMenu_ >= (int)menus_.size())
    return;

  auto &items = menus_[openMenu_].items;
  int dropW = dropWidth(items, 0) + 20;
  int dropH = static_cast<int>(items.size()) * itemHeight_ + 4;
  int dropX = r.x + menuX(openMenu_);
  int dropY = r.bottom();

  canvas->setColor(st.bgColor);
  canvas->fillRoundedRect(Rect(dropX, dropY, dropW, dropH), 4);
  canvas->setColor(st.borderColor);
  canvas->strokeRoundedRect(Rect(dropX, dropY, dropW, dropH), 4);
  canvas->setFont(st.font);

  for (int j = 0; j < (int)items.size(); j++) {
    int iy = dropY + 2 + j * itemHeight_;
    Rect ir(dropX + 4, iy, dropW - 8, itemHeight_);
    paintItem(canvas, ir, items[j], j == hoveredItem_, 0);
  }

  // Open submenu: panel to the right of the parent item.
  if (openSubmenu_ >= 0 && openSubmenu_ < (int)items.size()) {
    auto &sub = items[openSubmenu_].submenu;
    if (!sub.empty()) {
      int subW = dropWidth(sub, 1) + 20;
      int subH = static_cast<int>(sub.size()) * itemHeight_ + 4;
      int subX = dropX + dropW - 4;
      int subY = dropY + 2 + openSubmenu_ * itemHeight_;
      canvas->setColor(st.bgColor);
      canvas->fillRoundedRect(Rect(subX, subY, subW, subH), 4);
      canvas->setColor(st.borderColor);
      canvas->strokeRoundedRect(Rect(subX, subY, subW, subH), 4);
      canvas->setFont(st.font);
      for (int k = 0; k < (int)sub.size(); k++) {
        Rect sr(subX + 4, subY + 2 + k * itemHeight_, subW - 8, itemHeight_);
        paintItem(canvas, sr, sub[k], k == hoveredSub_, 1);
      }
    }
  }
}

// --- Events ---

bool MenuBar::handleEvent(Event &event) {
  if (!isEnabled())
    return false;

  if (event.type == EventType::KeyDown && openMenu_ < 0) {
    // Down/Enter/Space opens the first menu (or the mouse-hovered one).
    if (event.key == Key::Down || event.key == Key::Enter ||
        event.key == Key::Space) {
      int idx = hoveredMenu_ >= 0 ? hoveredMenu_ : 0;
      if (idx < (int)menus_.size()) {
        openMenu_ = idx;
        hoveredItem_ = 0;
        keyboardNav_ = true;
        update();
        return true;
      }
    }
    return false;
  }

  if (event.type == EventType::KeyDown && openMenu_ >= 0) {
    keyboardNav_ = true;
    event.accepted = true;

    auto &items = menus_[openMenu_].items;
    auto &sub = (openSubmenu_ >= 0 && openSubmenu_ < (int)items.size())
                    ? items[openSubmenu_].submenu
                    : items; // sub or top-level
    auto moveHover = [&](int delta) {
      int limit = (int)sub.size();
      int &hovered = (openSubmenu_ >= 0) ? hoveredSub_ : hoveredItem_;
      int next = hovered + delta;
      while (next >= 0 && next < limit && sub[next].separator)
        next += delta;
      if (next >= 0 && next < limit && !sub[next].separator)
        hovered = next;
      update();
    };

    switch (event.key) {
    case Key::Escape:
      if (openSubmenu_ >= 0) {
        openSubmenu_ = -1;
        hoveredSub_ = -1;
      } else {
        closeMenu();
      }
      return true;
    case Key::Down:
      moveHover(1);
      return true;
    case Key::Up:
      moveHover(-1);
      return true;
    case Key::Right:
      if (openSubmenu_ < 0 && hoveredItem_ >= 0 &&
          hoveredItem_ < (int)items.size() &&
          !items[hoveredItem_].submenu.empty()) {
        // Open the submenu of the hovered item.
        openSubmenu_ = hoveredItem_;
        hoveredSub_ = 0;
      } else if (openSubmenu_ < 0 && openMenu_ < (int)menus_.size() - 1) {
        openMenu_++;
        hoveredItem_ = 0;
      }
      update();
      return true;
    case Key::Left:
      if (openSubmenu_ >= 0) {
        openSubmenu_ = -1;
        hoveredSub_ = -1;
      } else if (openMenu_ > 0) {
        openMenu_--;
        hoveredItem_ = 0;
      }
      update();
      return true;
    case Key::Enter:
    case Key::Space:
      if (openSubmenu_ >= 0 && hoveredSub_ >= 0 &&
          hoveredSub_ < (int)sub.size() && !sub[hoveredSub_].separator) {
        activateSubItem(openMenu_, openSubmenu_, hoveredSub_);
      } else if (hoveredItem_ >= 0 && hoveredItem_ < (int)items.size() &&
                 !items[hoveredItem_].separator) {
        activateItem(openMenu_, hoveredItem_);
      }
      return true;
    default:
      break;
    }
  }

  int localX = event.pos.x - x();
  int localY = event.pos.y - y();
  int menuIdx = hitTestMenu(localX, localY);
  int itemIdx = openMenu_ >= 0 ? hitTestItem(localY) : -1;

  switch (event.type) {
  case EventType::MouseMove:
    if (keyboardNav_) {
      keyboardNav_ = false;
      break;
    }
    hoveredMenu_ = menuIdx;
    if (openMenu_ >= 0)
      hoveredItem_ = itemIdx;
    if (menuIdx >= 0 && menuIdx != openMenu_ && openMenu_ >= 0) {
      openMenu_ = menuIdx;
      hoveredItem_ = -1;
    }
    update();
    event.accepted = (openMenu_ >= 0 || menuIdx >= 0);
    return event.accepted;
  case EventType::MouseDown:
    if (event.button != MouseButton::Left)
      return false;
    {
      bool handled = handleMouseDown(menuIdx, itemIdx);
      event.accepted = handled;
      return handled;
    }
  default:
    break;
  }
  return false;
}

// Shared activation logic for a menu entry. `group` is the sibling list
// used for radio exclusivity (the entry's own submenu list for submenu
// entries).
void MenuBar::activateMenuEntry(MenuItem &item, std::vector<MenuItem> *group) {
  if (item.separator)
    return;
  if (item.checkable) {
    item.checked = !item.checked;
    if (item.callback)
      item.callback();
  } else if (item.radio) {
    if (group) {
      for (auto &mi : *group) {
        if (mi.radio && mi.radioGroup == item.radioGroup)
          mi.checked = false;
      }
    }
    item.checked = true;
    if (item.callback)
      item.callback();
  } else if (item.callback) {
    item.callback();
  }
}

void MenuBar::activateItem(int menuIdx, int itemIdx) {
  if (menuIdx < 0 || menuIdx >= (int)menus_.size())
    return;
  auto &items = menus_[menuIdx].items;
  if (itemIdx < 0 || itemIdx >= (int)items.size())
    return;
  activateMenuEntry(items[itemIdx], &items);
  closeMenu();
}

void MenuBar::activateSubItem(int menuIdx, int itemIdx, int subIdx) {
  if (menuIdx < 0 || menuIdx >= (int)menus_.size())
    return;
  auto &items = menus_[menuIdx].items;
  if (itemIdx < 0 || itemIdx >= (int)items.size())
    return;
  auto &sub = items[itemIdx].submenu;
  if (subIdx < 0 || subIdx >= (int)sub.size())
    return;
  activateMenuEntry(sub[subIdx], &sub);
  closeMenu();
}

bool MenuBar::handleMouseDown(int menuIdx, int itemIdx) {
  if (openMenu_ >= 0) {
    if (itemIdx >= 0 && !menus_[openMenu_].items[itemIdx].separator) {
      activateItem(openMenu_, itemIdx);
    }
    closeMenu();
    return true;
  }

  if (menuIdx >= 0) {
    openMenu_ = menuIdx;
    hoveredItem_ = -1;
    update();
    return true;
  }

  closeMenu();
  return false;
}

void MenuBar::closeMenu() {
  openMenu_ = -1;
  hoveredItem_ = -1;
  openSubmenu_ = -1;
  hoveredSub_ = -1;
  keyboardNav_ = false;
  update();
}

} // namespace ltgui
