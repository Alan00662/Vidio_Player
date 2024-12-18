#ifndef  __SPI_FLASH_V4_H_
#define  __SPI_FLASH_V4_H_

#include <stdint.h>

/*
 * W25Q128支持: Standard SPI, Dual SPI, Quad SPI这3中SPI模式, 这里选用标准模式
 * Standard SPI: 
 *     SPI bus operation Mode 0 (0, 0) and 3 (1, 1) are supported.
 *     Mode0: 空闲时CLK为低电平, 在第1个时钟边沿锁存数据
 *     Mode3: 空闲时CLK为高电平, 在第2个时钟边沿锁存数据
 * 读写操作:
 *     容量: 128Mb=16MB
 *     扇区Sector是擦除的最小单位-4KB, 每个扇区Sector又分为16页Page, 每页256B,
 *     页Page编程时(写操作),地址会自动递增,当一页编程结束时,芯片不会自动换页,
 *     必须重新发送页地址, 若接着编程会覆盖原先的数据(或者不能编程,总之会导致编程不正常).
 *     读数据时,地址也会自动递增,只需发送一次地址, 就可以连续读出整个芯片的数据.
 * 页编程时间: 0.7ms
 * 扇区擦除时间: 120ms
 * 整片擦除时间: 25s
 * 工作电压: 2.7-3.6V
 * 时钟频率: 104MHz(Standard/Dual/Quad SPI)
 */
 
/*
 * W25Q64:
 * 制造商: 0xEF-华邦电子(Winboad)
 * 工作电压: 2.7-3.6V
 * 容量: 64Mb=8MB 扇区大小:4KB 页大小:256B
 * 时钟频率: 80MHz(Standard/Dual/Quad SPI)
 * 页编程时间: 0.7ms
 * 扇区擦除时间: 120ms
 * 整片擦除时间: 15s
 */

/*
 * EN25QH16:
 * 制造商: 0x1C-宜扬科技(Eon Silicon)
 * 工作电压: 2.7-3.6V
 * 容量: 16Mb=2MB 扇区大小:4KB 页大小:256B
 * 时钟频率: 104MHz(Standard) 80MHz(Dual/Quad SPI)
 * 页编程时间: 1.3ms
 * 扇区擦除时间: 60ms
 * 整片擦除时间: 12s
 */

/*
 * XM25QH64:
 * 制造商: 0x20-武汉新芯
 * 工作电压: 2.7-3.6V
 * 容量: 64Mb=8MB 扇区大小:4KB 页大小:256B
 * 时钟频率: 104MHz(Standard/Dual/Quad SPI)
 * 页编程时间: 0.5ms
 * 扇区擦除时间: 40ms
 * 整片擦除时间: 32s
 */


#define  SF_PAGE_SIZE   256
#define  SF_SECTOR_SIZE 4096


//用C语言中的位域来描述JEDEC ID
//typedef struct jedec_id_s
//{
//	uint32_t manufacturer:8;  //制造商
//	uint32_t memory_type:8;   //存储类型
//	uint32_t capacity:8;      //容量描述
//	uint32_t reverse:8;
//}jedec_id_t;

//SPI Flash的相关属性信息
typedef struct spi_flash_info_s
{ 
	uint32_t jedec_id;       //JEDEC ID
	uint32_t capacity;       //Flash容量-Byte
	char *name;	             //Flash名字
}spi_flash_info_t;


//SPI Flash Operate Instructions at Standard SPI interface
#define  SF_ReadJEDECIDCmd    0x9F

#define  SF_ReadStatusReg1Cmd 0x05
#define  SF_ReadStatusReg2Cmd 0x35
#define  SF_WriteStatusRegCmd 0x01

#define  SF_WriteEnableCmd   0x06
#define  SF_WriteDisableCmd  0x04

#define  SF_ReadDataCmd      0x03
#define  SF_ReadDataFastCmd  0x0B
#define  SF_PageProgramCmd   0x02

#define  SF_SectorEraseCmd   0x20
#define  SF_BlockEraseCmd    0xD8
#define  SF_ChipEraseCmd     0xC7

#define  SF_PowerDownCmd       0xB9
#define  SF_RelasePowerDownCmd 0xAB


//擦除一个扇区后, 如果不进行Busy Check, 无论读取本扇区还是其他扇区, 都只能读取到0xFF

void sf_read_JEDEC_ID(uint8_t *buff, int len);
int  sf_read_status_reg(void);
void sf_write_status_reg(int reg_val);

void sf_write_enable(void);
void sf_write_disable(void);
void sf_erase_chip(void);
void sf_erase_sector_nocheck(uint32_t start_addr);
int sf_page_program(uint32_t start_addr, void *buff, int len);
int sf_read_chip(uint32_t start_addr, void *buff, int len);
void sf_busy_check(void);


int sf_read_disk(uint32_t start_addr, void *buff, int len);
int sf_write_disk(uint32_t start_addr, void *buff, int len);
void sf_10ms_tick(void);
int sf_probe(void);
uint32_t sf_get_capacity(void);



#endif


