// #include "../tools/benchmark/benchmark.hpp"
#include <atomic>
#include <iostream>

std::atomic_int atomicCnt = 0;

int main() {
  ++atomicCnt;
  std::cout << atomicCnt << std::endl;
}