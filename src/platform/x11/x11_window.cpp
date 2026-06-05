#include "platform/x11/x11_window.h"

#ifdef LTGUI_PLATFORM_LINUX

#include "platform/x11/x11_canvas.h"
#include <X11/keysym.h>
#include <cstring>
#include <sys/select.h>
#include <sys/time.h>

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

// Per-window clipboard storage for SelectionRequest handling
namespace {
    std::unordered_map< ::Window, X11Window*> g_windowMap;

    // Atoms we need for clipboard
    Atom g_clipboardAtom = 0;
    Atom g_targetsAtom = 0;
    Atom g_utf8Atom = 0;
    Atom g_selProperty = 0;

    void ensureClipboardAtoms() {
        if (!g_clipboardAtom && X11Window::display()) {
            Display* dpy = X11Window::display();
            g_clipboardAtom = XInternAtom(dpy, "CLIPBOARD", False);
            g_targetsAtom   = XInternAtom(dpy, "TARGETS", False);
            g_utf8Atom      = XInternAtom(dpy, "UTF8_STRING", False);
            g_selProperty   = XInternAtom(dpy, "LTGUI_SEL", False);
        }
    }
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
        ownsDisplayRef_ = true; // only decrement if we actually have a display
    }
}

X11Window::~X11Window() {
    destroy();
    delete canvas_;
    canvas_ = nullptr;

    if (ownsDisplayRef_ && s_display_) {
        s_displayRefCount_--;
        if (s_displayRefCount_ <= 0) {
            XCloseDisplay(s_display_);
            s_display_ = nullptr;
            s_displayRefCount_ = 0;
        }
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

    // Select events we want to receive (include PropertyChangeMask for clipboard)
    XSelectInput(s_display_, window_,
                 ExposureMask | StructureNotifyMask |
                 ButtonPressMask | ButtonReleaseMask |
                 PointerMotionMask | ButtonMotionMask |
                 KeyPressMask | KeyReleaseMask |
                 EnterWindowMask | LeaveWindowMask |
                 FocusChangeMask | PropertyChangeMask);

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

float X11Window::dpiScale() const {
    if (!s_display_) return 1.0f;

    // Query Xft.dpi from the X resource database.
    // This is the standard way to get the user's configured DPI on X11.
    const char* dpiStr = XGetDefault(s_display_, "Xft", "dpi");
    if (dpiStr && dpiStr[0]) {
        float dpi = std::atof(dpiStr);
        if (dpi > 0.0f) return dpi / 96.0f; // 96 DPI = 1.0 scale factor
    }

    // Fallback: derive from screen dimensions
    int screen = DefaultScreen(s_display_);
    int widthMM = DisplayWidthMM(s_display_, screen);
    int widthPx = DisplayWidth(s_display_, screen);
    if (widthMM > 0 && widthPx > 0) {
        float dpi = (float)widthPx / ((float)widthMM / 25.4f);
        return dpi / 96.0f;
    }

    return 1.0f;
}

bool X11Window::setClipboardText(const std::string& text) {
    if (!s_display_ || !window_) return false;
    ensureClipboardAtoms();

    clipboardText_ = text;
    clipboardOwned_ = true;
    XSetSelectionOwner(s_display_, g_clipboardAtom, window_, CurrentTime);
    return true;
}

std::string X11Window::getClipboardText() {
    if (!s_display_ || !window_) return {};
    ensureClipboardAtoms();

    // Request the selection content from the current owner.
    // Use an event-driven approach: XConvertSelection triggers an async
    // request; we then pump the event loop (processing other events too)
    // until SelectionNotify arrives or we time out.
    std::string result;
    pendingReadResult_ = &result;
    pendingReadDone_ = false;

    XConvertSelection(s_display_, g_clipboardAtom, g_utf8Atom,
                      g_selProperty, window_, CurrentTime);
    XFlush(s_display_);

    // Wait for SelectionNotify, processing all X11 events while we wait.
    // This avoids the old busy-poll loop and allows other window events
    // (expose, input) to be handled during the clipboard round-trip.
    uint64_t deadline = AnimationManager::instance().nowMs() + 1000; // 1s timeout
    while (!pendingReadDone_) {
        // Check timeout
        if (AnimationManager::instance().nowMs() > deadline) break;

        // Process any pending X11 events (including our SelectionNotify)
        if (XPending(s_display_)) {
            XEvent xev;
            XNextEvent(s_display_, &xev);

            if (xev.type == SelectionNotify && xev.xselection.requestor == window_) {
                if (xev.xselection.property != None) {
                    Atom type;
                    int format;
                    unsigned long nitems = 0, bytesAfter = 0;
                    unsigned char* data = nullptr;
                    XGetWindowProperty(s_display_, window_, g_selProperty,
                                       0, 65536 / 4, False, AnyPropertyType,
                                       &type, &format, &nitems, &bytesAfter, &data);
                    if (data && type == g_utf8Atom && format == 8) {
                        result.assign(reinterpret_cast<char*>(data), nitems);
                    }
                    if (data) XFree(data);
                    XDeleteProperty(s_display_, window_, g_selProperty);
                }
                pendingReadDone_ = true;
            } else {
                // Forward other events to their windows
                X11Window* w = findWindow(xev.xany.window);
                if (w) w->handleEvent(xev);
            }
        } else {
            // No events pending — short sleep to avoid busy-wait
            usleep(1000); // 1ms
        }
    }

    pendingReadResult_ = nullptr;
    pendingReadDone_ = false;
    return result;
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
        // Read modifier state from the X11 key event
        unsigned int state = xev.xkey.state;
        if (state & ShiftMask)   ev.modifiers |= static_cast<int>(KeyModifier::Shift);
        if (state & ControlMask) ev.modifiers |= static_cast<int>(KeyModifier::Control);
        if (state & Mod1Mask)    ev.modifiers |= static_cast<int>(KeyModifier::Alt);
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
        unsigned int state = xev.xkey.state;
        if (state & ShiftMask)   ev.modifiers |= static_cast<int>(KeyModifier::Shift);
        if (state & ControlMask) ev.modifiers |= static_cast<int>(KeyModifier::Control);
        if (state & Mod1Mask)    ev.modifiers |= static_cast<int>(KeyModifier::Alt);
        eventCallback_(ev);
        break;
    }

    case SelectionClear:
        // Another app claimed clipboard ownership — clear our local state
        if (xev.xselectionclear.selection == g_clipboardAtom) {
            clipboardText_.clear();
            clipboardOwned_ = false;
        }
        break;

    case SelectionRequest: {
        // Serve clipboard content to requesters using per-instance data
        const XSelectionRequestEvent& req = xev.xselectionrequest;
        XEvent resp = {};
        resp.xselection.type      = SelectionNotify;
        resp.xselection.requestor  = req.requestor;
        resp.xselection.selection  = req.selection;
        resp.xselection.target     = req.target;
        resp.xselection.time       = req.time;

        if (req.selection == g_clipboardAtom && req.owner == window_ && clipboardOwned_) {
            if (req.target == g_targetsAtom) {
                // Report supported targets
                Atom targets[] = { g_targetsAtom, g_utf8Atom };
                XChangeProperty(s_display_, req.requestor, req.property,
                                XA_ATOM, 32, PropModeReplace,
                                (unsigned char*)targets, 2);
                resp.xselection.property = req.property;
            } else if (req.target == g_utf8Atom || req.target == XA_STRING) {
                XChangeProperty(s_display_, req.requestor, req.property,
                                req.target, 8, PropModeReplace,
                                (const unsigned char*)clipboardText_.c_str(),
                                (int)clipboardText_.size());
                resp.xselection.property = req.property;
            } else {
                resp.xselection.property = None; // unsupported target
            }
        } else {
            resp.xselection.property = None;
        }
        XSendEvent(s_display_, req.requestor, False, NoEventMask, &resp);
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
