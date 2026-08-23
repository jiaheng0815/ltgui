#pragma once
#include "widget.h"
#include <functional>
#include <string>
#include <vector>

namespace ltgui {

class MenuBar : public Widget {
public:
  explicit MenuBar(Widget *parent = nullptr);

  using ItemCallback = std::function<void()>;

  int addMenu(const std::string &label);
  int addItem(int menuIdx, const std::string &text, ItemCallback cb = nullptr);
  void addSeparator(int menuIdx);

  // State queries
  int count() const { return static_cast<int>(menus_.size()); }
  int currentIndex() const { return openMenu_; }   // open menu, -1 = none
  int hoveredItem() const { return hoveredItem_; } // hovered item in open menu

  // Per-item attributes (call after addItem)
  void setItemShortcut(int menuIdx, int itemIdx, const std::string &shortcut);
  void setItemCheckable(int menuIdx, int itemIdx, bool checkable);
  void setItemChecked(int menuIdx, int itemIdx, bool checked);
  bool isItemChecked(int menuIdx, int itemIdx) const;
  void setItemRadio(int menuIdx, int itemIdx, int radioGroup);
  int addSubmenu(int menuIdx, int itemIdx, const std::string &label);
  int addSubItem(int menuIdx, int itemIdx, int subIdx, const std::string &text,
                 ItemCallback cb = nullptr);
  void addSubSeparator(int menuIdx, int itemIdx, int subIdx);

  LTGUI_DECLARE_WIDGET_TYPE(MenuBar)
  bool canAcceptFocus() const override { return true; }
  Size sizeHint() const override;

protected:
  void paintSelf(NativeCanvas *canvas) override;
  bool handleEvent(Event &event) override;

private:
  struct MenuItem {
    std::string text;
    ItemCallback callback;
    bool separator = false;
    bool checkable = false;
    bool checked = false;
    bool radio = false;
    int radioGroup = -1;
    std::string shortcut;
    std::vector<MenuItem> submenu;
  };
  struct Menu {
    std::string label;
    std::vector<MenuItem> items;
  };

  std::vector<Menu> menus_;
  int hoveredMenu_ = -1;
  int openMenu_ = -1;
  int hoveredItem_ = -1;
  int openSubmenu_ = -1; // submenu index on openMenu_'s item; -1 = closed
  int hoveredSub_ = -1;  // hovered item inside the open submenu
  int itemHeight_ = 26;
  int menuBarHeight_ = 30;
  bool keyboardNav_ = false;

  int menuX(int idx) const;
  int menuWidth(int idx) const;
  int hitTestMenu(int localX, int localY) const;

  // Dropdown/submenu panel geometry in this widget's LOCAL coordinates
  // (origin 0,0) — shared by paintSelf(), hit-testing and mouse routing so
  // the painted panel and its hit area can never diverge. Returns an empty
  // Rect when the panel is not open.
  Rect dropRectLocal() const;
  Rect submenuRectLocal() const;

  // Index of the item inside `panel` at the given local position, or -1.
  // Count limits the checked range (the panel rect may be smaller than the
  // full item count in visible-area terms).
  int hitTestItem(const Rect &panel, int localX, int localY, int count) const;
  bool handleMouseDown(int localX, int localY);

  // Register/unregister this menubar with its window while a dropdown is
  // open, so the window routes mouse/keyboard input to the panel.
  void notifyMenuOpened();
  void notifyMenuClosed();

  void activateItem(int menuIdx, int itemIdx);
  void activateSubItem(int menuIdx, int itemIdx, int subIdx);
  void closeMenu();
  static void activateMenuEntry(MenuItem &item, std::vector<MenuItem> *group);
  void paintItem(NativeCanvas *canvas, const Rect &r, const MenuItem &item,
                 bool hovered, int depth);
  int dropWidth(const std::vector<MenuItem> &items, int depth) const;
};

} // namespace ltgui
