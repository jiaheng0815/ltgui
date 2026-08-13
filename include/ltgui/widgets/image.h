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

  // Legacy char-based overload: 'f'=Fill, 'c'=Contain, 's'=Stretch
  [[deprecated("use setFitMode(FitMode) instead")]] void setFitMode(char mode) {
    setFitMode(mode == 'f'   ? FitMode::Fill
               : mode == 's' ? FitMode::Stretch
                             : FitMode::Contain);
  }

  LTGUI_DECLARE_WIDGET_TYPE(Image)
  Size sizeHint() const override;

protected:
  void paintSelf(NativeCanvas *canvas) override;

private:
  std::string path_;
  FitMode fitMode_ = FitMode::Contain;
  Size imageSize_;
  bool loaded_ = false;
};

} // namespace ltgui
