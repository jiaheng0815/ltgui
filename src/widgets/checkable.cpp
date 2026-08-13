#include "widgets/checkable.h"
#include "widget.h"

namespace ltgui {

void Checkable::setChecked(bool checked) {
    if (checked_ != checked) {
        checked_ = checked;
        if (host_) host_->update();
        onToggled.emit(checked_);
    }
}

} // namespace ltgui
