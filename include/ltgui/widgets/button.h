#pragma once
#include "widget.h"
#include "animation.h"
#include <string>
#include <functional>

namespace ltgui {

class Button : public Widget {
public:
    explicit Button(const std::string& text = "", Widget* parent = nullptr);

    std::string text() const { return text_; }
    void setText(const std::string& text);

    using ClickCallback = std::function<void()>;
    void onClick(ClickCallback cb) { clickCallback_ = std::move(cb); }

    WidgetType widgetType() const override { return WidgetType::Button; }
    bool canAcceptFocus() const override { return true; }
    Size sizeHint() const override;

protected:
    void paintSelf(NativeCanvas* canvas) override;
    bool handleEvent(Event& event) override;

private:
    std::string text_;
    bool pressed_ = false;
    bool hovered_ = false;
    int animDuration_ = 150;
    ClickCallback clickCallback_;
};

} // namespace ltgui
