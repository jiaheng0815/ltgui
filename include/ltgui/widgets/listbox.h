#pragma once
#include "widget.h"
#include "widgets/listitems.h"
#include "widgets/scrollstate.h"

namespace ltgui {

class ListBox : public Widget, public ListItems, public ScrollState {
public:
    explicit ListBox(Widget* parent = nullptr);

    LTGUI_DECLARE_WIDGET_TYPE(ListBox)
    bool canAcceptFocus() const override { return true; }
    Size sizeHint() const override;

protected:
    void paintSelf(NativeCanvas* canvas) override;
    bool handleEvent(Event& event) override;

private:
    int itemHeight_ = 26;

    int currentScrollOffset() { return scrollOffset(); }
    int visibleItems() const;
};

} // namespace ltgui
