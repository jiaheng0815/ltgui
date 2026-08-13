#include "clipboard.h"
#include "app.h"
#include "platform/native_window.h"
#include "window.h"

namespace ltgui {

void ClipboardData::setImage(const uint8_t *rgba, int w, int h) {
  if (!rgba || w <= 0 || h <= 0)
    return;
  imageW_ = w;
  imageH_ = h;
  imageRGBA_.assign(rgba, rgba + w * h * 4);
}

std::vector<ClipboardFormat> ClipboardData::availableFormats() const {
  std::vector<ClipboardFormat> fmts;
  if (!text_.empty())
    fmts.push_back(ClipboardFormat::Text);
  if (!html_.empty())
    fmts.push_back(ClipboardFormat::RichText);
  if (!imageRGBA_.empty())
    fmts.push_back(ClipboardFormat::Image);
  if (!files_.empty())
    fmts.push_back(ClipboardFormat::Files);
  return fmts;
}

bool ClipboardData::hasFormat(ClipboardFormat f) const {
  switch (f) {
  case ClipboardFormat::Text:
    return !text_.empty();
  case ClipboardFormat::RichText:
    return !html_.empty();
  case ClipboardFormat::Image:
    return !imageRGBA_.empty();
  case ClipboardFormat::Files:
    return !files_.empty();
  }
  return false;
}

// Find the first window with a valid native window handle.
// Previously this always returned wins[0], which is wrong in multi-window
// apps or when the first window was closed but others remain open.
static NativeWindow *getNativeWindow() {
  auto &wins = Application::instance().windows();
  for (auto *w : wins) {
    if (w && w->nativeWindow() && w->nativeWindow()->nativeHandle()) {
      return w->nativeWindow();
    }
  }
  return nullptr;
}

std::string Clipboard::getText() {
  auto *nw = getNativeWindow();
  return nw ? nw->getClipboardText() : "";
}

void Clipboard::setText(const std::string &text) {
  auto *nw = getNativeWindow();
  if (nw)
    nw->setClipboardText(text);
}

ClipboardData Clipboard::getData() {
  ClipboardData d;
  auto *nw = getNativeWindow();
  if (!nw)
    return d;
  d.setText(nw->getClipboardText());
  return d;
}

void Clipboard::setData(const ClipboardData &data) {
  auto *nw = getNativeWindow();
  if (!nw)
    return;
  nw->setClipboardText(data.text());
}

std::vector<ClipboardFormat> Clipboard::availableFormats() {
  std::vector<ClipboardFormat> fmts;
  auto *nw = getNativeWindow();
  if (!nw)
    return fmts;
  if (!nw->getClipboardText().empty())
    fmts.push_back(ClipboardFormat::Text);
  return fmts;
}

} // namespace ltgui
