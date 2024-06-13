# x86
### ./x86-atomic.sh

### Only std::atomic impl correct in every optimization flag

### Briefly introduce -O diff  
-Os Optimize the size of the code, first reduce the size of the executable file

### details of log/asm/x86
src/x86-atomic.cpp  
  

### impl of Lock
/slides/CMPXCHG.pdf  
1. CPU out-of-order firing  
need memory_order  

2. cache consistency
Haven't read it yet  
MESI是“modified”, “exclusive”, “shared”, 和 “invalid”首字母的大写，当使用MESI 协议的时候，  
cacheline可以处于这四个状态中的一个，因此除了物理地址和具体的数据之外，还需要为每一个cacheline设计一个2-bit的tag来标识该cacheline的状态  
  


### details of c++ std::atomic
std::atomic default level is memory_order_seq_cst
```c++
      __int_type
      operator++() volatile noexcept
      { return __atomic_add_fetch(&_M_i, 1, int(memory_order_seq_cst)); }
```
  
# arm
### getRaw-name-sh
log/atomic/armv8_ZNSt13__atomic_baseIiE.md  
    
_ZNSt13__atomic_baseIiEppEv  
_Z N St 13__atomic_baseIi E pp Ev  
_ZNKSt13__atomic_baseIiEcviEv  
_Z N K St 13__atomic_baseIi E cv i Ev  




compare -O0 and -O1, 
在`-O0`优化级别下，编译器不会进行任何优化，会尽可能直接地将C++代码翻译为汇编指令。从你提供的`-O0`级别的`objdump`输出来看，我们可以看到`tAsmLockCnt`函数的汇编代码相比`-O1`输出有更多的指令和更多的步骤。

在`-O0`级别下的实现中，我们可以看到：

- 函数开始时有`push %rbp`和`mov %rsp,%rbp`，这是设置栈帧的标准做法。
- 使用了局部变量`-0x4(%rbp)`来跟踪循环的次数，而在`-O1`中，这个循环计数是直接在寄存器中进行的。
- 循环中使用了`jmp`指令来跳转，这是一个无条件跳转。
- 每次循环迭代都会更新局部变量`-0x4(%rbp)`的值，并且使用`cmpl`和`jle`指令来检查循环是否应该继续。

在`-O1`级别下的实现中，编译器进行了一些优化：

- 函数的栈帧设置更简洁，没有使用局部变量来跟踪循环次数。
- 循环计数是直接在`%eax`寄存器中进行的，没有使用额外的内存访问来更新循环计数。
- 循环使用`sub`和`jne`指令来进行控制，这意味着编译器优化了循环的条件检查和跳转逻辑。

关于为什么在高优化级别下，汇编实现可能会出现错误的结果，这可能是因为编译器优化导致的内存顺序或其他未定义行为的问题。在多线程环境中，即使是原子指令，如果没有正确地使用内存屏障（memory barrier）来防止指令重排序，也可能会出现问题。

为了确保汇编实现的正确性，可以在`lock xadd`指令后添加一个`mfence`指令，或者使用`volatile`关键字来防止编译器对这些指令进行优化。但是，通常更推荐使用`std::atomic`这类的高级抽象，因为它们会自动处理这些复杂的内存顺序问题，并且提供了跨平台的一致性。

./aarch64-atomic.sh - lack some glibc files in v7
