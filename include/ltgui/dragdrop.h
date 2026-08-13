#pragma once
#include "geometry.h"
#include "signal.h"
#include <functional>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

namespace ltgui {

class Widget;

class DragData {
public:
  bool hasFormat(const std::string &mime) const;
  std::string text() const;
  void setData(const std::string &mime, const std::vector<uint8_t> &data);
  std::vector<uint8_t> data(const std::string &mime) const;
  void setText(const std::string &t) {
    setData("text/plain", std::vector<uint8_t>(t.begin(), t.end()));
  }
  void setFiles(const std::vector<std::string> &paths);
  std::vector<std::string> files() const;
  std::vector<std::string> formats() const;

private:
  std::unordered_map<std::string, std::vector<uint8_t>> data_;
};

class DragSource {
public:
  explicit DragSource(Widget *w) : widget_(w) {}

  void setDragData(std::shared_ptr<DragData> d) { dragData_ = std::move(d); }
  void addMimeType(const std::string &m) { mimeTypes_.push_back(m); }

  Widget *widget() const { return widget_; }
  std::shared_ptr<DragData> dragData() const { return dragData_; }
  const std::vector<std::string> &mimeTypes() const { return mimeTypes_; }

private:
  Widget *widget_;
  std::shared_ptr<DragData> dragData_;
  std::vector<std::string> mimeTypes_;
};

class DropTarget {
public:
  // DragOver returns bool (accept or reject the drag), so it keeps a
  // single std::function — Signal<T> emits void callbacks only.
  using DragOverCallback = std::function<bool(const DragData &)>;

  explicit DropTarget(Widget *w) : widget_(w) {}

  void setAcceptedMimeTypes(const std::vector<std::string> &types) {
    acceptedTypes_ = types;
  }
  // Emitted when a drop completes on this widget.
  Signal<const DragData &> onDrop;
  void onDragOver(DragOverCallback cb) { dragOverCb_ = std::move(cb); }

  Widget *widget() const { return widget_; }
  bool acceptsMime(const std::string &mime) const;
  void handleDrop(const DragData &data);
  bool handleDragOver(const DragData &data);

private:
  Widget *widget_;
  std::vector<std::string> acceptedTypes_;
  DragOverCallback dragOverCb_;
};

} // namespace ltgui
