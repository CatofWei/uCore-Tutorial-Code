#include "syscall.h"
#include "defs.h"
#include "loader.h"
#include "syscall_ids.h"
#include "timer.h"
#include "trap.h"

uint64 sys_write(int fd, uint64 va, uint len)
{
	debugf("sys_write fd = %d va = %x, len = %d", fd, va, len);
	if (fd != STDOUT)
		return -1;
	struct proc *p = curr_proc();
	char str[MAX_STR_LEN];
	// 将虚拟地址va开始处的字符数据写入内核缓冲。
	int size = copyinstr(p->pagetable, str, va, MIN(len, MAX_STR_LEN));
	debugf("size = %d", size);
	for (int i = 0; i < size; ++i) {
		console_putchar(str[i]);
	}
	return size;
}

__attribute__((noreturn)) void sys_exit(int code)
{
	exit(code);
	__builtin_unreachable();
}

uint64 sys_sched_yield()
{
	yield();
	return 0;
}

uint64 sys_gettimeofday(TimeVal *val, int _tz) // TODO: implement sys_gettimeofday in pagetable. (VA to PA)
{
	// YOUR CODE
	TimeVal timeVal;
	uint64 cycle = get_cycle();
	timeVal.sec = cycle / CPU_FREQ;
	timeVal.usec = (cycle % CPU_FREQ) * 1000000 / CPU_FREQ;
	pagetable_t pagetable = curr_proc()->pagetable;
	copyout(pagetable, (uint64)val, (char *)&timeVal, sizeof(TimeVal));

	/* The code in `ch3` will leads to memory bugs*/

	// uint64 cycle = get_cycle();
	// val->sec = cycle / CPU_FREQ;
	// val->usec = (cycle % CPU_FREQ) * 1000000 / CPU_FREQ;
	return 0;
}

uint64 anonymous_map(uint64 start, unsigned long long len, int prot) {
	long end = start + len;
	uint64 pa;
	end = PGROUNDUP(end);
	for (uint64 va = start; va + PGSIZE <= end ; va += PGSIZE) {
		if((pa = (uint64)kalloc())==0){
			return -1;
		}
		if(mappages(curr_proc()->pagetable, va, PGSIZE, pa,(prot << 1) | PTE_U) == -1) {
			return -1;
		}
//		printf("succeed map va: %p to pa: %p\n", va, pa);
	}
	return 0;
}

// TODO: add support for mmap and munmap syscall.
// hint: read through docstrings in vm.c. Watching CH4 video may also help.
// Note the return value and PTE flags (especially U,X,W,R)
uint64 sys_mmap(uint64 start, unsigned long long len, int prot, int flag, int fd) {
	if(!PGALIGNED(start)) {
		printf("va must aligned, %p\n", start);
		return -1;
	}
	if ((prot & ~(0x3)) != 0) {
		printf("prot is ilgeal\n");
		return -1;
	}
	if ((prot & (0x3)) == 0) {
		printf("meaningless prot\n");
		return -1;
	}
	return anonymous_map(start, len, prot);
}

uint64 sys_munmap(uint64 start, unsigned long long len) {
	uint64 end = start + len;
	end = PGROUNDUP(end);
	start = PGROUNDDOWN(start);
	for (uint64 va = start; va < end; va += PGSIZE) {
		pte_t * pte;
		if((pte = walk(curr_proc()->pagetable, va, 0)) == 0) {
			return -1;
		}
		if ((*pte & PTE_V) == 0) {
			return -1;
		}
		kfree((void *)PTE2PA(*pte));
		*pte = 0;
	}
	return 0;
}

/*
* LAB1: you may need to define sys_task_info here
*/
uint64 sys_task_info(TaskInfo *info) {
	TaskInfo taskInfo;
	taskInfo.status = Running;
	taskInfo.time = getTimeMilli() - curr_proc()->startTimeStamp;
	memmove(&taskInfo.syscall_times, &curr_proc()->syscall_times, sizeof(curr_proc()->syscall_times));
	copyout(curr_proc()->pagetable, (uint64)info, (char *)&taskInfo, sizeof(TaskInfo));
	return 0;
}

extern char trap_page[];

void syscall()
{

	struct trapframe *trapframe = curr_proc()->trapframe;
	int id = trapframe->a7, ret;
	uint64 args[6] = { trapframe->a0, trapframe->a1, trapframe->a2,
			   trapframe->a3, trapframe->a4, trapframe->a5 };
	tracef("syscall %d args = [%x, %x, %x, %x, %x, %x]", id, args[0],
	       args[1], args[2], args[3], args[4], args[5]);
	/*
	* LAB1: you may need to update syscall counter for task info here
	*/
	curr_proc()->syscall_times[id]++;
	switch (id) {
	case SYS_write:
		ret = sys_write(args[0], args[1], args[2]);
		break;
	case SYS_exit:
		sys_exit(args[0]);
		// __builtin_unreachable();
	case SYS_sched_yield:
		ret = sys_sched_yield();
		break;
	case SYS_gettimeofday:
		ret = sys_gettimeofday((TimeVal *)args[0], args[1]);
		break;
	case SYS_task_info:
		ret = sys_task_info((TaskInfo*)args[0]);
		break;
	case SYS_mmap:
		ret = sys_mmap((uint64)args[0], (unsigned long long)args[1], (int)args[2], (int)args[3],
			       (int)args[4]);
		break;
	case SYS_munmap:
		ret = sys_munmap((uint64)args[0], (unsigned long long)args[1]);
		break ;
	/*
	* LAB1: you may need to add SYS_taskinfo case here
	*/
	default:
		ret = -1;
		errorf("unknown syscall %d", id);
	}
	trapframe->a0 = ret;
	tracef("syscall ret %d", ret);
}
