#include "usb_app.h"

#include "usbd_desc.h"
#include "usbd_msc_core.h"
#include "usbd_usr.h"

#include "systick.h"
#include "cpu_related_v3.h"
#include "spi_flash_v4.h"
#include "sdio_sdcard_v5.h"
#include "libc.h"
#include "my_printf.h"
#include "user_task.h"

#include "usbd_msc_bot.h"


uint8_t Max_Lun;
USB_OTG_CORE_HANDLE USB_OTG_dev;


void USB_device_init(void)
{
	if(sd_init() == 0)
	{
		Max_Lun = 2;      //SPI Flash + SD, 如果不想挂载SD, 此处把Max_Lun设置为1即可
		my_printf("Detected SD disk ^_^\n\r");
	}
	else
	{
		Max_Lun = 1;      //Only SPI Flash
	}
	
	USBD_Init(&USB_OTG_dev,USB_OTG_FS_CORE_ID,&USR_desc,&USBD_MSC_cb,&USR_FS_cb);
}

//========================================================================================
static __attribute__((noinline)) void memcpy_align(void *dst, void *src, uint32_t size)
{
	uint32_t *dstp;
	uint32_t *srcp;
	uint32_t dat0, dat1, dat2, dat3;

	dstp = dst;
	srcp = src;
	size /= 4;
	while(size > 0)
	{
		dat0 = srcp[0];
		dat1 = srcp[1];
		dat2 = srcp[2];
		dat3 = srcp[3];
		srcp += 4;

		dstp[0] = dat0;
		dstp[1] = dat1;
		dstp[2] = dat2;
		dstp[3] = dat3;
		dstp += 4;
		
		size -= 4;
	}
}

#ifndef  min
#define min(a, b) ((a)<(b)?(a):(b))
#endif

static usb_disk_opt_t usb_disk_opt;

//读写磁盘(ST提供的代码是在USB中断中读写磁盘, 现放在main线程中读写磁盘)
void usb_read_write_disk(void)
{
	uint32_t opt_mark;
	int ret;
	uint32_t temp;
	uint32_t lun, write_len;
	uint64_t write_addr;

	ALLOC_CPU_SR();
	ENTER_CRITICAL();
	opt_mark = usb_disk_opt.opt_mark;
	usb_disk_opt.opt_mark = 0;
	EXIT_CRITICAL();

	if(opt_mark & USB_MAS_DISK_READ)
	{	
		if(usb_disk_opt.usb_in_trans_state == USB_IN_TRANS_IDLE)  //USB device上传为空闲态
		{
			my_printf("[%s] Error! USB device isn't in sending state!\n\r", __FUNCTION__);
		}
		else
		{
//			my_printf("R%u %u %u\n\r", usb_disk_opt.lun, (uint32_t)usb_disk_opt.read_addr, usb_disk_opt.read_trans_cnt);
//			my_printf("R_DK\n\r");
			switch (usb_disk_opt.lun)    //这里,根据lun的值确定所要操作的磁盘
			{
				case 0:			//磁盘0为 SPI FLASH盘	 
					ret = sf_read_disk((uint32_t)usb_disk_opt.read_addr, usb_disk_opt.usb_in_buff, usb_disk_opt.read_len);   		  
					break;	  
				case 1:			//磁盘1为SD卡
					ret = sd_read_disk(usb_disk_opt.read_addr, usb_disk_opt.usb_in_buff, usb_disk_opt.read_len);
					break;			    
				default:
					ret = 0;
					break;
			}

			if(ret == usb_disk_opt.read_len)
			{
				usb_disk_opt.usb_in_read_disk_speed += usb_disk_opt.read_len;

				usb_disk_opt.usb_in_buff_load_state = USB_IN_BUFF_LOADED;
				if(usb_disk_opt.read_trans_cnt == 0)
				{	//由OPT_PREPARE激活的读磁盘任务
					opt_mark |= USB_MAS_COPY_DATA;
//					my_printf("R_First\n\r");     //debug
				}
			}
			else
			{
				my_printf("[%s] Error! Read disk%u error! ret = %d!!\n\r", __FUNCTION__, usb_disk_opt.lun, ret);
			}
		}		
	}

	if(opt_mark & USB_MAS_COPY_DATA)
	{
		if(usb_disk_opt.usb_in_trans_state == USB_IN_TRANS_IDLE)  //USB device上传为空闲态
		{
			my_printf("[%s] Error! USB device isn't in sending state!\n\r", __FUNCTION__);
		}
		else if(usb_disk_opt.usb_in_buff_load_state == USB_IN_BUFF_EMPTY)
		{
			my_printf("[%s] Error! Not read disk, can't copy!!\n\r", __FUNCTION__);
		}
		else
		{
//			my_printf("R_CP\n\r");
			memcpy_align(MSC_BOT_Data, usb_disk_opt.usb_in_buff, usb_disk_opt.read_len);
			
			ENTER_CRITICAL();
			DCD_EP_Tx(&USB_OTG_dev, MSC_IN_EP, MSC_BOT_Data, usb_disk_opt.read_len);
	
			
			usb_disk_opt.read_addr += usb_disk_opt.read_len;
			usb_disk_opt.read_trans_cnt += usb_disk_opt.read_len;

			MSC_BOT_csw.dDataResidue -= usb_disk_opt.read_len;

			temp = usb_disk_opt.read_trans_size - usb_disk_opt.read_trans_cnt;
			if(temp != 0)
			{
				usb_disk_opt.read_len = min(temp, IN_BUFF_SIZE);
				usb_disk_opt.opt_mark |= USB_MAS_DISK_READ;   //激活读磁盘任务
				usb_disk_opt.usb_in_buff_load_state = USB_IN_BUFF_EMPTY;
				task_wakeup(&tcb_usb_device);
//				my_printf("R_WK\n\r");
			}
			else
			{
				usb_disk_opt.usb_in_trans_state = USB_IN_TRANS_IDLE; 
				MSC_BOT_State = BOT_LAST_DATA_IN;
//				my_printf("R_END\n\r");
			}
			EXIT_CRITICAL();
		}
	}


	if(opt_mark & USB_MAS_DISK_WRITE)
	{
		if(usb_disk_opt.usb_out_trans_state == USB_OUT_TRANS_IDLE)  //USB device上传为空闲态
		{
			my_printf("[%s] Error! USB device isn't in receiving state!\n\r", __FUNCTION__);
		}
		else if(usb_disk_opt.usb_out_buff_full_state == USB_OUT_BUFF_EMPTY)
		{
			my_printf("[%s] Error! Not received, can't write disk!!\n\r", __FUNCTION__);
		}
		else
		{	
			lun = usb_disk_opt.lun;
			write_addr = usb_disk_opt.write_addr;
			write_len = usb_disk_opt.write_len;
//			my_printf("W%u 0x%X %u\n\r", lun, (uint32_t)write_addr, usb_disk_opt.write_trans_cnt);

			memcpy_align(&usb_disk_opt.usb_out_buff[0], MSC_BOT_Data, write_len);

			usb_disk_opt.usb_out_write_disk_speed += write_len;

			usb_disk_opt.write_addr += write_len;
			usb_disk_opt.write_trans_cnt += write_len;

			MSC_BOT_csw.dDataResidue -= write_len;

			usb_disk_opt.usb_out_buff_full_state = USB_OUT_BUFF_EMPTY;			
			
			if(usb_disk_opt.write_trans_cnt == usb_disk_opt.write_trans_size)
			{	//已经完成接收, 需向主机报告命令状态
				usb_disk_opt.usb_out_trans_state = USB_OUT_TRANS_IDLE;
				MSC_BOT_SendCSW (&USB_OTG_dev, CSW_CMD_PASSED);
//				my_printf("W_END\n\r");
			}
			else
			{
				//还未完成接收, 使能接收, 接着接收主机发来的数据
				usb_disk_opt.write_len = min(MSC_MEDIA_PACKET, usb_disk_opt.write_trans_size-usb_disk_opt.write_trans_cnt);
				DCD_EP_PrepareRx(&USB_OTG_dev, MSC_OUT_EP, MSC_BOT_Data, usb_disk_opt.write_len);   /* enable the next transaction*/				
			}

			switch (lun)    //这里,根据lun的值确定所要操作的磁盘
			{
				case 0:			//磁盘0为 SPI FLASH盘	 
					ret = sf_write_disk((uint32_t)write_addr, usb_disk_opt.usb_out_buff, write_len);   		  
					break;	  
				case 1:			//磁盘1为SD卡		    
					ret = sd_write_disk(write_addr, usb_disk_opt.usb_out_buff, write_len);	   
					break;			    
				default:
					ret = 0;
					break;
			}
			if(ret != write_len)
			{
				my_printf("[%s] Error! Write disk%u error! ret = %d!!\n\r", __FUNCTION__, lun, ret);
			}
		}
	}


	if(opt_mark==0 || opt_mark==(USB_MAS_DISK_WRITE|USB_MAS_DISK_READ))
	{
		my_printf("[%s] Tip! opt mark is %X, lun=%u read_addr=0x%X read_len=%u write_addr=0x%X write_len=%u\r\n", \
		__FUNCTION__, opt_mark, usb_disk_opt.lun, \
		(uint32_t)usb_disk_opt.read_addr, usb_disk_opt.read_len, \
		(uint32_t)usb_disk_opt.write_addr, usb_disk_opt.write_len);
	}
}

void usb_mas_read_memory(int opt_flag, uint8_t lun, uint32_t Memory_Offset, uint32_t Transfer_Length)
{
	if(opt_flag == OPT_PREPARE)
	{	//获取lun Memory_Offset Transfer_Length, 并激活读磁盘任务
		usb_disk_opt.lun = lun;
		usb_disk_opt.read_addr = (uint64_t)Memory_Offset*DISK_BLOCK_SIZE;
		usb_disk_opt.read_trans_size = Transfer_Length*DISK_BLOCK_SIZE;

		usb_disk_opt.read_len = min(usb_disk_opt.read_trans_size, IN_BUFF_SIZE);
		usb_disk_opt.read_trans_cnt = 0;
		
		usb_disk_opt.usb_in_buff_load_state = USB_IN_BUFF_EMPTY; //USB device要上传的数据还未加载到缓存
		usb_disk_opt.usb_in_trans_state = USB_IN_TRANS_ONGOING;  //USB device处于上传数据状态
		
		usb_disk_opt.opt_mark |= USB_MAS_DISK_READ;  //激活读磁盘任务
		task_wakeup(&tcb_usb_device);
//		my_printf("R_Start\n\r");          //debug
//		my_printf("R_Start%u %u %u\n\r", lun, (uint32_t)usb_disk_opt.read_addr, usb_disk_opt.read_trans_size);
	}
	else
	{
		if(lun != usb_disk_opt.lun)
		{
			my_printf("[%s] Error! Changing drive disk error!!!\n\r", __FUNCTION__);
		}
		else
		{
			if(usb_disk_opt.read_trans_cnt != usb_disk_opt.read_trans_size)
			{	//还未完成本次IN事务
				usb_disk_opt.opt_mark |= USB_MAS_COPY_DATA;  //还未读取完成, 接着拷贝磁盘
				task_wakeup(&tcb_usb_device);
//				my_printf("R_WK_ISR\n\r");
			}
			else
			{	//已经完成本次IN事务
				my_printf("[s] R_REEOR!!!\n\r", __FUNCTION__);
			}
		}		
	}
}


void usb_mas_write_memory(int opt_flag, uint8_t lun, uint32_t Memory_Offset, uint32_t Transfer_Length)
{
	if(opt_flag == OPT_PREPARE)
	{ //获取lun Memory_Offset Transfer_Length
		usb_disk_opt.lun = lun;
		usb_disk_opt.write_addr = (uint64_t)Memory_Offset*DISK_BLOCK_SIZE;
		usb_disk_opt.write_trans_size = Transfer_Length*DISK_BLOCK_SIZE;

		usb_disk_opt.write_trans_cnt = 0;
		usb_disk_opt.write_len = min(MSC_MEDIA_PACKET, usb_disk_opt.write_trans_size-usb_disk_opt.write_trans_cnt);

		usb_disk_opt.usb_out_buff_full_state = USB_OUT_BUFF_EMPTY;
		usb_disk_opt.usb_out_trans_state = USB_OUT_TRANS_ONGOING;
		
		if(usb_disk_opt.write_len>=4096 && (usb_disk_opt.write_addr&(4096-1))!=0)
		{	//调整usb_disk_opt.write_addr按4096字节对齐
			usb_disk_opt.write_len = (-usb_disk_opt.write_addr)%4096;
//			my_printf("\n\rW_ADJ\n\r");
		}

		DCD_EP_PrepareRx(&USB_OTG_dev, MSC_OUT_EP, MSC_BOT_Data, usb_disk_opt.write_len);
//		my_printf("W_Start\n\r");            //debug
//		my_printf("W_Start%u 0x%X %u\n\r", lun, (uint32_t)usb_disk_opt.write_addr, usb_disk_opt.write_trans_size);
	}
	else
	{
		if(lun != usb_disk_opt.lun)
		{
			my_printf("[%s] Error! Changing drive disk error!!!\n\r", __FUNCTION__);
		}
		else
		{	//唤醒tcb_usb_device任务来拾取OUT事务发来的数据
			usb_disk_opt.usb_out_buff_full_state = USB_OUT_BUFF_FULL;
			usb_disk_opt.opt_mark |= USB_MAS_DISK_WRITE;  //写磁盘
			task_wakeup(&tcb_usb_device);
//			my_printf("W_WK_ISR\n\r");
//			my_printf("W_WK_ISR%u %u %u\n\r", lun, (uint32_t)usb_disk_opt.write_addr, usb_disk_opt.write_len);			
		}
	}
}


//统计USB传输速率
void usb_1s_hook(void)
{
	if(usb_disk_opt.usb_in_read_disk_speed != 0)
	{
		my_printf("usb in transaction read disk speed: %uKB\n\r", usb_disk_opt.usb_in_read_disk_speed>>10);
		usb_disk_opt.usb_in_read_disk_speed = 0;
	}
	
	if(usb_disk_opt.usb_out_write_disk_speed != 0)
	{
		my_printf("usb out transaction write disk speed: %uKB\n\r", usb_disk_opt.usb_out_write_disk_speed>>10);
		usb_disk_opt.usb_out_write_disk_speed = 0;
	}
}




