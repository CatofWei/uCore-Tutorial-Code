#include "loader.h"
#include "defs.h"
#include "trap.h"

static uint64 app_num;
static uint64 *app_info_ptr;
extern char _app_num[], ekernel[];

// Count finished programs. If all apps exited, shutdown.
int finished()
{
	static int fin = 0;
	if (++fin >= app_num)
		panic("all apps over");
	return 0;
}

// Get user progs' infomation through pre-defined symbol in `link_app.S`
void loader_init()
{
	if ((uint64)ekernel >= BASE_ADDRESS) {
		panic("kernel too large...\n");
	}
	app_info_ptr = (uint64 *)_app_num;
	app_num = *app_info_ptr;
	app_info_ptr++;
}

// Load nth user app at
// [BASE_ADDRESS + n * MAX_APP_SIZE, BASE_ADDRESS + (n+1) * MAX_APP_SIZE)
int load_app(int n, uint64 *info)
{
	uint64 start = info[n], end = info[n + 1], length = end - start;
	memset((void *)BASE_ADDRESS + n * MAX_APP_SIZE, 0, MAX_APP_SIZE);
	memmove((void *)BASE_ADDRESS + n * MAX_APP_SIZE, (void *)start, length);
	return length;
}

// load all apps and init the corresponding `proc` structure.
int run_all_app()
{
	for (int i = 0; i < app_num; ++i) {
		/**
		 * 进程加载时，初始化的进程控制块中，进程状态，进程id，进程内核栈，context的返回地址和栈是有用的
		 */
		struct proc *p = allocproc();

		struct trapframe *trapframe = p->trapframe;
		load_app(i, app_info_ptr);
		uint64 entry = BASE_ADDRESS + i * MAX_APP_SIZE;
		tracef("load app %d at %p", i, entry);
		/**
		 * context结构体，保存的是进程切换时，新进程在内核态的返回地址和内核栈，以及寄存器（暗含的意思时进程切换时
		 * 先返回新进程的内核态，再返回到用户态执行）
		 * trapframe中的则是进程在用户态的返回地址，用户栈，和寄存器
		 */
		trapframe->epc = entry;
		trapframe->sp = (uint64)p->ustack + USER_STACK_SIZE;
		p->state = RUNNABLE;
		/*
		* LAB1: you may need to initialize your new fields of proc here
		*/
	}
	return 0;
}