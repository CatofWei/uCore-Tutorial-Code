#include "console.h"
#include "defs.h"
#include "loader.h"
#include "timer.h"
#include "trap.h"

void clean_bss()
{
	extern char s_bss[];
	extern char e_bss[];
	memset(s_bss, 0, e_bss - s_bss);
}

void main()
{
	clean_bss();
	proc_init();
	kinit();
	// 由于内核页表是恒等映射，所以开启分页前后没有变化,内核还是能正常访问整个物理内存
	kvm_init();
	loader_init();
	trap_init();
	timer_init();
	run_all_app();
	infof("start scheduler!");
	scheduler();
}