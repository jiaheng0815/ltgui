#pragma once
#include "widget.h"
#include <string>

namespace ltgui {

class Image : public Widget {
public:
    explicit Image(Widget* parent = nullptr);

    bool load(const std::string& path);
    std::string path() const { return path_; }

    void setFitMode(char mode); // 'f'=fill, 'c'=contain, 's'=stretch

    WidgetType widgetType() const override { return WidgetType::Image; }
    Size sizeHint() const override;

protected:
    void paintSelf(NativeCanvas* canvas) override;

private:
    std::string path_;
    char fitMode_ = 'c'; // contain
    Size imageSize_;
    bool loaded_ = false;
};

} // namespace ltgui
