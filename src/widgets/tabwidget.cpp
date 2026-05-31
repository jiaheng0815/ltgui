#include "widgets/tabwidget.h"
#include "window.h"
#include "theme.h"
#include "platform/native_canvas.h"
#include <algorithm>

namespace ltgui {

TabWidget::TabWidget(Widget* parent) : Widget(parent) {
    style().bgColor = currentTheme().bgPrimary;
}

int TabWidget::addTab(const std::string& label) {
    Tab tab;
    tab.label = label;
    tab.content = new Widget(this);
    tab.content->style().bgColor = currentTheme().bgSecondary;
    tab.content->style().borderRadius = 4;
    tab.content->setVisible(false);

    tabs_.push_back(tab);
    if (current_ < 0) {
        setCurrentIndex(0);
    }
    invalidateSizeHint();
    update();
    return static_cast<int>(tabs_.size()) - 1;
}

void TabWidget::removeTab(int index) {
    if (index < 0 || index >= static_cast<int>(tabs_.size())) return;

    if (tabs_[index].content) {
        removeChild(tabs_[index].content);
        delete tabs_[index].content;
    }
    tabs_.erase(tabs_.begin() + index);

    if (current_ >= static_cast<int>(tabs_.size())) {
        current_ = static_cast<int>(tabs_.size()) - 1;
    }
    if (current_ >= 0 && current_ < static_cast<int>(tabs_.size())) {
        tabs_[current_].content->setVisible(true);
    }
    invalidateSizeHint();
    update();
}

int TabWidget::count() const { return static_cast<int>(tabs_.size()); }

void TabWidget::setCurrentIndex(int index) {
    if (index < 0 || index >= static_cast<int>(tabs_.size())) return;
    if (index == current_) return;

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

Widget* TabWidget::tabContent(int index) const {
    if (index >= 0 && index < static_cast<int>(tabs_.size())) {
        return tabs_[index].content;
    }
    return nullptr;
}

Widget* TabWidget::currentContent() const {
    return tabContent(current_);
}

Size TabWidget::sizeHint() const {
    if (!sizeHintDirty()) return cachedSizeHint();
    setCachedSizeHint({300, 200});
    return cachedSizeHint();
}

void TabWidget::setGeometry(const Rect& rect) {
    Widget::setGeometry(rect);
    // Re-layout active tab content to new size
    if (current_ >= 0 && current_ < static_cast<int>(tabs_.size())) {
        int cw = std::max(0, width() - 4);
        int ch = std::max(0, height() - tabBarHeight_ - 2);
        tabs_[current_].content->setGeometry(Rect(2, tabBarHeight_ + 2, cw, ch));
    }
}

int TabWidget::totalTabWidth() const {
    int w = 0;
    auto* win = window();
    auto* c = win ? win->canvas() : nullptr;
    for (auto& tab : tabs_) {
        if (c) {
            w += c->measureText(tab.label).width + 24 + 2;
        } else {
            w += 60;
        }
    }
    return w;
}

Rect TabWidget::tabRect(int index) const {
    Rect r = absoluteRect();
    auto* win = window();
    auto* c = win ? win->canvas() : nullptr;
    int startX = r.x + 2;
    for (int i = 0; i < index; i++) {
        Size ts = c ? c->measureText(tabs_[i].label) : Size{50, 0};
        startX += ts.width + 24 + 2;
    }
    Size ts = c ? c->measureText(tabs_[index].label) : Size{50, 0};
    return {startX, r.y + 2, ts.width + 24, tabBarHeight_ - 2};
}

void TabWidget::paintSelf(NativeCanvas* canvas) {
    Rect r = absoluteRect();
    Theme t = currentTheme();

    canvas->setColor(style().bgColor);
    canvas->fillRect(r);

    // Tab bar background
    canvas->setColor(t.bgTertiary);
    canvas->fillRoundedRect(Rect(r.x, r.y, r.width, tabBarHeight_), 6);

    // Tabs
    int x = r.x + 2;
    for (int i = 0; i < static_cast<int>(tabs_.size()); i++) {
        Size textSize = canvas->measureText(tabs_[i].label);
        int tw = textSize.width + 24;
        Rect tr(x, r.y + 2, tw, tabBarHeight_ - 2);

        if (i == current_) {
            canvas->setColor(t.bgSecondary);
            canvas->fillRoundedRect(tr, 4);
            canvas->setColor(t.accent);
            canvas->fillRect(Rect(tr.x, tr.bottom() - 2, tr.width, 2));
        } else if (i == hovered_) {
            canvas->setColor(Color(t.bgSecondary.r, t.bgSecondary.g, t.bgSecondary.b, 180));
            canvas->fillRoundedRect(tr, 4);
        }

        canvas->setColor(i == current_ ? t.accent : t.textSecondary);
        canvas->setFont(Font("Segoe UI", 12));
        canvas->drawText(tabs_[i].label, tr,
                         NativeCanvas::AlignCenter | NativeCanvas::AlignVCenter | NativeCanvas::SingleLine);

        x += tw + 2;
    }

    // Content area border
    if (current_ >= 0) {
        Rect contentArea(r.x + 1, r.y + tabBarHeight_ + 1, r.width - 2, r.height - tabBarHeight_ - 2);
        canvas->setColor(t.border);
        canvas->strokeRoundedRect(contentArea, 4);
    }
}

bool TabWidget::handleEvent(Event& event) {
    if (!isEnabled()) return false;

    int localX = event.pos.x - x();
    int localY = event.pos.y - y();

    if (localY >= 0 && localY <= tabBarHeight_) {
        int xcursor = 2;
        for (int i = 0; i < static_cast<int>(tabs_.size()); i++) {
            auto* win = window();
            auto* c = win ? win->canvas() : nullptr;
            Size textSize = c ? c->measureText(tabs_[i].label) : Size{50, 0};
            int tw = textSize.width + 24;
            if (localX >= xcursor && localX < xcursor + tw) {
                if (event.type == EventType::MouseMove) {
                    hovered_ = i;
                    update();
                    event.accepted = true;
                    return true;
                }
                if (event.type == EventType::MouseDown && event.button == MouseButton::Left) {
                    setCurrentIndex(i);
                    event.accepted = true;
                    return true;
                }
            }
            xcursor += tw + 2;
        }
        if (event.type == EventType::MouseMove && hovered_ >= 0) {
            hovered_ = -1;
            update();
        }
        return false;
    }

    // Forward events in content area to tab content children
    return Widget::handleEvent(event);
}

} // namespace ltgui
