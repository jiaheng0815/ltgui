#include "widgets/tableview.h"
#include "platform/native_canvas.h"
#include "platform/native_window.h"
#include "theme.h"
#include "window.h"
#include <algorithm>

namespace ltgui {

// --- SimpleTableModel ---

SimpleTableModel::SimpleTableModel(int rows, int cols) : colCount_(cols) {
  data_.resize(rows, std::vector<std::string>(cols));
}

std::string SimpleTableModel::cellText(int row, int col) const {
  if (row < 0 || row >= (int)data_.size())
    return {};
  if (col < 0 || col >= colCount_)
    return {};
  return data_[row][col];
}

void SimpleTableModel::setCellText(int row, int col, const std::string &text) {
  if (row < 0 || row >= (int)data_.size())
    return;
  if (col < 0 || col >= colCount_)
    return;
  data_[row][col] = text;
}

void SimpleTableModel::addRow(const std::vector<std::string> &cells) {
  data_.push_back({});
  auto &row = data_.back();
  for (auto &c : cells)
    row.push_back(c);
  row.resize(colCount_);
}

void SimpleTableModel::removeRow(int row) {
  if (row < 0 || row >= (int)data_.size())
    return;
  data_.erase(data_.begin() + row);
}

void SimpleTableModel::clear() { data_.clear(); }

void SimpleTableModel::sort(int column, bool ascending) {
  if (column < 0 || column >= colCount_)
    return;
  std::stable_sort(data_.begin(), data_.end(),
                   [column, ascending](const auto &a, const auto &b) {
                     if (column >= (int)a.size() || column >= (int)b.size())
                       return false;
                     int cmp = a[column].compare(b[column]);
                     return ascending ? (cmp < 0) : (cmp > 0);
                   });
}

// --- TableView ---

TableView::TableView(Widget *parent) : Widget(parent) {}

void TableView::setModel(std::shared_ptr<TableModel> model) {
  model_ = std::move(model);
  update();
}

void TableView::addColumn(const TableColumn &col) {
  columns_.push_back(col);
  update();
}

void TableView::setColumnWidth(int col, int width) {
  if (col >= 0 && col < (int)columns_.size()) {
    columns_[col].width = std::max(columns_[col].minWidth, width);
    update();
  }
}

void TableView::setCurrentIndex(int row) {
  selectedRow_ = row;
  selectedRows_ = {row};
  onRowSelected.emit(row);
  update();
}

void TableView::clearSelection() {
  selectedRow_ = -1;
  selectedRows_.clear();
  update();
}

void TableView::setSortColumn(int col, bool ascending) {
  sortedCol_ = col;
  sortAsc_ = ascending;
  update();
}

Size TableView::sizeHint() const {
  if (!sizeHintDirty())
    return cachedSizeHint();
  int w = totalWidth() + 2;
  int rows = model_ ? model_->rowCount() : 0;
  int h = headerHeight_ + rows * rowHeight_ + 2;
  setCachedSizeHint({w, h});
  return cachedSizeHint();
}

int TableView::visibleRows() const {
  int h = height() - headerHeight_;
  if (h <= 0)
    return 0;
  return h / rowHeight_;
}

int TableView::totalWidth() const {
  int w = 0;
  for (auto &c : columns_)
    w += c.width;
  return w;
}

int TableView::colX(int idx) const {
  int x = 0;
  for (int i = 0; i < idx; i++)
    x += columns_[i].width;
  return x;
}

int TableView::hitTestCol(int localX) const {
  int x = 0;
  for (int i = 0; i < (int)columns_.size(); i++) {
    if (localX >= x && localX < x + columns_[i].width)
      return i;
    x += columns_[i].width;
  }
  return -1;
}

int TableView::hitTestRow(int localY) const {
  int ry = localY - headerHeight_;
  if (ry < 0)
    return -1;
  return (ry + scrollY_ * rowHeight_) / rowHeight_;
}

// --- Paint ---

void TableView::paintSelf(NativeCanvas *canvas) {
  Rect r = absoluteRect();
  ResolvedStyle st = resolvedStyle();
  const Theme &t = currentTheme();
  paintBackground(canvas);

  int numRows = model_ ? model_->rowCount() : 0;
  int numCols = (int)columns_.size();
  int firstVis = scrollY_;
  int visCount = visibleRows();
  if (visCount <= 0)
    visCount = std::min(numRows, 20);

  // Header
  int headerY = r.y;
  canvas->setColor(t.tableHeaderBg);
  canvas->fillRoundedRect(Rect(r.x, headerY, totalWidth(), headerHeight_), 4);
  canvas->setColor(t.textPrimary);
  canvas->setFont(st.font);

  for (int c = 0; c < numCols; c++) {
    int cx = r.x + colX(c);
    int cw = columns_[c].width;
    Rect hdr(cx, headerY, cw, headerHeight_);
    std::string title = columns_[c].title;
    if (c == sortedCol_)
      title += (sortAsc_ ? " ▲" : " ▼");
    canvas->drawText(title, hdr.adjusted(4, 0, -4, 0),
                     NativeCanvas::AlignLeft | NativeCanvas::AlignVCenter);
    // Column separator
    if (c < numCols - 1) {
      canvas->setColor(t.tableBorder);
      canvas->drawLine({cx + cw - 1, headerY + 2},
                       {cx + cw - 1, headerY + headerHeight_ - 4}, 1);
      canvas->setColor(t.textPrimary);
    }
  }

  // Rows
  for (int i = firstVis; i < std::min(numRows, firstVis + visCount); i++) {
    int ry = r.y + headerHeight_ + (i - firstVis) * rowHeight_;
    Rect rowRect(r.x, ry, totalWidth(), rowHeight_);

    // Selection or alternating row color
    bool sel = (i == selectedRow_ ||
                std::find(selectedRows_.begin(), selectedRows_.end(), i) !=
                    selectedRows_.end());
    if (sel) {
      canvas->setColor(t.selectionBg);
      canvas->fillRect(rowRect);
      canvas->setColor(Color::White);
    } else {
      if (i % 2 == 1) {
        canvas->setColor(t.tableRowAlt);
        canvas->fillRect(rowRect);
      }
      canvas->setColor(t.textPrimary);
    }
    canvas->setFont(st.font);

    for (int c = 0; c < numCols; c++) {
      int cx = r.x + colX(c);
      std::string text = model_->cellText(i, c);
      // Skip rows with column mismatches
      Rect cellRect(cx, ry, columns_[c].width, rowHeight_);
      canvas->drawText(text, cellRect.adjusted(4, 0, -4, 0),
                       NativeCanvas::AlignLeft | NativeCanvas::AlignVCenter |
                           NativeCanvas::SingleLine);
    }
  }
}

// --- Events ---

bool TableView::handleEvent(Event &event) {
  if (!isEnabled())
    return false;

  int localX = event.pos.x - x();
  int localY = event.pos.y - y();

  switch (event.type) {
  case EventType::MouseDown:
    if (event.button != MouseButton::Left)
      return false;

    // Check column resize handle
    for (int c = 0; c < (int)columns_.size() - 1; c++) {
      int edge = colX(c) + columns_[c].width;
      if (std::abs(localX - edge) < 4 && columns_[c].resizable) {
        resizingCol_ = c;
        resizeStartX_ = localX;
        resizeStartW_ = columns_[c].width;
        if (auto *win = window())
          win->setCursor(CursorShape::SizeWE);
        event.accepted = true;
        return true;
      }
    }

    // Header click → sort
    if (localY < headerHeight_) {
      int col = hitTestCol(localX);
      if (col >= 0 && columns_[col].sortable) {
        bool asc = (sortedCol_ == col) ? !sortAsc_ : true;
        setSortColumn(col, asc);
        if (model_) {
          model_->sort(col, asc);
        }
        onHeaderClicked.emit(col);
        update();
      }
      event.accepted = true;
      return true;
    }

    // Row click → select
    {
      int row =
          (int)((localY - headerHeight_ + (int64_t)scrollY_ * rowHeight_) /
                rowHeight_);
      if (row >= 0 && model_ && row < model_->rowCount()) {
        setCurrentIndex(row);
        update();
        event.accepted = true;
        return true;
      }
    }
    break;

  case EventType::MouseMove:
    if (resizingCol_ >= 0) {
      int delta = localX - resizeStartX_;
      int newW =
          std::max(columns_[resizingCol_].minWidth, resizeStartW_ + delta);
      columns_[resizingCol_].width = newW;
      update();
      event.accepted = true;
      return true;
    }
    break;

  case EventType::MouseUp:
    if (resizingCol_ >= 0) {
      resizingCol_ = -1;
      if (auto *win = window())
        win->setCursor(CursorShape::Arrow);
      event.accepted = true;
      return true;
    }
    break;

  case EventType::MouseWheel:
    if (!model_)
      break;
    scrollY_ = std::max(0, std::min(model_->rowCount() - visibleRows(),
                                    scrollY_ - event.wheelDelta / 120));
    update();
    event.accepted = true;
    return true;

  default:
    break;
  }
  return false;
}

} // namespace ltgui
