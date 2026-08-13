#pragma once
#include "geometry.h"
#include "event.h"
#include "style.h"
#include <vector>
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
    MessageBox,
    InputDialog,
    TableView,
    FileDialog,
};

class Widget {
public:
    explicit Widget(Widget* parent = nullptr);
    virtual ~Widget();

    // Tree
    [[nodiscard]] Widget* parent() const { return parent_; }
    Widget* addChild(std::unique_ptr<Widget> child);
    std::unique_ptr<Widget> removeChild(Widget* child);
    [[nodiscard]] const std::vector<std::unique_ptr<Widget>>& children() const { return children_; }
    [[nodiscard]] Widget* childAt(int index) const;

    template<typename T, typename... Args>
    T* makeChild(Args&&... args) {
        auto ptr = std::make_unique<T>(std::forward<Args>(args)...);
        T* raw = ptr.get();
        addChild(std::move(ptr));
        return raw;
    }

    // Geometry
    [[nodiscard]] Rect geometry() const { return geometry_; }
    virtual void setGeometry(const Rect& rect);
    [[nodiscard]] virtual Size sizeHint() const;
    [[nodiscard]] Rect absoluteRect() const;

    int x() const { return geometry_.x; }
    int y() const { return geometry_.y; }
    int width() const { return geometry_.width; }
    int height() const { return geometry_.height; }

    // Hit test area — returns rect in this widget's local coordinates
    // (origin at 0,0). Callers must translate by the widget's position
    // within its parent when comparing against parent-space coordinates.
    [[nodiscard]] virtual Rect effectiveGeometry() const {
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
    [[nodiscard]] Layout* layout() const { return layout_.get(); }

    // Style
    void setStyle(const Style& style) { style_ = style; }
    [[nodiscard]] const Style& style() const { return style_; }
    Style& style() { return style_; }

    // State
    [[nodiscard]] bool isEnabled() const { return (flags_ & kFlagEnabled) != 0; }
    void setEnabled(bool enabled);
    [[nodiscard]] bool isVisible() const { return (flags_ & kFlagVisible) != 0; }
    void setVisible(bool visible);
    [[nodiscard]] bool hasFocus() const { return (flags_ & kFlagFocused) != 0; }
    void raiseToTop();

    void invalidateSizeHint() { sizeHintCache_.dirty = true; if (parent_) parent_->invalidateSizeHint(); }
    void scheduleRelayout();
    [[nodiscard]] Window* window() const { return window_; }
    void claimFocus();

    // Painting
    virtual void paint(NativeCanvas* canvas, const Rect& dirtyRect);
    void update();
    void update(const Rect& dirtyLocalRect);

    // Events
    virtual bool handleEvent(Event& event);

    // Hit testing — returns the deepest child at pos (or this)
    [[nodiscard]] virtual Widget* hitTest(const Point& pos);

    // Fast type check — avoids RTTI/dynamic_cast for sibling iteration.
    // Subclasses override this to return their WidgetType enum value.
    [[nodiscard]] virtual WidgetType widgetType() const { return WidgetType::Base; }

    // Whether this widget can receive keyboard focus via Tab navigation.
    // Default is false — only interactive widgets override to return true.
    [[nodiscard]] virtual bool canAcceptFocus() const { return false; }

    // Focus chain: returns the next/previous focusable widget in tree order.
    // Override to customize tab navigation. Window uses these for Tab/Shift+Tab.
    [[nodiscard]] virtual Widget* nextFocusWidget();
    [[nodiscard]] virtual Widget* previousFocusWidget();
    [[nodiscard]] Widget* lastFocusableDescendant();

protected:
    friend class Window;
    // Called by Window when this widget (or an ancestor) is attached to /
    // detached from a window. Not part of the public API.
    void setWindow(Window* window);

    virtual void paintSelf(NativeCanvas* canvas);
    virtual void paintChildren(NativeCanvas* canvas, const Rect& dirtyRect);
    virtual void paintBorder(NativeCanvas* canvas);

    // Paint background fill (using style().bgColor) and optional border
    // (using style().borderColor + style().borderWidth). Most widgets
    // call this at the start of paintSelf() for consistent appearance.
    void paintBackground(NativeCanvas* canvas);

    void propagateWindow(Window* window);

    // DPI-aware size helper: multiplies (w, h) by the window's DPI scale.
    // Returns the base size if no window is attached.
    [[nodiscard]] Size dpiScaleSize(int w, int h) const;

    // sizeHint cache support for subclasses — the cache is the ONLY mutable
    // state; all other flags are only modified through non-const paths.
    [[nodiscard]] bool sizeHintDirty() const { return sizeHintCache_.dirty; }
    [[nodiscard]] Size cachedSizeHint() const { return sizeHintCache_.value; }
    void setCachedSizeHint(const Size& s) const { sizeHintCache_.value = s; sizeHintCache_.dirty = false; }

private:
    // Dispatch an event to children. When `targeted` is true, only the child
    // under the cursor receives the event (MouseDown, MouseWheel). When false,
    // every child receives it (MouseUp, MouseMove).
    bool dispatchToChildren(Event& event, bool targeted);
    Widget* parent_ = nullptr;
    std::vector<std::unique_ptr<Widget>> children_;
    Rect geometry_;
    std::unique_ptr<Layout> layout_;
    Style style_;
    uint8_t flags_ = kFlagEnabled | kFlagVisible | kFlagNeedsLayout;
    static constexpr uint8_t kFlagEnabled     = 1 << 0;
    static constexpr uint8_t kFlagVisible     = 1 << 1;
    static constexpr uint8_t kFlagFocused     = 1 << 2;
    static constexpr uint8_t kFlagNeedsLayout = 1 << 3;
    Window* window_ = nullptr;

    // Only the sizeHint cache is mutable — it's updated lazily in the
    // const-qualified sizeHint() method.  All other state is guarded by
    // non-mutable flags_ above.
    struct SizeHintCache {
        Size value;
        bool dirty = true;
    };
    mutable SizeHintCache sizeHintCache_;

    // Reusable buffer for safe child iteration during event dispatch.
    // Pre-allocated capacity avoids heap allocation on every mouse event.
    mutable std::vector<Widget*> dispatchSnapshot_;
};

} // namespace ltgui
