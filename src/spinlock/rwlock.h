/* include/asm-x86_64/rwlock.h
 *
 *	Helpers used by both rw spinlocks and rw semaphores.
 *
 *	Based in part on code from semaphore.h and
 *	spinlock.h Copyright 1996 Linus Torvalds.
 *
 *	Copyright 1999 Red Hat, Inc.
 *	Copyright 2001,2002 SuSE labs 
 *
 *	Written by Benjamin LaHaise.
 *
 *	This program is free software; you can redistribute it and/or
 *	modify it under the terms of the GNU General Public License
 *	as published by the Free Software Foundation; either version
 *	2 of the License, or (at your option) any later version.
 */
#ifndef _ASM_X86_64_RWLOCK_H
#define _ASM_X86_64_RWLOCK_H

#include <linux/stringify.h>

#define RW_LOCK_BIAS		 0x01000000
#define RW_LOCK_BIAS_STR	"0x01000000"

#define __build_read_lock_ptr(rw, helper)   \
	asm volatile(LOCK "subl $1,(%0)" \ //获取读锁就是尝试在lock上减1，因为RW_LOCK_BIAS是非常大的一个数
	// 只有有写锁的时候会直接减去0x01000000变为0，其他时候是不可能小于0的，所以没有写锁的情况下，去抢占这个读锁是没有问题的
		     "js 2f" \ //判断符号位是否为1,也即是否为负数，如果是负数，那么说明已经有写锁啦，那你只能去2位置了
		     "1:" \
		    LOCK_SECTION_START("") \
		     "2: call " helper "" \ //调用helper方法，helper即为__read_lock_failed，下面把这段实现摘抄出来了，我没有获取到锁，只好调用这个方法
		     "jmp 1b" \
		    LOCK_SECTION_END \
		     ::"a" (rw) : "memory") //a:将输入变量放入eax

//这段代码是从i386的semaphore下摘出来的
	asm(
	"__read_lock_failed:"
		LOCK "incl	(%eax)"//原子性增加eax寄存器中的值（也就是lock变量的值）
	"1: rep; nop" //进行空操作，耗掉一点点时间
		"cmpl	$1,(%eax)"// cmp影响符号位，lock变量和1相减如果是负数，符号位为1
		"js 1b" //如果是负数那么就去1重新比较，直到可以获得读锁
		LOCK "decl	(%eax)"//说明eax也就是lock大于等于1，进行相减再次判断是否为负数，因为可能两个线程同时走这一步，严谨！！！！！
		"js __read_lock_failed"//负数说明又没抢到，继续循环吧
		"ret"//抢到了，返回
	);



#define __build_read_lock_const(rw, helper)   \
	asm volatile(LOCK "subl $1,%0\n\t" \
		     "js 2f\n" \
		     "1:\n" \
		    LOCK_SECTION_START("") \
		     "2:\tpushq %%rax\n\t" \
		     "leaq %0,%%rax\n\t" \
		     "call " helper "\n\t" \
		     "popq %%rax\n\t" \
		     "jmp 1b\n" \
		    LOCK_SECTION_END \
		     :"=m" (*((volatile int *)rw))::"memory")

#define __build_read_lock(rw, helper)	do { \
						if (__builtin_constant_p(rw)) \
							__build_read_lock_const(rw, helper); \
						else \
							__build_read_lock_ptr(rw, helper); \//走这里
					} while (0)

#define __build_write_lock_ptr(rw, helper) \
	asm volatile(LOCK "subl $" RW_LOCK_BIAS_STR ",(%0)" \ //核心，写锁只能有一个，上来直接把那个大数减完了
		     "jnz 2f" \ //看是否为0，不为0说明绝对有读锁在占着，直接失败去2处forward
		     "1:" \
		     LOCK_SECTION_START("") \
		     "2: call " helper "" \同读锁逻辑,截取helper如下
		     "jmp 1b" \
		     LOCK_SECTION_END \
		     ::"a" (rw) : "memory")

//这段代码是从i386的semaphore下摘出来的
	asm(
	"__write_lock_failed:"
		LOCK "addl	$" RW_LOCK_BIAS_STR ",(%eax)" //大同小异，先加后减
	"1: rep; nop\n\t"
		"cmpl	$" RW_LOCK_BIAS_STR ",(%eax)"
		"jne	1b"
		LOCK "subl	$" RW_LOCK_BIAS_STR ",(%eax)"
		"jnz	__write_lock_failed"
		"ret"
	);


#define __build_write_lock_const(rw, helper) \
	asm volatile(LOCK "subl $" RW_LOCK_BIAS_STR ",(%0)\n\t" \
		     "jnz 2f\n" \
		     "1:\n" \
		    LOCK_SECTION_START("") \
		     "2:\tpushq %%rax\n\t" \
		     "leaq %0,%%rax\n\t" \
		     "call " helper "\n\t" \
		     "popq %%rax\n\t" \
		     "jmp 1b\n" \
		    LOCK_SECTION_END \
		     :"=m" (*((volatile long *)rw))::"memory")

#define __build_write_lock(rw, helper)	do { \
						if (__builtin_constant_p(rw)) \
							__build_write_lock_const(rw, helper); \
						else \
							__build_write_lock_ptr(rw, helper); \ //上读锁走这里
					} while (0)

#endif