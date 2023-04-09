#include "trap.h"
#include "defs.h"
#include "loader.h"
#include "plic.h"
#include "syscall.h"
#include "timer.h"
#include "virtio.h"

extern char trampoline[], uservec[];
extern char userret[], kernelvec[];

void kerneltrap();

// set up to take exceptions and traps while in the kernel.
void set_usertrap()
{
	w_stvec(((uint64)TRAMPOLINE + (uservec - trampoline)) & ~0x3); // DIRECT
}

void set_kerneltrap()
{
	w_stvec((uint64)kernelvec & ~0x3); // DIRECT
}

// set up to take exceptions and traps while in the kernel.
void trap_init()
{
//	 intr_on();
	set_kerneltrap();
	// sie寄存器控制的是s特权级的中断
	//sstatus寄存器控制的是当处于s特权级时，cpu对各个中断的控制
	//也就是说sie属于s特权级的中断
	//sstatus属于s特权级
	//所以intr_on是用来开启当cpu处于s特权级时，对s特权级中断的响应
	//在前几章的实现中，我们都是没有开启s特权级的cpu对中断的响应，所以在内核态下不会发生时钟中断
	//但这章我们在读写磁盘时开启了外部中断，所以需要相应处理
	w_sie(r_sie() | SIE_SEIE | SIE_STIE | SIE_SSIE);
}

void unknown_trap()
{
	errorf("unknown trap: %p, stval = %p", r_scause(), r_stval());
	exit(-1);
}

void devintr(uint64 cause)
{
	int irq;
	switch (cause) {
	case SupervisorTimer:
		set_next_timer();
		// 时钟中断如果发生在内核态，不切换进程，原因分析在下面
		// 如果发生在用户态，照常处理
		if ((r_sstatus() & SSTATUS_SPP) == 0) {
			yield();
		} else{
			printf("ignore time interrupt from kernel\n");
		}
		break;
	case SupervisorExternal:
//		printf("external interrupt\n");
		irq = plic_claim();
		if (irq == UART0_IRQ) {
			// do nothing
		} else if (irq == VIRTIO0_IRQ) {
			virtio_disk_intr();
		} else if (irq) {
			infof("unexpected interrupt irq=%d\n", irq);
		}
		if (irq)
			plic_complete(irq);
		break;
	default:
		unknown_trap();
		break;
	}
}

//
// handle an interrupt, exception, or system call from user space.
// called from trampoline.S
//
void usertrap()
{
	set_kerneltrap();
	struct trapframe *trapframe = curr_proc()->trapframe;
	tracef("trap from user epc = %p", trapframe->epc);
	if ((r_sstatus() & SSTATUS_SPP) != 0)
		panic("usertrap: not from user mode");

	uint64 cause = r_scause();
	if (cause & (1ULL << 63)) {
		devintr(cause & 0xff);
	} else {
		switch (cause) {
		case UserEnvCall:
			trapframe->epc += 4;
			syscall();
			break;
		case StoreMisaligned:
		case StorePageFault:
		case InstructionMisaligned:
		case InstructionPageFault:
		case LoadMisaligned:
		case LoadPageFault:
			errorf("%d in application, bad addr = %p, bad instruction = %p, "
			       "core dumped.",
			       cause, r_stval(), trapframe->epc);
			exit(-2);
			break;
		case IllegalInstruction:
			errorf("IllegalInstruction in application, core dumped.");
			exit(-3);
			break;
		default:
			unknown_trap();
			break;
		}
	}
	usertrapret();
}

//
// return to user space
//
void usertrapret()
{
	set_usertrap();
	struct trapframe *trapframe = curr_proc()->trapframe;
	trapframe->kernel_satp = r_satp(); // kernel page table
	trapframe->kernel_sp =
		curr_proc()->kstack + KSTACK_SIZE; // process's kernel stack
	trapframe->kernel_trap = (uint64)usertrap;
	trapframe->kernel_hartid = r_tp(); // unuesd

	w_sepc(trapframe->epc);
	// set up the registers that trampoline.S's sret will use
	// to get to user space.

	// set S Previous Privilege mode to User.
	uint64 x = r_sstatus();
	x &= ~SSTATUS_SPP; // clear SPP to 0 for user mode
	x |= SSTATUS_SPIE; // enable interrupts in user mode
	w_sstatus(x);

	// tell trampoline.S the user page table to switch to.
	uint64 satp = MAKE_SATP(curr_proc()->pagetable);
	uint64 fn = TRAMPOLINE + (userret - trampoline);
	tracef("return to user @ %p", trapframe->epc);
	((void (*)(uint64, uint64))fn)(TRAPFRAME, satp);
}

void kerneltrap()
{
//	printf("kerneltrap\n");
	uint64 sepc = r_sepc();
	uint64 sstatus = r_sstatus();
	uint64 scause = r_scause();

	debugf("kernel tarp: epc = %p, cause = %d", sepc, scause);

	if ((sstatus & SSTATUS_SPP) == 0)
		panic("kerneltrap: not from supervisor mode");

	if (scause & (1ULL << 63)) {
		devintr(scause & 0xff);
	} else {
		errorf("invalid trap from kernel: %p, stval = %p sepc = %p\n",
		       scause, r_stval(), sepc);
		exit(-1);
	}
	// the yield() may have caused some traps to occur,
	// so restore trap registers for use by kernelvec.S's sepc instruction.
	w_sepc(sepc);
	w_sstatus(sstatus);
}
