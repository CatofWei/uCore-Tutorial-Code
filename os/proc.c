#include "proc.h"
#include "defs.h"
#include "loader.h"
#include "trap.h"
#define NULL ((void*)0)
/**
 * 预先分配好的进程表，包含NPROC个进程控制块，表示该操作系统最多运行NPROC个进程
 */
struct proc pool[NPROC];
struct proc sential_node;
/**
 * 预分配的内核栈
 */
char kstack[NPROC][PAGE_SIZE];
/**
 * 预分配的用户栈
 */
__attribute__((aligned(4096))) char ustack[NPROC][PAGE_SIZE];
__attribute__((aligned(4096))) char trapframe[NPROC][PAGE_SIZE];

extern char boot_stack_top[];
// 当前执行的进程
struct proc *current_proc;
struct proc *runnable_proc_head;
struct proc *runnable_proc_tail;
struct proc idle;

int threadid()
{
	return curr_proc()->pid;
}

struct proc *curr_proc()
{
	return current_proc;
}
void init_proc_queue() {
	memset(&sential_node, 0, sizeof(struct proc));
	runnable_proc_head = &sential_node;
	runnable_proc_tail = &sential_node;
}
void push_runnable_proc(struct proc * n) {
	n->next = NULL;
	runnable_proc_tail->next = n;
	runnable_proc_tail = n;
}
struct proc* pop_runnable_proc() {
	if (runnable_proc_head == runnable_proc_tail) {
		return NULL;
	}
	struct proc *temp = runnable_proc_head->next;
	runnable_proc_head -> next = temp->next;
	if (runnable_proc_tail == temp) {
		runnable_proc_tail = runnable_proc_head;
	}
	return temp;
}
int isEmpty() {
	if (runnable_proc_head == runnable_proc_tail) {
		return 1;
	}
	return 0;
}
// initialize the proc table at boot time.
void proc_init(void)
{
	init_proc_queue();
	struct proc *p;
	/**
	 * 初始化进程控制块
	 */
	for (p = pool; p < &pool[NPROC]; p++) {
		p->state = UNUSED;
		p->kstack = (uint64)kstack[p - pool];
		p->ustack = (uint64)ustack[p - pool];
		p->trapframe = (struct trapframe *)trapframe[p - pool];
		/*
		* LAB1: you may need to initialize your new fields of proc here
		*/
	}
	// idle为内核创建的系统闲置进程，开内核启动的开始阶段，都是idle进程在运行
	idle.kstack = (uint64)boot_stack_top;
	idle.pid = 0;
	idle.state = RUNNING;
	current_proc = &idle;
}

int allocpid()
{
	static int PID = 1;
	return PID++;
}

// Look in the process table for an UNUSED proc.
// If found, initialize state required to run in the kernel.
// If there are no free procs, or a memory allocation fails, return 0.
struct proc *allocproc(void)
{
	struct proc *p;
	for (p = pool; p < &pool[NPROC]; p++) {
		if (p->state == UNUSED) {
			goto found;
		}
	}
	return 0;

found:
	p->pid = allocpid();
	p->state = USED;
	memset(&p->context, 0, sizeof(p->context));
	memset(p->trapframe, 0, PAGE_SIZE);
	memset((void *)p->kstack, 0, PAGE_SIZE);
	p->next = NULL;
	// 进程切换时的返回地址
	p->context.ra = (uint64)usertrapret;
	// 进程切换时的栈
	p->context.sp = p->kstack + PAGE_SIZE;
	return p;
}

// Scheduler never returns.  It loops, doing:
//  - choose a process to run.
//  - swtch to start running that process.
//  - eventually that process transfers control
//    via swtch back to the scheduler.
void scheduler(void)
{
	struct proc *p;
	for (;;) {
		for (p = pool; p < &pool[NPROC]; p++) {
			// 选择就绪态的进程
			if (p->state == RUNNABLE) {
				/*
				* LAB1: you may need to init proc start time here
				*/
				// 设置状态为running
				p->state = RUNNING;
				// 指向新进程
				current_proc = p;
				swtch(&idle.context, &p->context);
			}
		}
	}
}
void schedulerV2(enum procstate nextstatus) {
	struct proc * old_proc = current_proc;
	if (nextstatus == RUNNABLE) {
		old_proc->state = nextstatus;
		push_runnable_proc(old_proc);
	}
	struct proc *next_proc = pop_runnable_proc();
	if (next_proc == NULL) {
		panic("there no runnable task!\n");
	}
	next_proc->state = RUNNING;
	current_proc = next_proc;
	printf("switch to proc_id:%d\n", current_proc->pid);
	swtch(&old_proc->context, &next_proc->context);
}

void idleLoop() {
	while (1) {
		schedulerV2(RUNNABLE);
		if (curr_proc() == &idle && isEmpty()) {
			panic("all apps finished!\n");
		}
	}
}

// Switch to scheduler.  Must hold only p->lock
// and have changed proc->state. Saves and restores
// intena because intena is a property of this
// kernel thread, not this CPU. It should
// be proc->intena and proc->noff, but that would
// break in the few places where a lock is held but
// there's no process.
void sched(void)
{
	struct proc *p = curr_proc();
	if (p->state == RUNNING)
		panic("sched running");
	swtch(&p->context, &idle.context);
}

// Give up the CPU for one scheduling round.
void yield(void)
{
//	current_proc->state = RUNNABLE;
//	sched();
	schedulerV2(RUNNABLE);
}

// Exit the current process.
void exit(int code)
{
	struct proc *p = curr_proc();
	printf("proc %d exit with %d\n", p->pid, code);
	p->state = UNUSED;
	schedulerV2(ZOMBIE);
//	yield();
//	finished();
//	sched();
}
