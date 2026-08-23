#pragma once
#include "signal.hpp"
#include <string>
#include <vector>

namespace ltgui {

class Widget;

// Mixin for widgets holding a list of string items with a current selection
// (ComboBox, ListBox). NOT a Widget subclass — combine via multiple
// inheritance. The host widget is stored so structural changes trigger
// invalidateSizeHint() + update().
class ListItems {
public:
  explicit ListItems(Widget *host) : host_(host) {}

  void addItem(const std::string &item);
  void removeItem(int index);
  void clear();
  int count() const { return static_cast<int>(items_.size()); }
  std::string item(int index) const;

  // Current selection; -1 = nothing selected. First added item is
  // auto-selected (index 0).
  int currentIndex() const { return selected_; }
  void setCurrentIndex(int index);

  // Legacy names — use currentIndex()/setCurrentIndex() instead.
  [[deprecated("use currentIndex() instead")]] int selectedIndex() const {
    return selected_;
  }
  [[deprecated("use setCurrentIndex() instead")]] void setSelected(int index) {
    setCurrentIndex(index);
  }

  // Emitted when the selection changes (after clamping).
  Signal<int> onSelectionChanged;
  // Emitted after any structural change (add/remove/clear).
  Signal<> onItemsChanged;

protected:
  Widget *host_ = nullptr;
  std::vector<std::string> items_;
  int selected_ = -1;

  // Subclass hook: called on structural changes AFTER the container was
  // modified but BEFORE selection is adjusted (e.g. ComboBox closes its
  // dropdown on clear()).
  virtual void onItemsStructureChanged() {}

  void notifyChanged();
};

} // namespace ltgui
