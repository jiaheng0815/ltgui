#pragma once
#include "geometry.h"
#include "event.h"
#include "style.h"
#include <vector>
#include <string>

namespace ltgui {

class Window;
class Layout;
class NativeCanvas;

class Widget {
public:
    explicit Widget(Widget* parent = nullptr);
    virtual ~Widget();

    // Tree
    Widget* parent() const { return parent_; }
    void addChild(Widget* child);
    void removeChild(Widget* child);
    const std::vector<Widget*>& children() const { return children_; }
    Widget* childAt(int index) const;

    // Geometry
    Rect geometry() const { return geometry_; }
    virtual void setGeometry(const Rect& rect);
    virtual Size sizeHint() const;
    virtual Size minimumSize() const;
    Rect absoluteRect() const;

    int x() const { return geometry_.x; }
    int y() const { return geometry_.y; }
    int width() const { return geometry_.width; }
    int height() const { return geometry_.height; }

    // Hit test area — may be larger than geometry (e.g., ComboBox dropdown)
    virtual Rect effectiveGeometry() const { return geometry_; }

    // Layout
    void setLayout(Layout* layout);
    Layout* layout() const { return layout_; }

    // Style
    void setStyle(const Style& style) { style_ = style; }
    const Style& style() const { return style_; }
    Style& style() { return style_; }

    // State
    bool isEnabled() const { return enabled_; }
    void setEnabled(bool enabled);
    bool isVisible() const { return visible_; }
    void setVisible(bool visible);
    bool hasFocus() const { return focused_; }

    // Window access
    Window* window() const { return window_; }
    void setWindow(Window* window);
    void claimFocus();

    // Painting
    virtual void paint(NativeCanvas* canvas);
    void update();

    // Events
    virtual bool handleEvent(Event& event);

    // Hit testing — returns the deepest child at pos (or this)
    virtual Widget* hitTest(const Point& pos);

protected:
    virtual void paintSelf(NativeCanvas* canvas);
    virtual void paintChildren(NativeCanvas* canvas);
    virtual void paintBorder(NativeCanvas* canvas);

    void propagateWindow(Window* window);

private:
    Widget* parent_ = nullptr;
    std::vector<Widget*> children_;
    Rect geometry_;
    Layout* layout_ = nullptr;
    Style style_;
    bool enabled_ = true;
    bool visible_ = true;
    bool focused_ = false;
    bool needsLayout_ = true;
    Window* window_ = nullptr;
};

} // namespace ltgui
