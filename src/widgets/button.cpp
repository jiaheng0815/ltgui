#include "widgets/button.h"
#include "platform/native_canvas.h"
#include "window.h"

namespace ltgui {

Button::Button(const std::string &text, Widget *parent)
    : TextWidget(text, parent) {
  style().borderRadius = 4;
  // Seed the background animation with the normal-state color so the
  // first paint doesn't start from black.
  bgAnim_.setImmediate(resolvedStyle().bgColor);
}

Size Button::sizeHint() const {
  if (!sizeHintDirty())
    return cachedSizeHint();
  setCachedSizeHint(textSizeHint({80, 30}, style().paddingHorz() + 24,
                                 style().paddingVert() + 10));
  return cachedSizeHint();
}

void Button::paintSelf(NativeCanvas *canvas) {
  Rect r = absoluteRect();
  ResolvedStyle st = resolvedStyle();

  // Fill background: accent when interactive (hover/pressed get the
  // theme's state-specific accent automatically), base bg otherwise.
  // While a state transition is animating, use the eased color; once it
  // settles, resolvedStyle() is authoritative again (keeps theme switches
  // fresh without re-styling).
  Color fillColor = bgAnim_.isAnimating()
                        ? bgAnim_.current()
                        : ((pressed_ || hovered_) ? st.accent : st.bgColor);
  canvas->setColor(fillColor);
  if (st.borderRadius > 0) {
    canvas->fillRoundedRect(r, st.borderRadius);
  } else {
    canvas->fillRect(r);
  }

  // Border
  if (st.borderWidth > 0) {
    Color borderColor = (pressed_ || hovered_) ? st.accent : st.borderColor;
    canvas->setColor(borderColor);
    if (st.borderRadius > 0) {
      canvas->strokeRoundedRect(r, st.borderRadius, st.borderWidth);
    } else {
      canvas->strokeRect(r, st.borderWidth);
    }
  }

  // Text
  canvas->setColor(hovered_ || pressed_ ? Color::White : st.fgColor);
  canvas->setFont(st.font);
  int flags = NativeCanvas::AlignCenter | NativeCanvas::AlignVCenter |
              NativeCanvas::SingleLine;
  canvas->drawText(text_, r, flags);
}

void Button::animateBg() {
  ResolvedStyle st = resolvedStyle();
  Color target = (pressed_ || hovered_) ? st.accent : st.bgColor;
  bgAnim_.setTarget(target, 150, Easing::EaseOut);
  update();
}

bool Button::handleEvent(Event &event) {
  if (!isEnabled())
    return false;

  switch (event.type) {
  case EventType::MouseMove: {
    bool inBounds = geometry().contains(event.pos);
    if (inBounds && !hovered_) {
      hovered_ = true;
      animateBg();
    } else if (!inBounds && hovered_) {
      hovered_ = false;
      if (pressed_) {
        pressed_ = false;
      }
      animateBg();
    }
    if (inBounds)
      event.accepted = true;
    return inBounds;
  }
  case EventType::MouseDown:
    if (event.button == MouseButton::Left) {
      pressed_ = true;
      claimFocus();
      animateBg();
      event.accepted = true;
      return true;
    }
    break;
  case EventType::MouseUp:
    if (event.button == MouseButton::Left && pressed_) {
      pressed_ = false;
      animateBg();
      if (hovered_)
        onClicked.emit();
      event.accepted = true;
      return true;
    }
    break;
  case EventType::KeyDown:
    if (event.key == Key::Enter || event.key == Key::Space) {
      pressed_ = true;
      animateBg();
      event.accepted = true;
      return true;
    }
    break;
  case EventType::KeyUp:
    if (event.key == Key::Enter || event.key == Key::Space) {
      bool activated = pressed_;
      pressed_ = false;
      animateBg();
      // Only emit if we actually saw the matching KeyDown — otherwise a
      // shortcut/Tab that consumed KeyDown could re-trigger the button.
      if (activated)
        onClicked.emit();
      event.accepted = true;
      return true;
    }
    break;
  case EventType::FocusOut:
    // Losing focus mid-press must never leave the button stuck pressed or
    // trigger a click when the key is later released.
    pressed_ = false;
    animateBg();
    event.accepted = true;
    return true;
  default:
    break;
  }
  return false;
}

} // namespace ltgui
