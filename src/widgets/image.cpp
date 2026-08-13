#include "widgets/image.h"
#include "window.h"
#include "platform/native_canvas.h"
#include <algorithm>

namespace ltgui {

Image::Image(Widget* parent) : Widget(parent) {
    style().bgColor = Color::Transparent;
    style().borderWidth = 0;
}

bool Image::load(const std::string& path) {
    path_ = path;
    auto* win = window();
    if (win && win->canvas()) {
        imageSize_ = win->canvas()->imageSize(path);
        loaded_ = imageSize_.width > 0 && imageSize_.height > 0;
    }
    invalidateSizeHint();
    update();
    return loaded_;
}

void Image::setFitMode(char mode) {
    fitMode_ = mode;
    update();
}

Size Image::sizeHint() const {
    if (!sizeHintDirty()) return cachedSizeHint();
    if (loaded_) {
        setCachedSizeHint(imageSize_);
    } else {
        setCachedSizeHint(dpiScaleSize(100, 100));
    }
    return cachedSizeHint();
}

void Image::paintSelf(NativeCanvas* canvas) {
    if (!loaded_ || path_.empty()) return;
    if (imageSize_.width <= 0 || imageSize_.height <= 0) return;

    Rect r = absoluteRect();
    if (r.width <= 0 || r.height <= 0) return;

    Rect drawRect = r;

    if (fitMode_ == 'c') {
        // Contain: fit within bounds, maintaining aspect ratio
        float imgRatio = static_cast<float>(imageSize_.width) / static_cast<float>(imageSize_.height);
        float rectRatio = static_cast<float>(r.width) / static_cast<float>(r.height);
        if (imgRatio > rectRatio) {
            int newH = static_cast<int>(r.width / imgRatio);
            drawRect.y += (r.height - newH) / 2;
            drawRect.height = newH;
        } else {
            int newW = static_cast<int>(r.height * imgRatio);
            drawRect.x += (r.width - newW) / 2;
            drawRect.width = newW;
        }
    } else if (fitMode_ == 's') {
        // Stretch: fill exactly
        drawRect = r;
    } else {
        // Fill: cover (crop)
        float imgRatio = static_cast<float>(imageSize_.width) / static_cast<float>(imageSize_.height);
        float rectRatio = static_cast<float>(r.width) / static_cast<float>(r.height);
        if (imgRatio > rectRatio) {
            int newW = static_cast<int>(r.height * imgRatio);
            drawRect.x -= (newW - r.width) / 2;
            drawRect.width = newW;
        } else {
            int newH = static_cast<int>(r.width / imgRatio);
            drawRect.y -= (newH - r.height) / 2;
            drawRect.height = newH;
        }
    }

    canvas->drawImage(path_, drawRect);
}

} // namespace ltgui
