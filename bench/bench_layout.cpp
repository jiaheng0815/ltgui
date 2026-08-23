// Layout benchmark: builds a 100x8 widget grid and re-lays it repeatedly.
// Run: python ltgui.py bench
#include "ltgui.h"

#include <chrono>
#include <cstdio>

using namespace ltgui;

int main() {
  auto t0 = std::chrono::steady_clock::now();

  auto root = std::make_unique<Widget>();
  root->setGeometry(Rect(0, 0, 1200, 800));
  auto mainLayout = std::make_unique<BoxLayout>(BoxLayout::TopToBottom, 4, 4);
  for (int c = 0; c < 100; c++) {
    auto *row = root->makeChild<Widget>();
    auto rl = std::make_unique<BoxLayout>(BoxLayout::LeftToRight, 2, 2);
    for (int b = 0; b < 8; b++)
      row->makeChild<Button>("B");
    row->setLayout(std::move(rl));
  }
  root->setLayout(std::move(mainLayout));
  auto t1 = std::chrono::steady_clock::now();

  // Re-layout 60 times with alternating sizes (sizeHint caches get exercised).
  constexpr int kRounds = 60;
  for (int i = 0; i < kRounds; i++)
    root->setGeometry(Rect(0, 0, 900 + i * 5, 700 - i));
  auto t2 = std::chrono::steady_clock::now();

  double buildMs =
      std::chrono::duration<double, std::milli>(t1 - t0).count();
  double relayoutMs =
      std::chrono::duration<double, std::milli>(t2 - t1).count();

  printf("bench_layout: build 800 widgets in %.2f ms\n", buildMs);
  printf("bench_layout: %d relayouts in %.2f ms (%.2f ms each)\n", kRounds,
         relayoutMs, relayoutMs / kRounds);
  return 0;
}
