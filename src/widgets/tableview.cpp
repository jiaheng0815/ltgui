#include "widgets/tableview.h"
#include "platform/native_canvas.h"
#include "platform/native_window.h"
#include "theme.h"
#include "widgets/textbox.h"
#include "window.h"
#include <algorithm>
#include <chrono>
#include <functional>

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

bool SimpleTableModel::setCellText(int row, int col, const std::string &text) {
  if (row < 0 || row >= (int)data_.size())
    return false;
  if (col < 0 || col >= colCount_)
    return false;
  data_[row][col] = text;
  return true;
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

// In-place cell editor: a single-line TextBox that commutes Enter/Tab into
// commit and Escape into cancel. The framework sends KeyDown to the focus
// widget; single-line TextBox leaves Enter unconsumed — handling it in the
// editor removes any reliance on event bubbling through the table.
class TableView::CellEditor : public TextBox {
public:
  using CommitFn = std::function<void()>;
  using CancelFn = std::function<void()>;

  CellEditor(CommitFn commit, CancelFn cancel)
      : TextBox(""), commit_(std::move(commit)), cancel_(std::move(cancel)) {
    setVisible(false);
  }

  bool handleEvent(Event &e) override {
    if (e.type == EventType::KeyDown) {
      switch (e.key) {
      case Key::Enter:
      case Key::Tab:
        if (commit_) {
          e.accepted = true;
          commit_();
        }
        return true;
      case Key::Escape:
        if (cancel_) {
          e.accepted = true;
          cancel_();
        }
        return true;
      default:
        break;
      }
    }
    return TextBox::handleEvent(e);
  }

private:
  CommitFn commit_;
  CancelFn cancel_;
};

TableView::TableView(Widget *parent) : Widget(parent) {
  editor_ = static_cast<CellEditor *>(
      addChild(std::make_unique<CellEditor>([this]() { endEdit(true); },
                                            [this]() { endEdit(false); })));
  editor_->setVisible(false);
}

void TableView::setModel(std::shared_ptr<TableModel> model) {
  model_ = std::move(model);
  // The new model may have fewer rows than the previous selection/scroll
  // state — reset them instead of clamping silently.
  scrollY_ = 0;
  selectedRow_ = -1;
  selectedRows_.clear();
  invalidateSizeHint();
  update();
}

void TableView::addColumn(const TableColumn &col) {
  columns_.push_back(col);
  invalidateSizeHint();
  update();
}

void TableView::setColumnWidth(int col, int width) {
  if (col >= 0 && col < (int)columns_.size()) {
    columns_[col].width = std::max(columns_[col].minWidth, width);
    invalidateSizeHint();
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
  // The model may have shrunk since the last scroll (e.g. removeRow) —
  // clamp the cached scroll offset so painting stays in range.
  int maxScroll = std::max(0, numRows - visibleRows());
  scrollY_ = std::max(0, std::min(scrollY_, maxScroll));
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
      // The in-place editor draws this cell's text itself.
      if (isEditing() && i == editRow_ && c == editCol_)
        continue;
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

// --- In-place editing ---

bool TableView::beginEdit(int row, int col) {
  if (!model_ || !model_->supportsEdit())
    return false;
  if (row < 0 || row >= model_->rowCount() || col < 0 ||
      col >= (int)columns_.size())
    return false;
  // Switching cells commits the previous edit first so the model never holds
  // two editors' intermediate states at once.
  if (isEditing())
    endEdit(true);
  editRow_ = row;
  editCol_ = col;
  editor_->setText(model_->cellText(row, col));
  updateEditorGeometry();
  editor_->setVisible(true);
  editor_->selectAll();
  editor_->claimFocus();  // moves the window's keyboard focus to the editor
  update();
  return true;
}

void TableView::endEdit(bool commit) {
  if (!isEditing())
    return;
  int row = editRow_, col = editCol_;
  if (commit && model_ && model_->supportsEdit() &&
      row >= 0 && row < model_->rowCount() && col >= 0 &&
      col < (int)columns_.size()) {
    const std::string newText = editor_->text();
    const std::string oldText = model_->cellText(row, col);
    if (model_->setCellText(row, col, newText) && newText != oldText)
      onCellEdited.emit(row, col);
  }
  editRow_ = -1;
  editCol_ = -1;
  editor_->setVisible(false);
  claimFocus();  // hand keyboard focus back to the table
  update();
}

void TableView::updateEditorGeometry() {
  if (!isEditing() || !editor_)
    return;
  int cx = colX(editCol_) + 2;
  int cw = columns_[editCol_].width - 4;
  int ry = headerHeight_ + (editRow_ - scrollY_) * rowHeight_ + 1;
  int rh = rowHeight_ - 2;
  editor_->setGeometry(Rect(cx, ry, std::max(8, cw), std::max(4, rh)));
}

void TableView::setGeometry(const Rect &rect) {
  Widget::setGeometry(rect);
  if (isEditing())
    updateEditorGeometry();
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

    if (isEditing())
      endEdit(true);  // clicking anywhere outside the editor commits it

    // Check column resize handle
    for (int c = 0; c < (int)columns_.size() - 1; c++) {
      int edge = colX(c) + columns_[c].width;
      if (localY >= 0 && localY < headerHeight_ &&
          std::abs(localX - edge) < 4 && columns_[c].resizable) {
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

    // Row click → select; double-click (same cell within 400ms) → edit.
    {
      int row =
          (int)((localY - headerHeight_ + (int64_t)scrollY_ * rowHeight_) /
                rowHeight_);
      if (row >= 0 && model_ && row < model_->rowCount()) {
        int col = hitTestCol(localX);
        uint64_t nowMs =
            std::chrono::duration_cast<std::chrono::milliseconds>(
                std::chrono::steady_clock::now().time_since_epoch())
                .count();
        if (col >= 0 && row == lastClickRow_ && col == lastClickCol_ &&
            nowMs - lastClickMs_ < 400) {
          setCurrentIndex(row);
          update();
          event.accepted = true;
          beginEdit(row, col);
          return true;
        }
        lastClickRow_ = row;
        lastClickCol_ = col;
        lastClickMs_ = nowMs;
        setCurrentIndex(row);
        update();
        event.accepted = true;
        return true;
      }
    }
    break;

  case EventType::KeyDown:
    // While editing, the keyboard shortcut keys are forwarded to the editor
    // (the editor is the focus widget in a real window; this path also makes
    // programmatic KeyDown dispatch deterministic).
    if (isEditing()) {
      if (event.key == Key::Enter || event.key == Key::Escape ||
          event.key == Key::Tab) {
        event.accepted = true;
        editor_->handleEvent(event);
        return true;
      }
      break;
    }
    // Enter on the focused table opens the editor for the selected row
    // (column 0). While editing, the CellEditor consumes Enter/Escape.
    if (event.key == Key::Enter && model_ && model_->supportsEdit() &&
        selectedRow_ >= 0) {
      beginEdit(selectedRow_, 0);
      event.accepted = true;
      return true;
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
