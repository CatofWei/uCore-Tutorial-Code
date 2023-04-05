#ifndef PROC_H
#define PROC_H

#include "riscv.h"
#include "types.h"

#define NPROC (16)
#define MAX_SYSCALL_NUM (500)

// Saved registers for kernel context switches.
struct context {
	uint64 ra;   // 这里的ra 和 sp 必须使用内核页表访问
	uint64 sp;   // 因为ra和sp位于内核地址空间

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
/*
* LAB1: you may need to define struct for TaskInfo here
*/

typedef enum {
	UnInit,
	Ready,
	Running,
	Exited,
} TaskStatus;

typedef struct {
	TaskStatus status;
	unsigned int syscall_times[MAX_SYSCALL_NUM];
	int time;
} TaskInfo;

enum procstate { UNUSED, USED, SLEEPING, RUNNABLE, RUNNING, ZOMBIE };

// Per-process state
struct proc {
	enum procstate state; // Process state
	int pid; // Process ID
	pagetable_t pagetable; // User page table
	uint64 ustack;     //virtial address of user stack bottom(bottom 是栈的低地址)
	uint64 kstack; // Virtual address of kernel stack bottom
	struct trapframe *trapframe; // data page for trampoline.S
	struct context context; // swtch() here to run process
	uint64 max_page;
	/*
	* LAB1: you may need to add some new fields here
	*/
	unsigned int syscall_times[MAX_SYSCALL_NUM];
	int startTimeStamp;
};


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