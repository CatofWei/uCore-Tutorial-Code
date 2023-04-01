#ifndef PROC_H
#define PROC_H

#include "types.h"

#define NPROC (16)

// Saved registers for kernel context switches.
struct context {
	/**
	 * 进程切换时的返回地址，返回后仍处于内核态
	 */
	uint64 ra;
	/**
	 * 内核栈顶
	 */
	uint64 sp;

	// callee-saved
	uint64 s0;
	uint64 s1;
	uint64 s2;
	uint64 s3;
	uint64 s4;
	uint64 s5;
	uint64 s6;
	uint64 s7;
	uint64 s8;
	uint64 s9;
	uint64 s10;
	uint64 s11;
};

enum procstate {
UNUSED,     // 未初始化
USED,       // 基本初始化，未加载用户程序
SLEEPING,   // 休眠状态(未使用，留待后续拓展)
RUNNABLE,   // 可运行
RUNNING,    // 当前正在运行
ZOMBIE,     // 已经 exit
};
/**
 * 进程控制块，保存着一个进程的信息
 */
// Per-process state
struct proc {
	enum procstate state; // 进程状态
	int pid; // 进程ID
	uint64 ustack; // 进程用户栈虚拟地址(用户页表)，从代码看这是栈底
	uint64 kstack; // 进程内核栈虚拟地址(内核页表)，从代码看这是栈底
	struct trapframe *trapframe; // data page for trampoline.S，进程中断帧
	struct context context; // swtch() here to run process，用于保存进程内核态的寄存器信息，进程切换时使用
	/*
	* LAB1: you may need to add some new fields here
	*/
};

/*
* LAB1: you may need to define struct for TaskInfo here
*/

struct proc *curr_proc();
void exit(int);
void proc_init();
void scheduler() __attribute__((noreturn));
void sched();
void yield();
struct proc *allocproc();
// swtch.S
void swtch(struct context *, struct context *);

#endif // PROC_H