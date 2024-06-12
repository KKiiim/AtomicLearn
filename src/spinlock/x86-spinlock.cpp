#ifndef __ASM_SPINLOCK_H
#define __ASM_SPINLOCK_H

#include <asm/atomic.h>
#include <asm/rwlock.h>
#include <asm/page.h>
#include <linux/config.h>

extern int printk(const char * fmt, ...)
	__attribute__ ((format (printf, 1, 2)));

/*
 * Your basic SMP spinlocks, allowing only a single CPU anywhere
 */

typedef struct {
    //自旋锁为无符号的整型变量 volatile保证变量都从内存中获取，不要缓存在寄存器里
	volatile unsigned int lock;
#ifdef CONFIG_DEBUG_SPINLOCK
	unsigned magic;
#endif
} spinlock_t;
//定义一个spinlock的魔数,用来调试用
#define SPINLOCK_MAGIC	0xdead4ead

#ifdef CONFIG_DEBUG_SPINLOCK
#define SPINLOCK_MAGIC_INIT	, SPINLOCK_MAGIC
#else
#define SPINLOCK_MAGIC_INIT	/*如果没有开启调试状态将什么也没有 */
#endif
//创建一个值为1的自旋锁
#define SPIN_LOCK_UNLOCKED (spinlock_t) { 1 SPINLOCK_MAGIC_INIT }
//初始化自旋锁 x是一根指针 所以解引用
#define spin_lock_init(x)	do { *(x) = SPIN_LOCK_UNLOCKED; } while(0)

/*
 * Simple spin lock operations.  There are two variants, one clears IRQ's
 * on the local processor, one does not.
 * 简单的自旋锁操作。有两种变体，一种清除本地处理器上的IRQ，另一种不清除。
 *
 * We make no fairness assumptions. They have a cost.
 * 我们没有做出公平的假设（非公平）。它们是有代价的
 */
//判断自旋锁是否被锁定 先通过取地址拿到spinlock里的lock 再转为字符，再解引用判断是否小于0
#define spin_is_locked(x)	(*(volatile signed char *)(&(x)->lock) <= 0)
//等待自旋锁释放，barrier()保证禁止编译器任意排序
#define spin_unlock_wait(x)	do { barrier(); } while(spin_is_locked(x))
//获取自旋锁内联汇编代码，这里只是code部分，剩下在用的时候肯定是有输出数和输入数的
#define spin_lock_string \
	"1:" \ //1位置
	"lock ; decb %0" \ //对lock变量的值进行自减，如果lock变量是0或者小于0，再减岂不是直接就小于0了吗，所以底下js判断是否符号位为1，也就是小于0
	"js 2f" \  //f是forward，往后跳，因为2：确实在后面，如果小于0，跳转到前面的2处 js判断的是符号位是否为1，为1当然就小于0啦
	LOCK_SECTION_START("") \ //涉及到ELF的知识
	"2:" \
	"rep;nop" \ //repeat空操作
	"cmpb $0,%0" \ //比较lock的值是否为0，%0可以从后面的代码看出，是输出参数，是lock变量 cmpb的b代表比一个字节，l代表4字节
	"jle 2b" \ //b是backward，往前跳，jle代表小于或等于0，继续2处
	"jmp 1b" \//jmp:无条件转移到指定内存地址，否则跳回到1处去进行减一操作，这也不一定还能拿到哦，还得判断，如果成功就拿到了锁！！！！！
	LOCK_SECTION_END //".previous\n\t"

/*
 * This works. Despite all the confusion.
 * 这很有效。尽管如此混乱。
 * (except on PPro SMP or if we are using OOSTORE)
 * (PPro errata 66, 92)
 */
 
#if !defined(CONFIG_X86_OOSTORE) && !defined(CONFIG_X86_PPRO_FENCE)

#define spin_unlock_string \ //这里和lock_string是不一样的，直接就把输入输出和clobber都填好了
	"movb $1,%0" \ //把1设置为lock的值
		:"=m" (lock->lock) : : "memory" //static spinlock_t lock;这里的lock是一个spinlock_t，所以指针再指向spinlock里的lock


static inline void _raw_spin_unlock(spinlock_t *lock)
{
#ifdef CONFIG_DEBUG_SPINLOCK
	if (lock->magic != SPINLOCK_MAGIC)
		BUG();
	if (!spin_is_locked(lock))
		BUG();
#endif
	__asm__ __volatile__(
		spin_unlock_string
	);
}

#else

#define spin_unlock_string \
	"xchgb %b0, %1" \
		:"=q" (oldval), "=m" (lock->lock) \
		:"0" (oldval) : "memory"

static inline void _raw_spin_unlock(spinlock_t *lock)
{
	char oldval = 1;
#ifdef CONFIG_DEBUG_SPINLOCK
	if (lock->magic != SPINLOCK_MAGIC)
		BUG();
	if (!spin_is_locked(lock))
		BUG();
#endif
	__asm__ __volatile__(
		spin_unlock_string
	);
}

#endif

static inline int _raw_spin_trylock(spinlock_t *lock)
{
	char oldval;
	__asm__ __volatile__(
		"xchgb %b0,%1" //交换lock的值（%1）和oldval
		:"=q" (oldval), "=m" (lock->lock) //q:将输入变量放入eax，ebx，ecx，edx中的一个 
		:"0" (0) : "memory"); //0:表示用它限制的操作数与某个指定的操作数(这里就是0)匹配
	return oldval > 0; //大于0说明lock变量为1，那说明就没有人拿到这个锁，那么久尝试获得到了锁
}

static inline void _raw_spin_lock(spinlock_t *lock)
{
#ifdef CONFIG_DEBUG_SPINLOCK
	__label__ here;
here:
	if (lock->magic != SPINLOCK_MAGIC) {
printk("eip: %p\n", &&here);
		BUG();
	}
#endif
	__asm__ __volatile__(
		spin_lock_string
		:"=m" (lock->lock)//输出操作数列表为lock : : "memory");
}


/*
 * Read-write spinlocks, allowing multiple readers
 * but only one writer.
 *
 * NOTE! it is quite common to have readers in interrupts
 * but no interrupt writers. For those circumstances we
 * can "mix" irq-safe locks - any writer needs to get a
 * irq-safe write-lock, but readers can get non-irqsafe
 * read-locks.
 */
typedef struct {
	volatile unsigned int lock;
#ifdef CONFIG_DEBUG_SPINLOCK
	unsigned magic;
#endif
} rwlock_t;//多个读者共享，写者互斥，和互斥自旋锁机构一模一样

#define RWLOCK_MAGIC	0xdeaf1eed

#ifdef CONFIG_DEBUG_SPINLOCK
#define RWLOCK_MAGIC_INIT	, RWLOCK_MAGIC
#else
#define RWLOCK_MAGIC_INIT	/* */
#endif

#define RW_LOCK_UNLOCKED (rwlock_t) { RW_LOCK_BIAS RWLOCK_MAGIC_INIT }//RW_LOCK_BIAS->0x0100 0000第七位

//初始化读写自旋锁
#define rwlock_init(x)	do { *(x) = RW_LOCK_UNLOCKED; } while(0)

#define rwlock_is_locked(x) ((x)->lock != RW_LOCK_BIAS)//RW_LOCK_BIAS代表没上锁(既没有读锁也没有写锁)

/*
 * On x86, we implement read-write locks as a 32-bit counter
 * with the high bit (sign) being the "contended" bit.
 * 在x86上，我们将读写锁实现为32位计数器，高位（符号）为“争用”位。
 *
 * The inline assembly is non-obvious. Think about it.
 *
 * Changed to use the same technique as rw semaphores.  See
 * semaphore.h for details.  -ben
 */
#endif
/* the spinlock helpers are in arch/i386/kernel/semaphore.c */
//helper__write_lock_failed和的实现去这个地方（arch/i386/kernel/semaphore.c）找，否则找不到
//获取读锁或者写锁失败后的helper实现
static inline void _raw_read_lock(rwlock_t *rw)
{
#ifdef CONFIG_DEBUG_SPINLOCK
	if (rw->magic != RWLOCK_MAGIC)
		BUG();
#endif
	__build_read_lock(rw, "__read_lock_failed");//在读写锁文件rwlock.h里有相应实现
}

static inline void _raw_write_lock(rwlock_t *rw)
{
#ifdef CONFIG_DEBUG_SPINLOCK
	if (rw->magic != RWLOCK_MAGIC)
		BUG();
#endif
	__build_write_lock(rw, "__write_lock_failed");
}

//读锁和写锁的释放也很简单，原子加1或者原子加0x0100 0000
#define _raw_read_unlock(rw)		asm volatile("lock ; incl %0" :"=m" ((rw)->lock) : : "memory")
#define _raw_write_unlock(rw)	asm volatile("lock ; addl $" RW_LOCK_BIAS_STR ",%0":"=m" ((rw)->lock) : : "memory")

static inline int _raw_write_trylock(rwlock_t *lock)
{
	atomic_t *count = (atomic_t *)lock;
	if (atomic_sub_and_test(RW_LOCK_BIAS, count))
		return 1;
	atomic_add(RW_LOCK_BIAS, count);
	return 0;
}

#endif /* __ASM_SPINLOCK_H */