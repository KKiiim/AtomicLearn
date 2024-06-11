#include "../tools/benchmark/benchmark.hpp"

int main() {
  // 1
  // auto now = std::chrono::system_clock::now();
  // test();
  // auto dur = std::chrono::system_clock::now() - now;

  // std::cout << "duration ms: " <<
  // std::chrono::duration_cast<std::chrono::milliseconds>(dur).count() <<
  // std::endl;

  // 2
  // test_func_duration(test);

  // 3
  {
    Benchmark bm("test");
    test();
  }

  return 0;
}