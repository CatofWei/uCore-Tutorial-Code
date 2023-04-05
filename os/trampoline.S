	#
        # code to switch between user and kernel space.
        #
        # this code is mapped at the same virtual address
        # (TRAMPOLINE) in user and kernel space so that
        # it continues to work when it switches page tables.
	#
	# kernel.ld causes this to be aligned
        # to a page boundary.
        #
	.section trampsec
.globl trampoline
trampoline:
.align 4
.globl uservec
uservec:
	#
        # trap.c sets stvec to point here, so
        # traps from user space start here,
        # in supervisor mode, but with a
        # user page table.
        #
        # sscratch points to where the process's p->trapframe is
        # mapped into user space, at TRAPFRAME.
        #

	# swap a0 and sscratch
        # so that a0 is TRAPFRAME
        # 此时处于内核态，但使用的是用户页表，因为用户页表和内核页表对于跳板代码的映射保持一致，所以切换页表时指令能够正确地连续执行
        csrrw a0, sscratch, a0

        # save the user registers in TRAPFRAME
        # a0在tramframe在用户地址空间，页表指向的也是用户地址空间，所以下面代码能正常执行
        # ra和sp位于用户地址空间
        sd ra, 40(a0)
        sd sp, 48(a0)
        sd gp, 56(a0)
        sd tp, 64(a0)
        sd t0, 72(a0)
        sd t1, 80(a0)
        sd t2, 88(a0)
        sd s0, 96(a0)
        sd s1, 104(a0)
        sd a1, 120(a0)
        sd a2, 128(a0)
        sd a3, 136(a0)
        sd a4, 144(a0)
        sd a5, 152(a0)
        sd a6, 160(a0)
        sd a7, 168(a0)
        sd s2, 176(a0)
        sd s3, 184(a0)
        sd s4, 192(a0)
        sd s5, 200(a0)
        sd s6, 208(a0)
        sd s7, 216(a0)
        sd s8, 224(a0)
        sd s9, 232(a0)
        sd s10, 240(a0)
        sd s11, 248(a0)
        sd t3, 256(a0)
        sd t4, 264(a0)
        sd t5, 272(a0)
        sd t6, 280(a0)

        csrr t0, sscratch
        sd t0, 112(a0)
        csrr t1, sepc
        sd t1, 24(a0)
        # 加载位于内核地址空间的内核栈
        ld sp, 8(a0)
        ld tp, 32(a0)
        ld t0, 16(a0)
        ld t1, 0(a0)
        # 切换内核页表
        csrw satp, t1
        sfence.vma zero, zero
        # 跳到usertrap 在内核地址空间，此时已经切换到内核页表，所以没错
        jr t0

.globl userret
userret:
        # userret(TRAPFRAME, pagetable)
        # switch from kernel to user.
        # usertrapret() calls here.
        # a0: TRAPFRAME, in user page table.
        # a1: user page table, for satp.

        # switch to the user page table.
        # 内核为什么要将虚拟地址最高页映射到跳板代码？主要是为了切换到用户后，保持一致
        # 用户程序将虚拟地址最高页映射到跳板代码，而跳板代码是用户态和内核态切换的代码，因此必须切换页表
        # 那么如何在切换页表后，仍然能正常执行这段代码？那就是保证切换前后的页表将这段代码放在虚拟地址空间相同位置
        # 切换用户页表，但此时仍处于内核特权级
        csrw satp, a1
        # 刷新缓存
        sfence.vma zero, zero

        # put the saved user a0 in sscratch, so we
        # can swap it with our a0 (TRAPFRAME) in the last step.
        # 由于a0 = TRAMFRAE位于用户页表，因此切换后能正常访问
        ld t0, 112(a0)
        csrw sscratch, t0

        # restore all but a0 from TRAPFRAME
        ld ra, 40(a0)
        ld sp, 48(a0)
        ld gp, 56(a0)
        ld tp, 64(a0)
        ld t0, 72(a0)
        ld t1, 80(a0)
        ld t2, 88(a0)
        ld s0, 96(a0)
        ld s1, 104(a0)
        ld a1, 120(a0)
        ld a2, 128(a0)
        ld a3, 136(a0)
        ld a4, 144(a0)
        ld a5, 152(a0)
        ld a6, 160(a0)
        ld a7, 168(a0)
        ld s2, 176(a0)
        ld s3, 184(a0)
        ld s4, 192(a0)
        ld s5, 200(a0)
        ld s6, 208(a0)
        ld s7, 216(a0)
        ld s8, 224(a0)
        ld s9, 232(a0)
        ld s10, 240(a0)
        ld s11, 248(a0)
        ld t3, 256(a0)
        ld t4, 264(a0)
        ld t5, 272(a0)
        ld t6, 280(a0)

        # restore user a0, and save TRAPFRAME in sscratch
        # sscratch保存着traframe在用户空间的虚拟地址
        csrrw a0, sscratch, a0

        # return to user mode and user pc.
        # usertrapret() set up sstatus and sepc.
        # 返回，epc指向用户程序入口的虚拟地址，
        sret
