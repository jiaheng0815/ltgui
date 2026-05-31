#pragma once
#include "widget.h"
#include <string>

namespace ltgui {

class Tooltip : public Widget {
public:
    explicit Tooltip(Widget* parent = nullptr);

    std::string text() const { return text_; }
    void setText(const std::string& text);
    void showAt(const Point& screenPos);
    void dismiss();

    static void show(Widget* target, const std::string& text);

    Size sizeHint() const override;

protected:
    void paintSelf(NativeCanvas* canvas) override;

private:
    std::string text_;
    bool visible_ = false;
    Point position_;
};

} // namespace ltgui
