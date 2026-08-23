#pragma once
#include "signal.hpp"
#include "widget.h"
#include "widgets/scrollstate.h"
#include <memory>
#include <string>
#include <vector>

namespace ltgui {

class TreeView;

class TreeViewItem {
public:
  TreeViewItem(const std::string &text);
  ~TreeViewItem();

  std::string text() const { return text_; }
  void setText(const std::string &text);

  bool expanded() const { return expanded_; }
  void setExpanded(bool expanded);

  TreeViewItem *parent() const { return parent_; }
  const std::vector<std::unique_ptr<TreeViewItem>> &children() const {
    return children_;
  }
  int childCount() const { return static_cast<int>(children_.size()); }

  TreeViewItem *addChild(const std::string &text);
  void removeChild(int index);

private:
  friend class TreeView;
  bool hasDescendant(const TreeViewItem *item) const;

  std::string text_;
  std::vector<std::unique_ptr<TreeViewItem>> children_;
  TreeViewItem *parent_ = nullptr;
  TreeView *treeView_ = nullptr;
  bool expanded_ = false;
  int depth_ = 0;
};

class TreeView : public Widget, public ScrollState {
public:
  explicit TreeView(Widget *parent = nullptr);
  ~TreeView() override;

  TreeViewItem *rootItem();
  TreeViewItem *selectedItem() const { return selected_; }
  void setSelectedItem(TreeViewItem *item);

  // Emitted whenever the selected item changes.
  Signal<TreeViewItem *> onSelectionChanged;

  LTGUI_DECLARE_WIDGET_TYPE(TreeView)
  bool canAcceptFocus() const override { return true; }
  Size sizeHint() const override;

protected:
  void paintSelf(NativeCanvas *canvas) override;
  bool handleEvent(Event &event) override;

private:
  std::unique_ptr<TreeViewItem> root_;
  TreeViewItem *selected_ = nullptr;
  int itemHeight_ = 26;
  int indentWidth_ = 20;

  int visibleItems() const;
  int totalRows() const;
  int countVisible(TreeViewItem *item) const;
};

} // namespace ltgui
