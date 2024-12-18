#include "spi_flash_v4.h"
#include "spi_v3.h"
#include "libc.h"
#include "share.h"
#include "systick.h"
#include "my_printf.h"

//支持的SPI Flash列表
//manufacturer: 
//    0xEF-华邦电子(Winboad)
//    0x1C-宜扬科技(Eon Silicon)
//    0x20-武汉新芯
//    0xC8-兆易创新(GigaDevice)
//    0x68-博雅科技(BoyaMicro)
//    0x52-诺存微电子
const spi_flash_info_t spi_flash_tab[] = 
{
	{0xEF4018, 0x1000000, "W25Q128 128Mb"},
	{0xEF4017, 0x800000,  "W25Q64  64Mb"},
	{0xEF3015, 0x200000,  "W25Q16  16Mb"},
	{0x1C3015, 0x200000,  "EN25QH16 16Mb"},
	{0x207018, 0x1000000, "XM25QH128 128Mb"},
	{0x207017, 0x800000,  "XM25QH64 64Mb"},  /* 武汉新芯-NorFlash, 2.7-3.6V, 104MHz(x1,x2,x4)QPI */
	{0xC84018, 0x1000000, "GD25Q127 128Mb"},
	{0xC84017, 0x800000,  "GD25Q64  64Mb"},
	{0x684018, 0x1000000, "BY25Q128 128Mb"},
	{0x684017, 0x800000,  "BY25Q64  64Mb"},
	{0x522118, 0x1000000, "NM25Q128 128Mb"}, /* Up to 104MHz in clock frequency, Single Dual and Quad I/O */
	{0x522117, 0x800000,  "NM25Q64  64Mb"},
//	{0x522218, 0x1000000, "NM25Q128 128Mb"},
//	{0x522217, 0x800000,  "NM25Q64  64Mb"},
};

const spi_flash_info_t *spi_flash_info;

//=====================================================================================
//读取SPI Flash的: manufacturer, memory type, capacity
void sf_read_JEDEC_ID(uint8_t *buff, int len)
{
	FLASH_CS_L();
	spi_write_byte(FLASH_MOUNT_SPI, SF_ReadJEDECIDCmd);  //Read JEDEC ID Cmd(0x9F) 
	spi_read_buff(FLASH_MOUNT_SPI, buff, len);
	FLASH_CS_H();
}

//读状态寄存器SR1,寄存器默认值:0x00
//bit7  bit6  bit5  bit4  bit3  bit2  bit1  bit0
//SPR   RV    TB    BP2   BP1   BP0   WEL   BUSY
//SPR:默认0,状态寄存器保护位,配合WP使用
//TB,BP2,BP1,BP0:flash区域写保护设置
//WEL:Write Enable Lock
//BUSY: 1 busy, 0 free.
int sf_read_status_reg(void)
{
	int sta_reg;
	FLASH_CS_L();
	spi_write_byte(FLASH_MOUNT_SPI, SF_ReadStatusReg1Cmd); 
	sta_reg = spi_read_byte(FLASH_MOUNT_SPI);
	FLASH_CS_H();
	return sta_reg;
}

//写状态寄存器SR, 只有SPR, TB, BP2, BP1, BP0位可以写
void sf_write_status_reg(int reg_val)
{
	FLASH_CS_L();
	spi_write_byte(FLASH_MOUNT_SPI, SF_WriteStatusRegCmd); 
	spi_write_byte(FLASH_MOUNT_SPI, reg_val); 
	FLASH_CS_H();
}

//该函数把WEL位置一,这样才能进行
//Page Program, Sector Erase, Bloack Erase, Chip Erase
void sf_write_enable(void)
{
	FLASH_CS_L();
	spi_write_byte(FLASH_MOUNT_SPI, SF_WriteEnableCmd); 
	FLASH_CS_H();	
}

//该函数把WEL位清零,其实在进行完相应的操作后,WEL位会自动清零
void sf_write_disable(void)
{
	FLASH_CS_L();
	spi_write_byte(FLASH_MOUNT_SPI, SF_WriteDisableCmd); 
	FLASH_CS_H();	
}

//Page Program, Sector Erase, Bloack Erase, Chip Erase操作之后要忙等待
#define  sf_wait_busy()  while(0x01 == (sf_read_status_reg()&0x01))
//void sf_wait_busy(void)
//{
//	while(0x01 == (sf_read_status_reg()&0x01))
//	{	delay_us(200);	}
//}

//擦除整片, 擦之后进行忙检查(要耗费数十秒时间)
void sf_erase_chip(void)
{
	sf_write_enable();
	FLASH_CS_L();
	spi_write_byte(FLASH_MOUNT_SPI, SF_ChipEraseCmd);			
	FLASH_CS_H();
	sf_wait_busy();
}

//SPI Flash在进行擦除/写操作后, 如果不进行Busy Check, 后果:
//(1)无论读取擦除的扇区还是其他扇区, 都只能读取到0xFF
//(2)写操作一定会失败
//这是因为擦除与写操作都需要一定的时间
void sf_busy_check(void)
{
	sf_wait_busy();
}

//=====================================================================================
//SPI Flash 读操作
/*
 * @brief: SPI Flash数据读取函数
 */
int sf_read_chip(uint32_t start_addr, void *buff, int len)
{
	int ret;

	FLASH_CS_L();
	spi_write_byte(FLASH_MOUNT_SPI, SF_ReadDataCmd);            //发出读命令之后, 发地址信息
	spi_write_byte(FLASH_MOUNT_SPI, (uint8_t)(start_addr>>16));
	spi_write_byte(FLASH_MOUNT_SPI, (uint8_t)(start_addr>>8));
	spi_write_byte(FLASH_MOUNT_SPI, (uint8_t)(start_addr));
	
//	ret = spi_read_buff(FLASH_MOUNT_SPI, buff, len);
	if(FLASH_MOUNT_SPI == SPI1)
		ret = spi1_read_dma(buff, len);
	else if(FLASH_MOUNT_SPI == SPI3)
		ret = spi3_read_dma(buff, len);
	else
		ret = spi_read_buff(FLASH_MOUNT_SPI, buff, len);
	FLASH_CS_H();

	return ret;
}

/*
The Fast Read instruction is similar to the Read Data instruction except that
it can operate at the highest possible frequency of Fr. This is accomplished
by adding eight dummy clocks after the 24-bit address. The dummy clocks allow
the device's internal circuit additional time for setting up the initial address.
During the dummy clocks the data value on the DO pin is a don't care.
*/
int sf_fast_read_chip(uint32_t start_addr, void *buff, int len)
{
	int ret;

	FLASH_CS_L();
	spi_write_byte(FLASH_MOUNT_SPI, SF_ReadDataFastCmd);        //发出快速读命令之后, 发地址信息
	spi_write_byte(FLASH_MOUNT_SPI, (uint8_t)(start_addr>>16));
	spi_write_byte(FLASH_MOUNT_SPI, (uint8_t)(start_addr>>8));
	spi_write_byte(FLASH_MOUNT_SPI, (uint8_t)(start_addr));
	spi_read_byte(FLASH_MOUNT_SPI);                            //Adding eight dummy clocks after the 24-bit address

	ret = spi_read_buff(FLASH_MOUNT_SPI, buff, len);
	FLASH_CS_H();

	return ret;	
}

//=====================================================================================
//SPI Flash 写操作
/*
 * @brief: SPI Flash按页写函数
 */
int sf_page_program(uint32_t start_addr, void *buff, int len)
{
	if(len > SF_PAGE_SIZE)
		return -1;
	
	if(start_addr & (SF_PAGE_SIZE -1))  //没有按页地址对齐
		return -2;
	
	int ret;

	
	sf_write_enable();
	FLASH_CS_L();
	spi_write_byte(FLASH_MOUNT_SPI, SF_PageProgramCmd);        //发出快速读命令之后, 发地址信息
	spi_write_byte(FLASH_MOUNT_SPI, (uint8_t)(start_addr>>16));
	spi_write_byte(FLASH_MOUNT_SPI, (uint8_t)(start_addr>>8));
	spi_write_byte(FLASH_MOUNT_SPI, (uint8_t)(start_addr));

//	ret = spi_write_buff(FLASH_MOUNT_SPI, buff, len);
	if(FLASH_MOUNT_SPI == SPI1)
		ret = spi1_write_dma(buff, len, 1);
	else if(FLASH_MOUNT_SPI == SPI3)
		ret = spi3_write_dma(buff, len, 1);
	else
		ret = spi_write_buff(FLASH_MOUNT_SPI, buff, len);
	FLASH_CS_H();
	sf_wait_busy();            //页编程时间约为1ms

	return ret;	
}

//擦除扇区, 擦之后不进行忙检查
void sf_erase_sector_nocheck(uint32_t start_addr)
{
	sf_write_enable();
	FLASH_CS_L();
	spi_write_byte(FLASH_MOUNT_SPI, SF_SectorEraseCmd);
	spi_write_byte(FLASH_MOUNT_SPI, (uint8_t)(start_addr>>16));
	spi_write_byte(FLASH_MOUNT_SPI, (uint8_t)(start_addr>>8));
	spi_write_byte(FLASH_MOUNT_SPI, (uint8_t)(start_addr));				
	FLASH_CS_H();	
}

/*
 * @brief: SPI Flash写一个已经擦除了的扇区
 */
static int sf_write_erased_sector(uint32_t start_addr, void *buff, int len)
{
	if(len != SF_SECTOR_SIZE)
		return -1;
	
	if(start_addr & (SF_SECTOR_SIZE-1))  //没有按扇区地址对齐
		return -2;	
	
	int i, page_cnt = SF_SECTOR_SIZE/SF_PAGE_SIZE;
	uint32_t offset = 0;
	uint8_t *pdat = buff;
	for(i=0; i<page_cnt; i++)
	{
		sf_page_program(start_addr+offset, &pdat[offset], SF_PAGE_SIZE);
		offset += SF_PAGE_SIZE;
	}
	return len;
}


//==========================================================================================
//扇区数据块
typedef struct sf_block_s
{
	int dirty;       //数据块在丢弃前是否需要写回SPI Flash, 0-不写回  1-写回
	int erased;      //数据块所在的SPI Flash扇区擦除是否已经擦除, 0-未擦除 1-已经擦除
	uint32_t addr;   //数据块所在SPI Flash的扇区地址, 按SF_SECTOR_SIZE对齐
	uint8_t  buff[SF_SECTOR_SIZE];   //SPI Flash扇区数据块
}sf_block_t;

static sf_block_t sf_block;
static int sf_tick_cnt = 0;

//写策略: 先判断待写的数据是否在buff缓冲区中:
//            若待写的数据在buff缓存区, 则把数据写入buff缓存区.
//            若待写的数据不在buff缓存区, 则根据dirty位淘汰当前buff中的数据,
//            然后根据待写数据的地址从磁盘中读满buff, 最后把待写的数据放入buff缓存区.
//        往buff中放数据时:
//            若buff缓存区能放下本次要写的数据, 则直接把数据放入buff缓存区，并标记dirty位。
//            若buff缓冲区不能放下本次要写的数据, 放满buff后立即把buff写入磁盘, 
//            对于剩余的数据按如下方法处理:
//            剩余数据/sizeof(buff)部分先拷贝到buff, 然后把buff中的数据立即写入磁盘,
//            剩余数据%sizeof(buff)部分处理方法: 先读取这部分数据在磁盘对应地址处的
//            数据到buff, 然后把这部分数据写入buff, 并标记dirty位. 
//            若没有%部分,即只有/部分, 那么要清除dirty位. 
//读策略: 若待读取的数据全都不在buff缓存区, 则直接从磁盘中读取数据.
//        若待读取的数据部分或者全部在buff缓存区, 则根据数据的分布从磁盘或(和)buff缓存区中读取对应的数据.

int sf_write_disk(uint32_t start_addr, void *buff, int len)
{
	/* 要写的数据还没有加载到block缓冲区 */
	if(start_addr >= sf_block.addr+SF_SECTOR_SIZE || start_addr<sf_block.addr) {
		//step1: 淘汰旧数据块
		if(sf_block.dirty) { //数据脏,需要写回SPI Flash
			if(!sf_block.erased)
				sf_erase_sector_nocheck(sf_block.addr);
			sf_wait_busy();
			sf_write_erased_sector(sf_block.addr, sf_block.buff, SF_SECTOR_SIZE);
		}
		
		//step2: 加载新的数据块
		sf_block.addr = start_addr & (~(uint32_t)(SF_SECTOR_SIZE-1));
		sf_wait_busy();
		sf_read_chip(sf_block.addr, sf_block.buff, SF_SECTOR_SIZE);
		sf_block.dirty = 0;
		sf_block.erased = 0;
	}

	/* 要写的数据至少部分已经加载到了block缓冲区 */
	//step1: 把部分数据写入block缓冲区
	uint32_t offset = start_addr & (SF_SECTOR_SIZE-1);
	uint32_t len1 = len>(SF_SECTOR_SIZE-offset)?(SF_SECTOR_SIZE-offset):len; //写入block缓冲区的数据长度
	uint32_t len2 = len-len1;                                                //剩余数据的长度
	u_memcpy(&sf_block.buff[offset], buff, len1);
	sf_block.dirty = 1;
	if(!sf_block.erased) {
		sf_erase_sector_nocheck(sf_block.addr);
		sf_block.erased = 1;
	}
	if(len2 == 0) {
		sf_tick_cnt = 20;  //20*10=200ms后自动把脏了的数据块写回SPI Flash
		return len;
	}

	//step2: (len2 > 0)处理写入block缓冲区后还剩余的数据
	sf_tick_cnt = 0;       //立即把脏数据写回SPI Flash
	if(!sf_block.erased)
		sf_erase_sector_nocheck(sf_block.addr);
	sf_wait_busy();
	sf_write_erased_sector(sf_block.addr, sf_block.buff, SF_SECTOR_SIZE);
	
	                      //数据量太大只能采用堵塞的方式连续擦写了
	int i, idx=0, blk_cnt=len2/SF_SECTOR_SIZE;
	uint8_t *pdat = buff;
	idx += len1;
	sf_block.addr += SF_SECTOR_SIZE;
	for(i=0; i<blk_cnt; i++) {     //数据量太大只能采用堵塞的方式连续擦写了
		sf_erase_sector_nocheck(sf_block.addr);
		sf_wait_busy();
		u_memcpy(sf_block.buff, &pdat[idx], SF_SECTOR_SIZE);
		sf_write_erased_sector(sf_block.addr, sf_block.buff, SF_SECTOR_SIZE);
		idx += SF_SECTOR_SIZE;
		sf_block.addr += SF_SECTOR_SIZE;
	}

	if(len2%SF_SECTOR_SIZE) {     //剩余不足一个整块的部分
		sf_read_chip(sf_block.addr, sf_block.buff, SF_SECTOR_SIZE); //借助block缓冲区来写不足一个扇区的数据
		u_memcpy(sf_block.buff, &pdat[idx], len-idx);

		sf_erase_sector_nocheck(sf_block.addr);
		sf_block.dirty = 1;
		sf_block.erased = 1;
		sf_tick_cnt = 20;
	}
	else {                       //执行了上面的for循环
		sf_block.addr -= SF_SECTOR_SIZE;
		sf_block.dirty = 0;
		sf_block.erased = 0;
		sf_tick_cnt = 0;
	}

	return len;
}

int sf_read_disk(uint32_t start_addr, void *buff, int len)
{
	/* 待读取的数据全不在缓冲区(出现的频率高, 次数多), 直接从flash中读取所需数据即可 */
	if(start_addr>=(sf_block.addr+SF_SECTOR_SIZE) || (start_addr+len)<=sf_block.addr)
	{
		sf_wait_busy();       //防止出现一个扇区刚擦完芯片处于Busy状态而出现读取失败
		return sf_read_chip(start_addr, buff, len);
	}
	
	/* 待读取的数据部分或者全部在缓冲区 */
	uint32_t flash_len, cache_len, flash_len2;
	uint8_t  *pdat = buff;
	int ret;
	
	if(start_addr < sf_block.addr)
	{
		flash_len = sf_block.addr - start_addr;
		sf_wait_busy(); 
		sf_read_chip(start_addr, pdat, flash_len);
		pdat += flash_len;
			
		cache_len = start_addr + len - sf_block.addr;
		u_memcpy(pdat, sf_block.buff, cache_len);
		pdat += cache_len;
		
		if(start_addr+len <= sf_block.addr+SF_SECTOR_SIZE) {
			//读取的数据分为2部分: flash  缓冲区
			ret = flash_len + cache_len;
			return ret;
		}
		else {
			//读取的数据分为3部分: flash  缓冲区 flash
			flash_len2 = (start_addr + len) - (sf_block.addr + SF_SECTOR_SIZE);
			sf_wait_busy(); 
			sf_read_chip(sf_block.addr + SF_SECTOR_SIZE, pdat, flash_len2);
			
			ret = flash_len + cache_len + flash_len2;
			return ret;
		}
	}
	else
	{
		uint32_t index = start_addr - sf_block.addr;

		cache_len = (sf_block.addr + SF_SECTOR_SIZE) - start_addr;
		if(len < cache_len)
			cache_len = len;
		u_memcpy(pdat, &sf_block.buff[index], cache_len);
		pdat += cache_len;
		
		if(start_addr+len <= sf_block.addr+SF_SECTOR_SIZE) {
			//读取的数据就1部分:  缓冲区
			ret = cache_len;
			return ret;
		}
		else {
			//读取的数据分为2部分: 缓冲区 flash
			flash_len2 = (start_addr + len) - (sf_block.addr + SF_SECTOR_SIZE);
			sf_wait_busy(); 
			sf_read_chip(sf_block.addr + SF_SECTOR_SIZE, pdat, flash_len2);		
			
			ret = cache_len + flash_len2;
			return ret;
		}
	}
}

void sf_10ms_tick(void)
{
	if(sf_tick_cnt > 0)
	{
		sf_tick_cnt--;
		if(sf_tick_cnt == 0)
		{
			if(!sf_block.dirty)  //数据并不脏, 没必要写回
				return;
			
			if(0x01 == (sf_read_status_reg()&0x01)) {  //SPI Flash Busy
				sf_tick_cnt = 1;
				return;
			}
			                    //把脏了的数据块写回已经擦除了的SPI Flash
			sf_write_erased_sector(sf_block.addr, sf_block.buff, SF_SECTOR_SIZE);
			sf_block.dirty = 0;
			sf_block.erased = 0;
		}
	}
}

int sf_probe(void)
{
	int i;
	uint8_t buff[4];
	uint32_t jedec_id;
	
	sf_read_JEDEC_ID(buff, 3);
	jedec_id = (uint32_t)buff[0]<<16 | (uint32_t)buff[1]<<8 | buff[2];
	for(i=0; i<ARRAY_SIZE(spi_flash_tab); i++)
	{
		if((jedec_id&0xFF00FF) == (spi_flash_tab[i].jedec_id&0xFF00FF)) {
			spi_flash_info = &spi_flash_tab[i];
			my_printf("\n\rSPI Flash: %s\n\r", spi_flash_tab[i].name);
			sf_block.addr = 0;
			sf_read_chip(sf_block.addr, sf_block.buff, SF_SECTOR_SIZE);
			sf_block.dirty = 0;
			sf_block.erased = 0;			
			return 0;
		}
	}
	
	my_printf("SPI Flash detect fail, jedec_id = 0x%06X\n\r", jedec_id);
	return -1;
}

/*
 * @brief: 返回SPI Flash的容量(以字节为单位)
 */
uint32_t sf_get_capacity(void)
{
	if( spi_flash_info )
		return spi_flash_info->capacity;
	return 0;
}



