#pragma once
#include "widget.h"
#include "animation.h"

namespace ltgui {

class ScrollArea : public Widget {
public:
    explicit ScrollArea(Widget* parent = nullptr);

    void setWidget(Widget* widget);
    Widget* widget() const { return contentWidget_; }

    Size sizeHint() const override;

protected:
    void paintSelf(NativeCanvas* canvas) override;
    bool handleEvent(Event& event) override;

private:
    Widget* contentWidget_ = nullptr;
    AnimatedFloat scrollYAnim_{0.0f};
    int scrollX_ = 0;
    int scrollY_ = 0;
    int contentWidth_ = 0;
    int contentHeight_ = 0;

    void updateScrollBars();
    void scrollTo(int x, int y);
    int currentScrollY();
};

} // namespace ltgui
