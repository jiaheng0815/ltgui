#pragma once
#include "widget.h"
#include "widgets/listitems.h"
#include "animation.h"

namespace ltgui {

class ListBox : public Widget, public ListItems {
public:
    explicit ListBox(Widget* parent = nullptr);

    LTGUI_DECLARE_WIDGET_TYPE(ListBox)
    bool canAcceptFocus() const override { return true; }
    Size sizeHint() const override;

protected:
    void paintSelf(NativeCanvas* canvas) override;
    bool handleEvent(Event& event) override;

private:
    AnimatedFloat scrollAnim_{0.0f};
    int scrollTarget_ = 0;
    int itemHeight_ = 26;

    int currentScrollOffset();
    int visibleItems() const;
};

} // namespace ltgui
