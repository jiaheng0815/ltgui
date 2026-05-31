#pragma once
#include "widget.h"
#include "animation.h"
#include <string>
#include <vector>
#include <functional>

namespace ltgui {

class ListBox : public Widget {
public:
    explicit ListBox(Widget* parent = nullptr);

    void addItem(const std::string& item);
    void removeItem(int index);
    void clear();
    int count() const;

    std::string item(int index) const;
    int selectedIndex() const { return selected_; }
    void setSelected(int index);

    using SelectionChangedCallback = std::function<void(int)>;
    void onSelectionChanged(SelectionChangedCallback cb) { selectionCallback_ = std::move(cb); }

    Size sizeHint() const override;

protected:
    void paintSelf(NativeCanvas* canvas) override;
    bool handleEvent(Event& event) override;

private:
    std::vector<std::string> items_;
    int selected_ = -1;
    AnimatedFloat scrollAnim_{0.0f};
    int scrollTarget_ = 0;
    int itemHeight_ = 26;
    SelectionChangedCallback selectionCallback_;

    int currentScrollOffset();
    int visibleItems() const;
};

} // namespace ltgui
