#include "widgets/treeview.h"
#include "theme.h"
#include "platform/native_canvas.h"
#include <algorithm>
#include <functional>

namespace ltgui {

TreeViewItem::TreeViewItem(const std::string& text) : text_(text) {}

TreeViewItem::~TreeViewItem() = default;

void TreeViewItem::setText(const std::string& text) { text_ = text; }
void TreeViewItem::setExpanded(bool expanded) { expanded_ = expanded; }

TreeViewItem* TreeViewItem::addChild(const std::string& text) {
    auto child = std::make_unique<TreeViewItem>(text);
    child->parent_ = this;
    child->depth_ = depth_ + 1;
    child->treeView_ = treeView_;
    TreeViewItem* raw = child.get();
    children_.push_back(std::move(child));
    return raw;
}

void TreeViewItem::removeChild(int index) {
    if (index >= 0 && index < static_cast<int>(children_.size())) {
        if (treeView_ && treeView_->selectedItem() == children_[index].get()) {
            treeView_->setSelectedItem(nullptr);
        }
        children_.erase(children_.begin() + index); // destroys the child
    }
}

// --- TreeView ---

TreeView::TreeView(Widget* parent) : Widget(parent) {
    style().bgColor = currentTheme().bgSecondary;
    style().fgColor = currentTheme().textPrimary;
    style().borderWidth = 1;
    style().borderColor = currentTheme().border;
    style().borderRadius = 4;
    root_ = std::make_unique<TreeViewItem>("");
    root_->expanded_ = true;
    root_->treeView_ = this;
}

TreeView::~TreeView() = default;

TreeViewItem* TreeView::rootItem() { return root_.get(); }

void TreeView::setSelectedItem(TreeViewItem* item) {
    selected_ = item;
    update();
    if (selectionCallback_) selectionCallback_(item);
}

Size TreeView::sizeHint() const {
    if (!sizeHintDirty()) return cachedSizeHint();
    setCachedSizeHint({200, 200});
    return cachedSizeHint();
}

int TreeView::visibleItems() const { return std::max(1, (height() - 2) / itemHeight_); }

int TreeView::countVisible(TreeViewItem* item) const {
    int count = 1;
    if (item->expanded_) {
        for (auto& child : item->children_) count += countVisible(child.get());
    }
    return count;
}

int TreeView::totalRows() const {
    return root_ ? countVisible(root_.get()) : 0;
}

void TreeView::paintSelf(NativeCanvas* canvas) {
    Rect r = absoluteRect();
    Theme t = currentTheme();

    canvas->setColor(style().bgColor);
    canvas->fillRoundedRect(r, style().borderRadius);

    if (style().borderWidth > 0) {
        canvas->setColor(style().borderColor);
        canvas->strokeRoundedRect(r, style().borderRadius, style().borderWidth);
    }

    if (!root_) return;

    canvas->setFont(Font::systemDefault(12));
    int visible = visibleItems();
    int scrollOffset = static_cast<int>(scrollAnim_.value());
    int total = totalRows();
    int maxScroll = std::max(0, total - visible);
    if (scrollOffset > maxScroll) scrollOffset = maxScroll;

    // Recursively draw visible tree items
    int rowCounter = 0;
    std::function<void(TreeViewItem*, int)> drawItem;
    drawItem = [&](TreeViewItem* item, int depth) {
        int relRow = rowCounter - scrollOffset;
        rowCounter++;

        if (relRow < 0 || relRow >= visible) {
            if (item->expanded_) {
                for (auto& child : item->children_) drawItem(child.get(), depth + 1);
            }
            return;
        }

        int y = r.y + 1 + relRow * itemHeight_;
        Rect itemRect(r.x + 1, y, r.width - 2, itemHeight_);

        if (item == selected_) {
            canvas->setColor(t.accent);
            canvas->fillRoundedRect(itemRect.adjusted(1, 1, -1, -1), 3);
            canvas->setColor(Color::White);
        }

        int x = r.x + 4 + depth * indentWidth_;

        // Expand/collapse arrow
        bool hasChildren = !item->children_.empty();
        if (hasChildren) {
            int ax = x;
            int ay = y + itemHeight_ / 2;
            canvas->setColor(t.textSecondary);
            if (item->expanded_) {
                // Down triangle
                canvas->drawLine({ax, ay - 3}, {ax + 6, ay - 3}, 2);
                canvas->drawLine({ax + 3, ay - 2}, {ax + 3, ay + 3}, 2);
            } else {
                // Right triangle
                canvas->drawLine({ax, ay - 4}, {ax, ay + 5}, 2);
                canvas->drawLine({ax + 2, ay - 2}, {ax + 2, ay + 3}, 2);
                canvas->drawLine({ax + 4, ay}, {ax + 4, ay + 1}, 2);
            }
            x += 14;
        }

        canvas->setColor(item == selected_ ? Color::White : style().fgColor);
        Rect textRect(x, y, r.right() - x - 2, itemHeight_);
        canvas->drawText(item->text_, textRect,
                         NativeCanvas::AlignLeft | NativeCanvas::AlignVCenter | NativeCanvas::SingleLine);

        if (item->expanded_) {
            for (auto& child : item->children_) drawItem(child.get(), depth + 1);
        }
    };

    drawItem(root_.get(), 0);
}

bool TreeView::handleEvent(Event& event) {
    if (!isEnabled()) return false;

    int localY = event.pos.y - y() - 1;
    int visible = visibleItems();
    int scrollOffset = static_cast<int>(scrollAnim_.value());

    if (event.type == EventType::MouseDown && event.button == MouseButton::Left) {
        int targetRow = scrollOffset + localY / itemHeight_;

        int currentRow = 0;
        TreeViewItem* clicked = nullptr;
        int clickDepth = 0;

        std::function<void(TreeViewItem*, int)> findItem;
        findItem = [&](TreeViewItem* item, int depth) {
            if (clicked) return;
            if (currentRow == targetRow) {
                clicked = item;
                clickDepth = depth;
                return;
            }
            currentRow++;
            if (item->expanded_) {
                for (auto& child : item->children_) findItem(child.get(), depth + 1);
            }
        };
        findItem(root_.get(), 0);

        if (clicked) {
            int clickX = event.pos.x - x() - 4 - clickDepth * indentWidth_;
            bool onArrow = !clicked->children_.empty() && clickX >= 0 && clickX < 14;
            if (onArrow) {
                clicked->setExpanded(!clicked->expanded_);
                update();
            } else {
                setSelectedItem(clicked);
            }
        }
        event.accepted = true;
        return true;
    }

    if (event.type == EventType::MouseWheel) {
        int total = totalRows();
        int maxScroll = std::max(0, total - visible);
        scrollTarget_ = std::max(0, std::min(maxScroll, scrollTarget_ - event.wheelDelta));
        scrollAnim_.setTarget(static_cast<float>(scrollTarget_), 200, Easing::EaseOut);
        update();
        event.accepted = true;
        return true;
    }

    return false;
}

} // namespace ltgui
