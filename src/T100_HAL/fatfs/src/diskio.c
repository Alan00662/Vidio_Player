/*-----------------------------------------------------------------------*/
/* Low level disk I/O module skeleton for FatFs     (C)ChaN, 2016        */
/*-----------------------------------------------------------------------*/
/* If a working storage control module is available, it should be        */
/* attached to the FatFs via a glue function rather than modifying it.   */
/* This is an example of glue functions to attach various exsisting      */
/* storage control modules to the FatFs module with a defined API.       */
/*-----------------------------------------------------------------------*/

#include "diskio.h"		/* FatFs lower layer API */

#include "spi_flash_v4.h"
#include "sdio_sdcard_v5.h"


/* Definitions of physical drive number for each drive */
//#define DEV_RAM		0	/* Example: Map Ramdisk to physical drive 0 */
//#define DEV_MMC		1	/* Example: Map MMC/SD card to physical drive 1 */
#define DEV_USB		2	/* Example: Map USB MSD to physical drive 2 */

#define  DEV_SF  0    /* Map SPI Flash to physical drive 0 */
#define  DEV_MMC 1    /* Map MMC/SD card to physical drive 1 */

//#define  SF_DISK_SIZE    (1024*1024*4)
//#define  SF_SECTOR_SIZE  512
//#define  SF_BLOCK_SIZE   8
//#define  SF_SECTOR_COUNT (SF_DISK_SIZE/SF_SECTOR_SIZE)

/*-----------------------------------------------------------------------*/
/* Get Drive Status                                                      */
/*-----------------------------------------------------------------------*/

DSTATUS disk_status (
	BYTE pdrv		/* Physical drive nmuber to identify the drive */
)
{
	DSTATUS stat = 0;
//	int result;

	switch (pdrv) {
	case DEV_SF :
//		result = sf_disk_status();
		// translate the reslut code here

		return stat;

	case DEV_MMC :
//		result = MMC_disk_status();

		// translate the reslut code here

		return stat;

	case DEV_USB :
//		result = USB_disk_status();

		// translate the reslut code here

		return stat;
	}
	return STA_NOINIT;
}



/*-----------------------------------------------------------------------*/
/* Inidialize a Drive                                                    */
/*-----------------------------------------------------------------------*/

DSTATUS disk_initialize (
	BYTE pdrv				/* Physical drive nmuber to identify the drive */
)
{
	DSTATUS stat = 0;
//	int result;

	switch (pdrv) {
	case DEV_SF :
//		result = sf_disk_initialize();
		// translate the reslut code here

		return stat;

	case DEV_MMC :
//		result = MMC_disk_initialize();

		// translate the reslut code here

		return stat;

	case DEV_USB :
//		result = USB_disk_initialize();

		// translate the reslut code here

		return stat;
	}
	return STA_NOINIT;
}



/*-----------------------------------------------------------------------*/
/* Read Sector(s)                                                        */
/*-----------------------------------------------------------------------*/

DRESULT disk_read (
	BYTE pdrv,		/* Physical drive nmuber to identify the drive */
	BYTE *buff,		/* Data buffer to store read data */
	DWORD sector,	/* Start sector in LBA */
	UINT count		/* Number of sectors to read */
)
{
	DRESULT res = RES_OK;
	int result;
	int len;
	uint64_t start_addr;

//	start_addr = sector*512;    //BUG  2021/03/03发现此BUG, 要铭记BUG的原因, 有如下2种解决办法
//	start_addr = (uint64_t)sector*512;   //表示先把sector扩展成uint64_t然后再做乘法运算, 而不是把乘法运算的结果扩成uint64_t
	start_addr = (uint64_t)sector<<9;
	len = count<<9;
	
	switch (pdrv) {
	case DEV_SF :
		result = sf_read_disk(start_addr, buff, len);
		if(result != len) {
			res = RES_ERROR;
		}

		return res;

	case DEV_MMC :
		result = sd_read_disk(start_addr, buff, len);
		if(result != len) {
			res = RES_ERROR;
		}
		return res;

	case DEV_USB :
		// translate the arguments here

//		result = USB_disk_read(buff, sector, count);

		// translate the reslut code here

		return res;
	}

	return RES_PARERR;
}



/*-----------------------------------------------------------------------*/
/* Write Sector(s)                                                       */
/*-----------------------------------------------------------------------*/

DRESULT disk_write (
	BYTE pdrv,			/* Physical drive nmuber to identify the drive */
	const BYTE *buff,	/* Data to be written */
	DWORD sector,		/* Start sector in LBA */
	UINT count			/* Number of sectors to write */
)
{
	DRESULT res = RES_OK;
	int result;
	int len;
	uint64_t start_addr;

//	start_addr = sector*512;
//	len = count*512;
	start_addr = (uint64_t)sector << 9;
	len = count << 9;
	
	switch (pdrv) {
	case DEV_SF :
		result = sf_write_disk(start_addr, (void *)buff, len);
		if(result != len) {
			res = RES_ERROR;
		}
		return res;

	case DEV_MMC :
		result = sd_write_disk(start_addr, (void *)buff, len);
		if(result != len) {
			res = RES_ERROR;
		}
		return res;

	case DEV_USB :
		// translate the arguments here

//		result = USB_disk_write(buff, sector, count);

		// translate the reslut code here

		return res;
	}

	return RES_PARERR;
}



/*-----------------------------------------------------------------------*/
/* Miscellaneous Functions                                               */
/*-----------------------------------------------------------------------*/

DRESULT disk_ioctl (
	BYTE pdrv,		/* Physical drive nmuber (0..) */
	BYTE cmd,		/* Control code */
	void *buff		/* Buffer to send/receive control data */
)
{
	DRESULT res = RES_OK;
//	int result;

	switch (pdrv) {
	case DEV_SF :
		if(cmd == CTRL_SYNC)   /* 磁盘一般都具有内存缓冲区, 此命令表示把缓冲区中的数据同步到(写入)物理磁盘 */
		{	;	}
		else if(cmd == GET_SECTOR_COUNT) /* Get media size (needed at _USE_MKFS == 1) */
		{
			*((DWORD *)buff) = sf_get_capacity() / 512;
		}
		else if(cmd == GET_SECTOR_SIZE)  /* Get sector size (needed at _MAX_SS != _MIN_SS) */
		{	*((DWORD *)buff) = 512;	}
		else if(cmd == GET_BLOCK_SIZE)   /* Get erase block size (needed at _USE_MKFS == 1) */
		{	*((DWORD *)buff) = 8;	}
		else if(cmd == CTRL_TRIM)        /* Inform device that the data on the block of sectors is no longer used (needed at _USE_TRIM == 1) */
		{	;	}
		else
		{	res = RES_PARERR;	}

		return res;

	case DEV_MMC :
		if(cmd == CTRL_SYNC)   /* 磁盘一般都具有内存缓冲区, 此命令表示把缓冲区中的数据同步到(写入)物理磁盘 */
		{	;	}
		else if(cmd == GET_SECTOR_COUNT) /* Get media size (needed at _USE_MKFS == 1) */
		{
			*((DWORD *)buff) = SDCardInfo.CardCapacity / 512;
		}
		else if(cmd == GET_SECTOR_SIZE)  /* Get sector size (needed at _MAX_SS != _MIN_SS) */
		{	*((DWORD *)buff) = 512;	}
		else if(cmd == GET_BLOCK_SIZE)   /* Get erase block size (needed at _USE_MKFS == 1) */
		{	*((DWORD *)buff) = 8;	}
		else if(cmd == CTRL_TRIM)        /* Inform device that the data on the block of sectors is no longer used (needed at _USE_TRIM == 1) */
		{	;	}
		else
		{	res = RES_PARERR;	}

		return res;

	case DEV_USB :

		// Process of the command the USB drive

		return res;
	}

	return RES_PARERR;
}

