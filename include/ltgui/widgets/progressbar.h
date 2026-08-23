#pragma once
#include "animation.h"
#include "widgets/range.h"

namespace ltgui {

class ProgressBar : public Range {
public:
  explicit ProgressBar(Widget *parent = nullptr);

  void setValue(int value) override;
  void setRange(int min, int max) override;

  bool indeterminate() const { return indeterminate_; }
  void setIndeterminate(bool on);

  LTGUI_DECLARE_WIDGET_TYPE(ProgressBar)
  Size sizeHint() const override;

protected:
  void paintSelf(NativeCanvas *canvas) override;

private:
  // Recomputes the animated fill target (0..1) from value_/range.
  void updateDisplayTarget();

  bool indeterminate_ = false;
  AnimatedFloat displayValue_{0.0f};
  AnimatedFloat indeterminatePhase_{0.0f};
};

} // namespace ltgui
