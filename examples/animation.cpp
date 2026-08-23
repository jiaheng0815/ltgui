// Animation example: AnimatedFloat, WidgetAnimation (bounce pulse), Timer.
#include "ltgui.h"
#include <cstdio>
#include <cstring>
#include <iostream>

using namespace ltgui;

int main(int argc, char *argv[]) {
  for (int i = 1; i < argc; i++) {
    if (std::strcmp(argv[i], "--debug") == 0)
      Logger::instance().setGlobalDebug(true);
  }

  Window window;
  if (!window.create(480, 320, "ltgui Animation")) {
    std::cerr << "Failed to create window." << std::endl;
    return 1;
  }

  auto root = std::make_unique<Widget>();
  root->style().bgColor = currentTheme().bgPrimary;
  auto layout = std::make_unique<BoxLayout>(BoxLayout::TopToBottom, 12, 12);

  // 1. AnimatedFloat: loop + yoyo with bounce easing; onFinished fires every
  //    boundary. A repeating timer polls its value into a label (AnimatedFloat
  //    has no per-frame callback by design).
  auto *bounceLabel = root->makeChild<Label>("Bounce: 0.00");
  bounceLabel->style().fgColor = currentTheme().textSecondary;
  AnimatedFloat bounce(0.0f);
  bounce.onFinished.connect([&]() { LOG_INFO("Demo", "bounce cycle finished"); });
  bounce.setLoop(true);
  bounce.setYoyo(true);
  bounce.setTarget(1.0f, 900, Easing::EaseOutBounce);
  Timer poller;
  poller.start(16, true, [&]() {
    char buf[32];
    std::snprintf(buf, sizeof(buf), "Bounce: %.2f", bounce.value());
    bounceLabel->setText(buf);
  });

  // 2. WidgetAnimation: a "pulse" that eases 0.0 -> 1.0 with an out-bounce
  auto *pulseLabel = root->makeChild<Label>("Pulse: idle");
  WidgetAnimation pulse;
  pulse.setDuration(700);
  pulse.setEasing(Easing::EaseOutBounce);
  pulse.setStartValue(0.0f);
  pulse.setEndValue(1.0f);
  pulse.setValueCallback([pulseLabel](float v) {
    char buf[32];
    std::snprintf(buf, sizeof(buf), "Pulse: %.2f", v);
    pulseLabel->setText(buf);
  });
  pulse.onFinished.connect([pulseLabel]() { pulseLabel->setText("Pulse: done"); });

  // 3. Timer: 100ms stopwatch tick + a one-shot that fires once after 3s
  auto *timerLabel = root->makeChild<Label>("Stopwatch: 0.0s");
  int ticks = 0;
  Timer stopwatch;
  stopwatch.start(100, true, [&]() {
    ticks++;
    timerLabel->setText("Stopwatch: " + std::to_string(ticks / 10) +
                        "." + std::to_string(ticks % 10) + "s");
  });
  Timer::singleShot(3000, [&]() { timerLabel->setText("Stopwatch: one-shot fired"); });

  auto *replayBtn = root->makeChild<Button>("Replay bounce + pulse");
  replayBtn->onClicked.connect([&]() {
    bounce.setImmediate(0.0f);
    bounce.setTarget(1.0f, 900, Easing::EaseOutBounce);
    pulse.play();
  });

  layout->addStretch(0);
  layout->addStretch(0);

  root->setLayout(std::move(layout));
  window.setCentralWidget(std::move(root));
  window.show();

  return Application::instance().run();
}
