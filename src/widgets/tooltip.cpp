#include "widgets/tooltip.h"
#include "window.h"
#include "theme.h"
#include "platform/native_canvas.h"

namespace ltgui {

Tooltip::Tooltip(Widget* parent) : Widget(parent) {
    style().bgColor = Color(40, 40, 40);
    style().fgColor = Color::White;
    style().borderRadius = 4;
    style().setPadding(8, 4);
    style().font = Font::systemDefault(11);
}

void Tooltip::setText(const std::string& text) {
    text_ = text;
    invalidateSizeHint();
    update();
}

void Tooltip::showAt(const Point& pos) {
    position_ = pos;
    Size hint = sizeHint();
    setGeometry(Rect(pos.x, pos.y, hint.width, hint.height));
    setVisible(true);
    update();
}

void Tooltip::dismiss() {
    setVisible(false);
}

Size Tooltip::sizeHint() const {
    if (!sizeHintDirty()) return cachedSizeHint();
    if (auto* win = window()) {
        if (auto* c = win->canvas()) {
            c->setFont(style().font);
            Size textSize = c->measureText(text_);
            setCachedSizeHint({textSize.width + style().paddingHorz(),
                               textSize.height + style().paddingVert()});
            return cachedSizeHint();
        }
    }
    setCachedSizeHint({60, 22});
    return cachedSizeHint();
}

void Tooltip::paintSelf(NativeCanvas* canvas) {
    if (!isVisible()) return;
    Rect r = absoluteRect();

    // Shadow (simple darker rect offset)
    canvas->setColor(Color(0, 0, 0, 40));
    canvas->fillRoundedRect(r.translated(1, 2), style().borderRadius);

    paintBackground(canvas);

    // Text
    canvas->setColor(style().fgColor);
    canvas->setFont(style().font);
    canvas->drawText(text_, r.adjusted(style().paddingLeft, style().paddingTop,
                                       -style().paddingRight, -style().paddingBottom),
                     NativeCanvas::AlignLeft | NativeCanvas::AlignVCenter | NativeCanvas::SingleLine);
}

void Tooltip::show(Widget* target, const std::string& text) {
    auto* win = target->window();
    if (!win) return;

    // Remove any existing tooltips from the parent to prevent accumulation
    auto* parent = win->centralWidget();
    if (parent) {
        auto& kids = parent->children();
        for (int i = static_cast<int>(kids.size()) - 1; i >= 0; --i) {
            if (kids[i]->widgetType() == WidgetType::Tooltip) {
                parent->removeChild(kids[i].get()); // destroys the tooltip
            }
        }
    }

    Rect targetAbs = target->absoluteRect();
    Point pos(targetAbs.x + 8, targetAbs.bottom() + 4);

    auto tooltip = std::make_unique<Tooltip>();
    tooltip->setText(text);
    // Add to tree BEFORE showAt so sizeHint() can access window() to
    // measure text.  Without this, sizeHint() falls back to {60,22}.
    auto* raw = tooltip.get();
    if (parent) parent->addChild(std::move(tooltip));
    raw->showAt(pos);
}

} // namespace ltgui
