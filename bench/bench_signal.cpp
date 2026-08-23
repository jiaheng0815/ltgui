// Signal benchmark: 100 slots, 100k emissions.
#include "ltgui.h"

#include <chrono>
#include <cstdio>

using namespace ltgui;

int main() {
  Signal<int> sig;
  long long sink = 0;
  for (int i = 0; i < 100; i++)
    sig.connect([&sink](int v) { sink += v; });

  constexpr int kEmits = 100000;
  auto t0 = std::chrono::steady_clock::now();
  for (int i = 0; i < kEmits; i++)
    sig.emit(i);
  auto t1 = std::chrono::steady_clock::now();

  double ms =
      std::chrono::duration<double, std::milli>(t1 - t0).count();
  double per = ms * 1e6 / kEmits;
  printf("bench_signal: %d emits x100 slots in %.2f ms (%.0f ns/emit, sink=%lld)\n",
         kEmits, ms, per, sink);
  return 0;
}
