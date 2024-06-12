#include "../tools/benchmark/benchmark.hpp"
#include <atomic>
#include <iostream>
#include <thread>
#include <vector>

const constexpr int TestN = 10000000;
std::atomic_int atomicCnt = 0;
int normalCnt = 0;
int asmLockCnt = 0;

void tAtomicCnt() {
  for (int n = 0; n < TestN; ++n) {
    ++atomicCnt;
  }
}

void tAsmLockCnt() {
  for (int n = 0; n < TestN; ++n) {
    // The inline assembly syntax for GCC/Clang is as follows:
    // asm ( assembler template
    //     : output operands                  /* optional */
    //     : input operands                   /* optional */
    //     : list of clobbered registers      /* optional */
    //     );

    asm volatile(
        "lock xaddl %1, %0; mfence" // The 'l' suffix specifies that the
                                    // operation is on a 32-bit integer.
        : "=m"(asmLockCnt)          // Output operand: memory(m) location of
                                    // 'asmLockCnt' will be writen(=).
        : "r"(1),
          "m"(asmLockCnt) // The C++ variable asmLocakCnt is linked to
                          // %1 in the assembly instruction.
        : "memory", "cc"  // Clobber list: flags that the memory and
                          // condition codes may be modified.
    );
  }
}

void tNormalCnt() {
  for (int n = 0; n < TestN; ++n) {
    ++normalCnt;
  }
}

int main() {
  {
    Benchmark bm("normal");
    std::vector<std::jthread> pool;
    for (int n = 0; n < 10; ++n) {
      pool.emplace_back(tNormalCnt);
    }
  }

  {
    Benchmark bm("atomic");
    std::vector<std::jthread> pool;
    for (int n = 0; n < 10; ++n) {
      pool.emplace_back(tAtomicCnt);
    }
  }

  {
    Benchmark bm("asm");
    std::vector<std::jthread> pool; // c++ 20
    for (int n = 0; n < 10; ++n) {
      pool.emplace_back(tAsmLockCnt);
    }
  }
  std::cout << "The non-atomic counter is " << normalCnt << '\n'
            << "The atomic counter is " << atomicCnt << '\n'
            << "The asmLock counter is " << asmLockCnt << std::endl;
}