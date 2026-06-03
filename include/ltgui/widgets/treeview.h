#pragma once
#include "widget.h"
#include "animation.h"
#include <string>
#include <vector>
#include <memory>
#include <functional>

namespace ltgui {

class TreeView;

class TreeViewItem {
public:
    TreeViewItem(const std::string& text);
    ~TreeViewItem();

    std::string text() const { return text_; }
    void setText(const std::string& text);

    bool expanded() const { return expanded_; }
    void setExpanded(bool expanded);

    TreeViewItem* parent() const { return parent_; }
    const std::vector<std::unique_ptr<TreeViewItem>>& children() const { return children_; }
    int childCount() const { return static_cast<int>(children_.size()); }

    TreeViewItem* addChild(const std::string& text);
    void removeChild(int index);

private:
    friend class TreeView;
    std::string text_;
    std::vector<std::unique_ptr<TreeViewItem>> children_;
    TreeViewItem* parent_ = nullptr;
    TreeView* treeView_ = nullptr;
    bool expanded_ = false;
    int depth_ = 0;
};

class TreeView : public Widget {
public:
    explicit TreeView(Widget* parent = nullptr);
    ~TreeView() override;

    TreeViewItem* rootItem();
    TreeViewItem* selectedItem() const { return selected_; }
    void setSelectedItem(TreeViewItem* item);

    using SelectionChangedCallback = std::function<void(TreeViewItem*)>;
    void onSelectionChanged(SelectionChangedCallback cb) { selectionCallback_ = std::move(cb); }

    WidgetType widgetType() const override { return WidgetType::TreeView; }
    Size sizeHint() const override;

protected:
    void paintSelf(NativeCanvas* canvas) override;
    bool handleEvent(Event& event) override;

private:
    std::unique_ptr<TreeViewItem> root_;
    TreeViewItem* selected_ = nullptr;
    AnimatedFloat scrollAnim_{0.0f};
    int scrollTarget_ = 0;
    int itemHeight_ = 26;
    int indentWidth_ = 20;
    SelectionChangedCallback selectionCallback_;

    int visibleItems() const;
    int totalRows() const;
    int rowForItem(TreeViewItem* item) const;
    TreeViewItem* itemForRow(TreeViewItem* parent, int& row, int target) const;
    int rowHeight(TreeViewItem* item, int depth) const;
    int countVisible(TreeViewItem* item) const;
};

} // namespace ltgui
