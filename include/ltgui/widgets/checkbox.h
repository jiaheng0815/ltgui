#pragma once
#include "widget.h"
#include <string>
#include <functional>

namespace ltgui {

class CheckBox : public Widget {
public:
    explicit CheckBox(const std::string& text = "", Widget* parent = nullptr);

    std::string text() const { return text_; }
    void setText(const std::string& text);

    bool isChecked() const { return checked_; }
    void setChecked(bool checked);

    using ToggleCallback = std::function<void(bool)>;
    void onToggled(ToggleCallback cb) { toggleCallback_ = std::move(cb); }

    WidgetType widgetType() const override { return WidgetType::CheckBox; }
    bool canAcceptFocus() const override { return true; }
    Size sizeHint() const override;

protected:
    void paintSelf(NativeCanvas* canvas) override;
    bool handleEvent(Event& event) override;

private:
    std::string text_;
    bool checked_ = false;
    ToggleCallback toggleCallback_;
};

} // namespace ltgui
