#include "widgets/range.h"
#include <algorithm>

namespace ltgui {

void Range::setValue(int value) {
  value = std::max(min_, std::min(max_, value));
  if (value_ != value) {
    value_ = value;
    update();
    onValueChanged.emit(value_);
  }
}

void Range::setRange(int min, int max) {
  min_ = min;
  max_ = max;
  int oldValue = value_;
  if (value_ < min_)
    value_ = min_;
  if (value_ > max_)
    value_ = max_;
  update();
  if (value_ != oldValue)
    onValueChanged.emit(value_);
}

} // namespace ltgui
