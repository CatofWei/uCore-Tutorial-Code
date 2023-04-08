#ifndef BUF_H
#define BUF_H

#include "fs.h"
#include "types.h"
// 磁盘块缓存结构体
struct buf {
	int valid; // has data been read from disk?
	int disk; // does disk "own" buf?
	uint dev; // 磁盘块所在磁盘设备编号
	uint blockno;// 磁盘块编号
	uint refcnt;
	struct buf *prev; // LRU cache list
	struct buf *next;
	// 磁盘块内容
	uchar data[BSIZE];
};

void binit(void);
struct buf *bread(uint, uint);
void brelse(struct buf *);
void bwrite(struct buf *);
void bpin(struct buf *);
void bunpin(struct buf *);

#endif // BUF_H
