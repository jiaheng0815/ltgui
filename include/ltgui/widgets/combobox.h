#pragma once
#include "widget.h"
#include <string>
#include <vector>
#include <functional>

namespace ltgui {

class ComboBox : public Widget {
public:
    explicit ComboBox(Widget* parent = nullptr);

    void addItem(const std::string& item);
    void removeItem(int index);
    void clear();
    int count() const;

    std::string currentText() const;
    int currentIndex() const { return selected_; }
    void setCurrentIndex(int index);

    using SelectionChangedCallback = std::function<void(int)>;
    void onSelectionChanged(SelectionChangedCallback cb) { selectionCallback_ = std::move(cb); }

    Size sizeHint() const override;
    Rect effectiveGeometry() const override;

protected:
    void paintSelf(NativeCanvas* canvas) override;
    bool handleEvent(Event& event) override;

private:
    std::vector<std::string> items_;
    int selected_ = -1;
    bool dropped_ = false;
    SelectionChangedCallback selectionCallback_;
};

} // namespace ltgui
