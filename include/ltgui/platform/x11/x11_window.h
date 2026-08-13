#pragma once
#include "platform/platform.h"

#ifdef LTGUI_PLATFORM_LINUX

#include "platform/native_window.h"
#include <X11/Xlib.h>
#include <X11/Xutil.h>

// Clean up X11 macro pollution
#undef None
#undef FocusIn
#undef FocusOut
#undef ButtonPress
#undef ButtonRelease
#undef Button4
#undef Button5

#include <string>

namespace ltgui {

class X11Window : public NativeWindow {
public:
  X11Window();
  ~X11Window() override;

  bool create(int width, int height, const std::string &title) override;
  void destroy() override;
  void show() override;
  void hide() override;
  void close() override;
  void setTitle(const std::string &title) override;
  void setSize(int width, int height) override;
  Size getSize() const override;
  void invalidate(const Rect &rect) override;
  void *nativeHandle() const override;
  float dpiScale() const override;

  NativeCanvas *getCanvas() override;

  bool setClipboardText(const std::string &text) override;
  std::string getClipboardText() override;

  // Access the shared X11 display for select()/poll() integration
  static Display *display() { return s_display_; }
  static int displayFd() {
    return s_display_ ? ConnectionNumber(s_display_) : -1;
  }

  // Called by the application event loop
  void processEvents();

  // Static: process events for all X11 windows on the shared display
  static bool processAllPending();

private:
  void handleEvent(XEvent &xev);
  Key mapKeySym(KeySym ks) const;

  static void registerWindow(X11Window *w);
  static void unregisterWindow(X11Window *w);
  static X11Window *findWindow(::Window xid);

  static Display *s_display_;
  static int s_displayRefCount_;

  ::Window window_ = 0;
  NativeCanvas *canvas_ = nullptr;
  Size size_;
  bool mapped_ = false;
  bool ownsDisplayRef_ = false;
  Atom wmDeleteMessage_;
  Atom wmProtocols_;

  // Per-window clipboard state (replaces old file-scope globals)
  std::string clipboardText_;   // text we own on the clipboard
  bool clipboardOwned_ = false; // true if we currently own the selection
  // Pending async clipboard read
  std::string *pendingReadResult_ = nullptr;
  bool pendingReadDone_ = false;
};

} // namespace ltgui

#endif // LTGUI_PLATFORM_LINUX
