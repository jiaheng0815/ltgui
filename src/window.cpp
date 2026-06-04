#include "window.h"
#include "app.h"
#include "widget.h"
#include "widgets/combobox.h"
#include "log.h"

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
    if (nativeWindow_) {
        nativeWindow_->destroy();
    }
}

bool Window::create(int width, int height, const std::string& title) {
    if (!nativeWindow_) return false;

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

        // Load default font for CPU fallback. GDI+ needs the font registered
        // with the system to find it by family name.
        Font defaultFont = Font::systemDefault(12);
        const char* fontPaths[] = {
#ifdef LTGUI_PLATFORM_WINDOWS
            "D:/code/ltgui/font/Deng.ttf",
            "font/Deng.ttf",
            "C:/Windows/Fonts/simfang.ttf",
            "C:/Windows/Fonts/msyh.ttf",
            "C:/Windows/Fonts/segoeui.ttf",
#elif defined(LTGUI_PLATFORM_LINUX)
            "/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf",
            "/usr/share/fonts/TTF/DejaVuSans.ttf",
#elif defined(LTGUI_PLATFORM_MACOS)
            "/System/Library/Fonts/Helvetica.ttc",
#endif
        };
        for (const char* path : fontPaths) {
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

Size Window::getSize() const {
    if (nativeWindow_) {
        return nativeWindow_->getSize();
    }
    return {};
}

void Window::setCentralWidget(std::unique_ptr<Widget> widget) {
    centralWidget_ = std::move(widget);
    if (centralWidget_) {
        centralWidget_->setWindow(this);
        Size sz = getSize();
        if (!sz.isEmpty()) {
            centralWidget_->setGeometry(Rect(0, 0, sz.width, sz.height));
        }
        update();
    }
}

void Window::update() {
    invalidate(Rect(0, 0, getSize().width, getSize().height));
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

void Window::handleEvent(Event& event) {
    switch (event.type) {
    case EventType::Paint:
        if (canvas_) {
            canvas_->beginPaint();
            // GPU clears the entire backbuffer each frame, so we must
            // paint the full window regardless of accumulated dirty rects.
            if (!dirtyValid_ || useGpu_) {
                Size sz = getSize();
                accumulatedDirty_ = Rect(0, 0, sz.width, sz.height);
            }
            onPaint(canvas_, accumulatedDirty_);
            canvas_->endPaint();
            dirtyValid_ = false;
            event.accepted = true;
        }
        break;
    case EventType::Resize:
        if (canvas_) {
            canvas_->resize(event.width, event.height);
        }
        if (centralWidget_) {
            centralWidget_->setGeometry(Rect(0, 0, event.width, event.height));
        }
        event.accepted = true;
        break;
    case EventType::Close:
        Application::instance().quit();
        event.accepted = true;
        break;
    case EventType::KeyDown:
        // Check registered shortcuts first (before widget dispatch)
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
            bool shift = (event.modifiers &
                          static_cast<int>(KeyModifier::Shift)) != 0;
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
        // Fall through to focus widget
        if (validateFocusWidget()) {
            focusWidget_->handleEvent(event);
        }
        break;
    case EventType::KeyUp:
        if (validateFocusWidget()) {
            focusWidget_->handleEvent(event);
        }
        break;
    case EventType::MouseDown: {
        // Close any open ComboBox dropdown if click is outside it
        ComboBox::closeIfClickOutside(event.pos);
        if (centralWidget_) {
            Widget* prevFocus = focusWidget_;
            bool handled = centralWidget_->handleEvent(event);
            // Clear focus only if no widget handled the click — a focused
            // widget that claims focus again (setFocusWidget no-ops when
            // same) must not lose focus here.
            if (focusWidget_ == prevFocus && !handled) {
                setFocusWidget(nullptr);
            }
        }
        break;
    }
    default:
        if (centralWidget_) {
            centralWidget_->handleEvent(event);
        }
        break;
    }
}

bool Window::validateFocusWidget() {
    if (focusWidget_ && focusWidget_->window() != this) {
        setFocusWidget(nullptr);
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
