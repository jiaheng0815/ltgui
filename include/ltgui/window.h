#pragma once
#include "api.h"
#include "event.h"
#include "geometry.h"
#include "platform/native_canvas.h"
#include "platform/native_window.h"
#include "shortcut.h"
#include <memory>
#include <string>
#include <vector>

namespace ltgui {

class Widget;
class ComboBox;
namespace gpu {
class GpuCanvas;
}

class LTGUI_API Window {
public:
  Window();
  virtual ~Window();

  bool create(int width = 800, int height = 600,
              const std::string &title = "ltgui");
  void close();
  void show();
  void hide();

  void setTitle(const std::string &title);
  void setSize(int width, int height);
  Size size() const;
  [[deprecated("use size() instead")]] Size getSize() const { return size(); }

  void setCentralWidget(std::unique_ptr<Widget> widget);
  Widget *centralWidget() const { return centralWidget_.get(); }

  void update();
  void invalidate(const Rect &rect);
  void *nativeHandle() const;

  NativeCanvas *canvas() { return canvas_; }
  NativeWindow *nativeWindow() { return nativeWindow_.get(); }
  bool isGpuAccelerated() const { return useGpu_; }

  // DPI scale factor for this window
  float dpiScale() const {
    if (nativeWindow_)
      return nativeWindow_->dpiScale();
    return 1.0f;
  }

  void setCursor(CursorShape shape) {
    if (nativeWindow_)
      nativeWindow_->setCursor(shape);
  }

  // Focus management
  Widget *focusWidget() const { return focusWidget_; }

  // Modal dialog plumbing. While a modal dialog is registered, the window
  // paints it over the central widget and routes all mouse/keyboard events
  // to it first. Returns the previous modal dialog so nested dialogs can
  // restore the outer one when they finish.
  Widget *setModalDialog(Widget *dialog);
  Widget *modalDialog() const { return modalDialog_; }

  // Open MenuBar dropdown plumbing: while registered the window routes
  // mouse/keyboard input to the menubar so its open dropdown receives
  // events even outside the menubar's base rect.
  void setOpenMenuBar(Widget *menubar);
  Widget *openMenuBar() const { return openMenuBar_; }

  // Open ContextMenu plumbing (analogous to the MenuBar dropdown).
  void setOpenContextMenu(Widget *menu);
  Widget *openContextMenu() const { return openContextMenu_; }

  // Keyboard shortcuts: registered shortcuts are checked before widget
  // event dispatch. If a shortcut matches, its callback fires and the
  // key event is consumed.
  void registerShortcut(const Shortcut &sc, Shortcut::Callback cb);
  void unregisterShortcut(const Shortcut &sc);

protected:
  virtual void onPaint(NativeCanvas *canvas, const Rect &dirtyRect);

private:
  friend class Application;
  void handleEvent(Event &event);

  friend class Widget;
  friend class ComboBox;
  friend class Dialog;
  void setFocusWidget(Widget *w);

  // Per-event-type handlers (split from handleEvent for readability)
  void handlePaintEvent(Event &event);
  void handleResizeEvent(Event &event);
  void handleCloseEvent(Event &event);
  void handleKeyEvent(Event &event);
  void handleMouseEvent(Event &event);

  // Guard against dangling focusWidget_: if the focus widget is no longer in
  // this window's tree (e.g. it was destroyed or reparented), clear and
  // return false. Callers should bail out on false.
  bool validateFocusWidget();

  std::unique_ptr<NativeWindow> nativeWindow_;
  std::unique_ptr<gpu::GpuCanvas> gpuCanvas_;
  NativeCanvas *canvas_ = nullptr;
  std::unique_ptr<Widget> centralWidget_;
  Widget *focusWidget_ = nullptr;
  ComboBox *openCombo_ = nullptr;
  Widget *openMenuBar_ = nullptr;
  Widget *openContextMenu_ = nullptr;
  Widget *modalDialog_ = nullptr;
  Rect accumulatedDirty_;
  bool dirtyValid_ = false;
  bool useGpu_ = false;

  struct ShortcutEntry {
    Shortcut shortcut;
    Shortcut::Callback callback;
  };
  std::vector<ShortcutEntry> shortcuts_;
};

} // namespace ltgui
