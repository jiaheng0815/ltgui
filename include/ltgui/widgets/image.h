#pragma once
#include "widget.h"
#include <string>

namespace ltgui {

class Image : public Widget {
public:
  // How the image is fitted into the widget's rectangle.
  enum class FitMode {
    Fill,    // cover: scale and crop to fill the rect
    Contain, // scale to fit inside, preserving aspect ratio
    Stretch  // scale to fill exactly, distorting aspect ratio
  };

  explicit Image(Widget *parent = nullptr);

  bool load(const std::string &path);
  std::string path() const { return path_; }

  void setFitMode(FitMode mode) {
    fitMode_ = mode;
    update();
  }
  FitMode fitMode() const { return fitMode_; }

  LTGUI_DECLARE_WIDGET_TYPE(Image)
  Size sizeHint() const override;

protected:
  void paintSelf(NativeCanvas *canvas) override;

private:
  std::string path_;
  // Path whose load could not be attempted yet because the widget had no
  // window attached; retried at paint time once a window exists.
  std::string pendingPath_;
  FitMode fitMode_ = FitMode::Contain;
  Size imageSize_;
  bool loaded_ = false;
};

} // namespace ltgui
