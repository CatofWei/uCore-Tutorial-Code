#include "loader.h"
#include "defs.h"
#include "trap.h"

static int app_num;
static uint64 *app_info_ptr;
extern char _app_num[];

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
	app_info_ptr = (uint64 *)_app_num;
	app_num = *app_info_ptr;
	app_info_ptr++;
}
/**
 * 应用地址空间布局
 */
pagetable_t bin_loader(uint64 start, uint64 end, struct proc *p)
{

	pagetable_t pg = uvmcreate();
	// trapframe为内核数据结构
	if (mappages(pg, TRAPFRAME, PGSIZE, (uint64)p->trapframe,
		     PTE_R | PTE_W) < 0) {
		panic("mappages fail");
	}
	if (!PGALIGNED(start)) {
		panic("user program not aligned, start = %p", start);
	}
	if (!PGALIGNED(end)) {
		// Fix in ch5
		warnf("Some kernel data maybe mapped to user, start = %p, end = %p",
		      start, end);
	}
	// 对齐到下一页
	end = PGROUNDUP(end);
	uint64 length = end - start;
	// 用户程序放在虚拟地址0x1000上
	if (mappages(pg, BASE_ADDRESS, length, start,
		     PTE_U | PTE_R | PTE_W | PTE_X) != 0) {
		panic("mappages fail");
	}
	p->pagetable = pg;
	// 虚拟地址空间布局，
	uint64 ustack_bottom_vaddr = BASE_ADDRESS + length + PAGE_SIZE;
	if (USTACK_SIZE != PAGE_SIZE) {
		// Fix in ch5
		panic("Unsupported");
	}
	// 用户栈在地址空间中和用户内存隔着一页
	mappages(pg, ustack_bottom_vaddr, USTACK_SIZE, (uint64)kalloc(),
		 PTE_U | PTE_R | PTE_W | PTE_X);
	p->ustack = ustack_bottom_vaddr;
	// 应用程序入口地址为虚拟地址0x1000
	p->trapframe->epc = BASE_ADDRESS;
	// 用户栈
	p->trapframe->sp = p->ustack + USTACK_SIZE;
	// exit 的时候会 unmap 页表中 [BASE_ADDRESS, max_page * PAGE_SIZE) 的页
	p->max_page = PGROUNDUP(p->ustack + USTACK_SIZE - 1) / PAGE_SIZE;
	return pg;
}

// load all apps and init the corresponding `proc` structure.
int run_all_app()
{
	for (int i = 0; i < app_num; ++i) {
		struct proc *p = allocproc();
		tracef("load app %d", i);
		bin_loader(app_info_ptr[i], app_info_ptr[i + 1], p);
		p->state = RUNNABLE;
		/*
		* LAB1: you may need to initialize your new fields of proc here
		*/
	}
	return 0;
}