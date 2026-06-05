#pragma once
#include "geometry.h"
#include "event.h"
#include "style.h"
#include <vector>
#include <string>
#include <memory>

namespace ltgui {

class Window;
class Layout;
class NativeCanvas;

// Widget type identifiers — use widgetType() instead of dynamic_cast
// when you need to check the concrete type of a widget at runtime.
enum class WidgetType {
    Base,
    Button,
    Label,
    TextBox,
    CheckBox,
    RadioButton,
    Slider,
    ListBox,
    ScrollArea,
    ComboBox,
    ProgressBar,
    Tooltip,
    TabWidget,
    Image,
    TreeView,
    ContextMenu,
    MenuBar,
    Dialog,
    TableView,
    FileDialog,
};

class Widget {
public:
    explicit Widget(Widget* parent = nullptr);
    virtual ~Widget();

    // Tree
    Widget* parent() const { return parent_; }
    Widget* addChild(std::unique_ptr<Widget> child);
    std::unique_ptr<Widget> removeChild(Widget* child);
    const std::vector<std::unique_ptr<Widget>>& children() const { return children_; }
    Widget* childAt(int index) const;

    template<typename T, typename... Args>
    T* makeChild(Args&&... args) {
        auto ptr = std::make_unique<T>(std::forward<Args>(args)...);
        T* raw = ptr.get();
        addChild(std::move(ptr));
        return raw;
    }

    // Geometry
    Rect geometry() const { return geometry_; }
    virtual void setGeometry(const Rect& rect);
    virtual Size sizeHint() const;
    Rect absoluteRect() const;

    int x() const { return geometry_.x; }
    int y() const { return geometry_.y; }
    int width() const { return geometry_.width; }
    int height() const { return geometry_.height; }

    // Hit test area — returns rect in this widget's local coordinates
    // (origin at 0,0). Callers must translate by the widget's position
    // within its parent when comparing against parent-space coordinates.
    virtual Rect effectiveGeometry() const {
        Rect r = {0, 0, geometry_.width, geometry_.height};
        for (auto& child : children_) {
            if (child->isVisible()) {
                Rect childArea = child->effectiveGeometry();
                childArea = childArea.translated(child->geometry_.x, child->geometry_.y);
                r = r.united(childArea);
            }
        }
        return r;
    }

    // Layout
    void setLayout(std::unique_ptr<Layout> layout);
    Layout* layout() const { return layout_.get(); }

    // Style
    void setStyle(const Style& style) { style_ = style; }
    const Style& style() const { return style_; }
    Style& style() { return style_; }

    // State
    bool isEnabled() const { return (flags_ & kFlagEnabled) != 0; }
    void setEnabled(bool enabled);
    bool isVisible() const { return (flags_ & kFlagVisible) != 0; }
    void setVisible(bool visible);
    bool hasFocus() const { return (flags_ & kFlagFocused) != 0; }
    void raiseToTop();

    void invalidateSizeHint() { flags_ |= kFlagSizeHintDirty; if (parent_) parent_->invalidateSizeHint(); }
    void scheduleRelayout();
    Window* window() const { return window_; }
    void setWindow(Window* window);
    void claimFocus();

    // Painting
    virtual void paint(NativeCanvas* canvas, const Rect& dirtyRect);
    void update();
    void update(const Rect& dirtyLocalRect);

    // Events
    virtual bool handleEvent(Event& event);

    // Hit testing — returns the deepest child at pos (or this)
    virtual Widget* hitTest(const Point& pos);

    // Fast type check — avoids RTTI/dynamic_cast for sibling iteration.
    // Subclasses override this to return their WidgetType enum value.
    virtual WidgetType widgetType() const { return WidgetType::Base; }

    // Whether this widget can receive keyboard focus via Tab navigation.
    // Default is false — only interactive widgets override to return true.
    virtual bool canAcceptFocus() const { return false; }

    // Focus chain: returns the next/previous focusable widget in tree order.
    // Override to customize tab navigation. Window uses these for Tab/Shift+Tab.
    virtual Widget* nextFocusWidget();
    virtual Widget* previousFocusWidget();
    Widget* lastFocusableDescendant();

protected:
    virtual void paintSelf(NativeCanvas* canvas);
    virtual void paintChildren(NativeCanvas* canvas, const Rect& dirtyRect);
    virtual void paintBorder(NativeCanvas* canvas);

    void propagateWindow(Window* window);

    // sizeHint cache support for subclasses
    bool sizeHintDirty() const { return (flags_ & kFlagSizeHintDirty) != 0; }
    Size cachedSizeHint() const { return cachedSizeHint_; }
    void setCachedSizeHint(const Size& s) const { cachedSizeHint_ = s; flags_ &= ~kFlagSizeHintDirty; }

private:
    Widget* parent_ = nullptr;
    std::vector<std::unique_ptr<Widget>> children_;
    Rect geometry_;
    std::unique_ptr<Layout> layout_;
    Style style_;
    mutable uint8_t flags_ = kFlagEnabled | kFlagVisible | kFlagNeedsLayout | kFlagSizeHintDirty;
    static constexpr uint8_t kFlagEnabled       = 1 << 0;
    static constexpr uint8_t kFlagVisible       = 1 << 1;
    static constexpr uint8_t kFlagFocused       = 1 << 2;
    static constexpr uint8_t kFlagNeedsLayout   = 1 << 3;
    static constexpr uint8_t kFlagSizeHintDirty = 1 << 4;
    Window* window_ = nullptr;

    mutable Size cachedSizeHint_;
};

} // namespace ltgui
