// TableModel benchmark: row inserts, then 20 sorted passes, then cell writes.
#include "ltgui.h"

#include <chrono>
#include <cstdio>
#include <vector>

using namespace ltgui;

int main() {
  SimpleTableModel model(0, 4);
  constexpr int kRows = 20000;

  auto t0 = std::chrono::steady_clock::now();
  for (int i = 0; i < kRows; i++)
    model.addRow({std::to_string(i), "x", "y", "z"});
  auto t1 = std::chrono::steady_clock::now();

  constexpr int kSorts = 20;
  int before = model.cellText(0, 0).size();
  for (int i = 0; i < kSorts; i++)
    model.sort(i % 2 == 0 ? 0 : 2, !(i % 2 == 0));
  (void)before;
  auto t2 = std::chrono::steady_clock::now();

  long long sum = 0;
  for (int i = 0; i < kRows; i += 2) {
    model.setCellText(i, 1, "edited");
    sum += model.cellText(i, 1).size();
  }
  auto t3 = std::chrono::steady_clock::now();

  auto ms = [](decltype(t0) a, decltype(t0) b) {
    return std::chrono::duration<double, std::milli>(b - a).count();
  };
  printf("bench_model: %d inserts %.2f ms | %d sorts %.2f ms | %d writes %.2f ms\n",
         kRows, ms(t0, t1), kSorts, ms(t1, t2), kRows / 2, ms(t2, t3));
  return 0;
}
