#pragma once
#include "widget.h"
#include "widgets/listitems.h"

namespace ltgui {

class ComboBox : public Widget, public ListItems {
public:
    explicit ComboBox(Widget* parent = nullptr);
    ~ComboBox() override;

    std::string currentText() const;

    LTGUI_DECLARE_WIDGET_TYPE(ComboBox)
    bool canAcceptFocus() const override { return true; }
    Size sizeHint() const override;
    Rect effectiveGeometry() const override;

    // Called by Window before hit-testing; returns true if a click-away closed this
    bool closeIfClickOutside(const Point& absPos);

protected:
    void paintSelf(NativeCanvas* canvas) override;
    bool handleEvent(Event& event) override;
    // Close the dropdown when the list becomes empty.
    void onItemsStructureChanged() override;

private:
    void openDropdown();
    void closeDropdown();
    void invalidateExtended();

    bool dropdownOpen_ = false;
    bool opensDownward_ = true;
    int dropHeight_ = 0;
};

} // namespace ltgui
