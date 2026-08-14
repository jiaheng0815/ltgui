#pragma once
#include "widget.h"
#include "widgets/listitems.h"

namespace ltgui {

class ComboBox : public Widget, public ListItems {
public:
  explicit ComboBox(Widget *parent = nullptr);
  ~ComboBox() override;

  std::string currentText() const;

  LTGUI_DECLARE_WIDGET_TYPE(ComboBox)
  bool canAcceptFocus() const override { return true; }
  Size sizeHint() const override;
  Rect effectiveGeometry() const override;

  // Called by Window before hit-testing; returns true if a click-away closed
  // this
  bool closeIfClickOutside(const Point &absPos);

protected:
  void paintSelf(NativeCanvas *canvas) override;
  bool handleEvent(Event &event) override;
  // Close the dropdown when the list becomes empty.
  void onItemsStructureChanged() override;

private:
  void openDropdown();
  void closeDropdown();
  void invalidateExtended();
  // Move this widget back to its pre-open slot in the parent's child
  // list. raiseToTop() reorders children_ (needed so the dropdown paints
  // on top), but layouts position children by vector order — leaving the
  // combo raised permanently shifts it to the end of its row after any
  // relayout (e.g. a sibling label's setText on selection).
  void restoreZOrder();

  bool dropdownOpen_ = false;
  bool opensDownward_ = true;
  int dropHeight_ = 0;
  int zIndex_ = -1; // index in parent's children before raiseToTop()
};

} // namespace ltgui
