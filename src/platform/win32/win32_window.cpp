#include "platform/win32/win32_window.h"

#ifdef LTGUI_PLATFORM_WINDOWS

#include "app.h"
#include "log.h"
#include "platform/win32/win32_canvas.h"
#include <cassert>
#include <imm.h>
#include <windowsx.h>

// WinUser.h only defines WM_DPICHANGED when _WIN32_WINNT >= 0x0605; make
// sure the message is usable on toolchains that default to an older target.
#ifndef WM_DPICHANGED
#define WM_DPICHANGED 0x02E0
#endif

namespace ltgui {

bool Win32Window::classRegistered_ = false;

static void setDpiAwareness() {
  // Try PerMonitorV2 (Win10 1703+), fall back to System DPI awareness
  HMODULE shcore = LoadLibraryA("shcore.dll");
  if (shcore) {
    typedef HRESULT(WINAPI * SetProcessDpiAwarenessFunc)(int);
    auto fn = (SetProcessDpiAwarenessFunc)GetProcAddress(
        shcore, "SetProcessDpiAwareness");
    if (fn)
      fn(2); // PROCESS_PER_MONITOR_DPI_AWARE = 2
    FreeLibrary(shcore);
  } else {
    SetProcessDPIAware();
  }
}

// AdjustWindowRectExForDpi (Win10 1607+) sizes the window frame for the
// given DPI; on older systems (or non-DPI-aware processes) fall back to
// the classic 96-dpi based AdjustWindowRect.
static void adjustRectForDpi(RECT *rect, DWORD style, BOOL menu, UINT dpi) {
  typedef BOOL(WINAPI * AdjustWindowRectExForDpiFunc)(RECT *, DWORD, BOOL,
                                                      DWORD, UINT);
  auto fn = (AdjustWindowRectExForDpiFunc)GetProcAddress(
      GetModuleHandleW(L"user32.dll"), "AdjustWindowRectExForDpi");
  if (fn) {
    fn(rect, style, menu, 0, dpi);
  } else {
    AdjustWindowRect(rect, style, menu);
  }
}

static LPCWSTR cursorIdOf(CursorShape shape) {
  switch (shape) {
  case CursorShape::Arrow:
    return IDC_ARROW;
  case CursorShape::IBeam:
    return IDC_IBEAM;
  case CursorShape::Wait:
    return IDC_WAIT;
  case CursorShape::Crosshair:
    return IDC_CROSS;
  case CursorShape::SizeWE:
    return IDC_SIZEWE;
  case CursorShape::SizeNS:
    return IDC_SIZENS;
  case CursorShape::SizeAll:
    return IDC_SIZEALL;
  case CursorShape::Hand:
    return IDC_HAND;
  case CursorShape::Denied:
    return IDC_NO;
  }
  return IDC_ARROW;
}

Win32Window::Win32Window() { registerClass(); }

Win32Window::~Win32Window() { destroy(); }

void Win32Window::registerClass() {
  if (classRegistered_)
    return;

  setDpiAwareness();

  WNDCLASSEXW wc = {};
  wc.cbSize = sizeof(WNDCLASSEXW);
  wc.style = CS_HREDRAW | CS_VREDRAW | CS_DBLCLKS;
  wc.lpfnWndProc = [](HWND hwnd, UINT msg, WPARAM wParam,
                      LPARAM lParam) -> LRESULT {
    Win32Window *self = nullptr;
    if (msg == WM_NCCREATE) {
      auto *cs = reinterpret_cast<CREATESTRUCT *>(lParam);
      self = static_cast<Win32Window *>(cs->lpCreateParams);
      SetWindowLongPtrW(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(self));
      self->hwnd_ = hwnd;
    } else {
      self = reinterpret_cast<Win32Window *>(
          GetWindowLongPtrW(hwnd, GWLP_USERDATA));
    }
    if (self) {
      return self->handleMessage(hwnd, msg, wParam, lParam);
    }
    return DefWindowProcW(hwnd, msg, wParam, lParam);
  };
  wc.hInstance = GetModuleHandleW(nullptr);
  wc.hCursor = LoadCursorW(nullptr, IDC_ARROW);
  wc.hbrBackground =
      nullptr; // Suppress system background paint to prevent flicker
  wc.lpszClassName = L"ltgui_Window";

  if (RegisterClassExW(&wc) == 0 && GetLastError() != ERROR_CLASS_ALREADY_EXISTS) {
    LOG_ERROR("Win32", "RegisterClassExW failed, error=%lu", GetLastError());
    return;
  }
  classRegistered_ = true;
}

bool Win32Window::create(int width, int height, const std::string &title) {
  if (hwnd_)
    return true;

  int len = MultiByteToWideChar(CP_UTF8, 0, title.c_str(), -1, nullptr, 0);
  std::wstring wtitle(len, L'\0');
  MultiByteToWideChar(CP_UTF8, 0, title.c_str(), -1, &wtitle[0], len);

  // The frame is computed on the 96-dpi baseline here because the target
  // monitor's DPI is not known until the window exists (dpiScale_ is
  // queried below with GetDC); PerMonitorV2 rescales the window via
  // WM_DPICHANGED once it is created.
  RECT rect = {0, 0, width, height};
  AdjustWindowRect(&rect, WS_OVERLAPPEDWINDOW, FALSE);

  hwnd_ = CreateWindowExW(0, L"ltgui_Window", wtitle.c_str(),
                          WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT,
                          rect.right - rect.left, rect.bottom - rect.top,
                          nullptr, nullptr, GetModuleHandleW(nullptr), this);

  if (!hwnd_)
    return false;

  size_.width = width;
  size_.height = height;

  // Query DPI scale for this window
  HDC hdc = GetDC(hwnd_);
  if (hdc) {
    dpiScale_ = GetDeviceCaps(hdc, LOGPIXELSX) / 96.0f;
    ReleaseDC(hwnd_, hdc);
  }

  canvas_ = std::make_unique<Win32Canvas>(hwnd_);
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
  // DestroyWindow sends WM_DESTROY synchronously (not WM_CLOSE), so it
  // does NOT re-enter the WM_CLOSE handler.  PostMessage(WM_CLOSE) would
  // create an infinite loop when called from inside the close handler.
  LOG_DEBUG("Win32", "close() called, hwnd=%p", hwnd_);
  if (hwnd_) {
    DestroyWindow(hwnd_);
    LOG_DEBUG("Win32", "DestroyWindow returned, hwnd=%p", hwnd_);
  }
}

void Win32Window::setTitle(const std::string &title) {
  if (!hwnd_)
    return;
  int len = MultiByteToWideChar(CP_UTF8, 0, title.c_str(), -1, nullptr, 0);
  std::wstring wtitle(len, L'\0');
  MultiByteToWideChar(CP_UTF8, 0, title.c_str(), -1, &wtitle[0], len);
  SetWindowTextW(hwnd_, wtitle.c_str());
}

void Win32Window::setSize(int width, int height) {
  if (!hwnd_)
    return;
  RECT rect = {0, 0, width, height};
  adjustRectForDpi(&rect, WS_OVERLAPPEDWINDOW, FALSE,
                   static_cast<UINT>(dpiScale_ * 96.0f));
  SetWindowPos(hwnd_, nullptr, 0, 0, rect.right - rect.left,
               rect.bottom - rect.top, SWP_NOMOVE | SWP_NOZORDER);
  size_.width = width;
  size_.height = height;
  if (canvas_)
    canvas_->resize(width, height);
}

Size Win32Window::getSize() const { return size_; }

void Win32Window::invalidate(const Rect &rect) {
  if (hwnd_) {
    RECT r = {rect.x, rect.y, rect.right(), rect.bottom()};
    InvalidateRect(hwnd_, &r, FALSE);
  }
}

void Win32Window::setCursor(CursorShape shape) {
  cursorShape_ = shape;
  SetCursor(LoadCursorW(nullptr, cursorIdOf(shape)));
}

bool Win32Window::setClipboardText(const std::string &text) {
  // Prepare the payload first — a failure here must not touch the
  // user's current clipboard contents.
  int wlen = MultiByteToWideChar(CP_UTF8, 0, text.c_str(), -1, nullptr, 0);
  if (wlen <= 0)
    return false;

  HGLOBAL hMem = GlobalAlloc(GMEM_MOVEABLE, wlen * sizeof(wchar_t));
  if (!hMem)
    return false;

  wchar_t *p = static_cast<wchar_t *>(GlobalLock(hMem));
  if (!p) {
    GlobalFree(hMem);
    return false;
  }
  MultiByteToWideChar(CP_UTF8, 0, text.c_str(), -1, p, wlen);
  GlobalUnlock(hMem);

  if (!OpenClipboard(nullptr)) {
    GlobalFree(hMem);
    return false;
  }
  EmptyClipboard();
  bool ok = SetClipboardData(CF_UNICODETEXT, hMem) != nullptr;
  CloseClipboard();
  // On success the clipboard owns hMem; only free it if the hand-off
  // failed.
  if (!ok)
    GlobalFree(hMem);
  return ok;
}

std::string Win32Window::getClipboardText() {
  if (!OpenClipboard(nullptr))
    return {};
  HANDLE hData = GetClipboardData(CF_UNICODETEXT);
  if (!hData) {
    CloseClipboard();
    return {};
  }
  wchar_t *p = static_cast<wchar_t *>(GlobalLock(hData));
  std::string result;
  if (p) {
    int len =
        WideCharToMultiByte(CP_UTF8, 0, p, -1, nullptr, 0, nullptr, nullptr);
    if (len > 0) {
      // len includes the terminating NUL — size the buffer so the
      // conversion fits, then drop the NUL from the result string.
      result.resize(len);
      WideCharToMultiByte(CP_UTF8, 0, p, -1, &result[0], len, nullptr, nullptr);
      result.pop_back();
    }
    GlobalUnlock(hData);
  }
  CloseClipboard();
  return result;
}

void *Win32Window::nativeHandle() const { return hwnd_; }

NativeCanvas *Win32Window::getCanvas() { return canvas_.get(); }

LRESULT Win32Window::handleMessage(HWND hwnd, UINT msg, WPARAM wParam,
                                   LPARAM lParam) {
  // Guard against cross-thread SendMessage into the widget/GPU pipeline.
  // Use the message's hwnd (not hwnd_): after WM_DESTROY, hwnd_ is null
  // but the system still delivers WM_NCDESTROY through this handler.
  assert(isMainThread() && "Win32 window message on non-main thread");
  if (!eventCallback_)
    return DefWindowProcW(hwnd, msg, wParam, lParam);

  switch (msg) {
  case WM_PAINT: {
    // BeginPaint/EndPaint pair: EndPaint validates the region captured
    // at BeginPaint time, so regions invalidated while we paint remain
    // dirty and re-trigger WM_PAINT.
    PAINTSTRUCT ps;
    HDC hdc = BeginPaint(hwnd, &ps);
    (void)hdc; // painting itself happens via Win32Canvas (dirty-rect blit)
    Event ev;
    ev.type = EventType::Paint;
    ev.width = size_.width;
    ev.height = size_.height;
    eventCallback_(ev);
    EndPaint(hwnd, &ps);
    return 0;
  }

  case WM_SIZE: {
    size_.width = LOWORD(lParam);
    size_.height = HIWORD(lParam);
    if (canvas_)
      canvas_->resize(size_.width, size_.height);

    Event ev;
    ev.type = EventType::Resize;
    ev.width = size_.width;
    ev.height = size_.height;
    eventCallback_(ev);
    return 0;
  }

  case WM_CLOSE: {
    LOG_DEBUG("Win32", "WM_CLOSE received, hwnd=%p", hwnd_);
    Event ev;
    ev.type = EventType::Close;
    eventCallback_(ev);
    LOG_DEBUG("Win32", "WM_CLOSE handler returned, hwnd=%p", hwnd_);
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
    pressedButton_ = MouseButton::Left;
    SetCapture(hwnd_);
    Event ev;
    ev.type = EventType::MouseDown;
    ev.button = MouseButton::Left;
    ev.pos = {GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam)};
    eventCallback_(ev);
    return 0;
  }

  case WM_LBUTTONUP: {
    Event ev;
    ev.type = EventType::MouseUp;
    ev.button = MouseButton::Left;
    ev.pos = {GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam)};
    // Dispatch before releasing capture so the synthesized MouseUp from
    // WM_CAPTURECHANGED always follows the real one.
    eventCallback_(ev);
    ReleaseCapture();
    return 0;
  }

  case WM_RBUTTONDOWN: {
    pressedButton_ = MouseButton::Right;
    SetCapture(hwnd_);
    Event ev;
    ev.type = EventType::MouseDown;
    ev.button = MouseButton::Right;
    ev.pos = {GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam)};
    eventCallback_(ev);
    return 0;
  }

  case WM_RBUTTONUP: {
    Event ev;
    ev.type = EventType::MouseUp;
    ev.button = MouseButton::Right;
    ev.pos = {GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam)};
    eventCallback_(ev);
    ReleaseCapture();
    return 0;
  }

  case WM_MBUTTONDOWN: {
    pressedButton_ = MouseButton::Middle;
    SetCapture(hwnd_);
    Event ev;
    ev.type = EventType::MouseDown;
    ev.button = MouseButton::Middle;
    ev.pos = {GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam)};
    eventCallback_(ev);
    return 0;
  }

  case WM_MBUTTONUP: {
    Event ev;
    ev.type = EventType::MouseUp;
    ev.button = MouseButton::Middle;
    ev.pos = {GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam)};
    eventCallback_(ev);
    ReleaseCapture();
    return 0;
  }

  case WM_CAPTURECHANGED: {
    // Capture was taken away (system menu, Alt+Tab, another window).
    // Synthesize a MouseUp so pressed state resets. A normal click also
    // passes through here via ReleaseCapture, but its MouseUp was already
    // dispatched, so the extra event is a no-op for widgets.
    POINT pt;
    GetCursorPos(&pt);
    ScreenToClient(hwnd, &pt);
    Event ev;
    ev.type = EventType::MouseUp;
    ev.button = pressedButton_;
    ev.pos = {pt.x, pt.y};
    eventCallback_(ev);
    return 0;
  }

  case WM_MOUSEWHEEL: {
    Event ev;
    ev.type = EventType::MouseWheel;
    ev.wheelDelta = GET_WHEEL_DELTA_WPARAM(wParam) / WHEEL_DELTA;
    // WM_MOUSEWHEEL lParam is screen coordinates; convert to client
    POINT pt = {GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam)};
    ScreenToClient(hwnd_, &pt);
    ev.pos = {pt.x, pt.y};
    eventCallback_(ev);
    return 0;
  }

  case WM_SETCURSOR:
    // Keep the cursor in sync with the requested shape even after the
    // window re-takes the pick (mouse leaving and re-entering, etc.).
    if (LOWORD(lParam) == HTCLIENT) {
      SetCursor(LoadCursorW(nullptr, cursorIdOf(cursorShape_)));
      return TRUE;
    }
    break;

  case WM_DPICHANGED: {
    // Window moved to a monitor with a different DPI (PerMonitorV2).
    // Refresh the scale, then adopt the system-suggested rect; the
    // following WM_SIZE re-sizes the canvas and dispatches Resize
    // through the existing path.
    float newScale = HIWORD(wParam) / 96.0f;
    if (dpiScale_ != newScale) {
      dpiScale_ = newScale;
      RECT *pr = reinterpret_cast<RECT *>(lParam);
      SetWindowPos(hwnd_, nullptr, pr->left, pr->top, pr->right - pr->left,
                   pr->bottom - pr->top, SWP_NOZORDER | SWP_NOACTIVATE);
    }
    return 0;
  }

  case WM_KEYDOWN: {
    // Filter auto-repeat: bit 30 of lParam is 1 if this is a repeated key
    if (lParam & 0x40000000)
      return 0;

    Event ev;
    ev.type = EventType::KeyDown;
    // Track modifier state
    if (GetKeyState(VK_CONTROL) & 0x8000)
      ev.modifiers |= static_cast<int>(KeyModifier::Control);
    if (GetKeyState(VK_SHIFT) & 0x8000)
      ev.modifiers |= static_cast<int>(KeyModifier::Shift);
    if (GetKeyState(VK_MENU) & 0x8000)
      ev.modifiers |= static_cast<int>(KeyModifier::Alt);
    // Map common virtual keys
    switch (wParam) {
    case VK_BACK:
      ev.key = Key::Backspace;
      break;
    case VK_TAB:
      ev.key = Key::Tab;
      break;
    case VK_RETURN:
      ev.key = Key::Enter;
      break;
    case VK_ESCAPE:
      ev.key = Key::Escape;
      break;
    case VK_SPACE:
      ev.key = Key::Space;
      break;
    case VK_LEFT:
      ev.key = Key::Left;
      break;
    case VK_RIGHT:
      ev.key = Key::Right;
      break;
    case VK_UP:
      ev.key = Key::Up;
      break;
    case VK_DOWN:
      ev.key = Key::Down;
      break;
    case VK_HOME:
      ev.key = Key::Home;
      break;
    case VK_END:
      ev.key = Key::End;
      break;
    case VK_PRIOR:
      ev.key = Key::PageUp;
      break;
    case VK_NEXT:
      ev.key = Key::PageDown;
      break;
    case VK_DELETE:
      ev.key = Key::Delete;
      break;
    case VK_INSERT:
      ev.key = Key::Insert;
      break;
    case VK_F1:
    case VK_F2:
    case VK_F3:
    case VK_F4:
    case VK_F5:
    case VK_F6:
    case VK_F7:
    case VK_F8:
    case VK_F9:
    case VK_F10:
    case VK_F11:
    case VK_F12:
      ev.key = static_cast<Key>(static_cast<int>(Key::F1) +
                                static_cast<int>(wParam - VK_F1));
      break;
    default:
      ev.key = Key::Unknown;
      break;
    }
    eventCallback_(ev);
    return 0;
  }

  case WM_SYSKEYDOWN: {
    // Let system key combinations (Alt+F4, Alt+Space, etc.) reach
    // DefWindowProcW so the OS can handle them.
    return DefWindowProcW(hwnd, msg, wParam, lParam);
  }

  case WM_CHAR: {
    // Filter auto-repeat for character input too
    if (lParam & 0x40000000)
      return 0;

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
    case VK_BACK:
      ev.key = Key::Backspace;
      break;
    case VK_TAB:
      ev.key = Key::Tab;
      break;
    case VK_RETURN:
      ev.key = Key::Enter;
      break;
    case VK_ESCAPE:
      ev.key = Key::Escape;
      break;
    case VK_SPACE:
      ev.key = Key::Space;
      break;
    case VK_LEFT:
      ev.key = Key::Left;
      break;
    case VK_RIGHT:
      ev.key = Key::Right;
      break;
    case VK_UP:
      ev.key = Key::Up;
      break;
    case VK_DOWN:
      ev.key = Key::Down;
      break;
    case VK_HOME:
      ev.key = Key::Home;
      break;
    case VK_END:
      ev.key = Key::End;
      break;
    case VK_DELETE:
      ev.key = Key::Delete;
      break;
    default:
      ev.key = Key::Unknown;
      break;
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
    // Don't force the IME open — the user's input mode is their choice.
    // Keep the context fetch/release pair and let DefWindowProcW handle
    // the rest of the message.
    if (wParam) {
      HIMC hIMC = ImmGetContext(hwnd_);
      if (hIMC)
        ImmReleaseContext(hwnd_, hIMC);
    }
    return DefWindowProcW(hwnd, msg, wParam, lParam);

  case WM_IME_STARTCOMPOSITION: {
    HIMC hIMC = ImmGetContext(hwnd_);
    if (hIMC) {
      COMPOSITIONFORM cf = {};
      cf.dwStyle = CFS_POINT;
      // Position near bottom-left of client area — caller should
      // call setImeCursorPos() to position at the actual text cursor
      cf.ptCurrentPos.x = imeCompX_;
      cf.ptCurrentPos.y = imeCompY_;
      ImmSetCompositionWindow(hIMC, &cf);

      // Set candidate window position below the composition window
      CANDIDATEFORM candForm = {};
      candForm.dwIndex = 0;
      candForm.dwStyle = CFS_CANDIDATEPOS;
      candForm.ptCurrentPos.x = imeCompX_;
      candForm.ptCurrentPos.y = imeCompY_ + 20;
      ImmSetCandidateWindow(hIMC, &candForm);

      ImmReleaseContext(hwnd_, hIMC);
    }
    return 0;
  }

  case WM_IME_COMPOSITION: {
    HIMC hIMC = ImmGetContext(hwnd_);
    if (hIMC) {
      // Send preedit (composition) string as ImeComposition event
      if (lParam & GCS_COMPSTR) {
        LONG len = ImmGetCompositionStringW(hIMC, GCS_COMPSTR, nullptr, 0);
        if (len > 0) {
          std::wstring wstr(len / sizeof(wchar_t), L'\0');
          ImmGetCompositionStringW(hIMC, GCS_COMPSTR, &wstr[0], len);

          // Convert to UTF-8 for the event
          int utf8Len = WideCharToMultiByte(CP_UTF8, 0, wstr.c_str(),
                                            static_cast<int>(wstr.size()),
                                            nullptr, 0, nullptr, nullptr);
          std::string utf8(utf8Len, '\0');
          WideCharToMultiByte(CP_UTF8, 0, wstr.c_str(),
                              static_cast<int>(wstr.size()), &utf8[0], utf8Len,
                              nullptr, nullptr);

          // Query the actual cursor position within the composition.
          // GCS_CURSORPOS is reported as a byte offset even for the W
          // API — convert to UTF-16 character count before comparing
          // with wstr.size() / substringing.
          LONG cursorPos =
              ImmGetCompositionStringW(hIMC, GCS_CURSORPOS, nullptr, 0);
          if (cursorPos >= 0)
            cursorPos /= static_cast<LONG>(sizeof(wchar_t));
          int imeCursorByte =
              static_cast<int>(utf8.size()); // fallback: end of string
          if (cursorPos >= 0 && static_cast<size_t>(cursorPos) <= wstr.size()) {
            // Convert the UTF-16 prefix up to the cursor to UTF-8 byte offset
            std::wstring wstrBefore =
                wstr.substr(0, static_cast<size_t>(cursorPos));
            int beforeLen =
                WideCharToMultiByte(CP_UTF8, 0, wstrBefore.c_str(),
                                    static_cast<int>(wstrBefore.size()),
                                    nullptr, 0, nullptr, nullptr);
            if (beforeLen > 0) {
              imeCursorByte = beforeLen;
            }
          }

          Event ev;
          ev.type = EventType::ImeComposition;
          ev.imeText = utf8;
          ev.imeCursor = imeCursorByte;
          eventCallback_(ev);
        }
      }

      // Committed text → post as WM_CHAR messages
      if (lParam & GCS_RESULTSTR) {
        LONG len = ImmGetCompositionStringW(hIMC, GCS_RESULTSTR, nullptr, 0);
        if (len > 0) {
          std::wstring wstr(len / sizeof(wchar_t), L'\0');
          ImmGetCompositionStringW(hIMC, GCS_RESULTSTR, &wstr[0], len);
          for (wchar_t wc : wstr) {
            PostMessageW(hwnd_, WM_CHAR, wc, 0);
          }
        }
      }
      ImmReleaseContext(hwnd_, hIMC);
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

  return DefWindowProcW(hwnd, msg, wParam, lParam);
}

} // namespace ltgui

#endif // LTGUI_PLATFORM_WINDOWS
