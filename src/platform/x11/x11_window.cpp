#include "platform/x11/x11_window.h"

#ifdef LTGUI_PLATFORM_LINUX

#include "platform/x11/x11_canvas.h"
#include <X11/keysym.h>
#include <cstring>

// X11 headers define macros that conflict with ltgui enums
#undef None
#undef FocusIn
#undef FocusOut
#undef ButtonPress
#undef ButtonRelease
#undef Button4
#undef Button5

namespace ltgui {

#include <unordered_map>

Display* X11Window::s_display_ = nullptr;
int X11Window::s_displayRefCount_ = 0;

namespace {
    std::unordered_map< ::Window, X11Window*> g_windowMap;
}

void X11Window::registerWindow(X11Window* w) {
    if (w && w->window_) {
        g_windowMap[w->window_] = w;
    }
}

void X11Window::unregisterWindow(X11Window* w) {
    if (w && w->window_) {
        g_windowMap.erase(w->window_);
    }
}

X11Window* X11Window::findWindow(::Window xid) {
    auto it = g_windowMap.find(xid);
    return (it != g_windowMap.end()) ? it->second : nullptr;
}

X11Window::X11Window() {
    if (!s_display_) {
        s_display_ = XOpenDisplay(nullptr);
    }
    if (s_display_) {
        s_displayRefCount_++;
    }
}

X11Window::~X11Window() {
    destroy();
    delete canvas_;
    canvas_ = nullptr;

    s_displayRefCount_--;
    if (s_displayRefCount_ <= 0 && s_display_) {
        XCloseDisplay(s_display_);
        s_display_ = nullptr;
        s_displayRefCount_ = 0;
    }
}

bool X11Window::create(int width, int height, const std::string& title) {
    if (!s_display_) return false;
    if (window_) return true;

    int screen = DefaultScreen(s_display_);
    ::Window root = RootWindow(s_display_, screen);

    window_ = XCreateSimpleWindow(s_display_, root,
                                   0, 0, width, height, 1,
                                   BlackPixel(s_display_, screen),
                                   WhitePixel(s_display_, screen));

    // Set window title
    setTitle(title);

    // Register WM_DELETE_WINDOW protocol
    wmDeleteMessage_ = XInternAtom(s_display_, "WM_DELETE_WINDOW", False);
    wmProtocols_     = XInternAtom(s_display_, "WM_PROTOCOLS", False);
    XSetWMProtocols(s_display_, window_, &wmDeleteMessage_, 1);

    // Select events we want to receive
    XSelectInput(s_display_, window_,
                 ExposureMask | StructureNotifyMask |
                 ButtonPressMask | ButtonReleaseMask |
                 PointerMotionMask | ButtonMotionMask |
                 KeyPressMask | KeyReleaseMask |
                 EnterWindowMask | LeaveWindowMask |
                 FocusChangeMask);

    size_.width = width;
    size_.height = height;

    canvas_ = new X11Canvas(s_display_, window_, screen);
    canvas_->resize(width, height);

    registerWindow(this);
    return true;
}

void X11Window::destroy() {
    if (window_ && s_display_) {
        unregisterWindow(this);
        XDestroyWindow(s_display_, window_);
        window_ = 0;
    }
}

void X11Window::show() {
    if (window_ && s_display_) {
        XMapWindow(s_display_, window_);
        mapped_ = true;
    }
}

void X11Window::hide() {
    if (window_ && s_display_) {
        XUnmapWindow(s_display_, window_);
        mapped_ = false;
    }
}

void X11Window::close() {
    if (window_ && s_display_ && eventCallback_) {
        Event ev;
        ev.type = EventType::Close;
        eventCallback_(ev);
    }
}

void X11Window::setTitle(const std::string& title) {
    if (window_ && s_display_) {
        XStoreName(s_display_, window_, title.c_str());
    }
}

void X11Window::setSize(int width, int height) {
    if (window_ && s_display_) {
        XResizeWindow(s_display_, window_, width, height);
        size_.width = width;
        size_.height = height;
        if (canvas_) canvas_->resize(width, height);
    }
}

Size X11Window::getSize() const {
    return size_;
}

void X11Window::invalidate(const Rect& rect) {
    if (window_ && s_display_) {
        if (rect.width > 0 && rect.height > 0) {
            XClearArea(s_display_, window_, rect.x, rect.y,
                       rect.width, rect.height, True);
        } else {
            XClearArea(s_display_, window_, 0, 0, 0, 0, True);
        }
        XFlush(s_display_);
    }
}

void* X11Window::nativeHandle() const {
    return reinterpret_cast<void*>(window_);
}

NativeCanvas* X11Window::getCanvas() {
    return canvas_;
}

void X11Window::processEvents() {
    if (!s_display_ || !window_) return;

    while (XPending(s_display_)) {
        XEvent xev;
        XNextEvent(s_display_, &xev);
        handleEvent(xev);
    }
}

void X11Window::handleEvent(XEvent& xev) {
    if (!eventCallback_) return;

    Event ev;

    switch (xev.type) {
    case Expose: {
        ev.type = EventType::Paint;
        ev.width = size_.width;
        ev.height = size_.height;
        eventCallback_(ev);
        break;
    }

    case ConfigureNotify: {
        int w = xev.xconfigure.width;
        int h = xev.xconfigure.height;
        if (w != size_.width || h != size_.height) {
            size_.width = w;
            size_.height = h;
            if (canvas_) canvas_->resize(w, h);

            ev.type = EventType::Resize;
            ev.width = w;
            ev.height = h;
            eventCallback_(ev);
        }
        break;
    }

    case ClientMessage:
        if (static_cast<Atom>(xev.xclient.data.l[0]) == wmDeleteMessage_) {
            ev.type = EventType::Close;
            eventCallback_(ev);
        }
        break;

    case 4: { // X11 ButtonPress
        int btn = xev.xbutton.button;
        ev.pos = {xev.xbutton.x, xev.xbutton.y};

        if (btn >= 4) {  // Scroll wheel (buttons 4/5)
            ev.type = EventType::MouseWheel;
            ev.wheelDelta = (btn == 4) ? 1 : -1;
        } else {
            ev.type = EventType::MouseDown;
            if (btn == 1) ev.button = MouseButton::Left;
            else if (btn == 2) ev.button = MouseButton::Middle;
            else if (btn == 3) ev.button = MouseButton::Right;
        }
        eventCallback_(ev);
        break;
    }

    case 5: { // X11 ButtonRelease
        int btn = xev.xbutton.button;
        if (btn < 4) {
            ev.type = EventType::MouseUp;
            ev.pos = {xev.xbutton.x, xev.xbutton.y};
            if (btn == 1) ev.button = MouseButton::Left;
            else if (btn == 2) ev.button = MouseButton::Middle;
            else if (btn == 3) ev.button = MouseButton::Right;
            eventCallback_(ev);
        }
        break;
    }

    case MotionNotify: {
        ev.type = EventType::MouseMove;
        ev.pos = {xev.xmotion.x, xev.xmotion.y};
        eventCallback_(ev);
        break;
    }

    case KeyPress: {
        ev.type = EventType::KeyDown;
        KeySym ks = XLookupKeysym(&xev.xkey, 0);
        ev.key = mapKeySym(ks);
        // Also get the character if possible
        char buf[8] = {};
        KeySym ks2;
        int len = XLookupString(&xev.xkey, buf, sizeof(buf), &ks2, nullptr);
        if (len == 1 && static_cast<unsigned char>(buf[0]) >= 32) {
            ev.charCode = static_cast<unsigned char>(buf[0]);
        }
        eventCallback_(ev);
        break;
    }

    case KeyRelease: {
        ev.type = EventType::KeyUp;
        KeySym ks = XLookupKeysym(&xev.xkey, 0);
        ev.key = mapKeySym(ks);
        eventCallback_(ev);
        break;
    }

    case EnterNotify:
        ev.type = EventType::FocusIn;
        eventCallback_(ev);
        break;

    case LeaveNotify:
        ev.type = EventType::FocusOut;
        eventCallback_(ev);
        break;
    }
}

Key X11Window::mapKeySym(KeySym ks) const {
    // Letters
    if (ks >= XK_a && ks <= XK_z)
        return static_cast<Key>(static_cast<int>(Key::A) + (ks - XK_a));
    if (ks >= XK_A && ks <= XK_Z)
        return static_cast<Key>(static_cast<int>(Key::A) + (ks - XK_A));

    // Digits
    if (ks >= XK_0 && ks <= XK_9)
        return static_cast<Key>(static_cast<int>(Key::Num0) + (ks - XK_0));

    // Function keys
    if (ks >= XK_F1 && ks <= XK_F12)
        return static_cast<Key>(static_cast<int>(Key::F1) + (ks - XK_F1));

    // Special keys
    switch (ks) {
    case XK_Escape:    return Key::Escape;
    case XK_Return:    return Key::Enter;
    case XK_space:     return Key::Space;
    case XK_BackSpace: return Key::Backspace;
    case XK_Tab:       return Key::Tab;
    case XK_Shift_L:   case XK_Shift_R:   return Key::Shift;
    case XK_Control_L: case XK_Control_R: return Key::Control;
    case XK_Alt_L:     case XK_Alt_R:     return Key::Alt;
    case XK_Left:      return Key::Left;
    case XK_Right:     return Key::Right;
    case XK_Up:        return Key::Up;
    case XK_Down:      return Key::Down;
    case XK_Home:      return Key::Home;
    case XK_End:       return Key::End;
    case XK_Page_Up:   return Key::PageUp;
    case XK_Page_Down: return Key::PageDown;
    case XK_Insert:    return Key::Insert;
    case XK_Delete:    return Key::Delete;
    default:           return Key::Unknown;
    }
}

bool X11Window::processAllPending() {
    if (!s_display_) return false;

    bool processed = false;
    while (XPending(s_display_)) {
        XEvent xev;
        XNextEvent(s_display_, &xev);

        X11Window* w = findWindow(xev.xany.window);
        if (w) {
            w->handleEvent(xev);
        }
        processed = true;
    }
    return processed;
}

} // namespace ltgui

#endif // LTGUI_PLATFORM_LINUX
