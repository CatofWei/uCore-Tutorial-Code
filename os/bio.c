// Buffer cache.
//
// The buffer cache is a linked list of buf structures holding
// cached copies of disk block contents.  Caching disk blocks
// in memory reduces the number of disk reads and also provides
// a synchronization point for disk blocks used by multiple processes.
//
// Interface:
// * To get a buffer for a particular disk block, call bread.
// * After changing buffer data, call bwrite to write it to disk.
// * When done with the buffer, call brelse.
// * Do not use the buffer after calling brelse.
// * Only one process at a time can use a buffer,
//     so do not keep them longer than necessary.

#include "bio.h"
#include "defs.h"
#include "fs.h"
#include "riscv.h"
#include "types.h"
#include "virtio.h"

struct {
	struct buf buf[NBUF];
	struct buf head;
} bcache;

void binit()
{
	struct buf *b;
	// 将缓存磁盘块缓存结构组织成一个双向链表
	// Create linked list of buffers
	bcache.head.prev = &bcache.head;
	bcache.head.next = &bcache.head;
	for (b = bcache.buf; b < bcache.buf + NBUF; b++) {
		b->next = bcache.head.next;
		b->prev = &bcache.head;
		bcache.head.next->prev = b;
		bcache.head.next = b;
	}
}

// Look through buffer cache for block on device dev.
// If not found, allocate a buffer.
static struct buf *bget(uint dev, uint blockno)
{
	struct buf *b;
	// Is the block already cached?
	// 首先遍历磁盘块链表，查询该磁盘块是否已经缓存
	for (b = bcache.head.next; b != &bcache.head; b = b->next) {
		if (b->dev == dev && b->blockno == blockno) {
			// 递增该缓存的计数
			b->refcnt++;
			return b;
		}
	}
	// 如果查缓存失败，则按照LRU策略找到未被引用的缓存块，只有未被引用的缓存块才能淘汰，否则出错。
	// Not cached.
	// Recycle the least recently used (LRU) unused buffer.
	for (b = bcache.head.prev; b != &bcache.head; b = b->prev) {
		if (b->refcnt == 0) {
			b->dev = dev;
			b->blockno = blockno;
			b->valid = 0;
			b->refcnt = 1;
			return b;
		}
	}
	panic("bget: no buffers");
	return 0;
}

const int R = 0;
const int W = 1;

// Return a buf with the contents of the indicated block.
struct buf *bread(uint dev, uint blockno)
{
	struct buf *b;
	// 首先在全局的磁盘块缓存中查找
	b = bget(dev, blockno);
	if (!b->valid) {
		// 若该磁盘块缓存尚未读取磁盘数据，则调用virtto驱动程序读取磁盘磁盘放入缓存
		virtio_disk_rw(b, R);
		b->valid = 1;
	}
	return b;
}

// Write b's contents to disk.
void bwrite(struct buf *b)
{
	virtio_disk_rw(b, W);
}

// Release a buffer.
// Move to the head of the most-recently-used list.
// 释放对磁盘块缓存的引用，并且递减引用计数，若引用计数归零，则将磁盘块移动到链表头部
// bget和brelse必须成对出现，否则会出现泄漏
void brelse(struct buf *b)
{
	b->refcnt--;
	if (b->refcnt == 0) {
		// no one is waiting for it.
		b->next->prev = b->prev;
		b->prev->next = b->next;
		b->next = bcache.head.next;
		b->prev = &bcache.head;
		bcache.head.next->prev = b;
		bcache.head.next = b;
	}
}

void bpin(struct buf *b)
{
	b->refcnt++;
}

void bunpin(struct buf *b)
{
	b->refcnt--;
}
