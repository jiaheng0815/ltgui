#include "layout.h"
#include "widget.h"
#include <algorithm>

namespace ltgui {

// BoxLayout

BoxLayout::BoxLayout(Direction dir, int spacing, int margin)
    : direction_(dir), spacing_(spacing), margin_(margin) {}

void BoxLayout::addStretch(int factor) {
    stretchFactors_.push_back(factor);
}

void BoxLayout::setSpacing(int spacing) {
    spacing_ = spacing;
}

void BoxLayout::setMargin(int margin) {
    margin_ = margin;
}

void BoxLayout::setDirection(Direction dir) {
    direction_ = dir;
}

void BoxLayout::layout(Widget* container) {
    if (!container) return;

    const auto& children = container->children();
    int n = static_cast<int>(children.size());
    if (n == 0) return;

    Rect area = container->geometry();
    int availW = area.width - 2 * margin_;
    int availH = area.height - 2 * margin_;

    if (availW <= 0 || availH <= 0) return;

    // Count visible children and their total size hints
    int visibleCount = 0;
    int totalHint = 0;
    int totalStretch = 0;

    for (int i = 0; i < n; i++) {
        Widget* child = children[i].get();
        if (!child->isVisible()) continue;
        visibleCount++;

        Size hint = child->sizeHint();
        if (direction_ == LeftToRight) {
            totalHint += hint.width;
        } else {
            totalHint += hint.height;
        }

        if (i < static_cast<int>(stretchFactors_.size())) {
            totalStretch += stretchFactors_[i];
        }
    }

    if (visibleCount == 0) return;

    // Calculate spacing
    int totalSpacing = spacing_ * (visibleCount - 1);

    // Available space after accounting for size hints and spacing
    int remaining = (direction_ == LeftToRight ? availW : availH) - totalHint - totalSpacing;
    if (remaining < 0) remaining = 0;

    // Layout children — positions are relative to container
    int pos = margin_;

    for (int i = 0; i < n; i++) {
        Widget* child = children[i].get();
        if (!child->isVisible()) continue;

        Size hint = child->sizeHint();
        int stretch = 0;
        if (i < static_cast<int>(stretchFactors_.size())) {
            stretch = stretchFactors_[i];
        }

        int childW, childH;

        if (direction_ == LeftToRight) {
            childW = hint.width;
            if (totalStretch > 0 && stretch > 0) {
                childW += remaining * stretch / totalStretch;
            }
            childH = availH;
            child->setGeometry(Rect(pos, margin_, childW, childH));
            pos += childW + spacing_;
        } else {
            childW = availW;
            childH = hint.height;
            if (totalStretch > 0 && stretch > 0) {
                childH += remaining * stretch / totalStretch;
            }
            child->setGeometry(Rect(margin_, pos, childW, childH));
            pos += childH + spacing_;
        }
    }
}

Size BoxLayout::preferredSize(const Widget* container) const {
    if (!container) return {0, 0};

    const auto& children = container->children();
    int totalW = 0, totalH = 0;
    int maxW = 0, maxH = 0;
    int visibleCount = 0;

    for (auto& child : children) {
        if (!child->isVisible()) continue;
        visibleCount++;
        Size hint = child->sizeHint();
        if (direction_ == LeftToRight) {
            totalW += hint.width;
            maxH = std::max(maxH, hint.height);
        } else {
            maxW = std::max(maxW, hint.width);
            totalH += hint.height;
        }
    }

    int totalSpacing = spacing_ * std::max(0, visibleCount - 1);

    if (direction_ == LeftToRight) {
        totalW += totalSpacing;
        totalH = maxH;
    } else {
        totalH += totalSpacing;
        totalW = maxW;
    }

    totalW += 2 * margin_;
    totalH += 2 * margin_;
    return {totalW, totalH};
}

// GridLayout

GridLayout::GridLayout(int cols, int rowSpacing, int colSpacing, int margin)
    : cols_(cols), rowSpacing_(rowSpacing), colSpacing_(colSpacing), margin_(margin) {}

void GridLayout::setColumnStretch(int col, int factor) {
    colStretch_[col] = factor;
}

void GridLayout::setRowStretch(int row, int factor) {
    rowStretch_[row] = factor;
}

void GridLayout::layout(Widget* container) {
    if (!container || cols_ <= 0) return;

    const auto& children = container->children();
    int n = static_cast<int>(children.size());
    if (n == 0) return;

    Rect area = container->geometry();
    int availW = area.width - 2 * margin_;
    int availH = area.height - 2 * margin_;

    if (availW <= 0 || availH <= 0) return;

    int rows = (n + cols_ - 1) / cols_;

    // Calculate column widths
    int totalColSpacing = colSpacing_ * (cols_ - 1);
    int colWidth = (availW - totalColSpacing) / cols_;

    // Calculate row heights
    int totalRowSpacing = rowSpacing_ * (rows - 1);
    int rowHeight = (availH - totalRowSpacing) / rows;

    int x = margin_;
    int y = margin_;

    for (int i = 0; i < n; i++) {
        int col = i % cols_;
        int row = i / cols_;

        int cx = x + col * (colWidth + colSpacing_);
        int cy = y + row * (rowHeight + rowSpacing_);

        // Apply column/row stretches if set
        int extraW = 0, extraH = 0;
        auto cs = colStretch_.find(col);
        if (cs != colStretch_.end() && cs->second > 0) {
            extraW = cs->second * 10; // Simple stretch
        }

        children[i]->setGeometry(Rect(cx, cy, colWidth + extraW, rowHeight + extraH));
    }
}

Size GridLayout::preferredSize(const Widget* container) const {
    if (!container) return {0, 0};

    const auto& children = container->children();
    int n = static_cast<int>(children.size());
    if (n == 0) return {0, 0};

    int maxColW = 0, maxRowH = 0;
    int rows = (n + cols_ - 1) / cols_;

    for (int i = 0; i < n; i++) {
        Size hint = children[i]->sizeHint();
        maxColW = std::max(maxColW, hint.width);
        maxRowH = std::max(maxRowH, hint.height);
    }

    int totalW = cols_ * maxColW + (cols_ - 1) * colSpacing_ + 2 * margin_;
    int totalH = rows * maxRowH + (rows - 1) * rowSpacing_ + 2 * margin_;

    return {totalW, totalH};
}

} // namespace ltgui
