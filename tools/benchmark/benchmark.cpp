#include "benchmark.hpp"
#include <set>

std::set<int> test_set;
void test() {
  for (int i = 0; i < 1000000; ++i) {
    test_set.insert(i);
  }
}

auto test_func_duration = [](auto &&func, auto &&...params) {
  auto now = std::chrono::system_clock::now();
  std::forward<decltype(func)>(func)(std::forward<decltype(params)>(params)...);
  auto dur = std::chrono::system_clock::now() - now;

  std::cout
      << "duration ms: "
      << std::chrono::duration_cast<std::chrono::milliseconds>(dur).count()
      << std::endl;
};
