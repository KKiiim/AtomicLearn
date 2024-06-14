#include <atomic>
#include <iostream>
#include <thread>

std::atomic<bool> flag1{false};
std::atomic<bool> flag2{false};
int data1 = 0;
int data2 = 0;

void thread_a() {
  data1 = 1; // Store data1
  flag1.store(true,
              std::memory_order_relaxed); // Set flag1 with relaxed memory order
}

void thread_b() {
  data2 = 2; // Store data2
  flag2.store(true,
              std::memory_order_release); // Set flag2 with release memory order
}

void thread_c() {
  while (!flag1.load(std::memory_order_relaxed))
    ; // Wait for flag1 with relaxed memory order
  if (flag2.load(
          std::memory_order_acquire)) { // Check flag2 with acquire memory order
    std::cout << "Data2 observed as: " << data2
              << std::endl; // May print 0 or 2
  }
}

void thread_d() {
  while (!flag2.load(std::memory_order_consume))
    ; // Wait for flag2 with consume memory order
  if (flag1.load(
          std::memory_order_seq_cst)) { // Check flag1 with seq_cst memory order
    std::cout << "Data1 observed as: " << data1 << std::endl; // Should print 1
  }
}

int main() {
  std::thread a(thread_a);
  std::thread b(thread_b);
  std::thread c(thread_c);
  std::thread d(thread_d);

  a.join();
  b.join();
  c.join();
  d.join();

  return 0;
}