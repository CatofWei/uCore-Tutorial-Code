#include "kalloc.h"
#include "defs.h"
#include "riscv.h"

extern char ekernel[];

struct linklist {
	struct linklist *next;
};

struct {
	struct linklist *freelist;
} kmem;
/**
 * @see kfree()
 * 可以认为每一个页，开头有一个linklist结构体（该结构体的地址等于页的起始地址），其中next字段指向下一页开头的linklist结构体
 * 也就是保存者下一页的起始地址，kem.freelist指向第一个页开头的linklist结构体，也就是保存着第一个页的起始地址。
 * 我们用链表来管理物理页，链表的每一个节点位于每个页的开头，其地址等于页起始地址。
 *
 * @param pa
 */
void freerange(void *pa_start, void *pa_end)
{
	char *p;
	// 对齐到下一页开始
	p = (char *)PGROUNDUP((uint64)pa_start);
	for (; p + PGSIZE <= (char *)pa_end; p += PGSIZE)
		kfree(p);
}
/**
 * 初始化物理内存,将[ekernel,PHYSTOP]物理内存组织成链表来理,此时还没有开启页表
 * 所以内核能正常访问整个物理内存
 */
void kinit()
{
	// ekernel为内核结束地址，PHYSTOP的物理内存的结束地址
	freerange(ekernel, (void *)PHYSTOP);
}

// Free the page of physical memory pointed at by v,
// which normally should have been returned by a
// call to kalloc().  (The exception is when
// initializing the allocator; see kinit above.)
void kfree(void *pa)
{
	struct linklist *l;
	if (((uint64)pa % PGSIZE) != 0 || (char *)pa < ekernel ||
	    (uint64)pa >= PHYSTOP)
		panic("kfree");
	// Fill with junk to catch dangling refs.
	memset(pa, 1, PGSIZE);
	l = (struct linklist *)pa;
	l->next = kmem.freelist;
	kmem.freelist = l;
}

// Allocate one 4096-byte page of physical memory.
// Returns a pointer that the kernel can use.
// Returns 0 if the memory cannot be allocated.
void *kalloc(void)
{
	struct linklist *l;
	l = kmem.freelist;
	if (l) {
		kmem.freelist = l->next;
		memset((char *)l, 5, PGSIZE); // fill with junk
	}
	return (void *)l;
}