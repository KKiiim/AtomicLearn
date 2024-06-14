
#include <atomic>
#include <iostream>
#include <thread>
#include <vector>

std::atomic<int> counter{0};

void increment() {
  for (int i = 0; i < 100; ++i) {
    counter.fetch_add(1, std::memory_order_relaxed);
  }
}

void print() {
  std::cout << counter.load(std::memory_order_relaxed) << std::endl;
  std::flush(std::cout);
}

void t() {
  increment();
  print();
}

int main() {
  std::vector<std::thread> threads;

  for (int i = 0; i < 10; ++i) {
    threads.emplace_back(t);
  }

  for (auto &t : threads) {
    t.join();
  }

  print();

  return 0;
}
