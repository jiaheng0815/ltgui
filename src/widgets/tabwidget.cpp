#include "widgets/tabwidget.h"
#include "platform/native_canvas.h"
#include "theme.h"
#include "window.h"
#include <algorithm>

namespace ltgui {

TabWidget::TabWidget(Widget *parent) : Widget(parent) {}

int TabWidget::addTab(const std::string &label) {
  Tab tab;
  tab.label = label;
  tab.content = addChild(std::make_unique<Widget>());
  tab.content->style().borderRadius = 4;
  tab.content->setVisible(false);

  tabs_.push_back(tab);
  if (current_ < 0) {
    setCurrentIndex(0);
  }
  invalidateSizeHint();
  invalidateTabWidths();
  update();
  return static_cast<int>(tabs_.size()) - 1;
}

void TabWidget::removeTab(int index) {
  if (index < 0 || index >= static_cast<int>(tabs_.size()))
    return;

  if (tabs_[index].content) {
    removeChild(tabs_[index].content); // destroys the content widget
    tabs_[index].content = nullptr;
  }
  tabs_.erase(tabs_.begin() + index);

  if (current_ >= static_cast<int>(tabs_.size())) {
    current_ = static_cast<int>(tabs_.size()) - 1;
  }
  if (current_ >= 0 && current_ < static_cast<int>(tabs_.size())) {
    tabs_[current_].content->setVisible(true);
  }
  invalidateSizeHint();
  invalidateTabWidths();
  update();
}

int TabWidget::count() const { return static_cast<int>(tabs_.size()); }

void TabWidget::setCurrentIndex(int index) {
  if (index < 0 || index >= static_cast<int>(tabs_.size()))
    return;
  if (index == current_)
    return;

  if (current_ >= 0 && current_ < static_cast<int>(tabs_.size())) {
    tabs_[current_].content->setVisible(false);
  }
  current_ = index;
  tabs_[current_].content->setVisible(true);

  // Children geometry is relative to this widget
  int cw = width() - 4;
  int ch = height() - tabBarHeight_ - 2;
  tabs_[current_].content->setGeometry(Rect(2, tabBarHeight_ + 2, cw, ch));

  update();
}

Widget *TabWidget::tabContent(int index) const {
  if (index >= 0 && index < static_cast<int>(tabs_.size())) {
    return tabs_[index].content;
  }
  return nullptr;
}

Widget *TabWidget::currentContent() const { return tabContent(current_); }

Size TabWidget::sizeHint() const {
  if (!sizeHintDirty())
    return cachedSizeHint();
  setCachedSizeHint(dpiScaleSize(300, 200));
  return cachedSizeHint();
}

void TabWidget::setGeometry(const Rect &rect) {
  Widget::setGeometry(rect);
  // Re-layout active tab content to new size
  if (current_ >= 0 && current_ < static_cast<int>(tabs_.size())) {
    int cw = std::max(0, width() - 4);
    int ch = std::max(0, height() - tabBarHeight_ - 2);
    tabs_[current_].content->setGeometry(Rect(2, tabBarHeight_ + 2, cw, ch));
  }
}

void TabWidget::ensureTabWidths() const {
  if (!tabWidthsDirty_)
    return;
  cachedTabWidths_.clear();
  cachedTabWidths_.reserve(tabs_.size());
  auto *win = window();
  auto *c = win ? win->canvas() : nullptr;
  if (c)
    c->setFont(style().font);
  for (auto &tab : tabs_) {
    cachedTabWidths_.push_back(c ? c->measureText(tab.label).width + 24 + 2
                                 : 62);
  }
  tabWidthsDirty_ = false;
}

int TabWidget::totalTabWidth() const {
  ensureTabWidths();
  int w = 0;
  for (int v : cachedTabWidths_)
    w += v;
  return w;
}

Rect TabWidget::tabRect(int index) const {
  ensureTabWidths();
  Rect r = absoluteRect();
  int startX = r.x + 2;
  for (int i = 0; i < index; i++) {
    startX += cachedTabWidths_[i];
  }
  return {startX, r.y + 2, cachedTabWidths_[index] - 2, tabBarHeight_ - 2};
}

void TabWidget::paintSelf(NativeCanvas *canvas) {
  Rect r = absoluteRect();
  ResolvedStyle st = resolvedStyle();
  const Theme &t = currentTheme();

  paintBackground(canvas);

  // Tab bar background
  canvas->setColor(t.bgTertiary);
  canvas->fillRoundedRect(Rect(r.x, r.y, r.width, tabBarHeight_), 6);

  // Tabs
  ensureTabWidths();
  int x = r.x + 2;
  for (int i = 0; i < static_cast<int>(tabs_.size()); i++) {
    int tw = cachedTabWidths_[i] - 2;
    Rect tr(x, r.y + 2, tw, tabBarHeight_ - 2);

    if (i == current_) {
      canvas->setColor(st.bgColor);
      canvas->fillRoundedRect(tr, 4);
      canvas->setColor(st.accent);
      canvas->fillRect(Rect(tr.x, tr.bottom() - 2, tr.width, 2));
    } else if (i == hoveredTab_) {
      canvas->setColor(Color(st.bgColor.r, st.bgColor.g, st.bgColor.b, 180));
      canvas->fillRoundedRect(tr, 4);
    }

    canvas->setColor(i == current_ ? st.accent : t.textSecondary);
    canvas->setFont(st.font);
    canvas->drawText(tabs_[i].label, tr,
                     NativeCanvas::AlignCenter | NativeCanvas::AlignVCenter |
                         NativeCanvas::SingleLine);

    x += cachedTabWidths_[i];
  }

  // Content area border
  if (current_ >= 0) {
    Rect contentArea(r.x + 1, r.y + tabBarHeight_ + 1, r.width - 2,
                     r.height - tabBarHeight_ - 2);
    canvas->setColor(st.borderColor);
    canvas->strokeRoundedRect(contentArea, 4);
  }
}

bool TabWidget::handleEvent(Event &event) {
  if (!isEnabled())
    return false;

  int localX = event.pos.x - x();
  int localY = event.pos.y - y();

  if (localY >= 0 && localY <= tabBarHeight_) {
    ensureTabWidths();
    int xcursor = 2;
    for (int i = 0; i < static_cast<int>(tabs_.size()); i++) {
      int tw = cachedTabWidths_[i] - 2;
      if (localX >= xcursor && localX < xcursor + tw) {
        if (event.type == EventType::MouseMove) {
          hoveredTab_ = i;
          update();
          event.accepted = true;
          return true;
        }
        if (event.type == EventType::MouseDown &&
            event.button == MouseButton::Left) {
          setCurrentIndex(i);
          event.accepted = true;
          return true;
        }
      }
      xcursor += cachedTabWidths_[i];
    }
    if (event.type == EventType::MouseMove && hoveredTab_ >= 0) {
      hoveredTab_ = -1;
      update();
    }
    return false;
  }

  // Forward events in content area to tab content children
  return Widget::handleEvent(event);
}

} // namespace ltgui
