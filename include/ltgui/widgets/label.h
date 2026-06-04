#pragma once
#include "widget.h"
#include <string>

namespace ltgui {

class Label : public Widget {
public:
    explicit Label(const std::string& text = "", Widget* parent = nullptr);

    std::string text() const { return text_; }
    void setText(const std::string& text);

    WidgetType widgetType() const override { return WidgetType::Label; }
    bool canAcceptFocus() const override { return false; }
    Size sizeHint() const override;

protected:
    void paintSelf(NativeCanvas* canvas) override;

private:
    std::string text_;
};

} // namespace ltgui
