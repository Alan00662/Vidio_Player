#include "myfat.h"
#include "rtc_v2.h"
#include "my_printf.h"
#include "libc.h"
#include "time.h"
#include "uart.h"
#include "spi_flash_v4.h"


/* 磁盘id-当前使用的分区号 */
#if _MULTI_PARTITION
PARTITION VolToPart[_VOLUMES] =
{
	{0, 1},     //磁盘0的第1个分区作为格式化(挂载)为fatfs文件系统
	{1, 1},     //磁盘1的第1个分区作为格式化(挂载)为fatfs文件系统
};
#endif


#if !_FS_READONLY && !_FS_NORTC

//User defined function to give a current time to fatfs module      */
//31-25: Year(0-127 org.1980), 24-21: Month(1-12), 20-16: Day(1-31) */
//15-11: Hour(0-23), 10-5: Minute(0-59), 4-0: Second(0-29 *2) */
/* RTC function */
DWORD get_fattime (void)	/* 返回UTC时间, 即零时区 */
{
	DWORD time = 0;
	DWORD tmp = 0;

	int t[6];
	get_time(t, sizeof(t));

	tmp = t[5] - 1980;       //Year
	time |= (tmp<<25) & 0xFE000000;

	tmp = t[4];             //month
	time |= (tmp<<21) & 0x01E00000;

	tmp = t[3];             //date
	time |= (tmp<<16) & 0x001F0000;

	tmp = t[2];             //hour
	time |= (tmp<<11) & 0x0000F800;

	tmp = t[1];             //min
	time |= (tmp<<5) & 0x000007E0;

	tmp = t[1]/2;           //second
	time |= tmp & 0x0000001F;

	return time;
}
#endif

//get_fattime()的反函数   fat文件的时间戳采用的是UTC时间, 显示时转化为北京时间(UTC+8)
int fattime2rtc(DWORD fattime, int *t, int len)
{
	if(len < 6*sizeof(int))
		return -1;

	t[5] = ((fattime&0xFE000000)>>25) + 1980;  //year
	t[4] = (fattime&0x01E00000) >> 21;         //month
	t[3] = (fattime&0x001F0000) >> 16;         //date

	t[2] = (fattime&0x0000F800) >> 11;         //hour
	t[1] = (fattime&0x000007E0) >> 5;          //minute
	t[0] = (fattime&0x0000001F) >> 0;          //second

	//由于文件的修改时间戳采用的是UTC时间(即零时区), 需要转换为北京时间
	utc2bj(t, len);

	return 0;
}

//====================================================================================================
FATFS sys_fs[_VOLUMES];
DWORD drv0_partition[4] = {100, 0, 0, 0};   /* 磁盘0的分区情况: 主分区1占磁盘100%, 其余3个分区未使用 */
DWORD drv1_partition[4] = {100, 0, 0, 0};   /* 磁盘1的分区情况: 主分区1占磁盘100%, 其余3个分区未使用 */


extern int uni_oem_data_check(void);   //在cc936.c文件中
extern int uni_oem_write_to_spi_flash(void);

/*
 * @breif: 挂载磁盘并初始化文件系统
 */
int fatfs_init(void)
{
	int ret;
	uint8_t mbr[512];  //磁盘分区表 Main Boot Record
	uint16_t end_flag;
	uint32_t start_sector;

	sf_read_disk(0, mbr, sizeof(mbr));
	start_sector = (mbr[457]<<24) | (mbr[456]<<16) | (mbr[455]<<8) | mbr[454];
	end_flag = (mbr[511]<<8) | mbr[510];
	if(end_flag == 0xAA55) {
		if(start_sector != 1024*256/_MAX_SS)
		{
			my_printf("SPI Flash partition table is not required, Erasing it.\n\r");
			sf_erase_sector_nocheck(0);
			sf_busy_check();
		}
		else
		{
			my_printf("SPI Flash partition table is OK.\n\r");
		}
	}

	ret = uni_oem_data_check();
	if(0 != ret) {
		my_printf("\n\rSPI Flash need update, Now updating...\n\r");
		ret = uni_oem_write_to_spi_flash();
		if(0 != ret)
		{
			my_printf("Error1! SPI Flash write error!!\n\r");
			return -100;
		}
		else
		{
			ret = uni_oem_data_check();
			if(ret != 0)
			{
				my_printf("Error2! SPI Flash check fail!!\n\r");
				return -101;
			}
			my_printf("SPI Flash uni2oem data update success.\n\r");
		}
	}
	else
	{
		my_printf("\n\rSPI Flash uni2oem data is OK.\n\r");
	}

	ret = f_mount(&sys_fs[0], "0:", 1);  //挂载磁盘0
	if(ret == FR_OK)   //挂载磁盘上的文件系统成功
	{	my_printf("Mount disk0 fatfs success ^_^\n\r");	}
	else               //挂载磁盘上的文件系统失败
	{
		my_printf("\n\rMount disk0 fatfs fail. Now, parting the disk and formating fatfs, ret=%d\n\r", ret);

		uint8_t work_buff[_MAX_SS];
		ret = f_fdisk(0, drv0_partition, work_buff, 1024*256/_MAX_SS);  //给磁盘0创建MBR分区
		if(ret != FR_OK) {
			my_printf("Disk0 create patition fail, ret=%d\n\r", ret);
			return ret;
		}

		ret = f_mkfs("0:", FM_ANY, 0, work_buff, sizeof(work_buff));
		if(ret != FR_OK) {
			my_printf("Disk0 create fatfs fail, ret=%d\n\r", ret);
			return ret;
		}

		ret = f_setlabel ("0:SF_DISK");	/* Set volume label */
		if(ret != FR_OK) {
			my_printf("Disk0 set label fail, ret=%d\n\r", ret);
			return ret;
		}
		my_printf("Disk0 formated fatfs success ^_^\n\r");
	}

	return ret;
}



