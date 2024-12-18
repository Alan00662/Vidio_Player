#include "my_malloc.h"
#include "my_printf.h"
#include "libc.h"


/* 节点的大小需为MALLOC_ALIGNMENT的整数倍--NODE_SIZE编译时确定 */
#define  NODE_SIZE  ((sizeof(mem_block_t)+MALLOC_ALIGNMENT-1)&(~(MALLOC_ALIGNMENT-1)))

/* 存储块的状态标记 */
#define  MEM_BLK_USED  ('u'<<24|'s'<<16|'e'<<8|'d')
#define  MEM_BLK_FREE  ('f'<<24|'r'<<16|'e'<<8|'e')


static heap_mem_t heap_mem;
static heap_mem_t heap_mem2;

/*
 * 内存管理初始化
 */
void mem_init(void)
{
	mem_block_t *pblk;

	//===================第一块内存=========================
	u_memset((void *)HEAP_START_ADDR, 0x00, HEAP_SIZE);

	heap_mem.start_addr = (uint32_t)HEAP_START_ADDR;
	heap_mem.total_size = HEAP_SIZE;

	pblk = (mem_block_t *)heap_mem.start_addr;
	pblk->ptr  = (void *)(heap_mem.start_addr + NODE_SIZE);
	pblk->size = heap_mem.total_size - NODE_SIZE;
	pblk->state = MEM_BLK_FREE;	
	pblk->prev  = NULL;
	pblk->next  = NULL;

	heap_mem.free_size = HEAP_SIZE;
	heap_mem.used_size = 0;
	heap_mem.free_blk_cnt = 1;
	heap_mem.used_blk_cnt = 0;
	
	//===================第二块内存=========================
	u_memset((void *)HEAP2_START_ADDR, 0x00, HEAP2_SIZE);
	
	heap_mem2.start_addr = (uint32_t)HEAP2_START_ADDR;
	heap_mem2.total_size = HEAP2_SIZE;

	pblk = (mem_block_t *)heap_mem2.start_addr;
	pblk->ptr  = (void *)(heap_mem2.start_addr + NODE_SIZE);
	pblk->size = heap_mem2.total_size - NODE_SIZE;
	pblk->state = MEM_BLK_FREE;	
	pblk->prev  = NULL;
	pblk->next  = NULL;

	heap_mem2.free_size = HEAP2_SIZE;
	heap_mem2.used_size = 0;
	heap_mem2.free_blk_cnt = 1;
	heap_mem2.used_blk_cnt = 0;
}

/*
 * 采用首次使用算法分配内存
 * 缺陷:一旦malloc的内存越界, 将破坏链表, 进而导致系统崩溃
 */
void *mem_malloc(uint32_t size)
{
//	my_printf("malloc: %d ", size);
	mem_block_t *next_blk, *cur_blk;

	cur_blk = (mem_block_t *)heap_mem.start_addr;
	size = (size+MALLOC_ALIGNMENT-1)&(~(MALLOC_ALIGNMENT-1));   //申请内存大小向MALLOC_ALIGNMENT对齐
	while(cur_blk != NULL)   //遍历链表
	{
		//找到了合适的内存块
		if(cur_blk->state==MEM_BLK_FREE && cur_blk->size>=size+NODE_SIZE)
		{
			next_blk = (mem_block_t *)((uint32_t)cur_blk->ptr + size);
			next_blk->ptr   = (void *)((uint32_t)next_blk + NODE_SIZE);
			next_blk->size  = cur_blk->size - size - NODE_SIZE;
			next_blk->state = MEM_BLK_FREE;
			next_blk->prev  = cur_blk;
			next_blk->next  = cur_blk->next;
			if(cur_blk->next != NULL)
				cur_blk->next->prev = next_blk;

			cur_blk->size  = size;
			cur_blk->state = MEM_BLK_USED;
			cur_blk->next  = next_blk;
			heap_mem.used_blk_cnt++;
			heap_mem.used_size += cur_blk->size+NODE_SIZE;
			heap_mem.free_size -= cur_blk->size+NODE_SIZE;
//			my_printf("addr: %p, size: %d\n\r", cur_blk->ptr, size);
			return cur_blk->ptr;
		}
		cur_blk = cur_blk->next;
	}
//	my_printf("malloc fail\n\r");
	return NULL;
}

/*
 * 申请内存的开始地址能被addr_align整除
 */
void *mem_malloc_align(uint32_t size, uint32_t addr_align)
{
//	my_printf("malloc_align: %d\n\r", size);
	if(addr_align & (addr_align-1))   //对齐数必须为2^n
		return NULL;
	
	if(addr_align <= MALLOC_ALIGNMENT)
		return mem_malloc(size);
	
	uint32_t deta, need_size, free_size;
	mem_block_t *use_blk, *free_blk, *cur_blk;
	cur_blk = (mem_block_t *)heap_mem.start_addr;
	size = (size+MALLOC_ALIGNMENT-1)&(~(MALLOC_ALIGNMENT-1));   //申请内存大小向MALLOC_ALIGNMENT对齐

	while(cur_blk != NULL)   //遍历链表, 看是否有符合要求的内存
	{
		deta = (-((uint32_t)cur_blk->ptr)) % addr_align;    //与对齐数之间的距离
		
		if(cur_blk->state==MEM_BLK_FREE && cur_blk->size>=size+NODE_SIZE && deta==0)
		{	//找到了合适的内存块 情形1
			free_blk = (mem_block_t *)((uint32_t)cur_blk->ptr + size);
			free_blk->ptr   = (void *)((uint32_t)free_blk + NODE_SIZE);
			free_blk->size  = cur_blk->size - size - NODE_SIZE;
			free_blk->state = MEM_BLK_FREE;
			free_blk->prev  = cur_blk;
			free_blk->next  = cur_blk->next;
			if(cur_blk->next != NULL)
				cur_blk->next->prev = free_blk;

			cur_blk->size  = size;
			cur_blk->state = MEM_BLK_USED;
			cur_blk->next  = free_blk;
			heap_mem.used_blk_cnt++;
			heap_mem.used_size += cur_blk->size+NODE_SIZE;
			heap_mem.free_size -= cur_blk->size+NODE_SIZE;
//			my_printf("addr1: %p, size: %d\n\r", cur_blk->ptr, size);
			return cur_blk->ptr;
		}
		
		if(deta > NODE_SIZE) {
			free_size = deta - NODE_SIZE;
		} else {
			free_size = deta + addr_align - NODE_SIZE;
		}
		need_size = free_size + NODE_SIZE + size;

		if(cur_blk->state==MEM_BLK_FREE && cur_blk->size>=need_size+NODE_SIZE && deta!=0)
		{	//找到了合适的内存块 情形2(需增加free块来实现)
//			my_printf("free_size = %u\n\r", free_size);   //必为MALLOC_ALIGNMENT的整数倍, 否则就出错  debug
		
			//use块
			use_blk = (mem_block_t *)((uint32_t)cur_blk->ptr + free_size);
			use_blk->ptr   = (void *)((uint32_t)use_blk + NODE_SIZE);
			use_blk->size  = size;
			use_blk->state = MEM_BLK_USED;
			use_blk->prev  = cur_blk;
			use_blk->next  = free_blk = (mem_block_t *)((uint32_t)cur_blk->ptr + need_size);
			
			//free块
			free_blk->ptr   = (void *)((uint32_t)free_blk + NODE_SIZE);
			free_blk->size  = cur_blk->size - need_size - NODE_SIZE;
			free_blk->state = MEM_BLK_FREE;
			free_blk->prev  = use_blk;
			free_blk->next  = cur_blk->next;
			if(cur_blk->next != NULL)
				cur_blk->next->prev = free_blk;
			
			cur_blk->size  = free_size;
			cur_blk->state = MEM_BLK_FREE;
			cur_blk->next  = use_blk;
		
			heap_mem.used_blk_cnt++;      //1个无用的块, 拆成了1个使用的块, 2个未使用的块
			heap_mem.free_blk_cnt++;
			heap_mem.used_size += use_blk->size+NODE_SIZE;
			heap_mem.free_size -= use_blk->size+NODE_SIZE;
//			my_printf("addr2: %p, size: %d\n\r", use_blk->ptr, size);
			return use_blk->ptr;
		}

		cur_blk = cur_blk->next;
	}	
//	my_printf("malloc_align fail\n\r");
	return NULL;
}

/*
 * 在第二块堆上分配内存, 同样适用mem_free释放
 */
void *mem2_malloc(uint32_t size)
{
//	my_printf("malloc: %d ", size);
	mem_block_t *next_blk, *cur_blk;

	cur_blk = (mem_block_t *)heap_mem2.start_addr;
	size = (size+MALLOC_ALIGNMENT-1)&(~(MALLOC_ALIGNMENT-1));   //申请内存大小向MALLOC_ALIGNMENT对齐
	while(cur_blk != NULL)   //遍历链表
	{
		//找到了合适的内存块
		if(cur_blk->state==MEM_BLK_FREE && cur_blk->size>=size+NODE_SIZE)
		{
			next_blk = (mem_block_t *)((uint32_t)cur_blk->ptr + size);
			next_blk->ptr   = (void *)((uint32_t)next_blk + NODE_SIZE);
			next_blk->size  = cur_blk->size - size - NODE_SIZE;
			next_blk->state = MEM_BLK_FREE;
			next_blk->prev  = cur_blk;
			next_blk->next  = cur_blk->next;
			if(cur_blk->next != NULL)
				cur_blk->next->prev = next_blk;

			cur_blk->size  = size;
			cur_blk->state = MEM_BLK_USED;
			cur_blk->next  = next_blk;
			heap_mem2.used_blk_cnt++;
			heap_mem2.used_size += cur_blk->size+NODE_SIZE;
			heap_mem2.free_size -= cur_blk->size+NODE_SIZE;
//			my_printf("addr: %p, size: %d\n\r", cur_blk->ptr, size);
			return cur_blk->ptr;
		}
		cur_blk = cur_blk->next;
	}
//	my_printf("malloc fail\n\r");
	return NULL;
}


/*
 * 当前块与紧邻的下一块合并为一块
 */
static void mem_blk_merge_next(mem_block_t *cur_blk)
{
	mem_block_t *next_blk;
	next_blk = cur_blk->next;
	cur_blk->size += next_blk->size + NODE_SIZE;
	cur_blk->next  = next_blk->next;
	if(next_blk->next != NULL)
		next_blk->next->prev = cur_blk;
	u_memset(next_blk, 0x00, NODE_SIZE);
}

/*
 * 释放内存,注意释放后空闲块的合并
 */
void mem_free(void *p)
{
	mem_block_t *cur_blk;
	heap_mem_t *heap_mem_ptr;

	//首先要检查所释放的内存是否为堆内存
	if((uint32_t)p>=HEAP_START_ADDR && (uint32_t)p<HEAP_START_ADDR+HEAP_SIZE)
		heap_mem_ptr = &heap_mem;
	else if((uint32_t)p>=HEAP2_START_ADDR && (uint32_t)p<HEAP2_START_ADDR+HEAP2_SIZE)
		heap_mem_ptr = &heap_mem2;
	else
		return;

	cur_blk = (mem_block_t *)((char *)p - NODE_SIZE);
	if(cur_blk->state == MEM_BLK_USED)     //防止反复释放同一块内存
	{
//		my_printf("free cur_blk\n\r");
		cur_blk->state = MEM_BLK_FREE;
		u_memset(cur_blk->ptr, 0x00, cur_blk->size);
		heap_mem_ptr->free_blk_cnt++;
		heap_mem_ptr->used_blk_cnt--;
		heap_mem_ptr->free_size += cur_blk->size+NODE_SIZE;  //回收内存
		heap_mem_ptr->used_size -= cur_blk->size+NODE_SIZE;

		if(cur_blk->next!=NULL && cur_blk->next->state==MEM_BLK_FREE)   //合并紧邻后块
		{
//			my_printf("free next_blk\n\r");
			mem_blk_merge_next(cur_blk);
			heap_mem_ptr->free_blk_cnt--;
		}
		if(cur_blk->prev!=NULL && cur_blk->prev->state==MEM_BLK_FREE)   //合并紧邻前块
		{
//			my_printf("free prev_blk\n\r");
			mem_blk_merge_next(cur_blk->prev);
			heap_mem_ptr->free_blk_cnt--;
		}
//		my_printf("\n\r");
	}
}


/*
 * 获取内存的已用大小与空闲大小
 */
uint32_t mem_used(uint32_t *used, uint32_t *free)
{
	*used = heap_mem.used_size + heap_mem2.used_size;
	*free = heap_mem.free_size + heap_mem2.free_size;
	return (*used + *free);
}

/*
 * 遍历内存块, 显示相关信息
 */
static void _mem_blkinfo(heap_mem_t *heap_mem_ptr)
{
	int used_blkcnt = 0, free_blkcnt = 0;
	const char *state = NULL;
	mem_block_t  *cur_blk = (mem_block_t *)heap_mem_ptr->start_addr;

	my_printf("%-6s %8s %12s %12s %12s %12s\n\r", "state", "size", "blk_dat_addr", "blk_addr", "blk_pre_addr", "blk_nxt_addr");
	while(cur_blk != NULL)
	{
		if(cur_blk->state == MEM_BLK_FREE)
		{
			free_blkcnt++;
			state = "free";
		}
		else
		{
			used_blkcnt++;
			state = "used";
		}
		my_printf("%-6s %8d   0x%08X   0x%08X   0x%08X   0x%08X\n\r", \
		state, cur_blk->size, (uint32_t)cur_blk->ptr, (uint32_t)cur_blk, (uint32_t)cur_blk->prev, (uint32_t)cur_blk->next);

		cur_blk = cur_blk->next;
	}
	my_printf("\n");
	my_printf("total block count: %d\n\r", free_blkcnt+used_blkcnt);
	my_printf("free block count: %d\n\r", free_blkcnt);
	my_printf("used block count: %d\n\r", used_blkcnt);
		
	if(free_blkcnt!=heap_mem_ptr->free_blk_cnt || used_blkcnt!=heap_mem_ptr->used_blk_cnt)
	{
		my_printf("mem error!!!\n\r");
		return;
	}
	
//	my_printf("total memory size: %d B\n\r", heap_mem.used_size + heap_mem.unused_size);
//	my_printf("free memory size: %d B\n\r",  heap_mem.unused_size);
//	my_printf("used memory size: %d B\n\r",  heap_mem.used_size);
	
	my_printf("total memory size: %d.%d KB\n\r", (heap_mem_ptr->used_size + heap_mem_ptr->free_size)>>10, \
	                                             ((heap_mem_ptr->used_size + heap_mem_ptr->free_size)&0x3FF)*100/1024);
	my_printf("free memory size: %d.%d KB\n\r",  heap_mem_ptr->free_size>>10, (heap_mem_ptr->free_size&0x3FF)*100/1024);
	my_printf("used memory size: %d.%d KB\n\r",  heap_mem_ptr->used_size>>10, (heap_mem_ptr->used_size&0x3FF)*100/1024);	
}

void mem_blkinfo(void)
{
	my_printf("memory1 block info:\n\n\r");
	_mem_blkinfo(&heap_mem);
	
	my_printf("\n\n");
	my_printf("memory2 block info:\n\n\r");
	_mem_blkinfo(&heap_mem2);
}




