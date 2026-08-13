#include "widgets/listitems.h"
#include "widget.h"
#include <algorithm>

namespace ltgui {

void ListItems::addItem(const std::string& item) {
    items_.push_back(item);
    onItemsStructureChanged();
    if (selected_ < 0) selected_ = 0;
    notifyChanged();
}

void ListItems::removeItem(int index) {
    if (index < 0 || index >= count()) return;
    items_.erase(items_.begin() + index);
    onItemsStructureChanged();
    if (selected_ == index) {
        selected_ = std::min(selected_, count() - 1);
    } else if (selected_ > index) {
        selected_--;
    }
    notifyChanged();
}

void ListItems::clear() {
    items_.clear();
    selected_ = -1;
    onItemsStructureChanged();
    notifyChanged();
}

std::string ListItems::item(int index) const {
    if (index >= 0 && index < count()) return items_[index];
    return {};
}

void ListItems::setCurrentIndex(int index) {
    if (index >= -1 && index < count()) {
        selected_ = index;
        if (host_) host_->update();
        onSelectionChanged.emit(selected_);
    }
}

void ListItems::notifyChanged() {
    if (host_) {
        host_->invalidateSizeHint();
        host_->update();
    }
    onItemsChanged.emit();
}

} // namespace ltgui
