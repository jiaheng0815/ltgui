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

void BoxLayout::setStretch(Widget* child, int factor) {
    if (child && factor > 0) {
        widgetStretch_[child] = factor;
    } else if (child) {
        widgetStretch_.erase(child);
    }
}

int BoxLayout::stretch(Widget* child) const {
    if (!child) return 0;
    auto it = widgetStretch_.find(child);
    return (it != widgetStretch_.end()) ? it->second : 0;
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
    if (n == 0) {
        widgetStretch_.clear();
        return;
    }

    // Purge stale widgetStretch_ entries for children no longer in this
    // container.  This prevents unbounded map growth and ABA problems where
    // a new Widget allocated at a recycled pointer address would incorrectly
    // inherit the old widget's stretch factor.
    for (auto it = widgetStretch_.begin(); it != widgetStretch_.end(); ) {
        bool alive = false;
        for (int i = 0; i < n && !alive; i++) {
            if (children[i].get() == it->first) alive = true;
        }
        if (alive) ++it;
        else       it = widgetStretch_.erase(it);
    }

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

        // Prefer widget-linked stretch; fall back to positional stretch
        auto it = widgetStretch_.find(child);
        if (it != widgetStretch_.end()) {
            totalStretch += it->second;
        } else if (i < static_cast<int>(stretchFactors_.size())) {
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
        // Prefer widget-linked stretch over positional
        auto sit = widgetStretch_.find(child);
        if (sit != widgetStretch_.end()) {
            stretch = sit->second;
        } else if (i < static_cast<int>(stretchFactors_.size())) {
            stretch = stretchFactors_[i];
        }

        int childW, childH;

        if (direction_ == LeftToRight) {
            childW = hint.width;
            if (totalStretch > 0 && stretch > 0) {
                childW += static_cast<int>(static_cast<int64_t>(remaining) * stretch / totalStretch);
            }
            childH = availH;
            child->setGeometry(Rect(pos, margin_, childW, childH));
            pos += childW + spacing_;
        } else {
            childW = availW;
            childH = hint.height;
            if (totalStretch > 0 && stretch > 0) {
                childH += static_cast<int>(static_cast<int64_t>(remaining) * stretch / totalStretch);
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

    // Compute per-column max width from content sizeHints (skip invisible)
    std::vector<int> colWidths(cols_, 0);
    std::vector<int> rowHeights(rows, 0);
    for (int i = 0; i < n; i++) {
        if (!children[i]->isVisible()) continue;
        int col = i % cols_;
        int row = i / cols_;
        Size hint = children[i]->sizeHint();
        colWidths[col] = std::max(colWidths[col], hint.width);
        rowHeights[row] = std::max(rowHeights[row], hint.height);
    }

    // Distribute remaining horizontal space proportionally to stretch factors.
    // Use int64_t for multiplication to avoid overflow with large stretch values.
    int totalColSpacing = colSpacing_ * (cols_ - 1);
    int64_t usedColW = 0;
    for (int w : colWidths) usedColW += w;
    int64_t extraW = std::max<int64_t>(0, static_cast<int64_t>(availW) - totalColSpacing - usedColW);
    int totalColStretch = 0;
    for (int c = 0; c < cols_; c++) {
        auto it = colStretch_.find(c);
        if (it != colStretch_.end()) totalColStretch += it->second;
    }
    if (totalColStretch > 0 && extraW > 0) {
        int64_t distributed = 0;
        for (int c = 0; c < cols_; c++) {
            auto it = colStretch_.find(c);
            if (it != colStretch_.end() && it->second > 0) {
                int64_t add = extraW * it->second / totalColStretch;
                colWidths[c] += static_cast<int>(add);
                distributed += add;
            }
        }
        // Distribute remainder (lost to integer truncation) to columns
        // with highest stretch, left-to-right, one pixel each.
        int64_t remainder = extraW - distributed;
        for (int c = 0; c < cols_ && remainder > 0; c++) {
            auto it = colStretch_.find(c);
            if (it != colStretch_.end() && it->second > 0) {
                colWidths[c] += 1;
                remainder--;
            }
        }
    }

    // Distribute remaining vertical space.
    // Use int64_t for multiplication to avoid overflow with large stretch values.
    int totalRowSpacing = rowSpacing_ * (rows - 1);
    int64_t usedRowH = 0;
    for (int h : rowHeights) usedRowH += h;
    int64_t extraH = std::max<int64_t>(0, static_cast<int64_t>(availH) - totalRowSpacing - usedRowH);
    int totalRowStretch = 0;
    for (int r = 0; r < rows; r++) {
        auto it = rowStretch_.find(r);
        if (it != rowStretch_.end()) totalRowStretch += it->second;
    }
    if (totalRowStretch > 0 && extraH > 0) {
        int64_t distributed = 0;
        for (int r = 0; r < rows; r++) {
            auto it = rowStretch_.find(r);
            if (it != rowStretch_.end() && it->second > 0) {
                int64_t add = extraH * it->second / totalRowStretch;
                rowHeights[r] += static_cast<int>(add);
                distributed += add;
            }
        }
        int64_t remainder = extraH - distributed;
        for (int r = 0; r < rows && remainder > 0; r++) {
            auto it = rowStretch_.find(r);
            if (it != rowStretch_.end() && it->second > 0) {
                rowHeights[r] += 1;
                remainder--;
            }
        }
    }

    // Precompute prefix-sum X offsets for O(1) per-child positioning.
    std::vector<int> colOffsets(cols_ + 1, margin_);
    for (int c = 0; c < cols_; c++) {
        colOffsets[c + 1] = colOffsets[c] + colWidths[c] + colSpacing_;
    }

    // Precompute prefix-sum Y offsets.
    std::vector<int> rowOffsets(rows + 1, margin_);
    for (int r = 0; r < rows; r++) {
        rowOffsets[r + 1] = rowOffsets[r] + rowHeights[r] + rowSpacing_;
    }

    // Position children (skip invisible)
    for (int i = 0; i < n; i++) {
        if (!children[i]->isVisible()) continue;
        int col = i % cols_;
        int row = i / cols_;
        children[i]->setGeometry(Rect(colOffsets[col], rowOffsets[row],
                                      colWidths[col], rowHeights[row]));
    }
}

Size GridLayout::preferredSize(const Widget* container) const {
    if (!container || cols_ <= 0) return {0, 0};

    const auto& children = container->children();
    int n = static_cast<int>(children.size());
    if (n == 0) return {0, 0};

    int rows = (n + cols_ - 1) / cols_;

    // Per-column max width, per-row max height (skip invisible)
    std::vector<int> colWidths(cols_, 0);
    std::vector<int> rowHeights(rows, 0);
    for (int i = 0; i < n; i++) {
        if (!children[i]->isVisible()) continue;
        int col = i % cols_;
        int row = i / cols_;
        Size hint = children[i]->sizeHint();
        colWidths[col] = std::max(colWidths[col], hint.width);
        rowHeights[row] = std::max(rowHeights[row], hint.height);
    }

    int totalW = 0;
    for (int w : colWidths) totalW += w;
    totalW += (cols_ - 1) * colSpacing_ + 2 * margin_;

    int totalH = 0;
    for (int h : rowHeights) totalH += h;
    totalH += (rows - 1) * rowSpacing_ + 2 * margin_;

    return {totalW, totalH};
}

} // namespace ltgui
