#ifndef  __MY_MALLOC_H_
#define  __MY_MALLOC_H_

#ifdef  __cplusplus
    extern "C" {
#endif

#include <stdint.h>	
#include "fsmc.h"

//可以在此处修改管理的内存与管理内存的大小, 其实只有内部128KB与外扩1MB这两种选择
	
/* 第一块堆内存 Malloc 内部108KB SRAM */		
//#define  HEAP_START_ADDR  0x20005000           //堆起始地址(必须MALLOC_ALIGNMENT对齐)
//#define  HEAP_SIZE        0x1B000              //堆大小

/* 第一块堆内存 Malloc 内部92KB SRAM */
#define  HEAP_START_ADDR  0x20009000           //堆起始地址(必须MALLOC_ALIGNMENT对齐)
#define  HEAP_SIZE        0x17000              //堆大小		

/*
 * 使用Malloc管理内部128KB SRAM, 64KB的CCM用于栈和静态区, 这看似不错,
 * 其实有坑--DMA访问的内存不能在栈或静态区. 不了解CPU架构与程序编译,运行原理
 * 的程序员必采此坑, 而且采坑后也很难知道原因. 知道此坑的程序员还是可以用malloc
 * 管理128KB SRAM, 把64KB CCM用作程序的栈 静态区
 */

//第二块堆内存(CCM)
#define  HEAP2_START_ADDR  0x10000000           //堆起始地址(必须MALLOC_ALIGNMENT对齐)
#define  HEAP2_SIZE        0x10000              //堆大小


//可以在此处修改管理内存的对齐字节
#define  MALLOC_ALIGNMENT 16                   //每次malloc申请的内存字节数为16的整数倍(或者说申请内存的开始地址为16的整数倍)

typedef struct mem_block_s
{
	struct mem_block_s *prev;  //这2个指针构成双向链表
	struct mem_block_s *next;
	void *ptr;                 //内存块地址
	int  size;                 //内存块大小
	uint32_t state;            //内存块状态 "free"-未分配  "used"-已经分配
}mem_block_t;

typedef struct heap_mem_s
{
	uint32_t start_addr;  //开始地址
	uint32_t total_size;  //总大小
	uint32_t free_size;   //剩余空间
	uint32_t used_size;   //已用空间
	int free_blk_cnt;     //未用block计数
	int used_blk_cnt;     //已用block计数
}heap_mem_t;


void mem_init(void);
void *mem_malloc(uint32_t size);
void *mem_malloc_align(uint32_t size, uint32_t addr_align);
void *mem2_malloc(uint32_t size);
void mem_free(void *p);
uint32_t mem_used(uint32_t *used, uint32_t *free);	
void mem_blkinfo(void);

#ifdef  __cplusplus
}  
#endif

#endif










