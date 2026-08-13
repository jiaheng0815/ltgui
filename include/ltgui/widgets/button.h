#pragma once
#include "widgets/textwidget.h"
#include <functional>

namespace ltgui {

class Button : public TextWidget {
public:
    explicit Button(const std::string& text = "", Widget* parent = nullptr);

    using ClickCallback = std::function<void()>;
    void onClick(ClickCallback cb) { clickCallback_ = std::move(cb); }

    LTGUI_DECLARE_WIDGET_TYPE(Button)
    bool canAcceptFocus() const override { return true; }
    Size sizeHint() const override;

protected:
    void paintSelf(NativeCanvas* canvas) override;
    bool handleEvent(Event& event) override;

private:
    bool pressed_ = false;
    bool hovered_ = false;
    ClickCallback clickCallback_;
};

} // namespace ltgui
