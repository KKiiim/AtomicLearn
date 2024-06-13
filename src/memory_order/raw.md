在多线程环境中，内存序（`memory_order`）用于指定原子操作的内存一致性要求。不同的内存序可以提供不同级别的同步保证。以下是每种内存序的功能和示例：

### `memory_order_relaxed`
最宽松的内存序，不提供任何同步或顺序保证。它仅保证原子操作本身的原子性。

**示例**：
```cpp
std::atomic<int> counter{0};

void increment() {
    counter.fetch_add(1, std::memory_order_relaxed);
}

void print() {
    std::cout << counter.load(std::memory_order_relaxed) << std::endl;
}
```
在这个例子中，多个线程可能同时执行`increment`函数，但由于是`memory_order_relaxed`，打印的顺序可能与实际发生的顺序不一致。

### `memory_order_consume`
用于处理依赖关系，确保当前线程所依赖的数据不会被重排序。但由于实现复杂，很多编译器将其视为`memory_order_acquire`。

**示例**：
```cpp
std::atomic<int*> ptr;
int data;

void producer() {
    data = 42;
    ptr.store(&data, std::memory_order_release);
}

void consumer() {
    int* p;
    while (!(p = ptr.load(std::memory_order_consume)));
    assert(*p == 42); // 永远为真
}
```
在这个例子中，`consumer`线程中的断言永远为真，因为`memory_order_consume`保证了指针`p`的加载不会在`data`的加载之前发生。

### `memory_order_acquire`
阻止加载操作重排序到原子操作之前。

**示例**：
```cpp
std::atomic<bool> ready{false};
int data;

void producer() {
    data = 42;
    ready.store(true, std::memory_order_release);
}

void consumer() {
    while (!ready.load(std::memory_order_acquire));
    assert(data == 42); // 永远为真
}
```
这里，`consumer`线程中的断言永远为真，因为`memory_order_acquire`保证了一旦`ready`被设置为`true`，`data`的值必定是42。

### `memory_order_release`
阻止存储操作重排序到原子操作之后。

**示例**：
```cpp
std::atomic<bool> ready{false};
int data;

void producer() {
    data = 42;
    ready.store(true, std::memory_order_release);
}

void consumer() {
    while (!ready.load(std::memory_order_acquire));
    assert(data == 42); // 永远为真
}
```
与`memory_order_acquire`相对应，`producer`线程中的`store`操作使用`memory_order_release`，确保`data`的写入在`ready`更新为`true`之前完成。

### `memory_order_acq_rel`
结合了`memory_order_acquire`和`memory_order_release`，适用于读-改-写操作。

**示例**：
```cpp
std::atomic<int> counter{0};

void thread1() {
    int expected = 1;
    while (!counter.compare_exchange_strong(expected, 2,
                                            std::memory_order_acq_rel)) {
        expected = 1;
    }
}

void thread2() {
    counter.store(1, std::memory_order_release);
}
```
在这个例子中，`thread1`中的`compare_exchange_strong`操作在更新`counter`前后提供了获取和释放语义。

### `memory_order_seq_cst`
最严格的内存序，提供顺序一致性，所有操作看起来就像是按照一定全局顺序执行的。

**示例**：
```cpp
std::atomic<int> counter{0};

void increment() {
    counter.fetch_add(1, std::memory_order_seq_cst);
}

void print() {
    std::cout << counter.load(std::memory_order_seq_cst) << std::endl;
}
```
在这个例子中，所有的增加操作看起来就像是按照一定的顺序执行的，`print`函数总是输出最新的`counter`值。

在多线程环境中，正确使用内存序对于保证程序的正确性和性能都非常关键。开发者应该根据具体情况选择合适的内存序，以避免不必要的同步开销。