#pragma once
#include "widget.h"
#include <string>
#include <vector>
#include <functional>

namespace ltgui {

class ComboBox : public Widget {
public:
    explicit ComboBox(Widget* parent = nullptr);
    ~ComboBox() override;

    void addItem(const std::string& item);
    void removeItem(int index);
    void clear();
    int count() const;

    std::string currentText() const;
    int currentIndex() const { return selected_; }
    void setCurrentIndex(int index);

    using SelectionChangedCallback = std::function<void(int)>;
    void onSelectionChanged(SelectionChangedCallback cb) { selectionCallback_ = std::move(cb); }

    WidgetType widgetType() const override { return WidgetType::ComboBox; }
    bool canAcceptFocus() const override { return true; }
    Size sizeHint() const override;
    Rect effectiveGeometry() const override;

    // Called by Window before hit-testing; returns true if a click-away closed this
    bool closeIfClickOutside(const Point& absPos);

protected:
    void paintSelf(NativeCanvas* canvas) override;
    bool handleEvent(Event& event) override;

private:
    void openDropdown();
    void closeDropdown();
    void invalidateExtended();

    std::vector<std::string> items_;
    int selected_ = -1;
    bool dropdownOpen_ = false;
    bool opensDownward_ = true;
    int dropHeight_ = 0;
    SelectionChangedCallback selectionCallback_;
};

} // namespace ltgui
