#ifndef FILE_H
#define FILE_H

#include "fs.h"
#include "proc.h"
#include "types.h"

#define PIPESIZE (512)
#define FILEPOOLSIZE (NPROC * FD_BUFFER_SIZE)

// in-memory copy of an inode,it can be used to quickly locate file entities on disk
struct inode {
	uint dev; // Device number
	uint inum; // Inode number
	int ref; // Reference count
	int valid; // inode has been read from disk?
	short type; // copy of disk inode
	short link;
	uint size; //这个inode所对应的文件的数据的大小
	// addrs[0],addrs[1],...,addrs[NDIRECT-1]直接索引该inode对应的文件的前NDIRECT个数据块的块编号
	// addrs[NDIRECT]为间接索引，其保存的数据块可以看作一个uint的数组，每个uint变量索引一个数据块
	uint addrs[NDIRECT + 1];
	// LAB4: You may need to add link count here
};

// Defines a file in memory that provides information about the current use of the file and the corresponding inode location
struct file {
	enum { FD_NONE = 0, FD_INODE, FD_STDIO } type;// FD_NONE表示该file结构体没绑定文件
	int ref; // reference count
	// 进程对该文件的读写权限
	char readable;
	char writable;
	//该文件对应的inode
	struct inode *ip; // FD_INODE
	//该文件的读写偏移量
	uint off;
};

//A few specific fd
enum {
	STDIN = 0,
	STDOUT = 1,
	STDERR = 2,
};
typedef struct {
	uint64 dev; // 文件所在磁盘驱动器号，不考虑
	uint64 ino; // inode 文件所在 inode 编号
	uint32 mode; // 文件类型
	uint32 nlink; // 硬链接数量，初始为1
	uint64 pad[7]; // 无需考虑，为了兼容性设计
} Stat;

extern struct file filepool[FILEPOOLSIZE];

void fileclose(struct file *);
struct file *filealloc();
int fileopen(char *, uint64);
int link(char * oldPath, char * newPath);
int unlink(char * newPath);
int fstat(int fd,uint64 stat);
uint64 inodewrite(struct file *, uint64, uint64);
uint64 inoderead(struct file *, uint64, uint64);
struct file *stdio_init(int);
int show_all_files();

#endif // FILE_H