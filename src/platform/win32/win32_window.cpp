#include "platform/win32/win32_window.h"

#ifdef LTGUI_PLATFORM_WINDOWS

#include "platform/win32/win32_canvas.h"
#include <windowsx.h>
#include <imm.h>

namespace ltgui {

bool Win32Window::classRegistered_ = false;

Win32Window::Win32Window() {
    registerClass();
}

Win32Window::~Win32Window() {
    destroy();
    delete canvas_;
    canvas_ = nullptr;
}

void Win32Window::registerClass() {
    if (classRegistered_) return;

    WNDCLASSEXW wc = {};
    wc.cbSize = sizeof(WNDCLASSEXW);
    wc.style = CS_HREDRAW | CS_VREDRAW | CS_DBLCLKS;
    wc.lpfnWndProc = [](HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) -> LRESULT {
        Win32Window* self = nullptr;
        if (msg == WM_NCCREATE) {
            auto* cs = reinterpret_cast<CREATESTRUCT*>(lParam);
            self = static_cast<Win32Window*>(cs->lpCreateParams);
            SetWindowLongPtrW(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(self));
            self->hwnd_ = hwnd;
        } else {
            self = reinterpret_cast<Win32Window*>(GetWindowLongPtrW(hwnd, GWLP_USERDATA));
        }
        if (self) {
            return self->handleMessage(msg, wParam, lParam);
        }
        return DefWindowProcW(hwnd, msg, wParam, lParam);
    };
    wc.hInstance = GetModuleHandleW(nullptr);
    wc.hCursor = LoadCursorW(nullptr, IDC_ARROW);
    wc.hbrBackground = reinterpret_cast<HBRUSH>(COLOR_WINDOW + 1);
    wc.lpszClassName = L"ltgui_Window";

    RegisterClassExW(&wc);
    classRegistered_ = true;
}

bool Win32Window::create(int width, int height, const std::string& title) {
    if (hwnd_) return true;

    int len = MultiByteToWideChar(CP_UTF8, 0, title.c_str(), -1, nullptr, 0);
    std::wstring wtitle(len, L'\0');
    MultiByteToWideChar(CP_UTF8, 0, title.c_str(), -1, &wtitle[0], len);

    RECT rect = {0, 0, width, height};
    AdjustWindowRect(&rect, WS_OVERLAPPEDWINDOW, FALSE);

    hwnd_ = CreateWindowExW(
        0, L"ltgui_Window", wtitle.c_str(),
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT,
        rect.right - rect.left, rect.bottom - rect.top,
        nullptr, nullptr, GetModuleHandleW(nullptr), this);

    if (!hwnd_) return false;

    size_.width = width;
    size_.height = height;

    canvas_ = new Win32Canvas(hwnd_);
    canvas_->resize(width, height);

    return true;
}

void Win32Window::destroy() {
    if (hwnd_) {
        DestroyWindow(hwnd_);
        hwnd_ = nullptr;
    }
}

void Win32Window::show() {
    if (hwnd_) {
        ShowWindow(hwnd_, SW_SHOW);
        UpdateWindow(hwnd_);
    }
}

void Win32Window::hide() {
    if (hwnd_) {
        ShowWindow(hwnd_, SW_HIDE);
    }
}

void Win32Window::close() {
    if (hwnd_) {
        PostMessageW(hwnd_, WM_CLOSE, 0, 0);
    }
}

void Win32Window::setTitle(const std::string& title) {
    if (!hwnd_) return;
    int len = MultiByteToWideChar(CP_UTF8, 0, title.c_str(), -1, nullptr, 0);
    std::wstring wtitle(len, L'\0');
    MultiByteToWideChar(CP_UTF8, 0, title.c_str(), -1, &wtitle[0], len);
    SetWindowTextW(hwnd_, wtitle.c_str());
}

void Win32Window::setSize(int width, int height) {
    if (!hwnd_) return;
    RECT rect = {0, 0, width, height};
    AdjustWindowRect(&rect, WS_OVERLAPPEDWINDOW, FALSE);
    SetWindowPos(hwnd_, nullptr, 0, 0,
                 rect.right - rect.left, rect.bottom - rect.top,
                 SWP_NOMOVE | SWP_NOZORDER);
    size_.width = width;
    size_.height = height;
    if (canvas_) canvas_->resize(width, height);
}

Size Win32Window::getSize() const {
    return size_;
}

void Win32Window::invalidate(const Rect& rect) {
    if (hwnd_) {
        RECT r = {rect.x, rect.y, rect.right(), rect.bottom()};
        InvalidateRect(hwnd_, &r, FALSE);
    }
}

void* Win32Window::nativeHandle() const {
    return hwnd_;
}

NativeCanvas* Win32Window::getCanvas() {
    return canvas_;
}

LRESULT Win32Window::handleMessage(UINT msg, WPARAM wParam, LPARAM lParam) {
    if (!eventCallback_) return DefWindowProcW(hwnd_, msg, wParam, lParam);

    switch (msg) {
    case WM_PAINT: {
        Event ev;
        ev.type = EventType::Paint;
        ev.width = size_.width;
        ev.height = size_.height;
        eventCallback_(ev);
        ValidateRect(hwnd_, nullptr);
        return 0;
    }

    case WM_SIZE: {
        size_.width = LOWORD(lParam);
        size_.height = HIWORD(lParam);
        if (canvas_) canvas_->resize(size_.width, size_.height);

        Event ev;
        ev.type = EventType::Resize;
        ev.width = size_.width;
        ev.height = size_.height;
        eventCallback_(ev);
        return 0;
    }

    case WM_CLOSE: {
        Event ev;
        ev.type = EventType::Close;
        eventCallback_(ev);
        return 0;
    }

    case WM_MOUSEMOVE: {
        if (!trackingMouse_) {
            TRACKMOUSEEVENT tme = {sizeof(TRACKMOUSEEVENT), TME_LEAVE, hwnd_, 0};
            TrackMouseEvent(&tme);
            trackingMouse_ = true;
        }
        Event ev;
        ev.type = EventType::MouseMove;
        ev.pos = {GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam)};
        eventCallback_(ev);
        return 0;
    }

    case WM_MOUSELEAVE: {
        trackingMouse_ = false;
        Event ev;
        ev.type = EventType::MouseMove;
        ev.pos = {-1, -1};
        eventCallback_(ev);
        return 0;
    }

    case WM_LBUTTONDOWN:
    case WM_LBUTTONDBLCLK: {
        SetCapture(hwnd_);
        Event ev;
        ev.type = EventType::MouseDown;
        ev.button = MouseButton::Left;
        ev.pos = {GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam)};
        eventCallback_(ev);
        return 0;
    }

    case WM_LBUTTONUP: {
        ReleaseCapture();
        Event ev;
        ev.type = EventType::MouseUp;
        ev.button = MouseButton::Left;
        ev.pos = {GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam)};
        eventCallback_(ev);
        return 0;
    }

    case WM_RBUTTONDOWN: {
        SetCapture(hwnd_);
        Event ev;
        ev.type = EventType::MouseDown;
        ev.button = MouseButton::Right;
        ev.pos = {GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam)};
        eventCallback_(ev);
        return 0;
    }

    case WM_RBUTTONUP: {
        ReleaseCapture();
        Event ev;
        ev.type = EventType::MouseUp;
        ev.button = MouseButton::Right;
        ev.pos = {GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam)};
        eventCallback_(ev);
        return 0;
    }

    case WM_MBUTTONDOWN: {
        SetCapture(hwnd_);
        Event ev;
        ev.type = EventType::MouseDown;
        ev.button = MouseButton::Middle;
        ev.pos = {GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam)};
        eventCallback_(ev);
        return 0;
    }

    case WM_MBUTTONUP: {
        ReleaseCapture();
        Event ev;
        ev.type = EventType::MouseUp;
        ev.button = MouseButton::Middle;
        ev.pos = {GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam)};
        eventCallback_(ev);
        return 0;
    }

    case WM_MOUSEWHEEL: {
        Event ev;
        ev.type = EventType::MouseWheel;
        ev.wheelDelta = GET_WHEEL_DELTA_WPARAM(wParam) / WHEEL_DELTA;
        ev.pos = {GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam)};
        eventCallback_(ev);
        return 0;
    }

    case WM_KEYDOWN:
    case WM_SYSKEYDOWN: {
        Event ev;
        ev.type = EventType::KeyDown;
        // Map common virtual keys
        switch (wParam) {
        case VK_BACK:   ev.key = Key::Backspace; break;
        case VK_TAB:    ev.key = Key::Tab; break;
        case VK_RETURN: ev.key = Key::Enter; break;
        case VK_ESCAPE: ev.key = Key::Escape; break;
        case VK_SPACE:  ev.key = Key::Space; break;
        case VK_LEFT:   ev.key = Key::Left; break;
        case VK_RIGHT:  ev.key = Key::Right; break;
        case VK_UP:     ev.key = Key::Up; break;
        case VK_DOWN:   ev.key = Key::Down; break;
        case VK_HOME:   ev.key = Key::Home; break;
        case VK_END:    ev.key = Key::End; break;
        case VK_PRIOR:  ev.key = Key::PageUp; break;
        case VK_NEXT:   ev.key = Key::PageDown; break;
        case VK_DELETE: ev.key = Key::Delete; break;
        case VK_INSERT: ev.key = Key::Insert; break;
        case VK_F1: case VK_F2: case VK_F3: case VK_F4:
        case VK_F5: case VK_F6: case VK_F7: case VK_F8:
        case VK_F9: case VK_F10: case VK_F11: case VK_F12:
            ev.key = static_cast<Key>(static_cast<int>(Key::F1) + static_cast<int>(wParam - VK_F1));
            break;
        default: ev.key = Key::Unknown; break;
        }
        eventCallback_(ev);
        return 0;
    }

    case WM_CHAR: {
        Event ev;
        ev.type = EventType::KeyDown;
        ev.charCode = static_cast<unsigned int>(wParam);
        ev.key = Key::Unknown;
        eventCallback_(ev);
        return 0;
    }

    case WM_KEYUP:
    case WM_SYSKEYUP: {
        Event ev;
        ev.type = EventType::KeyUp;
        switch (wParam) {
        case VK_BACK:   ev.key = Key::Backspace; break;
        case VK_TAB:    ev.key = Key::Tab; break;
        case VK_RETURN: ev.key = Key::Enter; break;
        case VK_ESCAPE: ev.key = Key::Escape; break;
        case VK_SPACE:  ev.key = Key::Space; break;
        case VK_LEFT:   ev.key = Key::Left; break;
        case VK_RIGHT:  ev.key = Key::Right; break;
        case VK_UP:     ev.key = Key::Up; break;
        case VK_DOWN:   ev.key = Key::Down; break;
        case VK_HOME:   ev.key = Key::Home; break;
        case VK_END:    ev.key = Key::End; break;
        case VK_DELETE: ev.key = Key::Delete; break;
        default: ev.key = Key::Unknown; break;
        }
        eventCallback_(ev);
        return 0;
    }

    case WM_SETFOCUS: {
        Event ev;
        ev.type = EventType::FocusIn;
        eventCallback_(ev);
        return 0;
    }

    case WM_KILLFOCUS: {
        Event ev;
        ev.type = EventType::FocusOut;
        eventCallback_(ev);
        return 0;
    }

    case WM_IME_SETCONTEXT:
        if (wParam) {
            HIMC hIMC = ImmGetContext(hwnd_);
            if (hIMC) {
                ImmSetOpenStatus(hIMC, TRUE);
                ImmReleaseContext(hwnd_, hIMC);
            }
        }
        return DefWindowProcW(hwnd_, msg, wParam, lParam);

    case WM_IME_STARTCOMPOSITION: {
        HIMC hIMC = ImmGetContext(hwnd_);
        if (hIMC) {
            COMPOSITIONFORM cf = {};
            cf.dwStyle = CFS_POINT;
            cf.ptCurrentPos.x = 10;
            cf.ptCurrentPos.y = 10;
            ImmSetCompositionWindow(hIMC, &cf);
            ImmReleaseContext(hwnd_, hIMC);
        }
        return 0;
    }

    case WM_IME_COMPOSITION: {
        if (lParam & GCS_RESULTSTR) {
            HIMC hIMC = ImmGetContext(hwnd_);
            if (hIMC) {
                LONG len = ImmGetCompositionStringW(hIMC, GCS_RESULTSTR, nullptr, 0);
                if (len > 0) {
                    std::wstring wstr(len / sizeof(wchar_t), L'\0');
                    ImmGetCompositionStringW(hIMC, GCS_RESULTSTR, &wstr[0], len);
                    for (wchar_t wc : wstr) {
                        PostMessageW(hwnd_, WM_CHAR, wc, 0);
                    }
                }
                ImmReleaseContext(hwnd_, hIMC);
            }
        }
        return 0;
    }

    case WM_IME_ENDCOMPOSITION:
        return 0;

    case WM_DESTROY: {
        hwnd_ = nullptr;
        return 0;
    }
    }

    return DefWindowProcW(hwnd_, msg, wParam, lParam);
}

} // namespace ltgui

#endif // LTGUI_PLATFORM_WINDOWS
