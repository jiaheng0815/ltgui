#pragma once
#include "animation.h"
#include "widget.h"

namespace ltgui {

class ScrollArea : public Widget {
public:
  explicit ScrollArea(Widget *parent = nullptr);

  void setWidget(std::unique_ptr<Widget> widget);
  Widget *widget() const;
  Widget *contentWidget() const;

  LTGUI_DECLARE_WIDGET_TYPE(ScrollArea)
  bool canAcceptFocus() const override { return true; }
  Size sizeHint() const override;

protected:
  void paintSelf(NativeCanvas *canvas) override;
  bool handleEvent(Event &event) override;

private:
  Widget *contentWidget_ = nullptr; // cached; validated by contentWidget()
  AnimatedFloat scrollYAnim_{0.0f};
  int scrollX_ = 0;
  int scrollY_ = 0;
  int contentWidth_ = 0;
  int contentHeight_ = 0;
  bool draggingScrollbar_ = false;
  int dragStartMouseY_ = 0;
  int dragStartScrollY_ = 0;

  void updateScrollBars();
  void scrollTo(int x, int y);
  int currentScrollY();
};

} // namespace ltgui
