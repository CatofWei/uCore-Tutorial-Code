#include "console.h"
#include "defs.h"
#include "loader.h"
#include "plic.h"
#include "timer.h"
#include "trap.h"
#include "virtio.h"

void clean_bss()
{
	extern char s_bss[];
	extern char e_bss[];
	memset(s_bss, 0, e_bss - s_bss);
}

void main()
{
	clean_bss();
	printf("hello world!\n");
	proc_init();
	kinit();
	kvm_init();
	trap_init();
	plicinit();
	// 初始化磁盘设备
	virtio_disk_init();
	// 初始化磁盘块读写缓存
	binit();
	fsinit();
	timer_init();
	load_init_app();
	infof("start scheduler!");
	show_all_files();
	scheduler();
}