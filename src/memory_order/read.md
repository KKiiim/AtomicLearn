
```c++
// test.cpp
int x, y, r;
void f()
{
    x = r;
    y = 1;
}
/*
这里我们看到，x = r 和 y = 1 并没有乱序。现使用优化选项 O2（或 O3）编译上面的代码（g++ -O2 -S test.cpp），生成汇编代码如下

使用编译器 barrier（又叫优化 barrier）。Linux 内核提供函数 barrier() 用于让编译器保证其之前的内存访问先于其之后的完成。内核实现 barrier() 如下（X86-64 架构）：
*/

#define barrier() __asm__ __volatile__("" ::: "memory")

int x, y, r;
void f()
{
    x = r;
    __asm__ __volatile__("" ::: "memory");
    y = 1;
}

volatile int x, y;
int r;
void f()
{
    x = r;
    y = 1;
}

#define ACCESS_ONCE(x) (*(volatile typeof(x) *)&(x))

int x, y, r;
void f()
{
    ACCESS_ONCE(x) = r;
    ACCESS_ONCE(y) = 1;
}

/*
乱序处理器（Out-of-order processors）处理指令通常有以下几步：

指令获取
指令被分发到指令队列
指令在指令队列中等待，直到输入操作对象可用（一旦输入操作对象可用，指令就可以离开队列，即便更早的指令未被执行）
指令被分配到适当的功能单元并执行
执行结果被放入队列（而不立即写入寄存器堆）
只有所有更早请求执行的指令的执行结果被写入寄存器堆后，指令执行的结果才被写入寄存器堆（执行结果重排序，让执行看起来是有序的）


对于单个 CPU 指令获取是有序的（通过队列实现）
对于单个 CPU 指令执行结果也是有序返回寄存器堆的（通过队列实现）
由此可知，在单 CPU 上，不考虑编译器优化导致乱序的前提下，多线程执行不存在内存乱序访问的问题。我们从内核源码也可以得到类似的结论（代码不完全的摘录）：

    
    
#ifdef CONFIG_SMP
#define smp_mb() mb()
#else
#define smp_mb() barrier()
#endif
这里可以看到，如果是 SMP 则使用 mb，mb 被定义为 CPU Memory barrier（后面会讲到），而非 SMP 时，直接使用编译器 barrier
*/

现在通过一个例子来说明多 CPU 下内存乱序访问：
// test2.cpp
#include <pthread.h>
#include <assert.h>
 
// -------------------
int cpu_thread1 = 0;
int cpu_thread2 = 1;
 
volatile int x, y, r1, r2;
 
void start()
{
    x = y = r1 = r2 = 0;
}
 
void end()
{
    assert(!(r1 == 0 && r2 == 0));
}
 
void run1()
{
    x = 1;
    r1 = y;
}
 
void run2()
{
    y = 1;
    r2 = x;
}
 
// -------------------
static pthread_barrier_t barrier_start;
static pthread_barrier_t barrier_end;
 
static void* thread1(void*)
{
    while (1) {
        pthread_barrier_wait(&barrier_start);
        run1();
        pthread_barrier_wait(&barrier_end);
    }
 
    return NULL;
}
 
static void* thread2(void*)
{
    while (1) {
        pthread_barrier_wait(&barrier_start);
        run2();
        pthread_barrier_wait(&barrier_end);
    }
 
    return NULL;
}
 
int main()
{
    assert(pthread_barrier_init(&barrier_start, NULL, 3) == 0);
    assert(pthread_barrier_init(&barrier_end, NULL, 3) == 0);
 
    pthread_t t1;
    pthread_t t2;
    assert(pthread_create(&t1, NULL, thread1, NULL) == 0);
    assert(pthread_create(&t2, NULL, thread2, NULL) == 0);
 
    cpu_set_t cs;
    CPU_ZERO(&cs);
    CPU_SET(cpu_thread1, &cs);
    assert(pthread_setaffinity_np(t1, sizeof(cs), &cs) == 0);
    CPU_ZERO(&cs);
    CPU_SET(cpu_thread2, &cs);
    assert(pthread_setaffinity_np(t2, sizeof(cs), &cs) == 0);
 
    while (1) {
        start();
        pthread_barrier_wait(&barrier_start);
        pthread_barrier_wait(&barrier_end);
        end();
    }
 
    return 0;
}





最后，我们使用 CPU Memory barrier 来解决内存乱序访问的问题（X86-64 架构下）：

    
    
int cpu_thread1 = 0;
int cpu_thread2 = 1;
 
void run1()
{
    x = 1;
    __asm__ __volatile__("mfence" ::: "memory");
    r1 = y;
}
 
void run2()
{
    y = 1;
    __asm__ __volatile__("mfence" ::: "memory");
    r2 = x;
}



CPU Memory barrier：

通用 barrier，保证读写操作有序的，mb() 和 smp_mb()
写操作 barrier，仅保证写操作有序的，wmb() 和 smp_wmb()
读操作 barrier，仅保证读操作有序的，rmb() 和 smp_rmb()

注意，所有的 CPU Memory barrier（除了数据依赖 barrier 之外）都隐含了编译器 barrier。这里的 smp 开头的 Memory barrier 会根据配置在单处理器上直接使用编译器 barrier，而在 SMP 上才使用 CPU Memory barrier（也就是 mb()、wmb()、rmb()，回忆上面相关内核代码）

最后需要注意一点的是，CPU Memory barrier 中某些类型的 Memory barrier 需要成对使用，否则会出错，详细来说就是：一个写操作 barrier 需要和读操作（或数据依赖）barrier 一起使用（当然，通用 barrier 也是可以的），反之依然。


Memory barrier 的范例
读内核代码进一步学习 Memory barrier 的使用。
Linux 内核实现的无锁（只有一个读线程和一个写线程时）环形缓冲区 kfifo 就使用到了 Memory barrier，实现源码如下：

    
    
/*
 * A simple kernel FIFO implementation.
 *
 * Copyright (C) 2004 Stelian Pop <stelian@popies.net>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 */
 
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/err.h>
#include <linux/kfifo.h>
#include <linux/log2.h>
 
/**
 * kfifo_init - allocates a new FIFO using a preallocated buffer
 * @buffer: the preallocated buffer to be used.
 * @size: the size of the internal buffer, this have to be a power of 2.
 * @gfp_mask: get_free_pages mask, passed to kmalloc()
 * @lock: the lock to be used to protect the fifo buffer
 *
 * Do NOT pass the kfifo to kfifo_free() after use! Simply free the
 * &struct kfifo with kfree().
 */
struct kfifo *kfifo_init(unsigned char *buffer, unsigned int size,
                         gfp_t gfp_mask, spinlock_t *lock)
{
    struct kfifo *fifo;
 
    /* size must be a power of 2 */
    BUG_ON(!is_power_of_2(size));
 
    fifo = kmalloc(sizeof(struct kfifo), gfp_mask);
    if (!fifo)
        return ERR_PTR(-ENOMEM);
 
    fifo->buffer = buffer;
    fifo->size = size;
    fifo->in = fifo->out = 0;
    fifo->lock = lock;
 
    return fifo;
}
EXPORT_SYMBOL(kfifo_init);
 
/**
 * kfifo_alloc - allocates a new FIFO and its internal buffer
 * @size: the size of the internal buffer to be allocated.
 * @gfp_mask: get_free_pages mask, passed to kmalloc()
 * @lock: the lock to be used to protect the fifo buffer
 *
 * The size will be rounded-up to a power of 2.
 */
struct kfifo *kfifo_alloc(unsigned int size, gfp_t gfp_mask, spinlock_t *lock)
{
    unsigned char *buffer;
    struct kfifo *ret;
 
    /*
     * round up to the next power of 2, since our 'let the indices
     * wrap' technique works only in this case.
     */
    if (!is_power_of_2(size)) {
        BUG_ON(size > 0x80000000);
        size = roundup_pow_of_two(size);
    }
 
    buffer = kmalloc(size, gfp_mask);
    if (!buffer)
        return ERR_PTR(-ENOMEM);
 
    ret = kfifo_init(buffer, size, gfp_mask, lock);
 
    if (IS_ERR(ret))
        kfree(buffer);
 
    return ret;
}
EXPORT_SYMBOL(kfifo_alloc);
 
/**
 * kfifo_free - frees the FIFO
 * @fifo: the fifo to be freed.
 */
void kfifo_free(struct kfifo *fifo)
{
    kfree(fifo->buffer);
    kfree(fifo);
}
EXPORT_SYMBOL(kfifo_free);
 
/**
 * __kfifo_put - puts some data into the FIFO, no locking version
 * @fifo: the fifo to be used.
 * @buffer: the data to be added.
 * @len: the length of the data to be added.
 *
 * This function copies at most @len bytes from the @buffer into
 * the FIFO depending on the free space, and returns the number of
 * bytes copied.
 *
 * Note that with only one concurrent reader and one concurrent
 * writer, you don't need extra locking to use these functions.
 */
unsigned int __kfifo_put(struct kfifo *fifo,
                         const unsigned char *buffer, unsigned int len)
{
    unsigned int l;
 
    len = min(len, fifo->size - fifo->in + fifo->out);
 
    /*
     * Ensure that we sample the fifo->out index -before- we
     * start putting bytes into the kfifo.
     */
 
    smp_mb();
 
    /* first put the data starting from fifo->in to buffer end */
    l = min(len, fifo->size - (fifo->in & (fifo->size - 1)));
    memcpy(fifo->buffer + (fifo->in & (fifo->size - 1)), buffer, l);
 
    /* then put the rest (if any) at the beginning of the buffer */
    memcpy(fifo->buffer, buffer + l, len - l);
 
    /*
     * Ensure that we add the bytes to the kfifo -before-
     * we update the fifo->in index.
     */
 
    smp_wmb();
 
    fifo->in += len;
 
    return len;
}
EXPORT_SYMBOL(__kfifo_put);
 
/**
 * __kfifo_get - gets some data from the FIFO, no locking version
 * @fifo: the fifo to be used.
 * @buffer: where the data must be copied.
 * @len: the size of the destination buffer.
 *
 * This function copies at most @len bytes from the FIFO into the
 * @buffer and returns the number of copied bytes.
 *
 * Note that with only one concurrent reader and one concurrent
 * writer, you don't need extra locking to use these functions.
 */
unsigned int __kfifo_get(struct kfifo *fifo,
                         unsigned char *buffer, unsigned int len)
{
    unsigned int l;
 
    len = min(len, fifo->in - fifo->out);
 
    /*
     * Ensure that we sample the fifo->in index -before- we
     * start removing bytes from the kfifo.
     */
 
    smp_rmb();
 
    /* first get the data from fifo->out until the end of the buffer */
    l = min(len, fifo->size - (fifo->out & (fifo->size - 1)));
    memcpy(buffer, fifo->buffer + (fifo->out & (fifo->size - 1)), l);
 
    /* then get the rest (if any) from the beginning of the buffer */
    memcpy(buffer + l, fifo->buffer, len - l);
 
    /*
     * Ensure that we remove the bytes from the kfifo -before-
     * we update the fifo->out index.
     */
 
    smp_mb();
 
    fifo->out += len;
 
    return len;
}
EXPORT_SYMBOL(__kfifo_get);
为了更好的理解上面的源码，这里顺带说一下此实现使用到的一些和本文主题无关的技巧：

使用与操作来求取环形缓冲区的下标，相比取余操作来求取下标的做法效率要高不少。使用与操作求取下标的前提是环形缓冲区的大小必须是 2 的 N 次方，换而言之就是说环形缓冲区的大小为一个仅有一个 1 的二进制数，那么 index & (size – 1) 则为求取的下标（这不难理解）
使用了 in 和 out 两个索引且 in 和 out 是一直递增的（此做法比较巧妙），这样能够避免一些复杂的条件判断（某些实现下，in == out 时还无法区分缓冲区是空还是满）
这里，索引 in 和 out 被两个线程访问。in 和 out 指明了缓冲区中实际数据的边界，也就是 in 和 out 同缓冲区数据存在访问上的顺序关系，由于未使用同步机制，那么保证顺序关系就需要使用到 Memory barrier 了。索引 in 和 out 都分别只被一个线程修改，而被两个线程读取。__kfifo_put 先通过 in 和 out 来确定可以向缓冲区中写入数据量的多少，这时，out 索引应该先被读取后才能真正的将用户 buffer 中的数据写入缓冲区，因此这里使用到了 smp_mb()，对应的，__kfifo_get 也使用 smp_mb() 来确保修改 out 索引之前缓冲区中数据已经被成功读取并写入用户 buffer 中了。对于 in 索引，在 __kfifo_put 中，通过 smp_wmb() 保证先向缓冲区写入数据后才修改 in 索引，由于这里只需要保证写入操作有序，故选用写操作 barrier，在 __kfifo_get 中，通过 smp_rmb() 保证先读取了 in 索引（这时候 in 索引用于确定缓冲区中实际存在多少可读数据）才开始读取缓冲区中数据（并写入用户 buffer 中），由于这里只需要保证读取操作有序，故选用读操作 barrier。

到这里，Memory barrier 就介绍完毕了。
```