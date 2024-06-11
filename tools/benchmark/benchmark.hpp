#ifndef TOOLS_BENCHMARK_BRNCHMARK_HPP
#define TOOLS_BENCHMARK_BRNCHMARK_HPP
#include <chrono>
#include <iostream>

void test();
class Benchmark {
public:
  Benchmark(std::string funcName) : funcName_(funcName) {
    m_begin = std::chrono::system_clock::now();
  }
  ~Benchmark() {
    auto dur = std::chrono::system_clock::now() - m_begin;
    std::cout
        << funcName_ << " | duration ms: "
        << std::chrono::duration_cast<std::chrono::milliseconds>(dur).count()
        << std::endl;
  }
  Benchmark(const Benchmark &) = delete;
  Benchmark &operator=(const Benchmark &) = delete;

private:
  using time_point = decltype(std::chrono::system_clock::now());
  time_point m_begin;
  std::string funcName_;
};

#endif