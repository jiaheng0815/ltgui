#include "window.h"
#include "app.h"
#include "log.h"
#include "platform/gpu/gpu_canvas.h"
#include "widget.h"
#include "widgets/combobox.h"
#include "widgets/contextmenu.h"

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
  // openCombo_ state. Also clear the popup registrations — their owners
  // may be destroyed by centralWidget_.reset() below.
  focusWidget_ = nullptr;
  openCombo_ = nullptr;
  openMenuBar_ = nullptr;
  openContextMenu_ = nullptr;
  modalDialog_ = nullptr;

  // Destroy central widget while nativeWindow_ is still valid, since
  // widget destructors may need to call update() which uses nativeWindow_.
  centralWidget_.reset();

  if (nativeWindow_) {
    nativeWindow_->destroy();
  }
}

bool Window::create(int width, int height, const std::string &title) {
  if (!nativeWindow_)
    return false;

  // Record the main thread BEFORE creating the native window, because
  // CreateWindowExW (and equivalents on other platforms) can synchronously
  // dispatch messages to the wndproc during creation.  Those messages flow
  // through handleMessage() which asserts isMainThread() in debug builds.
  setMainThread();

  // Set callback BEFORE create so WM_SIZE/Resize during creation is captured
  nativeWindow_->setEventCallback([this](Event &event) { handleEvent(event); });

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
    LOG_INFO("Window", "GPU acceleration enabled: %s",
             gpuCanvas_->gpuInfo().name.c_str());
  } else {
    gpuCanvas_.reset();
    // canvas_ stays on nativeWindow_->getCanvas() — no dangling pointer
    LOG_INFO("Window", "Using software renderer (GDI+/X11)");

    // Load default font for CPU fallback
    Font defaultFont = Font::systemDefault(12);
    for (const char *path : defaultFontSearchPaths()) {
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

void Window::setTitle(const std::string &title) {
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

void Window::update() { invalidate(Rect(0, 0, size().width, size().height)); }

void Window::invalidate(const Rect &rect) {
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

void *Window::nativeHandle() const {
  if (nativeWindow_) {
    return nativeWindow_->nativeHandle();
  }
  return nullptr;
}

void Window::handlePaintEvent(Event &event) {
  if (!canvas_)
    return;
  canvas_->beginPaint();
  // GPU clears the entire backbuffer each frame, so we must
  // paint the full window regardless of accumulated dirty rects.
  if (!dirtyValid_ || useGpu_) {
    Size sz = size();
    accumulatedDirty_ = Rect(0, 0, sz.width, sz.height);
  }
  onPaint(canvas_, accumulatedDirty_);
  canvas_->endPaint();
  // Widgets may call update() during onPaint (e.g. animation ticks inside
  // a paint handler). Those new invalidation rects were unioned into
  // accumulatedDirty_ above, and must NOT be lost — re-mark the native
  // window so the OS repaints that area on a subsequent frame, otherwise
  // the "dirty" widget would never repaint again.
  if (dirtyValid_) {
    Rect pending = accumulatedDirty_;
    dirtyValid_ = false;
    if (nativeWindow_)
      nativeWindow_->invalidate(pending);
  } else {
    dirtyValid_ = false;
  }
  event.accepted = true;
}

void Window::handleResizeEvent(Event &event) {
  if (canvas_) {
    canvas_->resize(event.width, event.height);
  }
  if (centralWidget_) {
    centralWidget_->setGeometry(Rect(0, 0, event.width, event.height));
  }
  event.accepted = true;
}

void Window::handleCloseEvent(Event &event) {
  LOG_DEBUG("Window", "Close event received, calling closeWindow");
  Application::instance().closeWindow(this);
  LOG_DEBUG("Window", "closeWindow returned");
  event.accepted = true;
}

void Window::handleKeyEvent(Event &event) {
  // A modal dialog gets keyboard input first — nothing behind it should
  // react while it is running.
  if (modalDialog_) {
    if (modalDialog_->handleEvent(event)) {
      event.accepted = true;
      return;
    }
    if (validateFocusWidget()) {
      focusWidget_->handleEvent(event);
    }
    return;
  }
  // An open MenuBar dropdown gets keyboard input first, mirroring the
  // ComboBox/Modal routing.
  if (openMenuBar_) {
    if (openMenuBar_->handleEvent(event)) {
      event.accepted = true;
      return;
    }
    // The menubar closed its menu — let the key continue its normal trip.
  }
  // KeyDown: check shortcuts first, then tab navigation, then focus widget
  if (event.type == EventType::KeyDown) {
    for (auto &entry : shortcuts_) {
      if (entry.shortcut.matches(event.key,
                                 static_cast<KeyModifier>(event.modifiers))) {
        if (entry.callback)
          entry.callback();
        event.accepted = true;
        return;
      }
    }
    // Tab navigation: Tab = next, Shift+Tab = previous
    if (event.key == Key::Tab && centralWidget_) {
      bool shift = hasModifier(event.modifiers, KeyModifier::Shift);
      Widget *next = nullptr;
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

void Window::handleMouseEvent(Event &event) {
  // A running modal dialog swallows all mouse input (its own handleEvent
  // decides what is inside the panel vs. on the overlay).
  if (modalDialog_) {
    modalDialog_->handleEvent(event);
    return;
  }
  // Close open ComboBox dropdown if click is outside it.
  if (auto *combo = openCombo_) {
    combo->closeIfClickOutside(event.pos);
  }
  // If an open ComboBox survived closeIfClickOutside, route the event
  // directly to the ComboBox to prevent sibling widget click stealing.
  if (openCombo_) {
    Widget *combo = openCombo_;
    Point savedPos = event.pos;
    Widget *p = combo->parent();
    if (p) {
      Rect pabs = p->absoluteRect();
      event.pos = {savedPos.x - pabs.x, savedPos.y - pabs.y};
    }
    combo->handleEvent(event);
    event.pos = savedPos;
    if (event.accepted)
      return;
  }
  // An open MenuBar dropdown gets the click first — the panel hangs below
  // the bar and would otherwise miss hits outside the menubar's base rect.
  // The menubar swallows any click while its menu is open (it closes the
  // menu itself if the click lands outside the panel).
  if (openMenuBar_) {
    Widget *menu = openMenuBar_;
    Point savedPos = event.pos;
    Widget *p = menu->parent();
    if (p) {
      Rect pabs = p->absoluteRect();
      event.pos = {savedPos.x - pabs.x, savedPos.y - pabs.y};
    }
    menu->handleEvent(event);
    event.pos = savedPos;
    if (event.accepted)
      return;
  }
  // An open context menu: clicks inside are routed to it; clicks outside
  // dismiss it and are swallowed so they don't leak to widgets below.
  if (openContextMenu_) {
    Widget *menu = openContextMenu_;
    Rect absBase = menu->absoluteRect();
    Rect absEff = menu->effectiveGeometry().translated(absBase.x, absBase.y);
    if (!absEff.contains(event.pos)) {
      // dismiss() is ContextMenu-specific; handleEvent() routes through
      // the public Widget interface (virtual dispatch).
      static_cast<ContextMenu *>(menu)->dismiss();
      event.accepted = true;
      return;
    }
    Point savedPos = event.pos;
    menu->handleEvent(event);
    event.pos = savedPos;
    return;
  }
  if (centralWidget_) {
    Widget *prevFocus = focusWidget_;
    bool handled = centralWidget_->handleEvent(event);
    // Clear focus only if no widget handled the click
    if (focusWidget_ == prevFocus && !handled) {
      setFocusWidget(nullptr);
    }
  }
}

void Window::handleEvent(Event &event) {
  switch (event.type) {
  case EventType::Paint:
    handlePaintEvent(event);
    break;
  case EventType::Resize:
    handleResizeEvent(event);
    break;
  case EventType::Close:
    handleCloseEvent(event);
    break;
  case EventType::KeyDown:
  case EventType::KeyUp:
    handleKeyEvent(event);
    break;
  case EventType::MouseDown:
    handleMouseEvent(event);
    break;
  default:
    // While a modal dialog runs, keep every remaining event (MouseUp,
    // MouseMove, MouseWheel, ...) inside the dialog instead of leaking
    // them to the central widget behind the overlay.
    if (modalDialog_) {
      modalDialog_->handleEvent(event);
      break;
    }
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
    Widget *old = focusWidget_;
    focusWidget_ = nullptr;

    // Notify the old widget that it lost focus
    Event ev;
    ev.type = EventType::FocusOut;
    old->handleEvent(ev);
    return false;
  }
  return focusWidget_ != nullptr;
}

void Window::setFocusWidget(Widget *w) {
  if (focusWidget_ == w)
    return;

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

void Window::onPaint(NativeCanvas *canvas, const Rect &dirtyRect) {
  if (centralWidget_ && canvas) {
    centralWidget_->paint(canvas, dirtyRect);
  }
  // A running modal dialog paints its overlay + panel over the window.
  // Pass the full window rect so the semi-transparent overlay stays
  // consistent even when only a small region was dirtied.
  if (modalDialog_ && canvas) {
    Size sz = size();
    modalDialog_->paint(canvas, Rect(0, 0, sz.width, sz.height));
  }
}

Widget *Window::setModalDialog(Widget *dialog) {
  Widget *prev = modalDialog_;
  if (prev == dialog)
    return prev;
  modalDialog_ = dialog;
  // Attach the dialog's subtree to this window so its children can
  // update()/request focus while it is shown.
  if (dialog)
    dialog->propagateWindow(this);
  return prev;
}

void Window::setOpenMenuBar(Widget *menubar) { openMenuBar_ = menubar; }

void Window::setOpenContextMenu(Widget *menu) { openContextMenu_ = menu; }

void Window::registerShortcut(const Shortcut &sc, Shortcut::Callback cb) {
  unregisterShortcut(sc); // avoid duplicates
  shortcuts_.push_back({sc, std::move(cb)});
}

void Window::unregisterShortcut(const Shortcut &sc) {
  for (auto it = shortcuts_.begin(); it != shortcuts_.end(); ++it) {
    if (it->shortcut == sc) {
      shortcuts_.erase(it);
      return;
    }
  }
}

} // namespace ltgui
