#include "window.h"
#include "app.h"
#include "widget.h"

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
        printf("[ltgui] GPU acceleration enabled: %s\n", gpuCanvas_->gpuInfo().name.c_str());
    } else {
        gpuCanvas_.reset();
        printf("[ltgui] Using software renderer (GDI+/X11)\n");
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

void Window::setCentralWidget(Widget* widget) {
    if (centralWidget_) {
        delete centralWidget_;
    }
    centralWidget_ = widget;
    if (widget) {
        widget->setWindow(this);
        Size sz = getSize();
        if (!sz.isEmpty()) {
            widget->setGeometry(Rect(0, 0, sz.width, sz.height));
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
            if (!dirtyValid_) {
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
    case EventType::KeyUp:
        if (focusWidget_) {
            focusWidget_->handleEvent(event);
        }
        break;
    case EventType::MouseDown: {
        if (centralWidget_) {
            Widget* prevFocus = focusWidget_;
            centralWidget_->handleEvent(event);
            // Clear focus if no widget claimed it
            if (focusWidget_ == prevFocus) {
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

void Window::setFocusWidget(Widget* w) {
    if (focusWidget_ == w) return;

    // Notify old focus widget
    if (focusWidget_) {
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

} // namespace ltgui
