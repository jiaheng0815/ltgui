#pragma once
#include "signal.hpp"
#include "widget.h"
#include <memory>
#include <string>
#include <vector>

namespace ltgui {

struct TableColumn {
  std::string title;
  int width = 100;
  int minWidth = 30;
  bool resizable = true;
  bool sortable = true;
};

class TableModel {
public:
  virtual ~TableModel() = default;
  virtual int rowCount() const = 0;
  virtual int columnCount() const = 0;
  virtual std::string cellText(int row, int col) const = 0;

  // Sort the model by a column. Default is no-op — override in subclasses
  // that support sorting (e.g. SimpleTableModel).
  virtual void sort(int column, bool ascending) {
    (void)column;
    (void)ascending;
  }
};

class SimpleTableModel : public TableModel {
public:
  SimpleTableModel(int rows, int cols);

  int rowCount() const override { return static_cast<int>(data_.size()); }
  int columnCount() const override { return colCount_; }

  std::string cellText(int row, int col) const override;
  void setCellText(int row, int col, const std::string &text);
  void addRow(const std::vector<std::string> &cells);
  void removeRow(int row);
  void clear();
  void sort(int column, bool ascending) override;

private:
  int colCount_;
  std::vector<std::vector<std::string>> data_;
};

class TableView : public Widget {
public:
  explicit TableView(Widget *parent = nullptr);

  void setModel(std::shared_ptr<TableModel> model);
  TableModel *model() const { return model_.get(); }

  void addColumn(const TableColumn &col);
  void setColumnWidth(int col, int width);

  // Primary selection — canonical naming.
  int currentIndex() const { return selectedRow_; }
  void setCurrentIndex(int row);

  // Legacy names — use currentIndex()/setCurrentIndex() instead.
  [[deprecated("use currentIndex() instead")]] int selectedRow() const {
    return selectedRow_;
  }
  [[deprecated("use setCurrentIndex() instead")]] void selectRow(int row) {
    setCurrentIndex(row);
  }

  std::vector<int> selectedRows() const { return selectedRows_; }
  void clearSelection();

  void setSortColumn(int col, bool ascending);

  // Emitted when a row is selected / a header column is clicked.
  Signal<int> onRowSelected;
  Signal<int> onHeaderClicked;

  LTGUI_DECLARE_WIDGET_TYPE(TableView)
  bool canAcceptFocus() const override { return true; }
  Size sizeHint() const override;

protected:
  void paintSelf(NativeCanvas *canvas) override;
  bool handleEvent(Event &event) override;

private:
  std::shared_ptr<TableModel> model_;
  std::vector<TableColumn> columns_;
  int selectedRow_ = -1;
  std::vector<int> selectedRows_;
  int sortedCol_ = -1;
  bool sortAsc_ = true;
  int scrollY_ = 0;
  int rowHeight_ = 26;
  int headerHeight_ = 30;
  int resizingCol_ = -1;
  int resizeStartX_ = 0;
  int resizeStartW_ = 0;

  int visibleRows() const;
  int totalWidth() const;
  int colX(int idx) const;
  int hitTestCol(int localX) const;
  int hitTestRow(int localY) const;
};

} // namespace ltgui
