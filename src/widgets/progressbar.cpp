#include "widgets/progressbar.h"
#include "platform/native_canvas.h"
#include "theme.h"
#include "window.h"
#include <algorithm>

namespace ltgui {

ProgressBar::ProgressBar(Widget *parent) : Range(parent) {
  style().borderRadius = 4;
}

void ProgressBar::setValue(int value) {
  value = std::max(minimum(), std::min(maximum(), value));
  if (value_ != value) {
    value_ = value;
    updateDisplayTarget();
    update();
    onValueChanged.emit(value_);
  }
}

void ProgressBar::setRange(int min, int max) {
  Range::setRange(min, max); // may clamp value_ and emit onValueChanged
  updateDisplayTarget();
  update();
}

void ProgressBar::updateDisplayTarget() {
  float pct = (maximum() > minimum())
                  ? static_cast<float>(value_ - minimum()) /
                        static_cast<float>(maximum() - minimum())
                  : 1.0f;
  displayValue_.setTarget(pct, 300, Easing::EaseOut);
}

void ProgressBar::setIndeterminate(bool on) {
  indeterminate_ = on;
  if (on) {
    indeterminatePhase_.setTarget(1.0f, 1200, Easing::Linear);
  } else {
    indeterminatePhase_.setImmediate(0.0f);
  }
  update();
}

Size ProgressBar::sizeHint() const {
  if (!sizeHintDirty())
    return cachedSizeHint();
  setCachedSizeHint(dpiScaleSize(200, 20));
  return cachedSizeHint();
}

void ProgressBar::paintSelf(NativeCanvas *canvas) {
  Rect r = absoluteRect();
  ResolvedStyle st = resolvedStyle();
  const Theme &t = currentTheme();

  // Track — uses the theme's dedicated progressBarTrack color.
  canvas->setColor(t.progressBarTrack);
  canvas->fillRoundedRect(r, st.borderRadius);

  if (indeterminate_) {
    // Indeterminate: sliding bar
    float phase = indeterminatePhase_.value();
    if (phase >= 0.999f) {
      indeterminatePhase_.setImmediate(0.0f);
      indeterminatePhase_.setTarget(1.0f, 1200, Easing::Linear);
    }

    int barW = r.width / 3;
    int offset = static_cast<int>(phase * (r.width + barW) - barW);

    Rect barRect(r.x + offset, r.y, barW, r.height);
    if (barRect.x < r.x) {
      barRect.width -= r.x - barRect.x;
      barRect.x = r.x;
    }
    if (barRect.right() > r.right()) {
      barRect.width = r.right() - barRect.x;
    }

    // Fill — theme's dedicated progressBarFill color.
    canvas->setColor(t.progressBarFill);
    canvas->fillRoundedRect(barRect, st.borderRadius);
  } else {
    // Determinate: filled portion
    float pct = displayValue_.value();
    int fillW = static_cast<int>(r.width * pct);
    if (fillW > 0) {
      Rect fillRect(r.x, r.y, fillW, r.height);
      canvas->setColor(t.progressBarFill);
      canvas->fillRoundedRect(fillRect, st.borderRadius);

      // If not full, cover the right rounded corner on the fill end.
      // Skip when the fill is narrower than the corner radius — the patch
      // would extend left of the track's left edge (and square off the
      // left corner).
      if (fillW > st.borderRadius && fillW < r.width - st.borderRadius) {
        canvas->fillRect(Rect(r.x + fillW - st.borderRadius, r.y,
                              st.borderRadius, r.height));
      }
    }

    // Percentage text
    if (r.width > 60) {
      canvas->setColor(t.textPrimary);
      canvas->setFont(st.font);
      int pctInt = static_cast<int>(pct * 100.0f + 0.5f);
      std::string pctText = std::to_string(pctInt) + "%";
      canvas->drawText(pctText, r,
                       NativeCanvas::AlignCenter | NativeCanvas::AlignVCenter |
                           NativeCanvas::SingleLine);
    }
  }
}

} // namespace ltgui
