#include "window.h"
#include "app.h"
#include "widget.h"
#include "widgets/combobox.h"
#include "log.h"
#include "platform/gpu/gpu_canvas.h"

#ifdef LTGUI_PLATFORM_WINDOWS
#include "platform/win32/win32_window.h"
#elif defined(LTGUI_PLATFORM_LINUX)
#include "platform/x11/x11_window.h"
#elif defined(LTGUI_PLATFORM_MACOS)
#include "platform/cocoa/cocoa_window.h"
#endif

namespace ltgui {

Window::Window() {
#ifdef LTGUI_PLATFORM_WINDOWS
    nativeWindow_ = std::make_unique<Win32Window>();
#elif defined(LTGUI_PLATFORM_LINUX)
    nativeWindow_ = std::make_unique<X11Window>();
#elif defined(LTGUI_PLATFORM_MACOS)
    nativeWindow_ = std::make_unique<CocoaWindow>();
#endif
    Application::instance().registerWindow(this);
}

Window::~Window() {
    Application::instance().unregisterWindow(this);

    // Clear focus and open combo BEFORE destroying the central widget.
    // Otherwise Widget::~Widget() may access a partially-destroyed Window
    // via its window_ pointer when trying to clear the focusWidget_ or
    // openCombo_ state.
    focusWidget_ = nullptr;
    openCombo_ = nullptr;

    // Destroy central widget while nativeWindow_ is still valid, since
    // widget destructors may need to call update() which uses nativeWindow_.
    centralWidget_.reset();

    if (nativeWindow_) {
        nativeWindow_->destroy();
    }
}

bool Window::create(int width, int height, const std::string& title) {
    if (!nativeWindow_) return false;

    // Record the main thread BEFORE creating the native window, because
    // CreateWindowExW (and equivalents on other platforms) can synchronously
    // dispatch messages to the wndproc during creation.  Those messages flow
    // through handleMessage() which asserts isMainThread() in debug builds.
    setMainThread();

    // Set callback BEFORE create so WM_SIZE/Resize during creation is captured
    nativeWindow_->setEventCallback([this](Event& event) {
        handleEvent(event);
    });

    if (!nativeWindow_->create(width, height, title)) {
        return false;
    }

    // Always get native canvas as fallback first
    canvas_ = nativeWindow_->getCanvas();

    // Try GPU acceleration
    gpuCanvas_ = std::make_unique<gpu::GpuCanvas>();
    if (gpuCanvas_->initialize(nativeWindow_->nativeHandle(), width, height)) {
        canvas_ = gpuCanvas_.get();
        useGpu_ = true;
        LOG_INFO("Window", "GPU acceleration enabled: %s", gpuCanvas_->gpuInfo().name.c_str());
    } else {
        gpuCanvas_.reset();
        // canvas_ stays on nativeWindow_->getCanvas() — no dangling pointer
        LOG_INFO("Window", "Using software renderer (GDI+/X11)");

        // Load default font for CPU fallback
        Font defaultFont = Font::systemDefault(12);
        for (const char* path : defaultFontSearchPaths()) {
            if (canvas_->loadFontFile(defaultFont, path)) {
                LOG_INFO("Window", "CPU fallback font loaded: %s", path);
                break;
            }
        }
    }

    return true;
}

void Window::close() {
    if (nativeWindow_) {
        nativeWindow_->close();
    }
}

void Window::show() {
    if (nativeWindow_) {
        nativeWindow_->show();
    }
}

void Window::hide() {
    if (nativeWindow_) {
        nativeWindow_->hide();
    }
}

void Window::setTitle(const std::string& title) {
    if (nativeWindow_) {
        nativeWindow_->setTitle(title);
    }
}

void Window::setSize(int width, int height) {
    if (nativeWindow_) {
        nativeWindow_->setSize(width, height);
    }
}

Size Window::size() const {
    if (nativeWindow_) {
        return nativeWindow_->getSize();
    }
    return {};
}

void Window::setCentralWidget(std::unique_ptr<Widget> widget) {
    centralWidget_ = std::move(widget);
    if (centralWidget_) {
        centralWidget_->setWindow(this);
        Size sz = size();
        if (!sz.isEmpty()) {
            centralWidget_->setGeometry(Rect(0, 0, sz.width, sz.height));
        }
        update();
    }
}

void Window::update() {
    invalidate(Rect(0, 0, size().width, size().height));
}

void Window::invalidate(const Rect& rect) {
    if (dirtyValid_) {
        accumulatedDirty_ = accumulatedDirty_.united(rect);
    } else {
        accumulatedDirty_ = rect;
        dirtyValid_ = true;
    }
    if (nativeWindow_) {
        nativeWindow_->invalidate(rect);
    }
}

void* Window::nativeHandle() const {
    if (nativeWindow_) {
        return nativeWindow_->nativeHandle();
    }
    return nullptr;
}

void Window::handlePaintEvent(Event& event) {
    if (!canvas_) return;
    canvas_->beginPaint();
    // GPU clears the entire backbuffer each frame, so we must
    // paint the full window regardless of accumulated dirty rects.
    if (!dirtyValid_ || useGpu_) {
        Size sz = size();
        accumulatedDirty_ = Rect(0, 0, sz.width, sz.height);
    }
    onPaint(canvas_, accumulatedDirty_);
    canvas_->endPaint();
    dirtyValid_ = false;
    event.accepted = true;
}

void Window::handleResizeEvent(Event& event) {
    if (canvas_) {
        canvas_->resize(event.width, event.height);
    }
    if (centralWidget_) {
        centralWidget_->setGeometry(Rect(0, 0, event.width, event.height));
    }
    event.accepted = true;
}

void Window::handleCloseEvent(Event& event) {
    LOG_DEBUG("Window", "Close event received, calling closeWindow");
    Application::instance().closeWindow(this);
    LOG_DEBUG("Window", "closeWindow returned");
    event.accepted = true;
}

void Window::handleKeyEvent(Event& event) {
    // KeyDown: check shortcuts first, then tab navigation, then focus widget
    if (event.type == EventType::KeyDown) {
        for (auto& entry : shortcuts_) {
            if (entry.shortcut.matches(event.key,
                    static_cast<KeyModifier>(event.modifiers))) {
                if (entry.callback) entry.callback();
                event.accepted = true;
                return;
            }
        }
        // Tab navigation: Tab = next, Shift+Tab = previous
        if (event.key == Key::Tab && centralWidget_) {
            bool shift = hasModifier(event.modifiers, KeyModifier::Shift);
            Widget* next = nullptr;
            if (shift) {
                if (focusWidget_)
                    next = focusWidget_->previousFocusWidget();
                if (!next)
                    next = centralWidget_->lastFocusableDescendant();
            } else {
                if (focusWidget_)
                    next = focusWidget_->nextFocusWidget();
                if (!next)
                    next = centralWidget_->nextFocusWidget();
            }
            if (next && next != focusWidget_) {
                next->claimFocus();
            }
            event.accepted = true;
            return;
        }
    }
    // Dispatch to focus widget (both KeyDown and KeyUp)
    if (validateFocusWidget()) {
        focusWidget_->handleEvent(event);
    }
}

void Window::handleMouseEvent(Event& event) {
    // Close open ComboBox dropdown if click is outside it.
    if (auto* combo = openCombo_) {
        combo->closeIfClickOutside(event.pos);
    }
    // If an open ComboBox survived closeIfClickOutside, route the event
    // directly to the ComboBox to prevent sibling widget click stealing.
    if (openCombo_) {
        Widget* combo = openCombo_;
        Point savedPos = event.pos;
        Widget* p = combo->parent();
        if (p) {
            Rect pabs = p->absoluteRect();
            event.pos = {savedPos.x - pabs.x, savedPos.y - pabs.y};
        }
        combo->handleEvent(event);
        event.pos = savedPos;
        if (event.accepted) return;
    }
    if (centralWidget_) {
        Widget* prevFocus = focusWidget_;
        bool handled = centralWidget_->handleEvent(event);
        // Clear focus only if no widget handled the click
        if (focusWidget_ == prevFocus && !handled) {
            setFocusWidget(nullptr);
        }
    }
}

void Window::handleEvent(Event& event) {
    switch (event.type) {
    case EventType::Paint:      handlePaintEvent(event);   break;
    case EventType::Resize:     handleResizeEvent(event);  break;
    case EventType::Close:      handleCloseEvent(event);   break;
    case EventType::KeyDown:
    case EventType::KeyUp:      handleKeyEvent(event);     break;
    case EventType::MouseDown:  handleMouseEvent(event);   break;
    default:
        if (centralWidget_) {
            centralWidget_->handleEvent(event);
        }
        break;
    }
}

bool Window::validateFocusWidget() {
    if (focusWidget_ && focusWidget_->window() != this) {
        // Directly clear the pointer to avoid infinite recursion:
        // setFocusWidget() calls validateFocusWidget() again.
        Widget* old = focusWidget_;
        focusWidget_ = nullptr;

        // Notify the old widget that it lost focus
        Event ev;
        ev.type = EventType::FocusOut;
        old->handleEvent(ev);
        return false;
    }
    return focusWidget_ != nullptr;
}

void Window::setFocusWidget(Widget* w) {
    if (focusWidget_ == w) return;

    // Notify old focus widget
    if (validateFocusWidget()) {
        Event ev;
        ev.type = EventType::FocusOut;
        focusWidget_->handleEvent(ev);
    }

    focusWidget_ = w;

    // Notify new focus widget
    if (focusWidget_) {
        Event ev;
        ev.type = EventType::FocusIn;
        focusWidget_->handleEvent(ev);
    }
}

void Window::onPaint(NativeCanvas* canvas, const Rect& dirtyRect) {
    if (centralWidget_ && canvas) {
        centralWidget_->paint(canvas, dirtyRect);
    }
}

void Window::registerShortcut(const Shortcut& sc, Shortcut::Callback cb) {
    unregisterShortcut(sc); // avoid duplicates
    shortcuts_.push_back({sc, std::move(cb)});
}

void Window::unregisterShortcut(const Shortcut& sc) {
    for (auto it = shortcuts_.begin(); it != shortcuts_.end(); ++it) {
        if (it->shortcut == sc) {
            shortcuts_.erase(it);
            return;
        }
    }
}

} // namespace ltgui
