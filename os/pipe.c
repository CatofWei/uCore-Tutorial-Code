#include "defs.h"
#include "proc.h"
#include "riscv.h"

int pipealloc(struct file *f0, struct file *f1)
{
	struct pipe *pi;
	pi = 0;
	if ((pi = (struct pipe *)kalloc()) == 0)
		goto bad;
	pi->readopen = 1;
	pi->writeopen = 1;
	pi->nwrite = 0;
	pi->nread = 0;
	f0->type = FD_PIPE;
	f0->readable = 1;
	f0->writable = 0;
	f0->pipe = pi;
	f1->type = FD_PIPE;
	f1->readable = 0;
	f1->writable = 1;
	f1->pipe = pi;
	return 0;
bad:
	if (pi)
		kfree((char *)pi);
	return -1;
}

void pipeclose(struct pipe *pi, int writable)
{
	if (writable) {
		pi->writeopen = 0;
	} else {
		pi->readopen = 0;
	}
	if (pi->readopen == 0 && pi->writeopen == 0) {
		kfree((char *)pi);
	}
}

int pipewrite(struct pipe *pi, uint64 addr, int n)
{
	// w 记录已经写的字节数
	int w = 0;
	uint64 size;
	struct proc *p = curr_proc();
	if (n <= 0) {
		panic("invalid read num");
	}
	while (w < n) {
		if (pi->readopen == 0) {
			return -1;
		}
		if (pi->nwrite == pi->nread + PIPESIZE) { // DOC: pipewrite-full
			// pipe write 端已满，阻塞
			yield();
		} else {
			// 一次读的 size 为 min(用户buffer剩余，pipe 剩余写容量，pipe 剩余线性容量)
			size = MIN(MIN(n - w,
				       pi->nread + PIPESIZE - pi->nwrite),
				   PIPESIZE - (pi->nwrite % PIPESIZE));
			if (copyin(p->pagetable,
				   &pi->data[pi->nwrite % PIPESIZE], addr + w,
				   size) < 0) {
				panic("copyin");
			}
			pi->nwrite += size;
			w += size;
		}
	}
	return w;
}

int piperead(struct pipe *pi, uint64 addr, int n)
{
	// r 记录已经写的字节数
	int r = 0;
	uint64 size = -1;
	struct proc *p = curr_proc();
	if (n <= 0) {
		panic("invalid read num");
	}
	// 若 pipe 可读内容为空，阻塞或者报错
	while (pi->nread == pi->nwrite) {
		if (pi->writeopen)
			yield();
		else
			return -1;
	}
	while (r < n && size != 0) { // DOC: piperead-copy
		if (pi->nread == pi->nwrite)
			break;
		// 一次读的 size 为：min(用户还剩读数量，可读内容，pipe剩余线性容量)
		size = MIN(MIN(n - r, pi->nwrite - pi->nread),
			   PIPESIZE - (pi->nread % PIPESIZE));
		if (copyout(p->pagetable, addr + r,
			    &pi->data[pi->nread % PIPESIZE], size) < 0) {
			panic("copyout");
		}
		pi->nread += size;
		r += size;
	}
	return r;
}
